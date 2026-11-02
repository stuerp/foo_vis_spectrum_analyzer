
/** $VER: HarmonicHarp.cpp (2026.11.02) **/

#include <pch.h>

#include "HarmonicHarp.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <thread>

namespace HarmonicHarp
{
    using EyeCandy::ThreeD;

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
        return std::format(L"Harmonic Harp, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        Projection.Width       = (double) _Width;
        Projection.Height      = (double) _Height;
        Projection.AspectRatio = (_Height > 0.f) ? (double) _Width / (double) _Height : 1.;
        Projection.LensOffsetX = 0.;
        Projection.LensOffsetY = 0.;
        Projection.FieldOfView = 52.;
        Projection.NearPlane   = .05;
        Projection.FarPlane    = 4.;

        World  = { };
        Camera = { };
        Object = { };

        Camera.Target = { 0., 0., 1.6 };
        Camera.Up     = { 0., 1., 0. };

        _BeatEnvelope      = 0.;
        _PreviousRMS       = 0.;
        _SceneJump         = 0.;
        _SceneJumpVelocity = 0.;

        _ZoomFieldOfView = 52.;

        _MousePitch = 25. * Pi / 180.;
        _MouseYaw   = 0.;

        _DragStartPitch = _MousePitch;
        _DragStartYaw   = _MouseYaw;

        _DragStartX = 0;
        _DragStartY = 0;
        _IsDragging = false;
        _Hue        = .56;

        for (size_t i = 0; i < StringCount; ++i)
        {
            const double Amount = (double) i / (double) (StringCount - 1);

            _Strings[i].Frequency      = LoFrequency * std::pow(FrequencyRatio, Amount);
            _Strings[i].Energy         = 0.;
            _Strings[i].SmoothedEnergy = 0.;
            _Strings[i].Phase          = Amount * TwoPi;
            _Strings[i].HueOffset      = Amount;
            _Strings[i].ColorBurst     = 0.;
        }
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

        UpdateDragRotation(renderTarget);

        GenerateWork();

        {
            _BackBufferRenderTarget->BeginDraw();

            Fade(_BackBufferRenderTarget.Get(), deltaTime);

            RenderScene(_BackBufferRenderTarget.Get());

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();
                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);

        RenderScanlines(renderTarget);
    }

