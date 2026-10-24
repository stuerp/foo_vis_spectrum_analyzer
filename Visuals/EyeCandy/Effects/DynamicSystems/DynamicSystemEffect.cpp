
/** $VER: DynamicSystemEffect.cpp (2026.10.22) - Base class for a chaotic map effect (https://en.wikipedia.org/wiki/List_of_chaotic_maps) */

#include <pch.h>

#include "DynamicSystemEffect.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace DynamicSystem
{
    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetSplineSubdivision(size_t subdivision) noexcept
    {
        _SplineSubdivision = std::clamp(subdivision, (size_t) 1, (size_t) 12);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetSplineSmoothing(double smoothing) noexcept
    {
        _SplineSmoothing = Clamp(smoothing);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        Projection.Width       = (double) _Width;
        Projection.Height      = (double) _Height;
        Projection.AspectRatio = (double) ((_Height > 0.f) ? _Width / _Height : 1.f);

        Projection.FieldOfView = 52.;
        Projection.NearPlane   = 1.;
        Projection.FarPlane    = 240.;
    }

    /// <summary>
    /// Analyzes an audio chunk.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak        = 0.;
        _RMS         = 0.;
        _StereoWidth = 0.;
        _Correlation = 1.;
        _PhaseEnergy = 0.;

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();

        const audio_sample * Frame = chunk.get_data();

        std::vector<double> ChannelPower(ChannelCount, 0.);

        double TotalPower   = 0.;
        double CenterPower  = 0.;
        double SpatialPower = 0.;
        double CrossProduct = 0.;
        double Peak         = 0.;

        const double ChannelScale = 1. / (double) ChannelCount;

        const audio_sample * const End = Frame + FrameCount * ChannelCount;

        while (Frame < End)
        {
            const audio_sample * Sample = Frame;
            const audio_sample * const FrameEnd = Frame + ChannelCount;

            double FrameSum   = 0.;
            double FramePower = 0.;

            size_t Channel = 0;

            while (Sample < FrameEnd)
            {
                const double Value = (double) *Sample++;
                const double Power = Value * Value;

                FrameSum   += Value;
                FramePower += Power;

                ChannelPower[Channel++] += Power;

                Peak = std::max(Peak, std::abs(Value));
            }

            const double Center = FrameSum * ChannelScale;
            const double CenterSquared = Center * Center;

            CrossProduct += 0.5 * (FrameSum * FrameSum - FramePower);

            TotalPower   += FramePower;
            CenterPower  += CenterSquared;
            SpatialPower += std::max(0., FramePower * ChannelScale - CenterSquared);

            Frame = FrameEnd;
        }

        _Peak = Peak;

        if (TotalPower <= Epsilon)
            return;

        _RMS = std::sqrt(TotalPower / (double) (FrameCount * ChannelCount));

        if (CenterPower > Epsilon)
            _StereoWidth = std::sqrt(SpatialPower / CenterPower);
        else
            _StereoWidth = (SpatialPower > Epsilon) ? 2. : 0.;

        _StereoWidth = std::clamp(_StereoWidth, 0., 2.);

        double CorrelationScale = 0.;

        for (size_t LeftChannel = 0; LeftChannel < ChannelCount; ++LeftChannel)
        {
            const double LeftPower = ChannelPower[LeftChannel];

            for (size_t RightChannel = LeftChannel + 1; RightChannel < ChannelCount; ++RightChannel)
            {
                CorrelationScale += std::sqrt(LeftPower * ChannelPower[RightChannel]);
            }
        }

        if (CorrelationScale > Epsilon)
            _Correlation = std::clamp(CrossProduct / CorrelationScale, -1., 1.);

        _PhaseEnergy = Clamp(SpatialPower * (double) ChannelCount / TotalPower);
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::Colorize(double value) const noexcept
    {
        const double Phase     = Fraction(value);
        const double AntiPhase = 0.5 * (1. - _SmoothedCorrelation);

        const auto H = (float) Fraction(Phase + (_AnimationTime * 0.018) + (AntiPhase * 0.12));
        const auto S = (float)      Clamp(0.80 + (_SmoothedWidth * 0.08));
        const auto V = (float) std::clamp(0.70 + (_SmoothedRMS   * 1.20) + (_SmoothedPeak * 0.18), 0.70, 1.); // Brightness

        return HSVToColor(H, S, V);
    }

    /// <summary>
    /// Draws a small local coordinate frame at the center of the attractor.
    /// </summary>
    void effect_t::RenderAxes(ID2D1RenderTarget * renderTarget, double axisLength) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f))
            return;

        const point3d_t Origin = World.Position;

        struct axis_t
        {
            point3d_t End;
            uint32_t Color;
        };

        const axis_t Axes[]
        {
            { { Origin.X + axisLength, Origin.Y,              Origin.Z              }, 0xff5555u },
            { { Origin.X,              Origin.Y + axisLength, Origin.Z              }, 0x55ff77u },
            { { Origin.X,              Origin.Y,              Origin.Z + axisLength }, 0x5599ffu }
        };

        const auto PreparedProjection = ThreeD::Prepare();

        if (!PreparedProjection.IsValid)
            return;

        const auto ProjectedOrigin = Project(Origin, PreparedProjection);

        if (!ProjectedOrigin.IsProjectable)
            return;

        for (const axis_t & Axis : Axes)
        {
            const auto ProjectedEnd = Project(Axis.End, PreparedProjection);

            if (!ProjectedEnd.IsProjectable)
                continue;

            constexpr float Intensity = 0.82f;

            const double ViewDepth = (ProjectedOrigin.ViewDepth + ProjectedEnd.ViewDepth) * 0.5;
            const float Thickness  = std::clamp(58.f / (float) std::max(ViewDepth, Projection.NearPlane), 0.75f, 1.85f);

            DrawGlowLine(renderTarget, ProjectedOrigin.Position, ProjectedEnd.Position, D2D1::ColorF(Axis.Color), Intensity, Thickness);
        }
    }

    /// <summary>
    /// Draws a 'glowing' line.
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, float thickness) noexcept
    {
        const int OuterRadius = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low: return 2;
                case EyeCandy::Quality::High: return 6;
                case EyeCandy::Quality::Medium:
                default: return 4;
            }
        }();

        thickness = std::clamp(thickness, 0.35f, 3.0f);

        DrawLine(renderTarget, from, to, color, intensity * 0.055f, (float)(OuterRadius * 2 + 1) * thickness);
        DrawLine(renderTarget, from, to, color, intensity * 0.180f, (float)(std::max(1, OuterRadius / 2) * 2 + 1) * thickness);
        DrawLine(renderTarget, from, to, color, intensity * 0.950f, std::max(0.75f, thickness));
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, float thickness) noexcept
    {
        intensity = Clamp(intensity);

        if (intensity <= 0.0001f)
            return;

        SetBrush(color, intensity);

        renderTarget->DrawLine(from, to, _Brush.Get(), std::max(0.5f, thickness), _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateCurveTheme(double deltaTime) noexcept
    {
        deltaTime = std::max(0.0, deltaTime);

        _ThemeElapsed += deltaTime;
        _ThemeResourceElapsed += deltaTime;

        if (!_ThemeTransitionActive && (_ThemeElapsed >= ThemeInterval))
        {
            _ThemeElapsed = std::fmod(_ThemeElapsed, ThemeInterval);

            BeginCurveThemeTransition((_CurveThemeTo + 1) % CurveThemes.size());
        }

        if (!_ThemeTransitionActive)
            return;

        _ThemeTransitionElapsed += deltaTime;

        const double Progress = Clamp(_ThemeTransitionElapsed / ThemeTransitionDuration);

        // Cubic smoothstep avoids a visible velocity discontinuity at both ends.
        _ThemeBlend = Progress * Progress * (3. - 2. * Progress);

        if (_ThemeResourceElapsed >= ThemeResourceUpdateInterval)
        {
            _ThemeResourceElapsed = std::fmod(_ThemeResourceElapsed, ThemeResourceUpdateInterval);
            _ThemeResourcesDirty = true;
        }

        if (Progress >= 1.0)
        {
            _CurveThemeFrom = _CurveThemeTo;

            _ThemeTransitionElapsed = ThemeTransitionDuration;
            _ThemeBlend             = 1.;
            _ThemeTransitionActive  = false;
            _ThemeResourcesDirty    = true;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::BeginCurveThemeTransition(size_t themeIndex) noexcept
    {
        if (CurveThemes.empty())
            return;

        themeIndex %= CurveThemes.size();

        if (themeIndex == _CurveThemeTo)
            return;

        // Transitions only start from a settled theme, so cycling remainsdeterministic even if a frame arrives with a large delta time.
        _CurveThemeFrom = _CurveThemeTo;
        _CurveThemeTo   = themeIndex;

        _ThemeTransitionElapsed = 0.;
        _ThemeResourceElapsed   = ThemeResourceUpdateInterval;
        _ThemeBlend             = 0.;
        _ThemeTransitionActive  = true;
        _ThemeResourcesDirty    = true;
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::SampleCurveTheme(const curve_theme_t & theme, double position) noexcept
    {
        position = Fraction(position);

        const double Scaled = position * 2.;
        const size_t First  = (Scaled < 1.) ? 0u : 1u;
        const float Amount  = (float)(Scaled - (double) First);

        return LerpColor(theme.Colors[First], theme.Colors[First + 1], Amount);
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::GetCurveColor(double position) noexcept
    {
        const D2D1_COLOR_F From = SampleCurveTheme(CurveThemes[_CurveThemeFrom], position);
        const D2D1_COLOR_F To   = SampleCurveTheme(CurveThemes[_CurveThemeTo],   position);

        return LerpColor(From, To, (float) _ThemeBlend);
    }

    /// <summary>
    /// Fades the previous content in the render target.
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = 0.935 + (0.030 * _PhaseEnergy);
        const double Decay       = std::pow(Persistence, (deltaTime * 60.));

        _Brush->SetColor(D2D1::ColorF(0.0015f, 0.0025f, 0.0060f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateSpline() noexcept
    {
        assert((1  <= _SplineSubdivision) && (_SplineSubdivision <= 12));
        assert((0. <= _SplineSmoothing)   && (_SplineSmoothing   <= 1.));

        _SplinePoints.clear();

        if (_Points.size() < 2)
            return;

        _SplinePoints.reserve((_Points.size() - 1) * _SplineSubdivision + 1);

        _SplinePoints.push_back(_Points.front());

        for (size_t i = 0; i + 1 < _Points.size(); ++i)
        {
            const point3d_t p0 = _Points[i > 0 ? i - 1 : i];
            const point3d_t p1 = _Points[i];
            const point3d_t p2 = _Points[i + 1];
            const point3d_t p3 = _Points[i + 2 < _Points.size() ? i + 2 : i + 1];

            for (size_t j = 1; j <= _SplineSubdivision; ++j)
            {
                const double Amount = (double) j / (double) _SplineSubdivision;

                const point3d_t Spline = EvaluateSpline(p0, p1, p2, p3, Amount);
                const point3d_t Linear
                {
                    Lerp(p1.X, p2.X, Amount),
                    Lerp(p1.Y, p2.Y, Amount),
                    Lerp(p1.Z, p2.Z, Amount)
                };

                _SplinePoints.push_back
                ({
                    Lerp(Linear.X, Spline.X, _SplineSmoothing),
                    Lerp(Linear.Y, Spline.Y, _SplineSmoothing),
                    Lerp(Linear.Z, Spline.Z, _SplineSmoothing)
                });
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::EvaluateSpline(point3d_t p0, point3d_t p1, point3d_t p2, point3d_t p3, double amount1) noexcept
    {
        const double Amount2 = amount1 * amount1;
        const double Amount3 = Amount2 * amount1;
        const double Weight0 = (1. - 3. * amount1 + 3. * Amount2 -      Amount3) / 6.;
        const double Weight1 = (4. - 6. * Amount2 + 3. * Amount3)                / 6.;
        const double Weight2 = (1. + 3. * amount1 + 3. * Amount2 - 3. * Amount3) / 6.;
        const double Weight3 = Amount3 / 6.;

        return
        {
            p0.X * Weight0 + p1.X * Weight1 + p2.X * Weight2 + p3.X * Weight3,
            p0.Y * Weight0 + p1.Y * Weight1 + p2.Y * Weight2 + p3.Y * Weight3,
            p0.Z * Weight0 + p1.Z * Weight1 + p2.Z * Weight2 + p3.Z * Weight3
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
            hr = renderTarget->CreateSolidColorBrush(Black, _Brush.ReleaseAndGetAddressOf());

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

            _BackBufferRenderTarget->Clear(Black);

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
        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        _StrokeStyle.Reset();
        _Brush.Reset();

        _RenderTarget = nullptr;

        __super::DeleteDeviceDependentResources();
    }
}
