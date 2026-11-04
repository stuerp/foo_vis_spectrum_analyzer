
/** $VER: FlowerScope.cpp (2026.11.02) **/

#include <pch.h>

#include "FlowerScope.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace FlowerScope
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
        return std::format(L"Rose Curve Scope, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const size_t ChannelCount = chunk.get_channel_count();
        const channel_pair_t SelectedChannels{ 0, ChannelCount > 1 ? 1u : 0u };

        Render(renderTarget, deltaTime, chunk, SelectedChannels);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & channels) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = Clamp(deltaTime);

        AnalyzeAudio(chunk, channels);
        Update(deltaTime);

        GenerateCurves();

        {
            _BackBufferRenderTarget->BeginDraw();

            Fade(_BackBufferRenderTarget.Get(), deltaTime);
            RenderCurves(_BackBufferRenderTarget.Get());

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();
                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
    }

    /// <summary>
    /// Calculates logarithmic spectral bands, broadband rhythm and stereo phase relationships.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels)
    {
        if (chunk.is_empty())
        {
            for (auto & Band : _Bands)
                Band.Energy = 0.;

            _Rhythm = 0.;
            _StereoCorrelation = 1.;
            _StereoPhase = 0.;

            return;
        }

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();

        _SampleRate = (double) chunk.get_sample_rate();

        if ((channels[0] >= ChannelCount) || (channels[1] >= ChannelCount) || (FrameCount < 16))
            return;

        const size_t MaxFrames = std::min<size_t>(FrameCount, (_Quality == EyeCandy::Quality::High) ? 1024 : 512);
        const size_t FrameStep = std::max<size_t>(1, FrameCount / MaxFrames);

        _LeftSamples .resize(MaxFrames);
        _RightSamples.resize(MaxFrames);

        double LeftSquare  = 0.;
        double RightSquare = 0.;
        double Cross       = 0.;
        double Quadrature  = 0.;
        double MeanSquare  = 0.;
        double Difference  = 0.;
        double Previous    = 0.;

        {
            const double WindowScale = TwoPi / (double)(MaxFrames - 1);

            const audio_sample * Frames = chunk.get_data();

            for (size_t i = 0; i < MaxFrames; ++i)
            {
                const size_t FrameIndex = std::min(i * FrameStep, FrameCount - 1) * ChannelCount;

                const double Left  = ClampAudio(Frames[FrameIndex + channels[0]]);
                const double Right = ClampAudio(Frames[FrameIndex + channels[1]]);

                const double Mono = (Left + Right) * .5;
                const double Window = .5 - std::cos((double) i * WindowScale) * .5;

                _LeftSamples[i]  = Left * Window;
                _RightSamples[i] = Right * Window;

                LeftSquare  += Left * Left;
                RightSquare += Right * Right;

                Cross += Left * Right;

                if (i > 0)
                    Quadrature += Left * _RightSamples[i - 1] - Right * _LeftSamples[i - 1];

                MeanSquare += Mono * Mono;
                Difference += std::abs(Mono - Previous);

                Previous = Mono;
            }
        }

        _Rhythm = Clamp(std::sqrt(MeanSquare / (double)MaxFrames) * 4.8 + Difference / (double)MaxFrames * 2.2);

        const double StereoMagnitude = std::sqrt(LeftSquare * RightSquare);

        _StereoCorrelation = StereoMagnitude > 1.e-9 ? std::clamp(Cross / StereoMagnitude, -1., 1.) : 1.;
        _StereoPhase = std::atan2(Quadrature, Cross) / Pi;

        const double MaxFrequency = std::min(HiFrequency, _SampleRate * 0.5);

        std::array<double, BandCount> RawEnergies{ };

        double WeightedBand = 0.;
        double EnergySum    = 0.;

        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount = (double) i / (double) (BandCount - 1);
            const double CenterFrequency = LoFrequency * std::pow(MaxFrequency / LoFrequency, Amount);
            const double AngularStep = TwoPi * CenterFrequency / _SampleRate;

            double Real      = 0.;
            double Imaginary = 0.;

            for (size_t j = 0; j < MaxFrames; ++j)
            {
                const double Mono = (_LeftSamples[j] + _RightSamples[j]) * .5;
                const double Angle = AngularStep * (double)j;

                Real      += Mono * std::cos(Angle);
                Imaginary -= Mono * std::sin(Angle);
            }

            const double Magnitude = 2. * std::sqrt(Real * Real + Imaginary * Imaginary) / (double)MaxFrames;
            const double FrequencyGain = 1.65 + 4.1 * std::pow(Amount, 1.18);

            RawEnergies[i] = Clamp(std::pow(Magnitude * FrequencyGain * 7., .74));

            WeightedBand += RawEnergies[i] * (double) i;
            EnergySum    += RawEnergies[i];
        }

        for (size_t i = 0; i < BandCount; ++i)
        {
            const size_t PreviousBand = i > 0 ? i - 1 : i;
            const size_t NextBand = i + 1 < BandCount ? i + 1 : i;

            _Bands[i].Energy = RawEnergies[PreviousBand] * .2 + RawEnergies[i] * .6 + RawEnergies[NextBand] * .2;
        }

        _HarmonicCenter = (EnergySum > 1.e-9) ? WeightedBand / EnergySum / (double) (BandCount - 1) : 0.;
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Update(double deltaTime)
    {
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return .55;

                default:
                case Speed::Normal:
                    return 1.;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        for (auto & Band : _Bands)
            Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 11. : 2.6);

        _SmoothedRhythm   = ExponentialSmoothing(_SmoothedRhythm, _Rhythm, deltaTime, _Rhythm > _SmoothedRhythm ? 15. : 3.2);
        _RhythmPulse      = std::max(_SmoothedRhythm, _RhythmPulse * std::pow(.86, deltaTime * 60.));
        _SmoothedHarmonic = ExponentialSmoothing(_SmoothedHarmonic, _HarmonicCenter, deltaTime, 2.4);

        const double BassEnergy = (_Bands[0].SmoothedEnergy + _Bands[1].SmoothedEnergy + _Bands[2].SmoothedEnergy) / 3.;

        _BassGlow            = ExponentialSmoothing(_BassGlow, BassEnergy, deltaTime, BassEnergy > _BassGlow ? 14. : 3.);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _StereoCorrelation, deltaTime, 4.2);
        _SmoothedPhase       = ExponentialSmoothing(_SmoothedPhase, _StereoPhase, deltaTime, 4.8);

        _Rotation    += deltaTime * SpeedFactor * (.08 + _SmoothedRhythm * .42);
        _PaletteTime += deltaTime;

        constexpr double PaletteDuration    = 4.5;
        constexpr double TransitionDuration = 1.5;

        const double PalettePosition = _PaletteTime / PaletteDuration;
        const size_t PaletteIndex    = (size_t) std::floor(PalettePosition);
        const double PaletteFraction = Fraction(PalettePosition);

        _PaletteA =  PaletteIndex      % PaletteCount;
        _PaletteB = (PaletteIndex + 1) % PaletteCount;
        _PaletteBlend = SmoothStep(1. - TransitionDuration / PaletteDuration, 1., PaletteFraction);

        _FlowerTime += deltaTime;

        constexpr double FlowerDuration = 7.;
        constexpr double FlowerTransitionDuration = 2.;

        const double FlowerPosition = _FlowerTime / FlowerDuration;
        const size_t FlowerIndex    = (size_t) std::floor(FlowerPosition);
        const double FlowerFraction = Fraction(FlowerPosition);

        _FlowerA  = FlowerIndex      % FlowerCount;
        _FlowerB = (FlowerIndex + 1) % FlowerCount;

        _FlowerBlend = SmoothStep(1. - FlowerTransitionDuration / FlowerDuration, 1., FlowerFraction);
    }

    /// <summary>
    /// Generates nested polar rose curves. Harmonic content selects petal count while stereo phase bends and twists the petals.
    /// </summary>
    void effect_t::GenerateCurves() noexcept
    {
        _CurvePointCount = (_Quality == EyeCandy::Quality::Low) ? 320u : (_Quality == EyeCandy::Quality::High) ? 960u : 640u;

        const double MotionTime     = _AnimationTime * .23;
        const double MotionX        = std::sin(MotionTime * 3.) * .58 + std::sin(MotionTime * 5. +  .7) * .28 + std::cos(MotionTime * 7. - .3) * .14;
        const double MotionY        = std::cos(MotionTime * 2.) * .56 + std::sin(MotionTime * 4. + 1.1) * .29 + std::sin(MotionTime * 9. + .2) * .15;

        const double MaximumOffsetX = _Width * .13;
        const double MaximumOffsetY = _Height * .12;

        _FlowerCenterX = _CenterX + MotionX * MaximumOffsetX;
        _FlowerCenterY = _CenterY + MotionY * MaximumOffsetY;

        const double HorizontalSpace = std::min(_FlowerCenterX, _Width -  _FlowerCenterX);
        const double VerticalSpace   = std::min(_FlowerCenterY, _Height - _FlowerCenterY);

        _CurveExtent      = std::max(1., std::min(HorizontalSpace, VerticalSpace) * 1.16);
        _CurveBasePetals  = 2u + (size_t)std::lround(_SmoothedHarmonic * 8.);
        _CurveStereoWidth = std::sqrt(std::clamp((1. - _SmoothedCorrelation) * .5, 0., 1.));

        _Curves.resize(RenderCurveCount);

        if (_Workers.empty())
        {
            for (size_t i = 0; i < RenderCurveCount; ++i)
                GenerateCurve(i);

            return;
        }

        {
            std::lock_guard Lock(_WorkerMutex);

            _PendingWorkers = _Workers.size();
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        {
            std::unique_lock Lock(_WorkerMutex);

            _CompletionCondition.wait(Lock, [this]() noexcept
            {
                return _PendingWorkers == 0;
            });
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateCurve(size_t curveIndex) noexcept
    {
        const size_t LayerIndex    = curveIndex / 2;
        const size_t Variant       = curveIndex & 1;
        const size_t Flower        = Variant == 0 ? _FlowerA : _FlowerB;
        const double FlowerOpacity = Variant == 0 ? 1. - _FlowerBlend : _FlowerBlend;

        auto & Curve = _Curves[curveIndex];

        Curve.Points.resize(_CurvePointCount + 1);

        const double Layer      = (double) LayerIndex / (double) (CurveCount - 1);
        const double BandEnergy = _Bands[(LayerIndex * 2 + _CurveBasePetals) % BandCount].SmoothedEnergy;
        const double Response   = std::pow(BandEnergy, .72);
        const size_t Petals     = std::max<size_t>(2, _CurveBasePetals + FlowerPetalOffset(Flower) + LayerIndex % 3);
        const double Radius     = _CurveExtent * (.48 + Layer * .12 + Response * .18 + _RhythmPulse * .08);
        const double Twist      = _SmoothedPhase * (1.2 + Layer * 2.6) + _CurveStereoWidth * (.35 + Layer * .8);
        const double Bend       = _SmoothedPhase * (.12 + Layer * .19);
        const double Turns      = (Petals & 1u) ? TwoPi : Pi * 2.;

        for (size_t j = 0; j <= _CurvePointCount; ++j)
        {
            const double Amount = (double) j / (double) _CurvePointCount;

            const double Theta      = Amount * Turns;
            const double Petal      = FlowerPattern(Flower, Theta, (double)Petals, Twist, Layer);
            const double StereoBend = 1. + Bend * std::sin(Theta * ((double)Petals + 1.) - _Rotation * 1.7);
            const double Radial     = Radius * Petal * StereoBend;
            const double Rotation   = _Rotation * (.38 + Layer * .16) + _SmoothedPhase * Layer;

            const double X = Radial * std::cos(Theta + Rotation) + Radius * _CurveStereoWidth * .08 * std::sin(Theta * 3. + Twist);
            const double Y = Radial * std::sin(Theta + Rotation) + Radius * _SmoothedPhase    * .08 * std::cos(Theta * 2. - Twist);

            Curve.Points[j] = D2D1::Point2F((float)(_FlowerCenterX + X), (float)(_FlowerCenterY + Y));
        }

        const double ColorValue = Fraction(_SmoothedHarmonic + Layer * .36 + _SmoothedPhase * .11 + _AnimationTime * .006);

        const D2D1_COLOR_F ColorA = Colorize(_PaletteA, ColorValue);
        const D2D1_COLOR_F ColorB = Colorize(_PaletteB, ColorValue);

        Curve.Color     = D2D1::ColorF((float) Lerp((double) ColorA.r, (double) ColorB.r, _PaletteBlend), (float) Lerp((double) ColorA.g, (double) ColorB.g, _PaletteBlend), (float) Lerp((double) ColorA.b, (double) ColorB.b, _PaletteBlend), 1.f);
        Curve.Width     = (float) (      .58 + Response * 2.8 + _RhythmPulse * .92 + (1. - Layer) * .32);
        Curve.Intensity = (float) (Clamp(.13 + Response * .56 + _RhythmPulse * .20 + (1. - Layer) * .09) * FlowerOpacity);

        const auto GlowProfile = [](size_t Palette) noexcept
        {
            switch (Palette)
            {
                case 0: return std::array<double, 5> { 12., 4.5, .065, .24, 1.08 };
                case 1: return std::array<double, 5> { 15., 6., .08, .28, 1.12 };
                case 2: return std::array<double, 5> { 9., 3.5, .055, .2, 1.18 };
                case 3: return std::array<double, 5> { 13., 5., .07, .25, 1.05 };
                case 4: return std::array<double, 5> { 17., 7., .085, .3, 1.06 };
                case 5: return std::array<double, 5> { 11., 4., .06, .23, 1.12 };
                case 6: return std::array<double, 5> { 8., 3., .045, .18, 1.22 };
                case 7: return std::array<double, 5> { 14., 5.5, .075, .27, 1.1 };
                case 8: return std::array<double, 5> { 18., 7.5, .09, .31, 1.04 };
                case 9: return std::array<double, 5> { 10., 4., .05, .2, 1.2 };
                case 10:return std::array<double, 5> { 13., 5., .07, .25, 1.14 };
                default:return std::array<double, 5> { 16., 6.5, .08, .29, 1.03 };
            }
        };

        const auto GlowA = GlowProfile(_PaletteA);
        const auto GlowB = GlowProfile(_PaletteB);

        const double LayerWave = .72 + .28 * std::sin((double)LayerIndex * 1.61803398875 + _Rotation * 2.);
        const double LayerGlow = (.78 + LayerWave * .42) * (1. + Response * .2);

        const double BassBloom = 1. + _BassGlow * (1.15 + (1. - Layer) * .70);
        const double BassCore  = 1. + _BassGlow * (0.28 +       Layer  * .16);

        Curve.OuterGlow    = (float) (Lerp(GlowA[0], GlowB[0], _PaletteBlend) * LayerGlow * BassBloom);
        Curve.InnerGlow    = (float) (Lerp(GlowA[1], GlowB[1], _PaletteBlend) * (.85 + LayerWave * .3) * (1. + _BassGlow * .72));

        Curve.OuterOpacity = (float) Clamp(Lerp(GlowA[2], GlowB[2], _PaletteBlend) * (.72 + LayerWave * .55) * (1. + _BassGlow * .55));
        Curve.InnerOpacity = (float) Clamp(Lerp(GlowA[3], GlowB[3], _PaletteBlend) * (.82 + LayerWave * .36) * (1. + _BassGlow * .32));
        Curve.CoreOpacity  = (float) Clamp(Lerp(GlowA[4], GlowB[4], _PaletteBlend) * BassCore);
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::FlowerPattern(size_t flower, double theta, double petals, double twist, double layer) const noexcept
    {
        const double Base = std::cos(petals * theta + twist * std::sin(theta * 2. + _Rotation));
        const double Secondary = std::cos((petals + 2.) * theta - twist * .73);

        switch (flower)
        {
            case 0: // Classic polar rose.
                return Base;

            case 1: // Rounded daisy.
                return std::copysign(std::pow(std::abs(Base), .58), Base) * (.92 + .08 * std::cos(theta * petals * 2.));

            case 2: // Pointed star flower.
                return std::copysign(std::pow(std::abs(Base), 1.75), Base) * (1. + .1 * std::cos(theta * petals));

            case 3: // Layered chrysanthemum.
                return std::clamp(Base * .72 + std::cos((petals * 2. + 1.) * theta - twist) * .28, -1., 1.);

            case 4: // Orchid-like asymmetric petals.
                return std::clamp(Base * (.78 + .22 * std::sin(theta + twist)) + std::sin(theta * 3. - _Rotation) * .14, -1., 1.);

            case 5: // Lotus with secondary inner petals.
                return std::clamp(Base * .76 + Secondary * .24 * (1. - layer * .35), -1., 1.);

            case 6: // Sunflower ripple.
                return std::clamp(Base * (.82 + .18 * std::cos(theta * petals * 3. + twist)), -1., 1.);

            case 7: // Clover lobes.
                return std::copysign(std::sqrt(std::abs(Base)), Base) * (.84 + .16 * std::cos(theta * 2.));

            case 8: // Spiral pinwheel.
                return std::cos(petals * theta + twist + theta * (.7 + layer * .9) + std::sin(theta * 3.) * .18);

            case 9: // Double rose.
                return std::clamp(Base * .66 + std::cos((petals + .5) * theta - twist * .7) * .34, -1., 1.);

            case 10: // Tulip cup.
                return std::clamp(std::copysign(std::pow(std::abs(Base), .72), Base) * (.76 + .24 * std::cos(theta)), -1., 1.);

            case 11: // Dahlia layers.
                return std::clamp(Base * .58 + Secondary * .24 + std::cos((petals * 3. - 1.) * theta + twist) * .18, -1., 1.);

            case 12: // Poppy with broad irregular petals.
                return std::clamp(std::copysign(std::pow(std::abs(Base), .42), Base) * (.88 + .12 * std::sin(theta * 3. + twist)), -1., 1.);

            case 13: // Iris blades.
                return std::clamp(std::copysign(std::pow(std::abs(Base), 2.25), Base) + Secondary * .16, -1., 1.);

            case 14: // Camellia spiral layers.
                return std::clamp(Base * .68 + std::cos((petals + 1.) * theta + theta * .9 - twist) * .32, -1., 1.);

            case 15: // Bell flower.
                return std::clamp(Base * (.7 + .3 * std::cos(theta * .5 + Pi * .5)) + std::sin(theta) * .12, -1., 1.);

            case 16: // Anemone fine crown.
                return std::clamp(Base * .72 + std::cos((petals * 2. + 3.) * theta + twist * .5) * .2 + std::sin(theta * 5.) * .08, -1., 1.);

            default: // Peony, dense and ruffled.
                return std::clamp(std::copysign(std::pow(std::abs(Base), .66), Base) * .62 + Secondary * .22 + std::cos((petals * 3. + 2.) * theta - twist) * .16, -1., 1.);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    size_t effect_t::FlowerPetalOffset(size_t flower) const noexcept
    {
        static constexpr std::array<size_t, FlowerCount> Offsets{ 0, 1, 2, 3, 1, 2, 4, 0, 3, 2, 1, 4, 0, 2, 3, 1, 4, 5 };

        return Offsets[flower % FlowerCount];
    }

    /// <summary>
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        _Workers.reserve(WorkerCount);

        for (size_t i = 0; i < WorkerCount; ++i)
        {
            _Workers.emplace_back(&effect_t::Worker, this, i);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            const std::lock_guard Lock(_WorkerMutex);

            _IsStopping = true;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        for (auto & Worker : _Workers)
        {
            if (Worker.joinable())
                Worker.join();
        }

        _Workers.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Worker(size_t workerIndex) noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                Generation = _Generation;
            }

            for (size_t i = workerIndex; i < RenderCurveCount; i += _Workers.size())
                GenerateCurve(i);

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
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Decay = std::pow(.928, deltaTime * 60.);

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderCurves(ID2D1RenderTarget * renderTarget) noexcept
    {
        for (const auto & Curve : _Curves)
            DrawSmoothCurve(renderTarget, Curve);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawSmoothCurve(ID2D1RenderTarget * renderTarget, const curve_t & curve) noexcept
    {
        if (curve.Points.size() < 4)
            return;

        ComPtr<ID2D1PathGeometry> Geometry;

        if (FAILED(_D2DFactory->CreatePathGeometry(Geometry.GetAddressOf())))
            return;

        ComPtr<ID2D1GeometrySink> Sink;

        if (FAILED(Geometry->Open(Sink.GetAddressOf())))
            return;

        Sink->BeginFigure(curve.Points.front(), D2D1_FIGURE_BEGIN_HOLLOW);

        for (size_t i = 0; i + 1 < curve.Points.size(); ++i)
        {
            const D2D1_POINT_2F P0 = i > 0 ? curve.Points[i - 1] : curve.Points[i];
            const D2D1_POINT_2F P1 = curve.Points[i];
            const D2D1_POINT_2F P2 = curve.Points[i + 1];
            const D2D1_POINT_2F P3 = i + 2 < curve.Points.size() ? curve.Points[i + 2] : P2;

            Sink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(P1.x + (P2.x - P0.x) / 6.f, P1.y + (P2.y - P0.y) / 6.f), D2D1::Point2F(P2.x - (P3.x - P1.x) / 6.f, P2.y - (P3.y - P1.y) / 6.f), P2));
        }

        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

        if (FAILED(Sink->Close()))
            return;

        SetBrush(curve.Color, curve.Intensity * curve.OuterOpacity);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width + curve.OuterGlow, _StrokeStyle.Get());

        SetBrush(curve.Color, curve.Intensity * curve.InnerOpacity);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width + curve.InnerGlow, _StrokeStyle.Get());

        SetBrush(curve.Color, curve.Intensity * curve.CoreOpacity);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width, _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(const D2D1_COLOR_F & color, float opacity) noexcept
    {
        _Brush->SetColor(color);
        _Brush->SetOpacity(Clamp(opacity));
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::Colorize(size_t palette, double value) const noexcept
    {
        value = Fraction(value);

        switch (palette)
        {
            case 0:  return HSVToColor((float)Fraction(.55 + value * .2), .88f, 1.f);
            case 1:  return HSVToColor((float)Fraction(.92 + value * .16), .9f, 1.f);
            case 2:  return HSVToColor((float)Fraction(.08 + value * .12), .94f, 1.f);
            case 3:  return HSVToColor((float)Fraction(.3 + value * .25), .82f, 1.f);
            case 4:  return HSVToColor((float)Fraction(.68 + value * .2), .9f, 1.f);
            case 5:  return HSVToColor((float)Fraction(.5 + value * .5), .76f, 1.f);
            case 6:  return D2D1::ColorF((float)(1. - value * .12), (float)(.22 + value * .7), (float)(.04 + value * .12));
            case 7:  return D2D1::ColorF((float)(.05 + value * .15), (float)(.62 + value * .36), (float)(.5 + value * .46));
            case 8:  return D2D1::ColorF((float)(.38 + value * .62), (float)(.03 + value * .2), (float)(.55 + value * .44));
            case 9:  return D2D1::ColorF((float)(.7 + value * .3), (float)(.67 + value * .33), (float)(.58 + value * .42));
            case 10: return HSVToColor((float)Fraction(.02 + value * .95), .98f, 1.f);
            default: return D2D1::ColorF((float)std::pow(value, .55), (float)std::pow(value, 1.2), (float)std::pow(value, 2.1));
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

        if ((_RenderTarget == renderTarget) && _Brush && _StrokeStyle && _BackBufferRenderTarget && _BackBuffer)
        {
            const D2D1_SIZE_U Size = _BackBufferRenderTarget->GetPixelSize();

            if ((Size.width == (UINT32)_Width) && (Size.height == (UINT32)_Height))
                return S_OK;
        }

        hr = renderTarget->CreateSolidColorBrush(Black, _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const D2D1_SIZE_U RequiredPixelSize = D2D1::SizeU(std::max<UINT32>(1u, (UINT32)std::ceil(_Width)), std::max<UINT32>(1u, (UINT32)std::ceil(_Height)));
        const D2D1_SIZE_F DesiredSize       = D2D1::SizeF(_Width, _Height);

        hr = renderTarget->CreateCompatibleRenderTarget(&DesiredSize, &RequiredPixelSize, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, _BackBufferRenderTarget.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _BackBufferRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _BackBufferRenderTarget->BeginDraw();
        _BackBufferRenderTarget->Clear(Black);

        hr = _BackBufferRenderTarget->EndDraw();

        if (FAILED(hr))
            return hr;

        hr = _BackBufferRenderTarget->GetBitmap(_BackBuffer.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _StrokeStyle.Reset();
        _Brush.Reset();

        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        __super::DeleteDeviceDependentResources();
    }
}
