
/** $VER: FlipDotMatrix.cpp (2026.11.02) **/

#include <pch.h>

#include "FlipDotMatrix.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace FlipDotMatrix
{
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
        return std::format(L"Flip-Dot Matrix, {} quality, {} speed, {} brightness", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _RefreshCursor      = 0;
        _RefreshAccumulator = 0.;
        _PreviousPulse      = 0.;
        _Time               = 0.;
        _WavePhase          = 0.;
        _RipplePhase        = 0.;
        _HueShift           = 0.;
        _PaletteSpread      = .18;
        _ColorIntensity     = .85;
        _HighlightIntensity = .24;

        _BandEnergy.fill(0.);
        _SmoothedBandEnergy.fill(0.);
        _PreviousBandEnergy.fill(0.);

        std::fill(_DotState.begin(), _DotState.end(), 0.f);
        std::fill(_DotVelocity.begin(), _DotVelocity.end(), 0.f);
        std::fill(_DotTarget.begin(), _DotTarget.end(), (uint8_t) 0);
        std::fill(_Pixels.begin(), _Pixels.end(), 0xff090b0du);
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
        RenderTexture();

        if (!_Bitmap || _Pixels.empty())
            return;

        const D2D1_RECT_U Source = D2D1::RectU(0, 0, _PixelWidth, _PixelHeight);

        hr = _Bitmap->CopyFromMemory(&Source, _Pixels.data(), _PixelWidth * sizeof(uint32_t));

        if (FAILED(hr))
        {
            DeleteDeviceDependentResources();
            return;
        }

        renderTarget->DrawBitmap(_Bitmap.Get(), _Rectangle, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak         = 0;
        _RMS          = 0.;
        _SpectralFlux = 0.;

        if (chunk.is_empty())
        {
            _BandEnergy.fill(0.);

            return;
        }

        const size_t MaxFrames = std::min<size_t>(chunk.get_sample_count(), 1536);

        const size_t ChannelCount = chunk.get_channel_count();
        const size_t SampleRate = chunk.get_sample_rate();

        if (MaxFrames < 32)
            return;

        _Samples.resize(MaxFrames);

        double Sum = 0.;

        // Process all frames.
        {
            const audio_sample * Frames = chunk.get_data();

            for (size_t i = 0; i < MaxFrames; ++i)
            {
                double Mono = 0.;

                for (size_t c = 0; c < ChannelCount; ++c)
                    Mono += Frames[i * ChannelCount + c];

                Mono = ClampAudio(Mono / (double)ChannelCount);

                // Apply a Hann window.
                const double Window = .5 - .5 * std::cos(TwoPi * (double) i / ((double) MaxFrames - 1.));

                _Samples[i] = Mono * Window;

                _Peak = std::max(_Peak, std::abs(Mono));

                Sum += Mono * Mono;
            }
        }

        _RMS = std::sqrt(Sum / (double)MaxFrames);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        if (!_HasAudio)
            return;

        const double NyquistFrequency = (double) SampleRate * 0.5;

        double MaximumEnergy = 0.;

        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount = (double) i / (double) (BandCount - 1);

            const double Frequency   = std::clamp(40. * std::pow(400., Amount), LoFrequency, NyquistFrequency);
            const double Coefficient = 2. * std::cos(TwoPi * Frequency / (double) SampleRate);

            double q1 = 0., q2 = 0.;

            for (double Sample : _Samples)
            {
                const double q0 = Sample + Coefficient * q1 - q2;

                q2 = q1;
                q1 = q0;
            }

            const double Power = std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
            const double Magnitude = std::sqrt(Power) / (double)MaxFrames;

            _BandEnergy[i] = Clamp(std::log1p(Magnitude * 90.) / std::log1p(90.));

            MaximumEnergy = std::max(MaximumEnergy, _BandEnergy[i]);
        }

        if (MaximumEnergy > 1.e-5)
        {
            for (double & Energy : _BandEnergy)
                Energy = Clamp(Energy / std::max(.28, MaximumEnergy));
        }

        for (size_t i = 0; i < BandCount; ++i)
        {
            _SpectralFlux += std::max(0., _BandEnergy[i] - _PreviousBandEnergy[i]);

            _PreviousBandEnergy[i] = _BandEnergy[i];
        }

        _SpectralFlux = Clamp(_SpectralFlux / (double) BandCount * 5.5);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = (_Speed == Speed::Slow) ? .55 : (_Speed == Speed::Fast) ? 1.75 : 1.;

        for (size_t i = 0; i < BandCount; ++i)
            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], _HasAudio ? _BandEnergy[i] : 0., deltaTime, _BandEnergy[i] > _SmoothedBandEnergy[i] ? 18. : 3.5);

        _BassEnergy   = (_SmoothedBandEnergy[0] + _SmoothedBandEnergy[1] + _SmoothedBandEnergy[ 2]) / 3.;
        _MidEnergy    = (_SmoothedBandEnergy[3] + _SmoothedBandEnergy[4] + _SmoothedBandEnergy[ 5] + _SmoothedBandEnergy[ 6] + _SmoothedBandEnergy[7]) * .2;
        _TrebleEnergy = (_SmoothedBandEnergy[8] + _SmoothedBandEnergy[9] + _SmoothedBandEnergy[10] + _SmoothedBandEnergy[11]) * .25;

        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 8.), _SpectralFlux);

        _Time        += deltaTime                                           * SpeedFactor;
        _WavePhase   += deltaTime * (.08 + _MidEnergy  * .28)               * SpeedFactor;
        _RipplePhase += deltaTime * (.11 + _BassEnergy * .42 + Pulse * .11) * SpeedFactor;

        const double TargetHueShift = _BassEnergy * .09 + _MidEnergy * .31 + _TrebleEnergy * .57 + Pulse * .11;

        _HueShift           = ExponentialSmoothing(_HueShift,           TargetHueShift,                                       deltaTime, (TargetHueShift > _HueShift) ? 10. : 3.);
        _PaletteSpread      = ExponentialSmoothing(_PaletteSpread,      .14 + _MidEnergy * .3 + _TrebleEnergy * .26,          deltaTime, 4.5);
        _ColorIntensity     = ExponentialSmoothing(_ColorIntensity,     .78 + _RMS * .18 + _TrebleEnergy * .24 + Pulse * .12, deltaTime, 5.);
        _HighlightIntensity = ExponentialSmoothing(_HighlightIntensity, .24 + _Peak * .34 + Pulse * .28,                      deltaTime, 8.);

        _RenderBrightness = (_Brightness == Brightness::Low) ? .72 : (_Brightness == Brightness::High) ? 1.22 : .96;

        UpdateTargets();
        UpdateDots(deltaTime);

        _PreviousPulse = Pulse;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateTargets() noexcept
    {
        if (_Columns == 0 || _Rows == 0)
            return;

        const double Aspect = (double) _Columns / (double) _Rows;

        for (UINT32 y = 0; y < _Rows; ++y)
        {
            const double py = ((double) y + .5) / (double) _Rows - .5;

            for (UINT32 x = 0; x < _Columns; ++x)
            {
                const double px = (((double) x + .5) / (double) _Columns - .5) * Aspect;

                const double Radius    = std::hypot(px - std::sin(_Time * .17) * .12, py - std::cos(_Time * .13) * .1);
                const double Wave      = std::sin(px * TwoPi * (2.8 + _MidEnergy * .5) + _WavePhase * TwoPi + std::sin(py * TwoPi * 2.) * .7);
                const double Ripple    = std::cos(Radius * TwoPi * (6. + _BassEnergy * 1.5) - _RipplePhase * TwoPi);
                const double Scan      = std::sin((px + py) * TwoPi * 2.2 - _Time * .7 + _TrebleEnergy * 1.2);
                const double Threshold = .18 - _RMS * .28;
                const double Pattern   = Wave * .43 + Ripple * .42 + Scan * .21;

                _DotTarget[(size_t)y * _Columns + x] = Pattern > Threshold ? 1 : 0;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateDots(double deltaTime) noexcept
    {
        const size_t DotCount = _DotState.size();

        if (DotCount == 0)
            return;

        const double RefreshRate = (420. + _TrebleEnergy * 520. + _SpectralFlux * 180.) * ((_Speed == Speed::Slow) ? .7 : (_Speed == Speed::Fast) ? 1.45 : 1.);

        _RefreshAccumulator += deltaTime * RefreshRate;

        size_t RefreshCount = std::min<size_t>((size_t) _RefreshAccumulator, DotCount);

        _RefreshAccumulator -= (double) RefreshCount;

        while (RefreshCount-- > 0)
        {
            const size_t Index = _RefreshCursor++ % DotCount;
            const float Target = _DotTarget[Index] ? 1.f : 0.f;

            if ((Target > .5f) != (_DotState[Index] > .5f))
                _DotVelocity[Index] += Target > .5f ? 5.5f : -5.5f;
        }

        const float Damping = (float)std::exp(-deltaTime * 13.);

        for (size_t i = 0; i < DotCount; ++i)
        {
            const float Target = _DotTarget[i] ? 1.f : 0.f;

            _DotVelocity[i] += (Target - _DotState[i]) * (float)(deltaTime * 62.);
            _DotVelocity[i] *= Damping;

            _DotState[i] = std::clamp(_DotState[i] + _DotVelocity[i] * (float)deltaTime, 0.f, 1.f);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderTexture() noexcept
    {
        if (_Pixels.empty())
            return;

        if (_Workers.empty())
        {
            RenderRows(0, _PixelHeight);

            return;
        }

        {
            std::lock_guard Lock(_WorkerMutex);

            _PendingWorkers = _Workers.size();
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        std::unique_lock Lock(_WorkerMutex);

        _CompletionCondition.wait(Lock, [this]() noexcept
        {
            return _PendingWorkers == 0;
        });
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());

        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        _Workers.reserve(WorkerCount);

        for (size_t i = 0; i < WorkerCount; ++i)
            _Workers.emplace_back(&effect_t::WorkerLoop, this, i);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            std::lock_guard Lock(_WorkerMutex);

            _IsStopping = true;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        for (std::thread & Worker : _Workers)
        {
            if (Worker.joinable())
                Worker.join();
        }

        _Workers.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::WorkerLoop(size_t workerIndex) noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
                {
                    return _IsStopping || _Generation != Generation;
                });

                if (_IsStopping)
                    return;

                Generation = _Generation;
            }

            const UINT32 Begin = (UINT32) ((uint64_t) _PixelHeight *  workerIndex      / _Workers.size());
            const UINT32 End   = (UINT32) ((uint64_t) _PixelHeight * (workerIndex + 1) / _Workers.size());

            RenderRows(Begin, End);

            {
                std::lock_guard Lock(_WorkerMutex);

                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderRows(UINT32 begin, UINT32 end) noexcept
    {
        const double CellWidth  = (double) _PixelWidth  / (double) _Columns;
        const double CellHeight = (double) _PixelHeight / (double) _Rows;

        const double DotRadius = std::min(CellWidth, CellHeight) * .39;

        for (UINT32 y = begin; y < end; ++y)
        {
            uint32_t * Pixel = _Pixels.data() + (size_t) y * _PixelWidth;

            const UINT32 DotY = std::min<UINT32>((UINT32) ((double) y / CellHeight), _Rows - 1);
            const double CenterY = ((double)DotY + .5) * CellHeight;

            for (UINT32 x = 0; x < _PixelWidth; ++x)
            {
                const UINT32 DotX = std::min<UINT32>((UINT32)((double)x / CellWidth), _Columns - 1);

                const double CenterX = ((double)DotX + .5) * CellWidth;
                const double dx = ((double)x + .5 - CenterX) / DotRadius;
                const double dy = ((double)y + .5 - CenterY) / DotRadius;
                const double Distance2 = dx * dx + dy * dy;
                const float State = _DotState[(size_t)DotY * _Columns + DotX];
                const double FlipAngle = (double)State * Pi;
                const double Facing = std::abs(std::cos(FlipAngle));
                const double VisibleRadius = .24 + Facing * .76;
                const bool Inside = Distance2 <= VisibleRadius * VisibleRadius;

                double Red = .025, Green = .03, Blue = .035;

                if (Inside)
                {
                    const double Edge = std::clamp((VisibleRadius - std::sqrt(Distance2)) * 7., 0., 1.);
                    const double Highlight = std::pow(std::clamp(1. - std::hypot(dx + .28, dy + .32) * 1.24, 0., 1.), 2.2);
                    const double PinHighlight = std::pow(std::clamp(1. - std::hypot(dx + .38, dy + .42) * 2.35, 0., 1.), 3.4);
                    const double BrightState = SmoothStep(.48, .94, (double)State);
                    const double DarkState = SmoothStep(.48, .06, (double)State);
                    const double ColumnAmount = (double)DotX / std::max<UINT32>(1, _Columns - 1);
                    const double RowAmount = (double)DotY / std::max<UINT32>(1, _Rows - 1);
                    const double BandColor = _SmoothedBandEnergy[(DotX + DotY * 3) % BandCount];
                    const double SpatialHue = (ColumnAmount - .5) * _PaletteSpread + (RowAmount - .5) * _PaletteSpread * .63;

                    const double H = Fraction(.03 + _HueShift + SpatialHue + BandColor * .24 + _RipplePhase * .025);
                    const double S = std::clamp(.72 + _TrebleEnergy * .22 + BandColor * .12, 0., 1.);
                    const double V = std::clamp(_ColorIntensity + BandColor * .2 + _Peak * .1, 0., 1.);

                    const D2D1_COLOR_F Bright   = HSVToColor((float) H, (float) S, (float) V);
                    const D2D1_COLOR_F Specular = HSVToColor((float) Fraction(H + .035), .18f, 1.f);

                    const double Mechanical = .055 + DarkState * .085;
                    const double Glow = (Highlight * _HighlightIntensity + PinHighlight * (_HighlightIntensity + .22)) * Facing * (.45 + BrightState * .55);

                    Red   = Lerp(Mechanical,        (double) Bright.r * _RenderBrightness, BrightState) * Edge + Glow * Specular.r;
                    Green = Lerp(Mechanical * 1.08, (double) Bright.g * _RenderBrightness, BrightState) * Edge + Glow * Specular.g;
                    Blue  = Lerp(Mechanical * .92,  (double) Bright.b * _RenderBrightness, BrightState) * Edge + Glow * Specular.b;
                }
                else
                {
                    const double Rim = std::clamp(1. - std::abs(std::sqrt(Distance2) - 1.) * 8., 0., 1.);

                    Red   += Rim * .035;
                    Green += Rim * .040;
                    Blue  += Rim * .045;
                }

                const uint32_t R = (uint32_t)std::clamp(Red   * 255., 0., 255.);
                const uint32_t G = (uint32_t)std::clamp(Green * 255., 0., 255.);
                const uint32_t B = (uint32_t)std::clamp(Blue  * 255., 0., 255.);

                *Pixel++ = 0xFF000000u | (R << 16) | (G << 8) | B;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        const double Resolution = (_Quality == Quality::Low) ? .34 : (_Quality == Quality::High) ? .67 : .50;

        const UINT32 Width   = std::max<UINT32>(64, (UINT32) std::ceil(_Width  * Resolution));
        const UINT32 Height  = std::max<UINT32>(64, (UINT32) std::ceil(_Height * Resolution));
        const UINT32 Rows    = (_Quality == Quality::Low) ? 22u : (_Quality == Quality::High) ? 38u : 30u;
        const UINT32 Columns = std::max<UINT32>(24, (UINT32) std::round((double) Rows * (double) Width / (double) Height));

        if (_RenderTarget == renderTarget && _Bitmap && Width == _PixelWidth && Height == _PixelHeight && Columns == _Columns && Rows == _Rows)
            return S_OK;

        DeleteDeviceDependentResources();

        _PixelWidth  = Width;
        _PixelHeight = Height;
        _Columns     = Columns;
        _Rows        = Rows;

        const size_t DotCount = (size_t) Columns * Rows;

        _DotState.assign(DotCount, 0.f);
        _DotVelocity.assign(DotCount, 0.f);
        _DotTarget.assign(DotCount, 0);
        _Pixels.assign((size_t) Width * Height, 0xff090b0du);

        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.f, 96.f);

        hr = renderTarget->CreateBitmap(D2D1::SizeU(Width, Height), _Pixels.data(), Width * sizeof(uint32_t), Properties, _Bitmap.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _RenderTarget = renderTarget;

        Reset();

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _Bitmap.Reset();
        _DotState.clear();
        _DotVelocity.clear();
        _DotTarget.clear();
        _Pixels.clear();
        _PixelWidth = _PixelHeight = _Columns = _Rows = 0;

        __super::DeleteDeviceDependentResources();
    }
}
