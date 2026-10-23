
/** $VER: LissajousTrails.cpp (2026.10.20) **/

#include <pch.h>

#include "LissajousTrails.h"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace LissajousTrails
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
        return std::format(L"Lissajous Trails, {} quality, {} speed, {} brightness",
            Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        const size_t ChannelCount = chunk.get_channels();
        const channel_pair_t SelectedChannels { 0, ChannelCount > 1 ? 1u : 0u };

        Render(renderTarget, deltaTime, chunk, SelectedChannels);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk, const channel_pair_t & channels) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0.0, 0.1);

        AnalyzeAudio(chunk, channels);
        Update(deltaTime);

        GenerateWork();

        {
            _BackBufferRenderTarget->BeginDraw();

            ID2D1RenderTarget * RenderTarget = _BackBufferRenderTarget.Get();

            Fade(RenderTarget, deltaTime);

            if (!_SplineSegments.empty())
                RenderCurve(RenderTarget);
            else
                RenderIdleCurve(RenderTarget);

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();

                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk_impl & chunk, const channel_pair_t & channels)
    {
        _Samples.clear();

        _Peak        = 0.;
        _RMS         = 0.;

        _StereoWidth = 0.;
        _Correlation = 1.;
        _PhaseEnergy = 0.;

        if (chunk.is_empty())
            return;

        const size_t ChannelCount = chunk.get_channels();
        const size_t FrameCount   = chunk.get_sample_count();
        const audio_sample * Data = chunk.get_data();

        const size_t LeftChannel  = channels[0];
        const size_t RightChannel = channels[1];

        if ((LeftChannel >= ChannelCount) || (RightChannel >= ChannelCount))
            return;

        size_t DesiredSamples = 768;

        switch (_Quality)
        {
            case EyeCandy::Quality::Low:
                DesiredSamples = 384;
                break;

            case EyeCandy::Quality::High:
                DesiredSamples = 1536;
                break;

            case EyeCandy::Quality::Medium:
            default:
                break;
        }

        const size_t OutputCount = std::min(FrameCount, DesiredSamples);

        if (OutputCount < 2)
            return;

        _Samples.reserve(OutputCount);

        const size_t FramesPerSample = FrameCount / OutputCount;
        const size_t RemainingFrames = FrameCount % OutputCount;

        const audio_sample * Frame = Data;

        double LeftSquared  = 0.;
        double RightSquared = 0.;
        double CrossProduct = 0.;
        double Peak         = 0.;

        size_t FrameRemainder = 0;

        for (size_t OutputIndex = 0; OutputIndex < OutputCount; ++OutputIndex)
        {
            size_t FramesInSample = FramesPerSample;

            FrameRemainder += RemainingFrames;

            if (FrameRemainder >= OutputCount)
            {
                FrameRemainder -= OutputCount;
                ++FramesInSample;
            }

            const audio_sample * End = Frame + FramesInSample * ChannelCount;

            double LeftAccumulator  = 0.;
            double RightAccumulator = 0.;

            for (; Frame < End; Frame += ChannelCount)
            {
                const double Left  = ClampAudio(Frame[LeftChannel]);
                const double Right = ClampAudio(Frame[RightChannel]);

                LeftAccumulator  += Left;
                RightAccumulator += Right;

                LeftSquared  += Left  * Left;
                RightSquared += Right * Right;
                CrossProduct += Left  * Right;

                Peak = std::max(Peak, std::max(std::abs(Left), std::abs(Right)));
            }

            const double Normalization = 1. / (double) FramesInSample;

            _Samples.push_back
            ({
                LeftAccumulator  * Normalization,
                RightAccumulator * Normalization
            });
        }

        _Peak = Peak;

        const double TotalPower = LeftSquared + RightSquared;

        if (TotalPower <= Epsilon)
        {
            // Silence has no meaningful stereo phase relationship.
            _Correlation = 1.;

            return;
        }

        const double Normalization = 1. / (double) FrameCount;

        _RMS = std::sqrt(0.5 * TotalPower * Normalization);

        const double MidSquared  = 0.25 * (TotalPower + 2. * CrossProduct);
        const double SideSquared = 0.25 * (TotalPower - 2. * CrossProduct);

        {
            // Stereo width is the side-channel RMS relative to the mid-channel RMS.
            if (MidSquared > Epsilon)
                _StereoWidth = std::sqrt(std::max(0., SideSquared) / MidSquared);
            else
                // Perfectly anti-phase audio has no mid energy, so treat it as maximum display width instead of dividing by zero.
                _StereoWidth = SideSquared > Epsilon ? 2. : 0.;

            _StereoWidth = std::clamp(_StereoWidth, 0., 2.);
        }

        {
            const double CorrelationPower = LeftSquared * RightSquared;

            if (CorrelationPower > Epsilon)
                _Correlation = std::clamp(CrossProduct / std::sqrt(CorrelationPower), -1., 1.);
            else
                // Correlation is undefined if one or both channels contain no energy. Using 1 avoids displaying silence as anti-phase.
                _Correlation = 1.;
        }

        _PhaseEnergy = std::clamp(2. * SideSquared / TotalPower, 0., 1.);
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
                case EyeCandy::Speed::Slow:
                    return 0.55;

                case EyeCandy::Speed::Fast:
                    return 1.75;

                case EyeCandy::Speed::Normal:
                default:
                    return 1.0;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, _Peak > _SmoothedPeak ? 22. : 5.);
        _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS, _RMS, deltaTime, (_RMS > _SmoothedRMS) ? 16. : 4.);
        _SmoothedWidth       = ExponentialSmoothing(_SmoothedWidth, _StereoWidth, deltaTime, 5.5f);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0f);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateWork() noexcept
    {
        if (_Samples.size() < 2 || _Workers.empty())
        {
            _Segments.clear();
            _SplineSegments.clear();

            return;
        }

        const double BaseRotation     = -Pi * 0.25;
        const double AnimatedRotation = 0.055 * std::sin(_AnimationTime * 0.37) * std::clamp(_SmoothedWidth, 0., 1.);

        const field_t Field
        {
            .Rotation    = BaseRotation + AnimatedRotation,
            .Energy      = std::clamp(0.25 + (_SmoothedRMS * 2.4) + (_SmoothedPeak * 0.45), 0.25, 1.),
            .PhaseAmount = 0.5 * (1. - _SmoothedCorrelation),
            .Time        = _AnimationTime
        };

        _Segments.resize(_Samples.size() - 1);
        {
            const std::lock_guard Lock(_WorkerMutex);

            _Field = Field;

            _NextSegment.store(0, std::memory_order_relaxed);

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

        GenerateSpline();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateSpline() noexcept
    {
        _SplineSegments.clear();

        if (_Segments.empty())
            return;

        size_t SubdivisionCount = 3;

        switch (_Quality)
        {
            case EyeCandy::Quality::Low:
                SubdivisionCount = 2;
                break;

            case EyeCandy::Quality::High:
                SubdivisionCount = 5;
                break;

            case EyeCandy::Quality::Medium:
            default:
                break;
        }

        const size_t SegmentCount = _Segments.size();

        _SplineSegments.reserve(SegmentCount * SubdivisionCount);

        const double InverseSubdivisionCount = 1. / (double) SubdivisionCount;

        D2D1_POINT_2F Previous = _Segments.front().From;

        for (size_t Index = 0; Index < SegmentCount; ++Index)
        {
            const segment_t & Source = _Segments[Index];

            const D2D1_POINT_2F P0 = Index > 1 ? _Segments[Index - 2].To : _Segments.front().From;
            const D2D1_POINT_2F P1 = Index > 0 ? _Segments[Index - 1].To : _Segments.front().From;
            const D2D1_POINT_2F P2 = Source.To;
            const D2D1_POINT_2F P3 = Index + 1 < SegmentCount ? _Segments[Index + 1].To : Source.To;

            for (size_t Subdivision = 1; Subdivision <= SubdivisionCount; ++Subdivision)
            {
                const double Amount = (double) Subdivision * InverseSubdivisionCount;

                const D2D1_POINT_2F Current = EvaluateSpline(P0, P1, P2, P3, Amount);

                _SplineSegments.push_back
                ({
                    Previous,
                    Current,
                    Source.Color,
                    Source.Intensity
                });

                Previous = Current;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_POINT_2F effect_t::EvaluateSpline(D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3, double amount1) noexcept
    {
        const double Amount2 = amount1 * amount1;
        const double Amount3 = Amount2 * amount1;

        const double Weight0 = (1. - 3. * amount1 + 3. * Amount2 - Amount3)      / 6.;
        const double Weight1 = (4. - 6. * Amount2 + 3. * Amount3)                / 6.;
        const double Weight2 = (1. + 3. * amount1 + 3. * Amount2 - 3. * Amount3) / 6.;
        const double Weight3 = Amount3 / 6.;

        return
        {
            (float) (p0.x * Weight0 + p1.x * Weight1 + p2.x * Weight2 + p3.x * Weight3),
            (float) (p0.y * Weight0 + p1.y * Weight1 + p2.y * Weight2 + p3.y * Weight3)
        };
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        // Exponential phosphor decay makes persistence independent of the display frame rate.
        // High phase energy leaves a slightly longer trail because wide stereo material generally contains the most interesting Lissajous motion.
        const double Persistence     = 0.88 + (0.055 * std::clamp(_PhaseEnergy, 0., 1.));
        const double ReferenceFrames = deltaTime * 60.;
        const double Decay           = std::pow(Persistence, ReferenceFrames);

        const float Opacity = (float) std::clamp(1. - Decay, 0., 1.);

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, Opacity));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderCurve(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f) || _SplineSegments.empty())
            return;

        for (const auto & Segment : _SplineSegments)
            DrawGlowLine(renderTarget, Segment.From, Segment.To, Segment.Color, (float) Segment.Intensity);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderIdleCurve(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2) || (_Height < 2))
            return;

        constexpr size_t IdlePoints = 180;

        const double Scale = std::min(_Width, _Height) * 0.43;

        D2D1_POINT_2F Previous{};

        for (size_t i = 0; i < IdlePoints; ++i)
        {
            const double Amount = (double) i / (double) (IdlePoints - 1);
            const double Angle = Amount * 2. * Pi;

            // A slowly evolving 3:2 Lissajous figure provides a subtle attractor while no audio is available.
            const D2D1_POINT_2F current
            {
                _CenterX + (float) (std::sin(Angle * 3. + _AnimationTime * 0.31)            * Scale),
                _CenterY + (float) (std::sin(Angle * 2. + _AnimationTime * 0.23 + Pi * 0.5) * Scale)
            };

            if (i != 0)
            {
                const uint32_t Color = Colorize(Amount + (_AnimationTime * 0.02));

                DrawGlowLine(renderTarget, Previous, current, Color, 0.085f);
            }

            Previous = current;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity) noexcept
    {
        const int OuterRadius = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return 2;

                case EyeCandy::Quality::High:
                    return 5;

                case EyeCandy::Quality::Medium:
                default:
                    return 3;
            }
        }();

        // Wide, dim plasma halo.
        DrawLine(renderTarget, from, to, color, intensity * 0.08f, OuterRadius);

        // Mid glow.
        DrawLine(renderTarget, from, to, color, intensity * 0.22f, std::max(1, OuterRadius / 2));

        // Bright phosphor core.
        DrawLine(renderTarget, from, to, color, intensity * 0.90f, 0);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, int radius) noexcept
    {
        intensity = Clamp(intensity);

        if (intensity <= 0.0001f)
            return;

        SetBrush(color, intensity);

        const float strokeWidth = (radius > 0) ? (float)((radius * 2) + 1) : 1.f;

        renderTarget->DrawLine(from, to, _Brush.Get(), strokeWidth, _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(uint32_t color, float opacity) noexcept
    {
        _Brush->SetColor(D2D1::ColorF(color, opacity));
        _Brush->SetOpacity(Clamp(opacity));
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::Colorize(double value) const noexcept
    {
        const double Phase     = Fraction(value);
        const double AntiPhase = 0.5 * (1. - _SmoothedCorrelation);

        // Cyan, magenta and electric-blue palette with a small phase-dependent hue shift. Anti-phase content gravitates toward magenta while correlated content remains cyan/green.
        const auto H = (float) Fraction(0.48 + (Phase * 0.34) + (AntiPhase * 0.15) + (_AnimationTime * 0.006));
        const auto S = (float) std::clamp(0.68 + (AntiPhase * 0.24), 0., 1.);
        const auto V = (float) std::clamp(0.62 + (_SmoothedRMS * 1.35) + (_SmoothedPeak * 0.2), 0.62, 1.);

        return HSVToBGRA(H, S, V);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const auto WorkerCount = (size_t) std::max(1u, std::thread::hardware_concurrency());

        _Workers.reserve(WorkerCount);

        try
        {
            for (size_t index = 0; index < WorkerCount; ++index)
                _Workers.emplace_back(&effect_t::Worker, this);
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

        for (auto & worker : _Workers)
        {
            if (worker.joinable())
                worker.join();
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
            field_t Field;
            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                Field = _Field;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t SegmentIndex = _NextSegment.fetch_add(1, std::memory_order_relaxed);

                if (SegmentIndex >= _Segments.size())
                    break;

                GenerateSegment(SegmentIndex, Field);
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
    void effect_t::GenerateSegment(size_t index, const field_t & field) noexcept
    {
        const D2D1_POINT_2F From = TransformSample(_Samples[index],     field.Rotation);
        const D2D1_POINT_2F To   = TransformSample(_Samples[index + 1], field.Rotation);

        const double NormalizedIndex = (double) (index + 1) / (double) (_Samples.size() - 1);
        const double ColorPhase      = NormalizedIndex + (field.Time * 0.035) + (field.PhaseAmount * 0.12);

        const double dx              = To.x - From.x;
        const double dy              = To.y - From.y;

        const double Motion          = std::sqrt(dx * dx + dy * dy);
        const double MotionWeight    = std::clamp(Motion * 0.08, 0.45, 1.);

        _Segments[index] =
        {
            From,
            To,
            Colorize(ColorPhase),
            field.Energy * MotionWeight * (0.72 + (field.PhaseAmount * 0.38))
        };
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_POINT_2F effect_t::TransformSample(stereo_sample_t sample, double stereoRotation) const noexcept
    {
        double Left  = ClampAudio(sample.Left);
        double Right = ClampAudio(sample.Right);

        // Gentle nonlinear compression keeps transient peaks inside the viewport without flattening quiet detail.
        Left  = std::tanh(Left  * 1.35);
        Right = std::tanh(Right * 1.35);

        const double Cos = std::cos(stereoRotation);
        const double Sin = std::sin(stereoRotation);

        const double RotatedX = Left * Cos - Right * Sin;
        const double RotatedY = Left * Sin + Right * Cos;

        constexpr double Margin = 0.025;

        // Use nearly the complete shortest window dimension. The soft clipping performed above keeps ordinary peaks inside the viewport,
        // while allowing the Lissajous trace to occupy substantially more of the available drawing area.
        const double Scale = std::min(_Width, _Height) * (0.5 - Margin) / 1.12;

        // Quiet material receives a restrained visual gain so the trace remains useful without pushing louder transients outside the window.
        const double VisualGain = std::clamp(1.38 - (_SmoothedRMS * 0.40f), 1.08, 1.34);

        // Apply the same scale to both axes. This preserves a strict 1:1 plotting aspect ratio regardless of the window dimensions or stereo width of the source material.
        const double DisplayScale = Scale * VisualGain;

        return
        {
            _CenterX + (float) (RotatedX * DisplayScale),
            _CenterY - (float) (RotatedY * DisplayScale)
        };
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

            if ((Size.width == (UINT32) _Width) && (Size.height == (UINT32) _Height))
                return S_OK;
        }

        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), _Brush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f);

            hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const D2D1_SIZE_U RequiredPixelSize = D2D1::SizeU
            (
                std::max<UINT32>(1u, (UINT32) std::ceil(_Width)),
                std::max<UINT32>(1u, (UINT32) std::ceil(_Height))
            );

            const D2D1_SIZE_F DesiredSize = D2D1::SizeF(_Width, _Height);

            hr = renderTarget->CreateCompatibleRenderTarget(&DesiredSize, &RequiredPixelSize, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, _BackBufferRenderTarget.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            _BackBufferRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            _BackBufferRenderTarget->BeginDraw();

            _BackBufferRenderTarget->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 1.f));

            hr = _BackBufferRenderTarget->EndDraw();

            if (FAILED(hr))
                return hr;

            hr = _BackBufferRenderTarget->GetBitmap(_BackBuffer.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

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
