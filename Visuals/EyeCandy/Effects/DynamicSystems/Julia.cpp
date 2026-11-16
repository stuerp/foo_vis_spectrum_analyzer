
/** $VER: Julia.cpp (2026.11.13) **/

#include <pch.h>

#include "Julia.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <thread>

// https://github.com/rastshawn/julia-visualizer
// https://github.com/rastshawn/julia
// https://github.com/Spudd86/julia-vis

namespace Julia
{
    effect_t::effect_t()
    {
        StartWorkers();
    }

    effect_t::~effect_t()
    {
        StopWorkers();
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Julia, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        _Time          = 0.;
        _Zoom          = 1.;
        _ZoomOffset    = 0.;
        _Rotation      = 0.;
        _PaletteOffset = 0.;
        _Real          = -0.70;
        _Imaginary     =  0.27015;

        _RasterWidth  = std::max((uint32_t) std::lround(_Width), 1u);
        _RasterHeight = std::max((uint32_t) std::lround(_Height), 1u);

        _Pixels.resize((size_t) _RasterWidth * _RasterHeight);

        std::fill(_Pixels.begin(), _Pixels.end(), 0xFF000000u);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        if (FAILED(CreateDeviceDependentResources(renderTarget)))
            return;

        deltaTime = Clamp(deltaTime);

        AnalyzeAudio(chunk);
        Update(deltaTime);

        if (FAILED(_Bitmap->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * sizeof(UINT32))))
        {
            DeleteDeviceDependentResources();

            return;
        }

