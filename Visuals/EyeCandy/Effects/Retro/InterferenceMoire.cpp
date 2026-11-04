
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
        _Time = 0.;
        _AngleA = 0.;
        _AngleB = .018;
        _AngleC = -.026;
        _OffsetAX = _OffsetAY = _OffsetBX = _OffsetBY = 0.;
        _CirclePhase = _DotPhase = 0.;
        _HueShift = 0.;
        _PaletteSpread = .22;
        _Saturation = .78;
        _BandEnergy.fill(0.);
        _SmoothedBandEnergy.fill(0.);
        _PreviousBandEnergy.fill(0.);
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

        _Peak         = 0.;
        _RMS          = 0.;
        _SpectralFlux = 0.;

        if (chunk.is_empty())
        {
            _BandEnergy.fill(0.);

            return;
        }

        const size_t MaxFrames    = std::min<size_t>(chunk.get_sample_count(), 1536);
        const size_t ChannelCount = chunk.get_channel_count();
        const size_t SampleRate   = chunk.get_sample_rate();

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

        const double NyquistFrequency = (double) SampleRate * 0.5;
        const double MaximumFrequency = std::min(HiFrequency, NyquistFrequency);

        double MaximumEnergy = 0.;

        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount = (double) i / (double) (BandCount - 1); // [0, 1]

            const double Frequency = LoFrequency * std::pow(MaximumFrequency / LoFrequency, Amount);

            // Configure the Goertzel detector.
            const double Omega       = TwoPi * Frequency / (double) SampleRate;
            const double Coefficient = 2. * std::cos(Omega);

            double q1 = 0., q2 = 0.;

            for (double Sample : _Samples)
            {
                const double q0 = Sample + Coefficient * q1 - q2;

                q2 = q1;
                q1 = q0;
            }

            // Convert the detector state to power.
            const double Power = std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
            const double Magnitude = std::sqrt(Power) / (double) MaxFrames;

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

        _SpectralFlux = Clamp(_SpectralFlux / (double)BandCount * 5.5);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = (_Speed == Speed::Slow) ? .55 : (_Speed == Speed::Fast) ? 1.75 : 1.;

        for (size_t i = 0; i < BandCount; ++i)
            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], _HasAudio ? _BandEnergy[i] : 0., deltaTime, _BandEnergy[i] > _SmoothedBandEnergy[i] ? LoFrequency : 3.5);

        _BassEnergy   = (_SmoothedBandEnergy[0] + _SmoothedBandEnergy[1] + _SmoothedBandEnergy[ 2]) / 3.;
        _MidEnergy    = (_SmoothedBandEnergy[3] + _SmoothedBandEnergy[4] + _SmoothedBandEnergy[ 5] + _SmoothedBandEnergy[ 6] + _SmoothedBandEnergy[7]) * .2;
        _TrebleEnergy = (_SmoothedBandEnergy[8] + _SmoothedBandEnergy[9] + _SmoothedBandEnergy[10] + _SmoothedBandEnergy[11]) * .25;

        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 8.), _SpectralFlux);

        _Time += deltaTime * SpeedFactor;

        _AngleA += deltaTime * (.007 + _SmoothedBandEnergy[4] * .018)                * SpeedFactor;
        _AngleB += deltaTime * (.011 + _SmoothedBandEnergy[6] * .024 + Pulse * .008) * SpeedFactor;
        _AngleC -= deltaTime * (.008 + _TrebleEnergy          * .026)                * SpeedFactor;

        _OffsetAX = std::sin(_Time * .19) * (.018 + _BassEnergy * .028);
        _OffsetAY = std::cos(_Time * .17) * (.016 + _SmoothedBandEnergy[3] * .026);
        _OffsetBX = std::cos(_Time * .13) * (.021 + _MidEnergy * .032);
        _OffsetBY = std::sin(_Time * .23) * (.017 + _TrebleEnergy * .025);

        _CirclePhase += deltaTime * (.08 + _BassEnergy   * .32 + Pulse * .08) * SpeedFactor;
        _DotPhase    += deltaTime * (.06 + _TrebleEnergy * .38)               * SpeedFactor;

        const double TargetHueShift = _BassEnergy * .11 + _MidEnergy * .27 + _TrebleEnergy * .43 + Pulse * .09;

        _HueShift         = ExponentialSmoothing(_HueShift, TargetHueShift, deltaTime, TargetHueShift > _HueShift ? 11. : 3.2);
        _PaletteSpread    = ExponentialSmoothing(_PaletteSpread, .16 + _MidEnergy * .24 + _TrebleEnergy * .18, deltaTime, 4.5);
        _Saturation       = ExponentialSmoothing(_Saturation, .68 + _TrebleEnergy * .27 + Pulse * .08, deltaTime, 5.);
        _RenderBrightness = (_Brightness == Brightness::Low) ? .76 : (_Brightness == Brightness::High) ? 1.2 : .96;

        _PreviousPulse = Pulse;
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::LineGrid(double x, double y, double angle, double frequency, double phase) const noexcept
    {
        const double cs = std::cos(angle), sn = std::sin(angle);
        const double Coordinate = x * cs + y * sn;

        return .5 + .5 * std::cos((Coordinate * frequency + phase) * TwoPi);
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::CircleGrid(double x, double y, double frequency, double phase, double offset) const noexcept
    {
        const double Radius = std::hypot(x + std::sin(offset) * .08, y + std::cos(offset * 1.17) * .08);

        return .5 + .5 * std::cos((Radius * frequency - phase) * TwoPi);
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::DotScreen(double x, double y, double angle, double frequency, double phase) const noexcept
    {
        const double cs = std::cos(angle), sn = std::sin(angle);
        const double u = x * cs + y * sn;
        const double v = y * cs - x * sn;
        const double A = .5 + .5 * std::cos((u * frequency + phase) * TwoPi);
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
            RenderRows(0, _PixelHeight);
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
        const double Aspect = (double) _PixelWidth / (double) _PixelHeight;

        const double LineFrequency   = 31. + _BassEnergy * 2.8;
        const double CircleFrequency = 27. + _MidEnergy * 3.2;
        const double DotFrequency    = 21. + _TrebleEnergy * 3.8;

        for (UINT32 y = begin; y < end; ++y)
        {
            const double py = ((double) y + .5) / (double) _PixelHeight - .5;

            uint32_t * Pixel = _Pixels.data() + (size_t) y * _PixelWidth;

            for (UINT32 x = 0; x < _PixelWidth; ++x)
            {
                const double px = (((double)x + .5) / (double)_PixelWidth - .5) * Aspect;

                const double LinesA = LineGrid(px + _OffsetAX, py + _OffsetAY, _AngleA, LineFrequency, _Time * .011);
                const double LinesB = LineGrid(px - _OffsetBX, py - _OffsetBY, _AngleB, LineFrequency * (1.002 + _SmoothedBandEnergy[5] * .003), -_Time * .009);
                const double LinesC = LineGrid(px,             py,             _AngleC, LineFrequency * (1.006 + _TrebleEnergy * .004), _Time * .007);

                const double CirclesA = CircleGrid(px + _OffsetBX, py + _OffsetAY, CircleFrequency, _CirclePhase, _Time * .13);
                const double CirclesB = CircleGrid(px - _OffsetAX, py - _OffsetBY, CircleFrequency * (1.004 + _BassEnergy * .004), -_CirclePhase * .83, -_Time * .11);

                const double DotsA = DotScreen(px + _OffsetAY, py - _OffsetAX, _AngleB * .73, DotFrequency, _DotPhase);
                const double DotsB = DotScreen(px - _OffsetBY, py + _OffsetBX, _AngleC * .81, DotFrequency * (1.005 + _TrebleEnergy * .005), -_DotPhase * .79);

                const double LineInterference   = std::abs(LinesA - LinesB) * .72 + std::abs(LinesB - LinesC) * .28;
                const double CircleInterference = std::abs(CirclesA - CirclesB);
                const double DotInterference    = std::abs(DotsA - DotsB);

                const double Combined        = std::clamp(LineInterference * .46 + CircleInterference * .31 + DotInterference * .34, 0., 1.);
                const double Accent          = std::clamp(std::abs(LinesA - CirclesA) * .35 + std::abs(DotsA - LinesC) * .28, 0., 1.);
                const double Luminance       = std::pow(Combined, .72) * _RenderBrightness;
                const double LocalColorShift = (LineInterference - CircleInterference) * _PaletteSpread + (DotInterference - .5) * _TrebleEnergy * .14;

                const double H = Fraction(.56 + _Time * .006 + _HueShift + Accent * _PaletteSpread + LocalColorShift);
                const double S = std::clamp(_Saturation + Accent * .16, 0., 1.);

                const D2D1_COLOR_F Color = HSVToColor((float) H, (float) S, (float) Clamp(Luminance));

                const uint32_t R = (uint32_t) std::clamp(Color.r * 255., 0., 255.);
                const uint32_t G = (uint32_t) std::clamp(Color.g * 255., 0., 255.);
                const uint32_t B = (uint32_t) std::clamp(Color.b * 255., 0., 255.);

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

        const UINT32 Width  = std::max<UINT32>(64, (UINT32) std::ceil(_Width  * Resolution));
        const UINT32 Height = std::max<UINT32>(64, (UINT32) std::ceil(_Height * Resolution));

        if (_RenderTarget == renderTarget && _Bitmap && Width == _PixelWidth && Height == _PixelHeight)
            return S_OK;

        DeleteDeviceDependentResources();

        _PixelWidth = Width;
        _PixelHeight = Height;

        _Pixels.assign((size_t)Width * Height, 0xff000000u);

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

        _Pixels.clear();
        _PixelWidth = 0;
        _PixelHeight = 0;

        __super::DeleteDeviceDependentResources();
    }
}
