
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

        for (auto & Band : _Bands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }

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
                    return 1.0;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        {
            for (auto & Band : _Bands)
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, (Band.Energy > Band.SmoothedEnergy) ? 18. : 3.5);

            _BassLevel   = (_Bands[ 0].SmoothedEnergy + _Bands[ 1].SmoothedEnergy + _Bands[ 2].SmoothedEnergy + _Bands[ 3].SmoothedEnergy) * .25;
            _MidLevel    = (_Bands[ 4].SmoothedEnergy + _Bands[ 5].SmoothedEnergy + _Bands[ 6].SmoothedEnergy + _Bands[ 7].SmoothedEnergy + _Bands[ 8].SmoothedEnergy) * .20;
            _TrebleLevel = (_Bands[ 9].SmoothedEnergy + _Bands[10].SmoothedEnergy + _Bands[11].SmoothedEnergy) * .33;
        }

        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 8.), _SpectralFlux);

        _Time += deltaTime * SpeedFactor;

        _WavePhase   += deltaTime * (.08 + _MidLevel  * .28)               * SpeedFactor;
        _RipplePhase += deltaTime * (.11 + _BassLevel * .42 + Pulse * .11) * SpeedFactor;

        const double TargetHueShift = _BassLevel * .09 + _MidLevel * .31 + _TrebleLevel * .57 + Pulse * .11;

        _HueShift           = ExponentialSmoothing(_HueShift,           TargetHueShift,                                       deltaTime, (TargetHueShift > _HueShift) ? 10. : 3.);
        _PaletteSpread      = ExponentialSmoothing(_PaletteSpread,      .14 + _MidLevel * .3 + _TrebleLevel * .26,          deltaTime, 4.5);
        _ColorIntensity     = ExponentialSmoothing(_ColorIntensity,     .78 + _RMS * .18 + _TrebleLevel * .24 + Pulse * .12, deltaTime, 5.);
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
                const double Wave      = std::sin(px * TwoPi * (2.8 + _MidLevel * .5) + _WavePhase * TwoPi + std::sin(py * TwoPi * 2.) * .7);
                const double Ripple    = std::cos(Radius * TwoPi * (6. + _BassLevel * 1.5) - _RipplePhase * TwoPi);
                const double Scan      = std::sin((px + py) * TwoPi * 2.2 - _Time * .7 + _TrebleLevel * 1.2);
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

        const double RefreshRate = (420. + _TrebleLevel * 520. + _SpectralFlux * 180.) * ((_Speed == Speed::Slow) ? .7 : (_Speed == Speed::Fast) ? 1.45 : 1.);

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
        const double CellWidth  = (double) _RasterWidth  / (double) _Columns;
        const double CellHeight = (double) _RasterHeight / (double) _Rows;

        const double DotRadius = std::min(CellWidth, CellHeight) * .39;

        const double ColumnScale = 1. / std::max<UINT32>(1, _Columns - 1);
        const double RowScale    = 1. / std::max<UINT32>(1, _Rows    - 1);

        for (UINT32 y = begin; y < end; ++y)
        {
            uint32_t * Pixel = _Pixels.data() + ((size_t) y * _RasterWidth);

            const UINT32 DotY = std::min<UINT32>((UINT32) ((double) y / CellHeight), _Rows - 1);

            const double CenterY = ((double) DotY + .5) * CellHeight;

            for (UINT32 x = 0; x < _RasterWidth; ++x)
            {
                const UINT32 DotX = std::min<UINT32>((UINT32) ((double) x / CellWidth), _Columns - 1);

                const double CenterX = ((double) DotX + .5) * CellWidth;

                const double dx = ((double) x + .5 - CenterX) / DotRadius;
                const double dy = ((double) y + .5 - CenterY) / DotRadius;

                const double Distance2     = dx * dx + dy * dy;
                const float State          = _DotState[(size_t) DotY * _Columns + DotX];
                const double FlipAngle     = (double) State * Pi;
                const double Facing        = std::abs(std::cos(FlipAngle));
                const double VisibleRadius = .24 + (Facing * .76);

                const bool Inside = Distance2 <= VisibleRadius * VisibleRadius;

                double Red = .025, Green = .03, Blue = .035;

                if (Inside)
                {
                    const double ColumnAmount = (double) DotX * ColumnScale;
                    const double RowAmount    = (double) DotY * RowScale;

                    const double BandColor  = _Bands[(DotX + DotY * 3) % BandCount].SmoothedEnergy;
                    const double SpatialHue = (ColumnAmount - .5) * _PaletteSpread + (RowAmount - .5) * _PaletteSpread * .63;

                    const double H = Fraction(.03 + _HueShift + SpatialHue + (BandColor * .24) + (_RipplePhase * .025));
                    const double S = Clamp(.72 + (_TrebleLevel * .22)     + (BandColor * .12));
                    const double V = Clamp(_ColorIntensity                 + (BandColor * .20) + (_Peak * .10));

                    const D2D1_COLOR_F Bright   = HSVToColor((float) H, (float) S, (float) V);
                    const D2D1_COLOR_F Specular = HSVToColor((float) Fraction(H + .035), .18f, 1.f);

                    const double DarkState      = SmoothStep(.48, .06, (double) State);
                    const double BrightState    = SmoothStep(.48, .94, (double) State);
                    const double Mechanical     = .055 + (DarkState * .085);

                    const double Highlight      = std::pow(Clamp(1. - std::hypot(dx + .28, dy + .32) * 1.24), 2.2);
                    const double PinHighlight   = std::pow(Clamp(1. - std::hypot(dx + .38, dy + .42) * 2.35), 3.4);

                    const double Edge           = Clamp((VisibleRadius - std::sqrt(Distance2)) * 7.);
                    const double Glow           = ((Highlight * _HighlightIntensity) + (PinHighlight * (.22 + _HighlightIntensity)) * Facing * (.45 + (BrightState * .55)));

                    Red   = (Lerp(Mechanical,        (double) Bright.r * _RenderBrightness, BrightState) * Edge) + (Specular.r * Glow);
                    Green = (Lerp(Mechanical * 1.08, (double) Bright.g * _RenderBrightness, BrightState) * Edge) + (Specular.g * Glow);
                    Blue  = (Lerp(Mechanical * 0.92, (double) Bright.b * _RenderBrightness, BrightState) * Edge) + (Specular.b * Glow);
                }
                else
                {
                    const double Rim = Clamp(1. - (std::abs(std::sqrt(Distance2) - 1.) * 8.0));

                    Red   += Rim * .035;
                    Green += Rim * .040;
                    Blue  += Rim * .045;
                }

                *Pixel++ = PackColor(Red, Green, Blue);
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

        if (_RenderTarget == renderTarget && _BackBuffer && Width == _RasterWidth && Height == _RasterHeight && Columns == _Columns && Rows == _Rows)
            return S_OK;

        DeleteDeviceDependentResources();

        _RasterWidth  = Width;
        _RasterHeight = Height;

        _Stride = _RasterWidth * BytesPerPixel;

        _Pixels.assign((size_t) Width * Height, 0xff090b0du);

        _Columns = Columns;
        _Rows    = Rows;

        const size_t DotCount = (size_t) Columns * Rows;

        _DotState   .assign(DotCount, 0.f);
        _DotVelocity.assign(DotCount, 0.f);
        _DotTarget  .assign(DotCount, 0);

        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.f, 96.f);

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

        _DotState.clear();
        _DotVelocity.clear();
        _DotTarget.clear();

        _Pixels.clear();

        _RasterWidth  = 0;
        _RasterHeight = 0;

        _Columns = 0;
        _Rows    = 0;

        __super::DeleteDeviceDependentResources();
    }
}