        renderTarget->DrawBitmap(_Bitmap.Get(), _ViewPort, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::OnMouseWheel(short wheelDelta) noexcept
    {
        if (wheelDelta == 0)
            return;

        const double WheelSteps = (double) wheelDelta / (double) WHEEL_DELTA;

        _ZoomOffset = std::clamp(_ZoomOffset + (WheelSteps * 0.10), -0.75, 3.0);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk) noexcept
    {
        _HasAudio = false;

        if (chunk.is_empty())
        {
            _RMS         *= .98;

            _BassLevel   *= .98;
            _MidLevel    *= .98;
            _TrebleLevel *= .98;

            return;
        }

        const auto FrameCount   = chunk.get_sample_count();
        const auto ChannelCount = chunk.get_channel_count();

        _SampleRate   = (double) chunk.get_sample_rate();

        const size_t MaxFrames = std::min((size_t) 1024, FrameCount);
        const audio_sample * const Frames = chunk.get_data() + ((FrameCount - MaxFrames) * ChannelCount);

        const double WindowStep = TwoPi / (double) (MaxFrames - 1);

        double Sum = 0.;

        {
            for (size_t i = 0; i < MaxFrames; ++i)
            {
                double Mono = 0.;

                for (size_t Channel = 0; Channel < ChannelCount; ++Channel)
                    Mono += (double) Frames[i * ChannelCount + Channel];

                Mono /= (double) ChannelCount;

                Sum += Mono * Mono;
            }
        }

        double RMS = std::sqrt(Sum / (double) MaxFrames);

        double Bass   = 0.;
        double Mid    = 0.;
        double Treble = 0.;

        {
            const double Frequencies[] = { 60., 120., 250., 500., 1'000., 2'000., 4'000., 8'000., 12'000. };

            double Energy[9] { };

            for (size_t Band = 0; Band < _countof(Energy); ++Band)
            {
                double re = 0.;
                double im = 0.;

                for (size_t i = 0; i < MaxFrames; ++i)
                {
                    double Mono = 0.;

                    for (size_t Channel = 0; Channel < ChannelCount; ++Channel)
                        Mono += (double) Frames[i * ChannelCount + Channel];

                    Mono /= (double) ChannelCount;

                    // Apply a Hann window.
                    const double Window = 0.5 - (0.5 * std::cos((double) i * WindowStep));
                    const double Angle  = TwoPi * Frequencies[Band] * (double) i / _SampleRate;

                    re += Mono * Window * std::cos(Angle);
                    im -= Mono * Window * std::sin(Angle);
                }

                const double BandPower = (re * re) + (im * im);

                Energy[Band] = std::sqrt(BandPower) / (double) MaxFrames;
            }

            RMS    = std::sqrt(std::clamp(RMS * 3., 0., 1.));

            Bass   = std::clamp((Energy[0] + Energy[1] + Energy[2]) *  8., 0., 1.);
            Mid    = std::clamp((Energy[3] + Energy[4] + Energy[5]) * 10., 0., 1.);
            Treble = std::clamp((Energy[6] + Energy[7] + Energy[8]) * 16., 0., 1.);
        }

        const double RMSResponse    = (RMS    > _RMS)         ? .18 : .030;

        const double BassResponse   = (Bass   > _BassLevel)   ? .16 : .045;
        const double MidResponse    = (Mid    > _MidLevel)    ? .12 : .035;
        const double TrebleResponse = (Treble > _TrebleLevel) ? .10 : .025;

        _RMS         += (RMS    - _RMS)         * RMSResponse;

        _BassLevel   += (Bass   - _BassLevel)   * BassResponse;
        _MidLevel    += (Mid    - _MidLevel)    * MidResponse;
        _TrebleLevel += (Treble - _TrebleLevel) * TrebleResponse;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        _Time += deltaTime;

        _Zoom           = 1. + _ZoomOffset + (_BassLevel   * .35);
        _Rotation      += deltaTime * (.025 + (_TrebleLevel * .12));
        _PaletteOffset += deltaTime * (.015 + (_TrebleLevel * .08));

        const double TargetReal      = -.745 + (_MidLevel    - .5) * .10 + std::sin(_Time * .08) * .018;
        const double TargetImaginary =  .245 + (_TrebleLevel - .5) * .10 + std::cos(_Time * .07) * .018;

        const double Response = 1. - std::exp(-deltaTime * 1.25);

        _Real      += (TargetReal      - _Real)      * Response;
        _Imaginary += (TargetImaginary - _Imaginary) * Response;

        RenderJulia();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderJulia() noexcept
    {
        if (_Workers.empty())
        {
            for (size_t y = 0; y < _RasterHeight; ++y)
                RenderRow(y);

            return;
        }
        {
            const std::lock_guard Lock(_WorkerMutex);

            _NextRow.store(0, std::memory_order_relaxed);

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
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        _Workers.reserve(WorkerCount);

        try
        {
            for (size_t i = 0; i < WorkerCount; ++i)
            {
                _Workers.emplace_back([this]() noexcept
                {
                    Worker();
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
    void effect_t::Worker() noexcept
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

            for (;;)
            {
                const size_t Row = _NextRow.fetch_add(1, std::memory_order_relaxed);

                if (Row >= _RasterHeight)
                    break;

                RenderRow(Row);
            }

            {
                const std::lock_guard Lock(_WorkerMutex);

                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderRow(size_t y) noexcept
    {
        const double Cos = std::cos(_Rotation);
        const double Sin = std::sin(_Rotation);

        const double InverseZoom   = 1. / _Zoom;
        const double InverseHeight = 1. / (double) _RasterHeight;

        const double ny = ((double) y * InverseHeight - .5) * 3.2 * InverseZoom;

        const double nyCos = ny * Cos;
        const double nySin = ny * Sin;

        const int MaximumIterations = 100 + (int) (_BassLevel * 100.);
        const double InverseMaximumIterations = 1. / (double) MaximumIterations;

        const double AspectRatio = (double) _RasterWidth * InverseHeight;
        const double Width = 3.2 * AspectRatio * InverseZoom;

        const double nxIncrement = Width / (double) _RasterWidth;
        const double zrIncrement = nxIncrement * Cos;
        const double ziIncrement = nxIncrement * Sin;

        const double nx = Width * -.5;

        double InitialReal      = (nx * Cos) - nySin;
        double InitialImaginary = (nx * Sin) + nyCos;

        size_t Row = y * (size_t) _RasterWidth;

        for (UINT32 x = 0; x < _RasterWidth; ++x, ++Row)
        {
            double zr = InitialReal;
            double zi = InitialImaginary;

            double zrSquared = zr * zr;
            double ziSquared = zi * zi;

            double MagnitudeSquared = zrSquared + ziSquared;

            int Iteration = 0;

            while (MagnitudeSquared < 256. && Iteration < MaximumIterations)
            {
                const double NextImaginary = (2. * zr * zi) + _Imaginary;

                zr = zrSquared - ziSquared + _Real;
                zi = NextImaginary;

                zrSquared = zr * zr;
                ziSquared = zi * zi;

                MagnitudeSquared = zrSquared + ziSquared;

                ++Iteration;
            }

            double SmoothIteration = (double) Iteration;

            if (Iteration < MaximumIterations)
                SmoothIteration += 1. - std::log2(std::log2(std::sqrt(MagnitudeSquared)));

            _Pixels[Row] = Colorize(SmoothIteration * InverseMaximumIterations, Iteration < MaximumIterations);

            InitialReal      += zrIncrement;
            InitialImaginary += ziIncrement;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    UINT32 effect_t::Colorize(double amount, bool escaped) const noexcept
    {
        const size_t Palette      = (size_t) _PaletteOffset % 8;
        const double PalettePhase = _PaletteOffset - std::floor(_PaletteOffset);

        const double Ridge        = 0.5 + (0.5 * std::cos(amount * TwoPi * 6.0));
        const double Detail       = std::pow(1. - amount, .38);

        const double Contrast     = .72 + (Ridge * .28);
        const double Brightness   = escaped ? std::clamp(.24 + (Detail * .68) + (_RMS * .18), 0., 1.) : .075 + (_RMS * .085);

        double H = std::fmod(.58 + (amount * 1.70) + (PalettePhase * .28) + (_TrebleLevel * .06), 1.);
        double S = .78 + (Ridge * .18);
        double V = Brightness * Contrast;

        if (!escaped)
        {
            H = std::fmod(H + .50 + (_MidLevel * .04), 1.);
            S = std::clamp(S * .72, 0., 1.);
            V = Brightness;
        }
        else
            V = std::clamp(V, .18, 1.);

        double R, G, B;

        HSVToRGB(H, S, V, R, G, B);

        return PackColor(R, G, B);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _Bitmap)
            return S_OK;

        const D2D1_SIZE_U Size = D2D1::SizeU(_RasterWidth, _RasterHeight);
        const D2D1_BITMAP_PROPERTIES Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

        hr = renderTarget->CreateBitmap(Size, nullptr, 0, &Properties, _Bitmap.ReleaseAndGetAddressOf());

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

        _Bitmap.Reset();

        __super::DeleteDeviceDependentResources();
    }
}
