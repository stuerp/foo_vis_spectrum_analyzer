
/** $VER: DitherField.cpp (2026.11.07) **/

#include <pch.h>

#include "DitherField.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace DitherField
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
        return std::format(L"Dither Field, {} quality, {} speed, {} brightness", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _PreviousPulse = 0.;
        _Time          = 0.;
        _FieldPhase    = 0.;
        _NoisePhase    = 0.;
        _HueShift      = 0.;
        _PaletteSpread = .2;

        _RadialAudio = 0.;
        _RadialScale = 1.;
        _RadialTwist = 0.;

        _DitherBlend = 0.;
        _DitherModeA = 0;
        _DitherModeB = 1;

        for (auto & Band : _Bands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }

        std::fill(_EnergyField.begin(), _EnergyField.end(), 0.f);
        std::fill(_ErrorBuffer.begin(), _ErrorBuffer.end(), 0.f);
        std::fill(_DiffusedField.begin(), _DiffusedField.end(), (uint8_t) 0);

        std::fill(_Pixels.begin(), _Pixels.end(), 0xff050609u);
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

        BuildEnergyField();
        BuildErrorDiffusion();

        RenderTexture();

        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _Stride);

            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak         = 0.;
        _RMS          = 0.;
        _SpectralFlux = 0.;

        if (chunk.is_empty())
        {
            for (auto & Band : _Bands)
                Band.Energy = 0.;

            return;
        }

        const audio_sample * const Frames = chunk.get_data();

        const auto FrameCount   = chunk.get_sample_count();
        const auto ChannelCount = chunk.get_channel_count();
        const auto SampleRate   = (double) chunk.get_sample_rate();

        // Analyze only the most recent frames of the chunk.
        const auto MaxFrames = std::min<size_t>(FrameCount, 1536);

        if (MaxFrames < 32)
            return;

        if (_Samples.size() < MaxFrames)
            _Samples.resize(MaxFrames);

        auto Samples = std::span<double>(_Samples.data(), MaxFrames);

        // Initialize the energy accumulators.
        double Sum  = 0.;           // Full-audio band

        // Process all frames.
        {
            for (size_t i = 0; i < MaxFrames; ++i)
            {
                double Mono = 0.;

                for (size_t c = 0; c < ChannelCount; ++c)
                    Mono += Frames[i * ChannelCount + c];

                Mono = ClampAudio(Mono / (double)ChannelCount);

                // Apply a Hann window.
                const double Window = .5 - .5 * std::cos(TwoPi * (double) i / ((double) MaxFrames - 1.));

                Samples[i] = Mono * Window;

                _Peak = std::max(_Peak, std::abs(Mono));

                Sum += Mono * Mono;
            }
        }

        _RMS = std::sqrt(Sum / (double) MaxFrames);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        if (!_HasAudio)
            return;

        // Perform a spectrum analysis of logarithmically spaced frequency bands.
        {
            const double NyquistFrequency = SampleRate * 0.5;
            const double MaximumFrequency = std::min(HiFrequency, NyquistFrequency);

            constexpr double TuningConstant = 90.;
            const double LogarithmicScale  = 1. / std::log1p(TuningConstant);

            double MaximumEnergy = 0.;

            for (size_t i = 0; i < BandCount; ++i)
            {
                const double BandAmount = (double) i * InverseBandCount; // [0, 1]

                double Power = 0.;

                {
                    const double Frequency = LoFrequency * std::pow(MaximumFrequency / LoFrequency, BandAmount);

                    // Configure the Goertzel detector.
                    const double Omega       = TwoPi * Frequency / SampleRate;
                    const double Coefficient = 2. * std::cos(Omega);

                    double q1 = 0., q2 = 0.;

                    for (double Sample : Samples)
                    {
                        const double q0 = Sample + Coefficient * q1 - q2;

                        q2 = q1;
                        q1 = q0;
                    }

                    // Convert the detector state to power.
                    Power = std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
                }

                const double Magnitude = std::sqrt(Power) / (double) MaxFrames;

                // Compress (logarithmic) and shape the band energy.
                _Bands[i].Energy = Clamp(std::log1p(Magnitude * TuningConstant) * LogarithmicScale);

                // Track the highest frequency band energy.
                MaximumEnergy = std::max(MaximumEnergy, _Bands[i].Energy);
            }

            // Normalize the spectrum.
            if (MaximumEnergy > 1.0e-5)
            {
                const auto Normalization = 1. / std::max(0.30, MaximumEnergy);

                for (auto & Band : _Bands)
                    Band.Energy = Clamp(Band.Energy * Normalization);
            }
        }

        // Calculate the half-wave rectified spectral flux. It responds when spectral energy increases such as during drum hits, note attacks, sudden treble bursts and new musical events.
        {
            _SpectralFlux = 0.;

            for (auto & Band : _Bands)
            {
                _SpectralFlux += std::max(0., Band.Energy - Band.PreviousEnergy);

                Band.PreviousEnergy = Band.Energy; // Remember the band energy for the next chunk.
            }

            _SpectralFlux = Clamp(_SpectralFlux / (double) BandCount * 5.5); // Boost the spectral flux artificially to make it react more strongly.
        }
    }

    /// <summary>
    /// Prepares the visual parameters.
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return 0.55;

                default:
                case Speed::Normal:
                    return 1.00;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        {
            for (auto & Band : _Bands)
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 18. : 3.5);

            _BassLevel   = (_Bands[ 0].SmoothedEnergy + _Bands[ 1].SmoothedEnergy + _Bands[ 2].SmoothedEnergy + _Bands[ 3].SmoothedEnergy) * .25;
            _MidLevel    = (_Bands[ 4].SmoothedEnergy + _Bands[ 5].SmoothedEnergy + _Bands[ 6].SmoothedEnergy + _Bands[ 7].SmoothedEnergy + _Bands[ 8].SmoothedEnergy) * .20;
            _TrebleLevel = (_Bands[ 9].SmoothedEnergy + _Bands[10].SmoothedEnergy + _Bands[11].SmoothedEnergy) * .33;
        }

        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 8.), _SpectralFlux);

        _Time += deltaTime * SpeedFactor;

        _FieldPhase += deltaTime * (.055 + _BassLevel   * .30 + _MidLevel * .18) * SpeedFactor;
        _NoisePhase += deltaTime * (.120 + _TrebleLevel * .52 + Pulse      * .17) * SpeedFactor;

        const double TargetHue = (_BassLevel * .08) + (_MidLevel * .29) + (_TrebleLevel * .55) + (Pulse * .10);

        _HueShift      = ExponentialSmoothing(_HueShift, TargetHue, deltaTime, TargetHue > _HueShift ? 9. : 3.);
        _PaletteSpread = ExponentialSmoothing(_PaletteSpread, .16 + _MidLevel * .27 + _TrebleLevel * .22, deltaTime, 4.);

        const double TargetRadialAudio = _BassLevel * .52 + _MidLevel * .31 + _TrebleLevel * .17 + Pulse * .22;

        _RadialAudio = ExponentialSmoothing(_RadialAudio, TargetRadialAudio, deltaTime, TargetRadialAudio > _RadialAudio ? 12. : 3.2);
        _RadialScale = ExponentialSmoothing(_RadialScale, 1. + _BassLevel * .18 + Pulse * .08, deltaTime, 5.);
        _RadialTwist = ExponentialSmoothing(_RadialTwist, _MidLevel * .8 + _TrebleLevel * 1.4, deltaTime, 4.5);

        constexpr double ModeDuration = 5.;
        constexpr double TransitionDuration = 1.25;

        const double Position = _Time / ModeDuration;
        const size_t Index = (size_t) std::floor(Position);
        const double Fractional = Fraction(Position);

        _DitherModeA =  Index      % DitherModeCount;
        _DitherModeB = (Index + 1) % DitherModeCount;
        _DitherBlend = SmoothStep(1. - TransitionDuration / ModeDuration, 1., Fractional);

        _RenderBrightness = (_Brightness == Brightness::Low) ? .74 : (_Brightness == Brightness::High) ? 1.2 : .96;

        _PreviousPulse = Pulse;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::BuildEnergyField() noexcept
    {
        if (_EnergyField.empty())
            return;

        const double Aspect = (double) _RasterWidth / (double) _RasterHeight;

        const double CenterX = std::sin(_Time * .17) * (.1 + _BassLevel * .1);
        const double CenterY = std::cos(_Time * .13) * (.08 + _MidLevel * .08);

        for (UINT32 y = 0; y < _RasterHeight; ++y)
        {
            const double py = ((double) y + .5) / (double) _RasterHeight - .5;

            for (UINT32 x = 0; x < _RasterWidth; ++x)
            {
                const double px = (((double) x + .5) / (double) _RasterWidth - .5) * Aspect;

                const double Radius = std::hypot(px - CenterX, py - CenterY);
                const double Angle  = std::atan2(py - CenterY, px - CenterX);

                const double ModulatedRadius = Radius * _RadialScale + std::sin(Angle * 3. + _FieldPhase * TwoPi) * _RadialAudio * .018;
                const double RadialEnvelope = std::exp(-Radius * (1.8 - _RadialAudio * .45));
                const double RadialPulse = .5 + .5 * std::cos(ModulatedRadius * TwoPi * (5.5 + _BassLevel * 2.4) - _FieldPhase * TwoPi * (1.2 + _RadialAudio * .35));
                const double RadialWarp = std::sin(ModulatedRadius * TwoPi * 2.2 - _NoisePhase * TwoPi) * _RadialAudio * .13;
                const double Wave = .5 + .5 * std::sin((px + RadialWarp) * TwoPi * (2.4 + _MidLevel * .9) + _FieldPhase * TwoPi + std::sin(py * TwoPi * 2.) * .9);
                const double Ripple = .5 + .5 * std::cos(ModulatedRadius * TwoPi * (7. + _BassLevel * 2.2) - _FieldPhase * TwoPi * 1.3);
                const double Spiral = .5 + .5 * std::sin(Angle * (4. + _TrebleLevel * 2.) + ModulatedRadius * TwoPi * (5. + _RadialTwist) - _NoisePhase * TwoPi);
                const double AudioLift = _BassLevel * .1 + _MidLevel * .11 + _TrebleLevel * .07;
                const double RadialLift = (RadialPulse - .5) * _RadialAudio * (.28 + RadialEnvelope * .22);

                _EnergyField[(size_t) y * _RasterWidth + x] = (float) std::clamp(Wave * .35 + Ripple * .32 + Spiral * .2 + RadialPulse * .13 + AudioLift + RadialLift - .13, 0., 1.);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::BuildErrorDiffusion() noexcept
    {
        if (_EnergyField.empty())
            return;

        std::copy(_EnergyField.begin(), _EnergyField.end(), _ErrorBuffer.begin());

        for (UINT32 y = 0; y < _RasterHeight; ++y)
        {
            const bool Reverse = (y & 1) != 0;

            for (UINT32 i = 0; i < _RasterWidth; ++i)
            {
                const UINT32 x = Reverse ? _RasterWidth - 1 - i : i;
                const size_t Index = (size_t)y * _RasterWidth + x;
                const float Old = Clamp(_ErrorBuffer[Index]);
                const uint8_t New = Old >= .5f ? 1 : 0;

                _DiffusedField[Index] = New;

                const float Error = Old - (float) New;
                const int Direction = Reverse ? -1 : 1;
                const int NextX = (int)x + Direction;

                if (NextX >= 0 && NextX < (int)_RasterWidth)
                    _ErrorBuffer[Index + Direction] += Error * (7.f / 16.f);

                if (y + 1 < _RasterHeight)
                {
                    const size_t Below = Index + _RasterWidth;

                    if ((int) x - Direction >= 0 && (int) x - Direction < (int) _RasterWidth)
                        _ErrorBuffer[Below - Direction] += Error * (3.f / 16.f);

                    _ErrorBuffer[Below] += Error * (5.f / 16.f);

                    if (NextX >= 0 && NextX < (int) _RasterWidth)
                        _ErrorBuffer[Below + Direction] += Error * (1.f / 16.f);
                }
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::OrderedThreshold(size_t mode, UINT32 x, UINT32 y) const noexcept
    {
        static constexpr uint8_t Bayer2[ 4] = { 0, 2, 3, 1 };
        static constexpr uint8_t Bayer4[16] = { 0, 8, 2,10, 12, 4,14, 6,  3,11, 1, 9, 15, 7,13, 5 };
        static constexpr uint8_t Bayer8[64] =
        {
             0,48,12,60, 3,51,15,63, 32,16,44,28,35,19,47,31,
             8,56, 4,52,11,59, 7,55, 40,24,36,20,43,27,39,23,
             2,50,14,62, 1,49,13,61, 34,18,46,30,33,17,45,29,
            10,58, 6,54, 9,57, 5,53, 42,26,38,22,41,25,37,21
        };
        static constexpr uint8_t Clustered4[16] = { 12, 5, 6,13, 4,0,1,7, 11,3,2,8, 15,10,9,14 };
        static constexpr uint8_t Clustered6[36] =
        { 
            34,25,21,17,29,33, 30,13, 9, 5,16,26, 22,10, 1, 0, 8,20,
            18, 6, 2, 3,11,23, 27,14, 7, 4,12,31, 35,32,24,19,28,36
        };
        static constexpr uint8_t Dispersed4[16] = { 0,12, 3,15, 8, 4,11, 7, 2,14, 1,13, 10,6,9,5 };

        switch (mode)
        {
            case 0: return ((double)Bayer2[(y & 1) * 2 + (x & 1)] + .5) / 4.;
            case 1: return ((double)Bayer4[(y & 3) * 4 + (x & 3)] + .5) / 16.;
            case 2: return ((double)Bayer8[(y & 7) * 8 + (x & 7)] + .5) / 64.;
            case 3: return ((double)Clustered4[(y & 3) * 4 + (x & 3)] + .5) / 16.;
            case 4: return ((double)Clustered6[(y % 6) * 6 + (x % 6)] + .5) / 37.;
            case 5: return ((double)Dispersed4[(y & 3) * 4 + (x & 3)] + .5) / 16.;
            case 6:
            {
                const UINT32 rx = (x + y) & 3;
                const UINT32 ry = (y + 4 - (x & 3)) & 3;

                return ((double) Bayer4[ry * 4 + rx] + .5) / 16.;
            }
            case 7:
            {
                const UINT32 rx = (x + y * 2) & 7;
                const UINT32 ry = (y + 8 - (x & 7)) & 7;

                return ((double) Bayer8[ry * 8 + rx] + .5) / 64.;
            }
            case 8:
            {
                const double cx = Fraction(((double)x + .5) / 6.) - .5;
                const double cy = Fraction(((double)y + .5) / 6.) - .5;

                return Clamp(std::hypot(cx, cy) * 1.42);
            }
            default:
            {
                const double cx = std::abs(Fraction(((double)x + .5) / 8.) - .5);
                const double cy = std::abs(Fraction(((double)y + .5) / 8.) - .5);

                return Clamp((cx + cy) * 1.05);
            }
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
            RenderRows(0, _RasterHeight);

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
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        _Workers.reserve(WorkerCount);

        try
        {
            for (size_t i = 0; i < WorkerCount; ++i)
            {
                _Workers.emplace_back([this, i]() noexcept
                {
                    Worker(i);
                });
            }
        }
        catch (...)
        {
            StopWorkers();
            throw;
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

            const UINT32 Begin = (UINT32) ((uint64_t) _RasterHeight *  workerIndex      / _Workers.size());
            const UINT32 End   = (UINT32) ((uint64_t) _RasterHeight * (workerIndex + 1) / _Workers.size());

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
        double WidthScale  = 1. / std::max<UINT32>(1, _RasterWidth  - 1);
        double HeightScale = 1. / std::max<UINT32>(1, _RasterHeight - 1);

        for (UINT32 y = begin; y < end; ++y)
        {
            size_t Index = (size_t) y * _RasterWidth;

            uint32_t * Pixel = _Pixels.data() + Index;

            for (UINT32 x = 0; x < _RasterWidth; ++x, ++Index)
            {
                const double Energy = _EnergyField[Index];

                const auto SampleMode = [&](size_t Mode) noexcept
                {
                    if (Mode + 1 == DitherModeCount)
                        return (double) _DiffusedField[Index];

                    return Energy >= OrderedThreshold(Mode, x, y) ? 1. : 0.;
                };

                const double Dither = Lerp(SampleMode(_DitherModeA), SampleMode(_DitherModeB), _DitherBlend);

                const double Column = (double) x * WidthScale;
                const double Row    = (double) y * HeightScale;

                const double H = Fraction(.56 + _HueShift + ((Column - .5) * _PaletteSpread) + ((Row - .5) * _PaletteSpread * .57));
                const double S = Clamp(.68 + _TrebleLevel * .26 + Energy * .08);
                const double V = Clamp((.045 + (Dither * .955)) * _RenderBrightness);

                const D2D1_COLOR_F Color = HSVToColor((float) H, (float) S, (float) V);

                *Pixel++ = PackColor(Color.r, Color.g, Color.b);
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

        const UINT32 Width  = std::max<UINT32>(64, (UINT32) std::ceil(_Width  * Resolution));
        const UINT32 Height = std::max<UINT32>(64, (UINT32) std::ceil(_Height * Resolution));

        if (_RenderTarget == renderTarget && _BackBuffer && Width == _RasterWidth && Height == _RasterHeight)
            return S_OK;

        DeleteDeviceDependentResources();

        _RasterWidth  = Width;
        _RasterHeight = Height;

        _Stride = _RasterWidth * BytesPerPixel;

        const size_t PixelCount = (size_t) Width * Height;

        _EnergyField.assign(PixelCount, 0.f);
        _ErrorBuffer.assign(PixelCount, 0.f);
        _DiffusedField.assign(PixelCount, 0);
        _Pixels.assign(PixelCount, 0xff050609u);

        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), USER_DEFAULT_SCREEN_DPI, USER_DEFAULT_SCREEN_DPI);

        hr = renderTarget->CreateBitmap(D2D1::SizeU(Width, Height), _Pixels.data(), Width * sizeof(uint32_t), Properties, _BackBuffer.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _RenderTarget = renderTarget;

//      Reset();

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _BackBuffer.Reset();

        _EnergyField.clear();
        _ErrorBuffer.clear();
        _DiffusedField.clear();

        _Pixels.clear();

        _RasterWidth  = 0;
        _RasterHeight = 0;

        __super::DeleteDeviceDependentResources();
    }
}