    /// <summary>
    /// Handles a rotation of the mouse wheel.
    /// </summary>
    void effect_t::OnMouseWheel(short wheelDelta) noexcept
    {
        if (wheelDelta == 0)
            return;

        constexpr double ZoomStep           =  3.;
        constexpr double MinimumFieldOfView = 36.;
        constexpr double MaximumFieldOfView = 72.;

        const double WheelSteps = (double) wheelDelta / (double) WHEEL_DELTA;

        // Positive wheel movement zooms in; negative movement zooms out.
        _ZoomFieldOfView = std::clamp(_ZoomFieldOfView - WheelSteps * ZoomStep, MinimumFieldOfView, MaximumFieldOfView);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak = 0.;
        _RMS  = 0.;

        if (chunk.is_empty())
        {
            for (auto & String : _Strings)
                String.Energy = 0.;

            return;
        }

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();

        _SampleRate = (double) chunk.get_sample_rate();

        const size_t MaxFrames = std::min<size_t>(FrameCount, 2048);

        if (MaxFrames < 32)
            return;

        _Samples.resize(MaxFrames);

        const audio_sample * Frames = chunk.get_data() + ((FrameCount - MaxFrames) * ChannelCount);

        double Sum = 0.;

        for (size_t i = 0; i < MaxFrames; ++i)
        {
            double Mono = 0.;

            for (size_t c = 0; c < ChannelCount; ++c)
                Mono += Frames[i * ChannelCount + c];

            Mono = ClampAudio(Mono / (double)ChannelCount);

            const double Window = 0.5 - 0.5 * std::cos(TwoPi * (double) i / ((double) MaxFrames - 1.));

            _Samples[i] = Mono * Window;

            _Peak = std::max(_Peak, std::abs(Mono));

            Sum += Mono * Mono;
        }

        _RMS = std::sqrt(Sum / (double) MaxFrames);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        const double NyquistFrequency = _SampleRate * 0.5;

        double Maximum = 1.e-9;

        for (size_t i = 0; i < StringCount; ++i)
        {
            auto & String = _Strings[i];

            const double LowerFrequency = (i     == 0)           ? LoFrequency : std::sqrt(_Strings[i - 1].Frequency * String.Frequency);
            const double UpperFrequency = (i + 1 == StringCount) ? HiFrequency : std::sqrt(_Strings[i + 1].Frequency * String.Frequency);

            double WeightedMagnitude = 0.;
            double WeightSum = 0.;

            for (size_t Probe = 0; Probe < 7; ++Probe)
            {
                const double ProbeAmount    = (double) Probe / 6.;
                const double ProbeFrequency = LowerFrequency * std::pow(UpperFrequency / LowerFrequency, ProbeAmount);
                const double Frequency      = std::clamp(ProbeFrequency, LoFrequency, NyquistFrequency);
                const double Coefficient    = 2. * std::cos(TwoPi * Frequency / _SampleRate);

                double q1 = 0., q2 = 0.;

                for (double Sample : _Samples)
                {
                    const double q0 = Sample + Coefficient * q1 - q2;

                    q2 = q1;
                    q1 = q0;
                }

                const double Power  = std::max(0., q1 * q1 + q2 * q2 - Coefficient * q1 * q2);
                const double Weight = 1. - std::abs(ProbeAmount * 2. - 1.) * .45;

                WeightedMagnitude += std::sqrt(Power) * Weight;
                WeightSum         += Weight;
            }

            const double Amount                    = (double) i / (double) (StringCount - 1);
            const double HighFrequencyCompensation = 1. + std::pow(Amount, 1.35) * 2.15;
            const double Magnitude                 = WeightedMagnitude / std::max(WeightSum, 1.e-9) / (double) MaxFrames;

            String.Energy = Clamp(std::log1p(Magnitude * 105. * HighFrequencyCompensation) / std::log1p(105.));

            Maximum = std::max(Maximum, String.Energy);
        }

        if (Maximum > .06)
        {
            for (auto & String : _Strings)
                String.Energy = Clamp(String.Energy / std::max(.28, Maximum));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = (_Speed == Speed::Slow) ? .6 : (_Speed == Speed::Fast) ? 1.6 : 1.;

        _AnimationTime += deltaTime * SpeedFactor;

        for (auto & String : _Strings)
        {
            String.SmoothedEnergy = ExponentialSmoothing(String.SmoothedEnergy, String.Energy, deltaTime, String.Energy > String.SmoothedEnergy ? 42. : 11.);

            const double BurstTarget = _HasAudio ? SmoothStep(.08, .62, String.Energy) : 0.;

            String.ColorBurst = ExponentialSmoothing(String.ColorBurst, BurstTarget, deltaTime, BurstTarget > String.ColorBurst ? 28. : 5.5);

            if (_HasAudio)
            {
                const double VisualFrequency = 2.2 + std::log2(std::max(LoFrequency, String.Frequency) / LoFrequency) * 1.35;

                String.Phase = Fraction(String.Phase + deltaTime * VisualFrequency * SpeedFactor);
            }
        }

        const double Onset = std::max(0., _RMS - _PreviousRMS) * 8. + std::max(0., _Peak - _SmoothedPeak) * 2.5;

        _BeatEnvelope = std::max(_BeatEnvelope * std::exp(-deltaTime * 8.5), Clamp(Onset));

        if (Onset > .06)
            _SceneJumpVelocity += std::min(.22, Onset * .13);

        _SceneJumpVelocity += (-_SceneJump * 38. - _SceneJumpVelocity * 10.) * deltaTime;
        _SceneJump         += _SceneJumpVelocity * deltaTime;

        _Hue = Fraction(_Hue + deltaTime * (.006 + _SmoothedTreble * .018));

        _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, _Peak > _SmoothedPeak ? 24. : 5.);
        _SmoothedRMS = ExponentialSmoothing(_SmoothedRMS, _RMS, deltaTime, _RMS > _SmoothedRMS ? 16. : 4.);

        _PreviousRMS = _RMS;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateDragRotation(ID2D1RenderTarget * renderTarget) noexcept
    {
        HWND Window = nullptr;

        ComPtr<ID2D1HwndRenderTarget> HwndRenderTarget;

        if (SUCCEEDED(renderTarget->QueryInterface(IID_PPV_ARGS(HwndRenderTarget.GetAddressOf()))))
            Window = HwndRenderTarget->GetHwnd();

        if (!Window)
            Window = GetForegroundWindow();

        POINT CursorPosition{ };

        if (!Window || !GetCursorPos(&CursorPosition) || !ScreenToClient(Window, &CursorPosition))
        {
            _IsDragging = false;
            return;
        }

        const bool IsLeftButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (IsLeftButtonDown && !_IsDragging)
        {
            _IsDragging = true;
            _DragStartX = CursorPosition.x;
            _DragStartY = CursorPosition.y;
            _DragStartYaw = _MouseYaw;
            _DragStartPitch = _MousePitch;
        }
        else
        if (!IsLeftButtonDown)
        {
            _IsDragging = false;
        }

        if (!_IsDragging)
            return;

        RECT ClientRectangle{ };

        if (!GetClientRect(Window, &ClientRectangle))
            return;

        const double Width  = (double) std::max<LONG>(1, ClientRectangle.right  - ClientRectangle.left);
        const double Height = (double) std::max<LONG>(1, ClientRectangle.bottom - ClientRectangle.top);

        const double DeltaX = (double) (CursorPosition.x - _DragStartX) / Width;
        const double DeltaY = (double) (CursorPosition.y - _DragStartY) / Height;

        constexpr double YawRange     = 120. * Pi / 180.;
        constexpr double PitchRange   =  90. * Pi / 180.;
        constexpr double MinimumPitch = -35. * Pi / 180.;
        constexpr double MaximumPitch =  65. * Pi / 180.;

        _MouseYaw = _DragStartYaw + DeltaX * YawRange;

        // Reversed vertical movement: dragging upward increases pitch.
        _MousePitch = std::clamp(_DragStartPitch - DeltaY * PitchRange, MinimumPitch, MaximumPitch);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateWork() noexcept
    {
        if (_Workers.empty())
        {
            for (size_t i = 0; i < StringCount; ++i)
                UpdateString(i);

            return;
        }
        {
            const std::lock_guard Lock(_WorkerMutex);

            _NextString.store(0, std::memory_order_relaxed);

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
    void effect_t::UpdateString(size_t index) noexcept
    {
        auto & String = _Strings[index];

        const double Amount = (double) index / (double) (StringCount - 1);

        const double Density         = Amount + .04 * std::sin(Amount * Pi) * (1. - 2. * Amount);
        const double BaseX           = Lerp(-.82, .82, Density);
        const double Energy          = _HasAudio ? String.SmoothedEnergy : 0.;
        const double FrequencyLength = 1. - std::pow(Amount, .82) * .58;
        const double HalfLength      = .62 * FrequencyLength;

        constexpr double SceneDepth = 1.6;

        const double PitchCosine = std::cos(_MousePitch);
        const double PitchSine   = std::sin(_MousePitch);

        const double YawCosine   = std::cos(_MouseYaw);
        const double YawSine     = std::sin(_MouseYaw);

        for (size_t i = 0; i < PointCount; ++i)
        {
            const double Along = (double) i / (double) (PointCount - 1);

            const double LocalY              = Lerp(-HalfLength, HalfLength, Along);
            const double StandingWave        = std::sin(Along * Pi);
            const double TemporalOscillation = std::sin(String.Phase * TwoPi);
            const double VibrationAmplitude  = _HasAudio ? std::pow(Energy, .68) * (.052 + Amount * .02) : 0.;
            const double Fundamental         = StandingWave * TemporalOscillation * VibrationAmplitude;
            const double SecondHarmonic      = std::sin(Along * TwoPi) * std::sin(String.Phase * TwoPi * 1.93 + Amount * Pi) * VibrationAmplitude * .08;
            const double LocalX              = BaseX + Fundamental + SecondHarmonic;

            const double PitchedY =  LocalY * PitchCosine;
            const double PitchedZ =  LocalY * PitchSine;

            const double RotatedX =  LocalX * YawCosine + PitchedZ * YawSine;
            const double RotatedZ = -LocalX * YawSine + PitchedZ * YawCosine;

            auto & Point = String.Points[i];

            Point.X = RotatedX;
            Point.Y = PitchedY + _SceneJump;
            Point.Z = SceneDepth + RotatedZ;
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
                _Workers.emplace_back([this]() noexcept
                {
                    Worker();
                });
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

        for (auto & WorkerThread : _Workers)
            if (WorkerThread.joinable())
                WorkerThread.join();

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

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || _Generation != Generation;
                });

                if (_IsStopping)
                    return;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t Index = _NextString.fetch_add(1, std::memory_order_relaxed);

                if (Index >= StringCount)
                    break;

                UpdateString(Index);
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
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Decay = std::pow(.94, deltaTime * 60.);

        _Brush->SetColor(D2D1::ColorF(.002f, .003f, .008f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderScene(ID2D1RenderTarget * renderTarget) noexcept
    {
        Projection.LensOffsetX = 0.;
        Projection.LensOffsetY = 0.;
        Projection.FieldOfView = _ZoomFieldOfView;
        Projection.NearPlane   = .05;
        Projection.FarPlane    = 4.;

        Camera.Target = { 0., 0., 1.6 };
        Camera.Up = { 0., 1., 0. };

        const auto PreparedProjection = ThreeD::Prepare();

        for (const auto & String : _Strings)
            RenderString(renderTarget, String, PreparedProjection);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderString(ID2D1RenderTarget * renderTarget, const string_t & string, const ThreeD::prepared_projection_t & projection) noexcept
    {
        const double Energy = _HasAudio ? string.SmoothedEnergy : 0.;

        const D2D1_COLOR_F BaseColor     = StringColor(string, .72 + Energy * .18);
        const D2D1_COLOR_F EruptionColor = BurstColor(string);

        const double Burst = _HasAudio ? string.ColorBurst : 0.;

        const D2D1_COLOR_F Color = D2D1::ColorF((float) Lerp((double) BaseColor.r, (double) EruptionColor.r, Burst), (float) Lerp((double) BaseColor.g, (double) EruptionColor.g, Burst), (float) Lerp((double) BaseColor.b, (double) EruptionColor.b, Burst), 1.f);

        for (size_t i = 1; i < PointCount; ++i)
        {
            const auto & PointA = string.Points[i - 1];
            const auto & PointB = string.Points[i];

            const auto A = ThreeD::Project({ PointA.X, PointA.Y, PointA.Z }, projection);
            const auto B = ThreeD::Project({ PointB.X, PointB.Y, PointB.Z }, projection);

            if (!A.IsProjectable || !B.IsProjectable)
                continue;

            const float Width = (float) (.65 + Energy * 2.2 + _BeatEnvelope * .35);

            _Brush->SetColor(Color);
            _Brush->SetOpacity((float) Clamp(.08 + Energy * .15));

            renderTarget->DrawLine(A.Position, B.Position, _Brush.Get(), Width + 5.f);

            _Brush->SetOpacity((float) Clamp(.32 + Energy * .48));

            renderTarget->DrawLine(A.Position, B.Position, _Brush.Get(), Width + 1.6f);

            _Brush->SetColor(D2D1::ColorF(Lerp(Color.r, 1.f, .55f), Lerp(Color.g, 1.f, .55f), Lerp(Color.b, 1.f, .55f), 1.f));
            _Brush->SetOpacity((float) Clamp(.5 + Energy * .5));

            renderTarget->DrawLine(A.Position, B.Position, _Brush.Get(), Width);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::StringColor(const string_t &, double brightness) const noexcept
    {
        // All inactive strings share the same warm, light-beige harp-wire color.
        const double Value = Clamp(brightness);

        return D2D1::ColorF((float) (Value * 1.), (float) (Value * .91), (float) (Value * .72), 1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::BurstColor(const string_t & string) const noexcept
    {
        // A responding frequency erupts into a distinct spectrum color assigned to that string.
        const float H = Fraction((float) (.01 + (string.HueOffset * .91) + (_Hue * .08)));
        const float S = (float) Clamp(.82 + string.ColorBurst * .16);
        const float V = (float) Clamp(.82 + string.ColorBurst * .18);

        return HSVToColor(H, S, V);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _Brush && _BackBufferRenderTarget && _BackBuffer)
        {
            const D2D1_SIZE_U Size = _BackBufferRenderTarget->GetPixelSize();

            if (Size.width == (UINT32)_Width && Size.height == (UINT32)_Height)
                return S_OK;
        }

        hr = renderTarget->CreateSolidColorBrush(White, _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const D2D1_SIZE_U PixelSize   = D2D1::SizeU(std::max<UINT32>(1, (UINT32) std::ceil(_Width)), std::max<UINT32>(1, (UINT32) std::ceil(_Height)));
        const D2D1_SIZE_F DesiredSize = D2D1::SizeF(_Width, _Height);

        hr = renderTarget->CreateCompatibleRenderTarget(&DesiredSize, &PixelSize, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, _BackBufferRenderTarget.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _BackBufferRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _BackBufferRenderTarget->BeginDraw();
        _BackBufferRenderTarget->Clear(D2D1::ColorF(.001f, .002f, .006f, 1.f));

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

        _Brush.Reset();
        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        __super::DeleteDeviceDependentResources();
    }
}
