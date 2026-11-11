
/** $VER: InterferenceMoire.cpp (2026.11.02) **/

#include <pch.h>

#include "InterferenceMoire.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace InterferenceMoire
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
        return std::format(L"Interference Moire, {} quality, {} speed, {} brightness", Qualities[(size_t)_Quality], Speeds[(size_t)_Speed], Brightnesses[(size_t)_Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _PreviousPulse = 0.;
        _Time          = 0.;

        _AngleA        = 0.;
        _AngleB        =  .018;
        _AngleC        = -.026;

        _OffsetAX      =
        _OffsetAY      =
        _OffsetBX      =
        _OffsetBY      = 0.;

        _CirclePhase   =
        _DotPhase      = 0.;

        _HueShift      = 0.;
        _PaletteSpread = .22;
        _Saturation    = .78;

        for (auto & Band : _Bands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }

        std::fill(_Pixels.begin(), _Pixels.end(), 0xff000000u);
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

                Mono = ClampAudio(Mono / (double) ChannelCount);

                // Apply a Hann window.
                const double Window = .5 - .5 * std::cos(TwoPi * (double) i / ((double) MaxFrames - 1.));

               _Samples[i] = Mono * Window;

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
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, (Band.Energy > Band.SmoothedEnergy) ? 18. : 3.5);

            _BassLevel   = (_Bands[ 0].SmoothedEnergy + _Bands[ 1].SmoothedEnergy + _Bands[ 2].SmoothedEnergy + _Bands[ 3].SmoothedEnergy) * .25;
            _MidLevel    = (_Bands[ 4].SmoothedEnergy + _Bands[ 5].SmoothedEnergy + _Bands[ 6].SmoothedEnergy + _Bands[ 7].SmoothedEnergy + _Bands[ 8].SmoothedEnergy) * .20;
            _TrebleLevel = (_Bands[ 9].SmoothedEnergy + _Bands[10].SmoothedEnergy + _Bands[11].SmoothedEnergy) * .25;
        }

        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 8.), _SpectralFlux);

        _Time += deltaTime * SpeedFactor;

        _AngleA += deltaTime * (.007 + _Bands[4].SmoothedEnergy * .018)                * SpeedFactor;
        _AngleB += deltaTime * (.011 + _Bands[6].SmoothedEnergy * .024 + Pulse * .008) * SpeedFactor;
        _AngleC -= deltaTime * (.008 + _TrebleLevel            * .026)                * SpeedFactor;

        _OffsetAX = std::sin(_Time * .19) * (.018 + _BassLevel              * .028);
        _OffsetAY = std::cos(_Time * .17) * (.016 + _Bands[3].SmoothedEnergy * .026);
        _OffsetBX = std::cos(_Time * .13) * (.021 + _MidLevel               * .032);
        _OffsetBY = std::sin(_Time * .23) * (.017 + _TrebleLevel            * .025);

        _CirclePhase += deltaTime * (.08 + _BassLevel   * .32 + Pulse * .08) * SpeedFactor;
        _DotPhase    += deltaTime * (.06 + _TrebleLevel * .38)               * SpeedFactor;

        const double TargetHueShift = (_BassLevel * .11) + (_MidLevel * .27) + (_TrebleLevel * .43) + (Pulse * .09);

        _HueShift         = ExponentialSmoothing(_HueShift,      TargetHueShift,                                      deltaTime, (TargetHueShift > _HueShift) ? 11. : 3.2);
        _PaletteSpread    = ExponentialSmoothing(_PaletteSpread, .16 + (_MidLevel    * .24) + (_TrebleLevel * .18), deltaTime, 4.5);
        _Saturation       = ExponentialSmoothing(_Saturation,    .68 + (_TrebleLevel * .27) + (Pulse         * .08), deltaTime, 5.0);

        _RenderBrightness = (_Brightness == Brightness::Low) ? .76 : (_Brightness == Brightness::High) ? 1.2 : .96;

        _PreviousPulse = Pulse;
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::LineGrid(double x, double y, double angle, double frequency, double phase) const noexcept
    {
        const double cs = std::cos(angle);
        const double sn = std::sin(angle);

        const double Coordinate = (x * cs) + (y * sn);

        return .5 + (.5 * std::cos((Coordinate * frequency + phase) * TwoPi));
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::CircleGrid(double x, double y, double frequency, double phase, double offset) const noexcept
    {
        const double Radius = std::hypot(x + std::sin(offset) * .08, y + std::cos(offset * 1.17) * .08);

        return .5 + (.5 * std::cos((Radius * frequency - phase) * TwoPi));
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::DotScreen(double x, double y, double angle, double frequency, double phase) const noexcept
    {
        const double cs = std::cos(angle);
        const double sn = std::sin(angle);

        const double u = x * cs + y * sn;
        const double v = y * cs - x * sn;

        const double A = .5 + .5 * std::cos((u * frequency + phase)       * TwoPi);
        const double B = .5 + .5 * std::cos((v * frequency - phase * .73) * TwoPi);

        return std::pow(A * B, .72);
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
        const double Aspect = (double) _RasterWidth / (double) _RasterHeight;

        const double LineFrequency   = 31. + (_BassLevel   * 2.8);
        const double CircleFrequency = 27. + (_MidLevel    * 3.2);
        const double DotFrequency    = 21. + (_TrebleLevel * 3.8);

        for (UINT32 y = begin; y < end; ++y)
        {
            const double py = ((double) y + .5) / (double) _RasterHeight - .5;

            uint32_t * Pixel = _Pixels.data() + ((size_t) y * _RasterWidth);

            for (UINT32 x = 0; x < _RasterWidth; ++x)
            {
                const double px = (((double) x + .5) / (double) _RasterWidth - .5) * Aspect;

                const double LinesA   = LineGrid(px + _OffsetAX, py + _OffsetAY, _AngleA, LineFrequency, _Time * .011);
                const double LinesB   = LineGrid(px - _OffsetBX, py - _OffsetBY, _AngleB, LineFrequency * (1.002 + _Bands[5].SmoothedEnergy * .003), -_Time * .009);
                const double LinesC   = LineGrid(px,             py,             _AngleC, LineFrequency * (1.006 + _TrebleLevel * .004), _Time * .007);

                const double CirclesA = CircleGrid(px + _OffsetBX, py + _OffsetAY, CircleFrequency, _CirclePhase, _Time * .13);
                const double CirclesB = CircleGrid(px - _OffsetAX, py - _OffsetBY, CircleFrequency * (1.004 + _BassLevel * .004), -_CirclePhase * .83, -_Time * .11);

                const double DotsA    = DotScreen(px + _OffsetAY, py - _OffsetAX, _AngleB * .73, DotFrequency, _DotPhase);
                const double DotsB    = DotScreen(px - _OffsetBY, py + _OffsetBX, _AngleC * .81, DotFrequency * (1.005 + _TrebleLevel * .005), -_DotPhase * .79);

                const double LineInterference   = std::abs(LinesA - LinesB) * .72 + std::abs(LinesB - LinesC) * .28;
                const double CircleInterference = std::abs(CirclesA - CirclesB);
                const double DotInterference    = std::abs(DotsA - DotsB);

                const double CombinedInterference = std::clamp(LineInterference * .46 + CircleInterference * .31 + DotInterference * .34, 0., 1.);
                const double Accent               = std::clamp(std::abs(LinesA - CirclesA) * .35 + std::abs(DotsA - LinesC) * .28, 0., 1.);
                const double LocalColorShift      = (LineInterference - CircleInterference) * _PaletteSpread + (DotInterference - .5) * _TrebleLevel * .14;

                const double H = Fraction(.56 + (_Time * .006) + _HueShift + (Accent * _PaletteSpread) + LocalColorShift);
                const double S = std::clamp(_Saturation + (Accent * .16), 0., 1.);
                const double V = std::pow(CombinedInterference, .72) * _RenderBrightness;

                const D2D1_COLOR_F Color = HSVToColor((float) H, (float) S, (float) Clamp(V));

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

        _Pixels.assign((size_t) Width * Height, 0xFF000000u);

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

        _Pixels.clear();

        _RasterWidth  = 0;
        _RasterHeight = 0;

        __super::DeleteDeviceDependentResources();
    }
}
