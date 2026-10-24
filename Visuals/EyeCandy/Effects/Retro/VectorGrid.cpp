
/** $VER: VectorGrid.cpp (2026.10.23) */

#include <pch.h>

#include "VectorGrid.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <array>
#include <format>

namespace VectorGrid
{
    namespace
    {
        constexpr double NearPlane           = 0.055;
        constexpr double FarPlane            = 3.20;
        constexpr double GridHalfWidth       = 2.65;
        constexpr double CameraHeight        = 0.54;
        constexpr double VerticalFieldOfView = 66.0;
        constexpr double HorizonFraction     = 0.20;

        double SpeedFactor(EyeCandy::Speed speed) noexcept
        {
            switch (speed)
            {
                case EyeCandy::Speed::Slow: return 0.55;
                case EyeCandy::Speed::Fast: return 1.55;
                case EyeCandy::Speed::Normal:
                default: return 1.0;
            }
        }
    }

    effect_t::effect_t()
    {
        StartWorkers();
    }

    effect_t::~effect_t()
    {
        StopWorkers();
    }

    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Vector Grid, {}, {} quality, {} speed",
            GetThemeName(GetColorTheme()), Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = Clamp(deltaTime);

        AnalyzeAudio(chunk);

        Update(deltaTime);
        UpdatePalette();

        BuildGrid();

        {
            renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

            RenderBackground(renderTarget);
            RenderSun(renderTarget);

            const auto Shake  = (float) ((_CameraShake * 2.2) + (_CameraKick * 3.0));

            const auto ShakeX = (float) (std::sin(_AnimationTime * 29.0) * Shake);
            const auto ShakeY = (float) (std::cos(_AnimationTime * 23.0) * Shake * 0.55);

            renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(ShakeX, ShakeY));

            RenderGrid(renderTarget);
            RenderHorizon(renderTarget);
            RenderBeatFlash(renderTarget);

            renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        }

        RenderScanlines(renderTarget);
        
        const auto Alpha = (float) std::clamp(0.20 + (_BeatPulse * 0.25) + (_BassImpulse * 0.18), 0., 0.75);

        RenderFrame(renderTarget, SetAlpha(_Palette.Horizon, Alpha));
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        switch (_Quality)
        {
            case EyeCandy::Quality::Low:
                _ColumnCount = 24;
                _RowCount    = 36;
                break;

            case EyeCandy::Quality::High:
                _ColumnCount = 48;
                _RowCount    = 72;
                break;

            case EyeCandy::Quality::Medium:
            default:
                _ColumnCount = 32;
                _RowCount    = 64;
                break;
        }

        _CellCount = _ColumnCount * _RowCount;

        _Grid.assign(_CellCount, {});

        _Travel                 = 0.;
        _WaveEnvelope           = 0.;
        _MidEnvelope            = 0.;
        _TrebleImpulse          = 0.;
        _PreviousMidEnvelope    = 0.;
        _PreviousTrebleEnvelope = 0.;
        _BeatDeformation        = 0.;
        _BeatPhase              = 0.;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak         = 0.;
        _RMS          = 0.;

        _BassEnergy   = 0.;
        _MidEnergy    = 0.;
        _TrebleEnergy = 0.;

        _Bands.fill(0.);

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const size_t SampleCount  = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const double SampleRate   = (double) chunk.get_sample_rate();
        const size_t FrameCount   = std::min(SampleCount, MaximumFrameCount);

        const audio_sample * Data = chunk.get_data();

        if (!Data || ChannelCount == 0 || FrameCount < 32 || SampleRate <= 0.)
            return;

        const size_t FirstFrame = SampleCount - FrameCount;

        std::array<double, MaximumFrameCount> Samples {};

        double Sum  = 0.;
        double Peak = 0.;

        for (size_t i = 0; i < FrameCount; ++i)
        {
            const audio_sample * Frame = Data + (FirstFrame + i) * ChannelCount;

            double Mono = 0.;

            for (size_t Channel = 0; Channel < ChannelCount; ++Channel)
                Mono += (double) Frame[Channel];

            Mono = ClampAudio(Mono / (double)ChannelCount);

            // Apply a Hann window.
            {
                const double Window = 0.5 - (std::cos(TwoPi * (double) i / (double) (FrameCount - 1)) * 0.5);

                Samples[i] = Mono * Window;
            }

            Sum += Mono * Mono;
            Peak = std::max(Peak, std::abs(Mono));
        }

        _Peak = Peak;
        _RMS  = std::sqrt(Sum / (double) FrameCount);

        _HasAudio = (_Peak > 1e-5) || (_RMS > 1e-6);

        if (!_HasAudio)
            return;

        const double Nyquist    = SampleRate * 0.50;
        const double CenterStep = std::pow(16'500. / 40., 1. / (double)(FrequencyBandCount - 1)); // 40 Hz to 16.5 kHz

        double Center = 40.; // Hz

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double Frequency   = std::clamp(Center, 20., Nyquist);
            const double Coefficient = 2. * std::cos(TwoPi * Frequency / SampleRate);

            double q1 = 0.;
            double q2 = 0.;

            for (size_t j = 0; j < FrameCount; ++j)
            {
                const double q0 = Samples[j] + Coefficient * q1 - q2;
                q2 = q1;
                q1 = q0;
            }

            const double Power     = std::max(0., q1 * q1 + q2 * q2 - Coefficient * q1 * q2);
            const double Magnitude = std::sqrt(Power) / (double) FrameCount;

            _Bands[i] = Clamp(std::log1p(Magnitude * 110.) / std::log1p(110.));

            Center *= CenterStep;
        }

        const double Bass   = (_Bands[0] * 0.50) + (_Bands[1] * 0.32) + (_Bands[2] * 0.18);

        _BassEnergy = Clamp(1. - std::exp(-Bass * 2.9 * _BassResponse));

        const double Mid    = (_Bands[3] * 0.25) + (_Bands[4] * 0.42) + (_Bands[5] * 0.33);
        const double Treble = (_Bands[6] * 0.46) + (_Bands[7] * 0.54);

        _MidEnergy    = Clamp(1. - std::exp(-Mid    * 2.35 * _MidResponse));
        _TrebleEnergy = Clamp(1. - std::exp(-Treble * 2.55 * _TrebleResponse));
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        if (_Grid.size() != (_CellCount) || _Grid.empty())
            Reset();

        UpdateThemeTransition(deltaTime);

        const double Speed = SpeedFactor(_Speed);

        _AnimationTime += deltaTime * Speed;

        _SmoothedPeak   = ExponentialSmoothing(_SmoothedPeak,   _Peak,         deltaTime, (_Peak         > _SmoothedPeak)   ? 24. : 5.);
        _SmoothedRMS    = ExponentialSmoothing(_SmoothedRMS,    _RMS,          deltaTime, (_RMS          > _SmoothedRMS)    ? 18. : 4.);

        _SmoothedBass   = ExponentialSmoothing(_SmoothedBass,   _BassEnergy,   deltaTime, (_BassEnergy   > _SmoothedBass)   ? 34. : 4.5);
        _SmoothedMid    = ExponentialSmoothing(_SmoothedMid,    _MidEnergy,    deltaTime, (_MidEnergy    > _SmoothedMid)    ? 20. : 5.);
        _SmoothedTreble = ExponentialSmoothing(_SmoothedTreble, _TrebleEnergy, deltaTime, (_TrebleEnergy > _SmoothedTreble) ? 18. : 6.);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
            _SmoothedBands[i] = ExponentialSmoothing(_SmoothedBands[i], _Bands[i], deltaTime, _Bands[i] > _SmoothedBands[i] ? 20. : 5.);

        _BeatAverage  = ExponentialSmoothing(_BeatAverage,  _BassEnergy, deltaTime, 1.45);
        _BeatEnvelope = ExponentialSmoothing(_BeatEnvelope, _BassEnergy, deltaTime, (_BassEnergy > _BeatEnvelope) ? 38. : 8.);

        const double Attack = std::max(0., _BeatEnvelope - _PreviousBassEnvelope);

        {
            _PreviousBassEnvelope = _BeatEnvelope;

            _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);

            const double Threshold = std::max(0.10, (_BeatAverage * 1.36) + 0.025);

            if (_HasAudio && (_BeatCooldown <= 0.) && (_BeatEnvelope > Threshold) && (_SmoothedPeak > 0.05))
            {
                const double Excess = (_BeatEnvelope - Threshold) / std::max(0.001, 1. - Threshold);

                _BeatPulse = Clamp((0.52 + (Excess * 2.0)) * _BeatResponse);

                _BeatCooldown = 0.115;
            }
            else
                _BeatPulse *= std::exp(-deltaTime * 7.0);
        }

        {
            const double ImpulseTarget = Clamp((Attack * 8. * _BassResponse) + (_BeatPulse * 0.82));

            _BassImpulse = ExponentialSmoothing(_BassImpulse, ImpulseTarget, deltaTime, (ImpulseTarget > _BassImpulse) ? 48. : 10.);
            _CameraShake = ExponentialSmoothing(_CameraShake, Clamp((_BassImpulse * 0.68) + (_BeatPulse * 0.42) + (_SmoothedBass * 0.08)), deltaTime, 9.);
        }

        {
            const double WaveTarget = std::clamp(_SmoothedBass * 0.78 + _BassImpulse * 0.90 + _SmoothedMid * 0.22, 0., 1.35);

            _WaveEnvelope = ExponentialSmoothing(_WaveEnvelope, WaveTarget, deltaTime, WaveTarget > _WaveEnvelope ? 28. : 5.5);
            _CameraKick   = ExponentialSmoothing(_CameraKick, _BassImpulse, deltaTime, _BassImpulse > _CameraKick ? 34. : 7.);
            _HorizonPulse = ExponentialSmoothing(_HorizonPulse, Clamp(_BeatPulse + _BassImpulse * 0.7), deltaTime, 10.);
        }

        // Qualified beats launch a broad deformation wave through the grid. The fast attack keeps the impact tightly synchronized, while the slower release lets the surface settle naturally.
        {
            const double DeformationTarget = Clamp(_BeatPulse * 0.82 + _BassImpulse * 0.58 + Attack * 7.);

            _BeatDeformation = ExponentialSmoothing(_BeatDeformation, DeformationTarget, deltaTime, DeformationTarget > _BeatDeformation ? 58. : 5.8);
            _BeatPhase       = std::fmod(_BeatPhase + deltaTime * (1.05 + _BeatDeformation * 0.75), 1.0);

            if (_BeatPulse > 0.72 && Attack > 0.004)
                _BeatPhase = 0.;
        }

        // Midrange adds rolling cross-grid motion, while treble attacks create short, fine ripples.
        {
            const double MidAttack    = std::max(0., _SmoothedMid - _PreviousMidEnvelope);

            _PreviousMidEnvelope = _SmoothedMid;

            const double MidTarget = std::clamp(_SmoothedMid * 0.82 + MidAttack * 5.2, 0., 1.25);

            _MidEnvelope = ExponentialSmoothing(_MidEnvelope, MidTarget, deltaTime, MidTarget > _MidEnvelope ? 24. : 6.5);
        }

        {
            const double TrebleAttack = std::max(0., _SmoothedTreble - _PreviousTrebleEnvelope);
            _PreviousTrebleEnvelope = _SmoothedTreble;

            const double TrebleTarget = std::clamp(_SmoothedTreble * 0.48 + TrebleAttack * 9.5, 0., 1.35);

            _TrebleImpulse = ExponentialSmoothing(_TrebleImpulse, TrebleTarget, deltaTime, TrebleTarget > _TrebleImpulse ? 42. : 12.);
        }

        // Keep the grid advancing towards the camera even during silence. Audio adds acceleration, but is not required for the perspective motion itself.
        const double TravelSpeed = (0.72 + (_SmoothedRMS * 0.32) + (_SmoothedBass * 0.48) + (_MidEnvelope * 0.18) + (_TrebleImpulse * 0.10) + (_BassImpulse * 0.58)) * Speed;

        _Travel = std::fmod(_Travel + (TravelSpeed * deltaTime), 1.);

        if (_Travel < 0.)
            _Travel += 1.;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdatePalette() noexcept
    {
        _Palette.BackgroundTop    = GetThemeColor(ThemeRole::BackgroundA, 0.f, 1.f, 0.72f, 1.f);
        _Palette.BackgroundBottom = GetThemeColor(ThemeRole::BackgroundB, 0.f, 1.f, 0.92f, 1.f);
        _Palette.Horizon          = GetThemeColor(ThemeRole::Ring,        0.f, 1.f, 1.f, 1.f);
        _Palette.Longitudinal     = GetThemeColor(ThemeRole::Rail,        0.f, 1.f, 1.f, 1.f);
        _Palette.Lateral          = GetThemeColor(ThemeRole::Core,        0.f, 1.f, 1.f, 1.f);
        _Palette.Accent           = GetThemeColor(ThemeRole::Flash,       0.f, 1.f, 1.f, 1.f);
        _Palette.Sun              = GetThemeColor(ThemeRole::Core, 0.035f, 0.82f, 1.f, 1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::EvaluateWave(double x, double z) const noexcept
    {
        const double NearWeight   = Clamp((FarPlane - z) / (FarPlane - NearPlane));
        const double CenterWeight = std::exp(-x * x * 0.19);
        const double DistanceFade = SmoothStep(0.02f, 0.92f, (float) NearWeight);

        // All surface deformation is audio-driven. In silence the grid remains flat while its perspective rows continue travelling towards the camera.
        const double AudioActivity = Clamp((_SmoothedRMS * 2.8) + (_SmoothedBass * 1.4) + (_MidEnvelope * 0.65) + (_TrebleImpulse * 0.35) + (_BassImpulse * 0.85));

        const double BassWave      = std::sin(z * 7.4             - _AnimationTime * 3.15 + (x * 0.92)) * (_SmoothedBass  * 0.125 + _BassImpulse * 0.085 + AudioActivity * 0.012);
        const double BassSwell     = std::cos(z * 3.15            - _AnimationTime * 1.55)              * (_SmoothedBass  * 0.070 + _BassImpulse * 0.055);
        const double CrossWave     = std::sin(x * 3.2  + z * 4.7  + _AnimationTime * 1.65)              * (_SmoothedRMS   * 0.048);
        const double MidWave       = std::cos(x * 5.8  - z * 3.1  + _AnimationTime * 2.35)              * (_MidEnvelope   * 0.082);
        const double MidFold       = std::sin(x * 7.2  + z * 5.4  - _AnimationTime * 2.8)               * (_MidEnvelope   * 0.035);
        const double TrebleRipple  = std::sin(x * 12.5 + z * 14.0 - _AnimationTime * 6.4)               * (_TrebleImpulse * 0.046);
        const double TrebleCross   = std::cos(x * 17.0 - z * 9.5  + _AnimationTime * 7.8)               * (_TrebleImpulse * 0.018);

        // The beat wave starts near the viewer, expands towards the horizon and bends adjacent columns in opposite directions, producing a visible shockwave instead of a uniform lift.
        const double BeatRidge       = std::exp(-std::pow(z - (0.34 + _Travel * 1.4), 2.) * 7.0) * _BassImpulse * 0.24;
        const double BeatFront       = 0.16 + _BeatPhase * (FarPlane - 0.30);
        const double BeatDistance    = z - BeatFront;
        const double BeatWidth       = 0.10 + _BeatPhase * 0.20;
        const double BeatShell       = std::exp(-(BeatDistance * BeatDistance) / (BeatWidth * BeatWidth));
        const double BeatShock       = BeatShell * (0.18 + std::abs(x) * 0.028) * _BeatDeformation;
        const double BeatFold        = std::sin(x * 4.6 + BeatDistance * 8.0) * BeatShell * _BeatDeformation * 0.065;
        const double BeatCompression = -std::exp(-z * 1.15) * _BeatDeformation * (0.022 + _BeatPulse * 0.018);

        return (BassWave + BassSwell + CrossWave + MidWave + MidFold + TrebleRipple + TrebleCross + BeatRidge + BeatShock + BeatFold + BeatCompression) * CenterWeight * DistanceFade;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::BuildGrid()
    {
        if (_Grid.size() != _CellCount)
            _Grid.assign(_CellCount, {});

        _GridRowStep      = (FarPlane - NearPlane) / (double)(_RowCount - 1);
        _GridTravelOffset = _Travel * _GridRowStep;

        if (_Workers.empty())
        {
            GenerateWork(0, 1);

            return;
        }

        std::unique_lock Lock(_WorkerMutex);

        _PendingWorkers = _Workers.size();
        ++_Generation;

        _WorkerCondition.notify_all();

        _CompletionCondition.wait(Lock, [this]()
        {
            return _PendingWorkers == 0;
        });
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderBackground(ID2D1RenderTarget * renderTarget) noexcept
    {
        renderTarget->Clear(Black);

        const float HorizonY = _Height * (float) HorizonFraction;

        _SkyBrush->SetStartPoint(D2D1::Point2F(0.f, 0.f));
        _SkyBrush->SetEndPoint(D2D1::Point2F(0.f, HorizonY));
        _SkyBrush->SetOpacity((float) std::clamp(0.72 + (_SmoothedRMS * 0.03), 0., 0.78));

        renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, _Width, HorizonY), _SkyBrush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderSun(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float Side = std::min(_Width, _Height);
        const D2D1_POINT_2F Center = D2D1::Point2F(_CenterX, _Height * (float) HorizonFraction - Side * 0.035f);
        const float Radius = Side * (0.105f + (float)_HorizonPulse * 0.009f);

        _SunBrush->SetCenter(Center);
        _SunBrush->SetGradientOriginOffset(D2D1::Point2F(0.f, -Radius * 0.12f));
        _SunBrush->SetRadiusX(Radius);
        _SunBrush->SetRadiusY(Radius);
        _SunBrush->SetOpacity((float)std::clamp(0.68 + _SmoothedMid * 0.10 + _BeatPulse * 0.08, 0., 0.90));

        renderTarget->FillEllipse(D2D1::Ellipse(Center, Radius, Radius), _SunBrush.Get());

        _CoreLineBrush->SetColor(SetAlpha(_Palette.BackgroundTop, 0.72f));

        const float StripeGap = std::max(3.f, Radius * 0.12f);

        for (float y = Center.y + Radius * 0.12f; y < Center.y + Radius; y += StripeGap)
        {
            const float dy = (y - Center.y) / Radius;
            const float Half = Radius * std::sqrt(std::max(0.f, 1.f - dy * dy));
            const float Height = std::max(1.5f, StripeGap * 0.42f);

            renderTarget->FillRectangle(D2D1::RectF(Center.x - Half, y, Center.x + Half, y + Height), _CoreLineBrush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderGrid(ID2D1RenderTarget * renderTarget) noexcept
    {
        if (_Grid.empty())
            return;

        const float Beat   = (float) Clamp(_BeatPulse + _BassImpulse * 0.55);
        const float Mid    = (float) Clamp(_MidEnvelope);
        const float Treble = (float) Clamp(_TrebleImpulse);

        // Depth lines. These travel toward the viewer and carry the strongest magenta glow.
        for (size_t Row = 0; Row < _RowCount; ++Row)
        {
            for (size_t Column = 1; Column < _ColumnCount; ++Column)
            {
                const auto & a = _Grid[Row * _ColumnCount + Column - 1];
                const auto & b = _Grid[Row * _ColumnCount + Column];

                if (!a.Projectable || !b.Projectable)
                    continue;

                const float NearAmount = (float) (1. - 0.5 * (a.Depth + b.Depth));

                // Preserve distant lines so the grid converges continuously into the horizon.

                const float Perspective = 0.84f + SmoothStep(0.f, 1.f, NearAmount) * 0.16f;
                const float WaveAccent = (float) Clamp(std::abs(a.Height + b.Height) * 2.6);

                const float Intensity = Clamp(0.76f + Perspective * (0.14f + (WaveAccent * 0.16f) + (Mid * 0.18f) + (Treble * 0.08f) + (Beat * 0.08f)));
                const float Thickness = WaveAccent * 0.42f;

                const D2D1_COLOR_F Color = LerpColor(_Palette.Lateral, _Palette.Horizon, Thickness);

                DrawGlowLine(renderTarget, a.Position, b.Position, Color, Intensity, 0.44f + Perspective * 0.66f);
            }
        }

        // Longitudinal rails. Cyan edges and violet inner rails form the classic vector-grid fan.
        for (size_t Column = 0; Column < _ColumnCount; ++Column)
        {
            const double NormalizedColumn = (double) Column / (double) (_ColumnCount - 1);
            const double Edge = std::abs(NormalizedColumn * 2. - 1.);

            const D2D1_COLOR_F Color = LerpColor(_Palette.Longitudinal, _Palette.Accent, (float) (1. - Edge) * 0.38f);

            for (size_t row = 1; row < _RowCount; ++row)
            {
                const auto & a = _Grid[(row - 1) * _ColumnCount + Column];
                const auto & b = _Grid[ row      * _ColumnCount + Column];

                if (!a.Projectable || !b.Projectable)
                    continue;

                // Skip the cyclic wrap segment, which would otherwise draw a line across the whole grid.
                if (std::abs(a.ViewDepth - b.ViewDepth) > (FarPlane - NearPlane) * 0.5)
                    continue;

                const float NearAmount  = (float)(1. - 0.5 * (a.Depth + b.Depth));
                const float Perspective = 0.86f + SmoothStep(0.f, 1.f, NearAmount) * 0.14f;

                const float Intensity = Clamp(0.74f + Perspective * (0.16f + Treble * 0.18f + Mid * 0.07f + Beat * 0.07f));
                const float Thickness = 0.40f + (Perspective * 0.58f) + (Treble * 0.12f);

                DrawGlowLine(renderTarget, a.Position, b.Position, Color, Intensity, Thickness);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHorizon(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float y     = _Height * (float) HorizonFraction;
        const float Pulse = (float) Clamp(_HorizonPulse);

        const float Intensity = 0.045f + Pulse * 0.035f;
        const float Thickness = 0.220f + Pulse * 0.080f;

        DrawGlowLine(renderTarget, D2D1::Point2F(0.f, y), D2D1::Point2F(_Width, y), _Palette.Horizon, Intensity, Thickness);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderBeatFlash(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float Amount = (float) Clamp((_BeatPulse * 0.62) + (_BassImpulse * 0.52));

        if (Amount <= 0.003f)
            return;

        const D2D1_POINT_2F Center = D2D1::Point2F(_CenterX, _Height * (float) HorizonFraction);

        const float Radius = std::min(_Width, _Height) * (0.08f + (Amount * 0.30f));

        _FlashBrush->SetCenter(Center);
        _FlashBrush->SetRadiusX(Radius);
        _FlashBrush->SetRadiusY(Radius * 0.55f);
        _FlashBrush->SetOpacity(Amount * 0.36f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, Radius, Radius * 0.55f), _FlashBrush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, D2D1_COLOR_F color, float intensity, float strokeWidth) noexcept
    {
        if (!IsFinite(from) || !IsFinite(to) || intensity <= 0.001f)
            return;

        const float OuterWidth = strokeWidth * 5.4f;

        _GlowBrush->SetColor(SetAlpha(color, std::clamp(intensity * 0.30f, 0.f, 0.52f)));

        renderTarget->DrawLine(from, to, _GlowBrush.Get(), OuterWidth, _RoundStrokeStyle.Get());

        _GlowBrush->SetColor(SetAlpha(color, std::clamp(intensity * 0.68f, 0.f, 0.90f)));

        renderTarget->DrawLine(from, to, _GlowBrush.Get(), strokeWidth * 2.25f, _RoundStrokeStyle.Get());

        const D2D1_COLOR_F WhiteCore = LerpColor(color, White, 0.08f);

        _CoreLineBrush->SetColor(SetAlpha(WhiteCore, Clamp(intensity * 1.28f)));

        renderTarget->DrawLine(from, to, _CoreLineBrush.Get(), strokeWidth, _RoundStrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t WorkerCount = (size_t) std::thread::hardware_concurrency();

        try
        {
            for (size_t workerIndex = 0; workerIndex < WorkerCount; ++workerIndex)
            {
                _Workers.emplace_back([this, workerIndex, WorkerCount]()
                {
                    size_t generation = 0;
                    std::unique_lock lock(_WorkerMutex);

                    for (;;)
                    {
                        _WorkerCondition.wait(lock, [this, generation]()
                        {
                            return _IsStopping || _Generation != generation;
                        });

                        if (_IsStopping)
                            return;

                        generation = _Generation;

                        lock.unlock();

                        GenerateWork(workerIndex, WorkerCount);

                        lock.lock();

                        if (--_PendingWorkers == 0)
                            _CompletionCondition.notify_one();
                    }
                });
            }
        }
        catch (...)
        {
            StopWorkers();
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            const std::scoped_lock lock(_WorkerMutex);
            _IsStopping = true;
            ++_Generation;
        }

        _WorkerCondition.notify_all();
        for (std::thread & worker : _Workers)
        {
            if (worker.joinable())
                worker.join();
        }

        _Workers.clear();
        _PendingWorkers = 0;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateWork(size_t workerIndex, size_t workerCount) noexcept
    {
        for (size_t Row = workerIndex; Row < _RowCount; Row += workerCount)
        {
            double z = NearPlane + 0.018 + (double)Row * _GridRowStep - _GridTravelOffset;

            while (z <= NearPlane)
                z += FarPlane - NearPlane;

            for (size_t Column = 0; Column < _ColumnCount; ++Column)
            {
                const double t = (double) Column / (double) (_ColumnCount - 1);
                const double x = Lerp(-GridHalfWidth, GridHalfWidth, t);

                _Grid[Row * _ColumnCount + Column] = ProjectGridPoint(x, z);
            }
        }
    }

    /// <summary>
    /// Projects a single grid point.
    /// </summary>
    effect_t::grid_sample_t effect_t::ProjectGridPoint(double x, double z) const noexcept
    {
        grid_sample_t Result {};

        if (!(z > NearPlane && z < FarPlane) || (_Width <= 0.f) || (_Height <= 0.f))
            return Result;

        const double y           = EvaluateWave(x, z);
        const double Aspect      = (double) _Width / std::max(1.0, (double) _Height);
        const double FocalLength = 1. / std::tan(VerticalFieldOfView * Pi / 360.);
        const double CameraY     = y - CameraHeight - _CameraKick * 0.025;
        const double nx          = x * (FocalLength / Aspect) / z;
        const double ny          = CameraY * FocalLength / z;

        const double HorizonY    = (double) _Height * HorizonFraction;
        const double sx          =            (nx + 1.) * (double) _CenterX;
        const double sy          = HorizonY -  ny       * (double) _CenterY;

        if (!std::isfinite(sx) || !std::isfinite(sy) || 
            sx < -(double)std::numeric_limits<float>::max() || sx > (double)std::numeric_limits<float>::max() ||
            sy < -(double)std::numeric_limits<float>::max() || sy > (double)std::numeric_limits<float>::max())
            return Result;

        Result.Position    = D2D1::Point2F((float) sx, (float) sy);
        Result.ViewDepth   = z;
        Result.Depth       = Clamp((z - NearPlane) / (FarPlane - NearPlane));
        Result.Height      = y;
        Result.Projectable = true;

        return Result;
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if (!_ThemeResourcesDirty && _GlowBrush && _CoreLineBrush && _SkyBrush && _SunBrush && _FlashBrush && _RoundStrokeStyle)
            return S_OK;

        hr = renderTarget->CreateSolidColorBrush(_Palette.Horizon, _GlowBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(White, _CoreLineBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(_Palette.Horizon, _HorizonBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const D2D1_GRADIENT_STOP SkyStops[] =
        {
            { 0.00f, LerpColor(_Palette.BackgroundTop,    Black, 0.992f) },
            { 0.58f, LerpColor(_Palette.BackgroundTop,    Black, 0.985f) },
            { 0.86f, LerpColor(_Palette.BackgroundBottom, Black, 0.972f) },
            { 1.00f, LerpColor(_Palette.BackgroundBottom, Black, 0.950f) }
        };

        hr = renderTarget->CreateGradientStopCollection(SkyStops, _countof(SkyStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _SkyStops.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0.f, _Height * (float)HorizonFraction)), _SkyStops.Get(), _SkyBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const D2D1_GRADIENT_STOP SunStops[] =
        {
            { 0.00f, LerpColor(_Palette.Sun, White, 0.72f) },
            { 0.18f, LerpColor(_Palette.Sun, White, 0.38f) },
            { 0.55f, SetAlpha (_Palette.Sun,    0.88f) },
            { 1.00f, SetAlpha (_Palette.Accent, 0.00f) }
        };

        hr = renderTarget->CreateGradientStopCollection(SunStops, _countof(SunStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _SunStops.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0.f, 0.f), 100.f, 100.f), _SunStops.Get(), _SunBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const D2D1_GRADIENT_STOP FlashStops[] =
        {
            { 0.00f, D2D1::ColorF(0xFFFFFF, 0.88f) },
            { 0.24f, SetAlpha(_Palette.Accent, 0.54f) },
            { 0.62f, SetAlpha(_Palette.Longitudinal, 0.20f) },
            { 1.00f, SetAlpha(_Palette.BackgroundBottom, 0.00f) }
        };

        hr = renderTarget->CreateGradientStopCollection(FlashStops, _countof(FlashStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _FlashStops.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0.f, 0.f), 100.f, 100.f), _FlashStops.Get(), _FlashBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto StrokeProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(StrokeProperties, nullptr, 0, _RoundStrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _ThemeResourcesDirty = false;
        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _GlowBrush.Reset();
        _CoreLineBrush.Reset();
        _HorizonBrush.Reset();
        _SkyStops.Reset();
        _SkyBrush.Reset();
        _SunStops.Reset();
        _SunBrush.Reset();
        _FlashStops.Reset();
        _FlashBrush.Reset();
        _RoundStrokeStyle.Reset();

        __super::DeleteDeviceDependentResources();
    }
}
