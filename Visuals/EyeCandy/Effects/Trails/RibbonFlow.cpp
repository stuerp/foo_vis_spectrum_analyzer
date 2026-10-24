
/** $VER: RibbonFlow.cpp (2026.10.23) */

#include <pch.h>

#include "RibbonFlow.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace RibbonFlow
{
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Ribbon Flow, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const size_t ChannelCount = chunk.get_channel_count();
        const channel_pair_t SelectedChannels { 0, ChannelCount > 1 ? 1u : 0u };

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

        GenerateRibbons();

        {
            _BackBufferRenderTarget->BeginDraw();

            Fade(_BackBufferRenderTarget.Get(), deltaTime);

            RenderRibbons(_BackBufferRenderTarget.Get());

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();
                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
    }

    /// <summary>
    /// Calculates logarithmically spaced spectral bands. A rising frequency compensation curve deliberately gives the upper bands more visual authority.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels)
    {
        for (auto & Band : _Bands)
            Band.Energy = 0.;

        if (chunk.is_empty())
            return;

        const size_t ChannelCount = chunk.get_channel_count();
        const size_t FrameCount = chunk.get_sample_count();

        if ((channels[0] >= ChannelCount) || (channels[1] >= ChannelCount) || (FrameCount < 16))
            return;

        const audio_sample * Data = chunk.get_data();
        const size_t AnalysisCount = std::min<size_t>(FrameCount, _Quality == EyeCandy::Quality::High ? 1024 : 512);
        const size_t FrameStep = std::max<size_t>(1, FrameCount / AnalysisCount);

        std::vector<double> Samples(AnalysisCount);

        for (size_t i = 0; i < AnalysisCount; ++i)
        {
            const size_t Frame  = std::min(i * FrameStep, FrameCount - 1);

            const double Mono = 0.5 * (ClampAudio(Data[Frame * ChannelCount + channels[0]]) + ClampAudio(Data[Frame * ChannelCount + channels[1]]));

            // Apply a Hann window.
            const double Window = 0.5 - (std::cos((TwoPi * (double) i) / (double) (AnalysisCount - 1)) * 0.5);

            Samples[i] = Mono * Window;
        }

        constexpr double MinFrequency = 40.;

        const double MaxFrequency = std::min(16'500., _SampleRate * 0.50); // 40 Hz to 16.5 kHz

        std::array<double, BandCount> RawEnergies {};

        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount      = ((double) i + 0.5) / (double) BandCount;
            const double Frequency   = MinFrequency * std::pow(MaxFrequency / MinFrequency, Amount);
            const double AngularStep = TwoPi * Frequency / _SampleRate;

            double Real      = 0.;
            double Imaginary = 0.;

            for (size_t j = 0; j < AnalysisCount; ++j)
            {
                const double Angle = AngularStep * (double) j;

                Real      += Samples[j] * std::cos(Angle);
                Imaginary -= Samples[j] * std::sin(Angle);
            }

            const double Magnitude = 2. * std::sqrt((Real * Real) + (Imaginary * Imaginary)) / (double) AnalysisCount;

            // Broad visual gain and mild compression make every frequency region react strongly while retaining useful dynamic range.
            const double FrequencyGain = 1.75 + (4.25 * std::pow(Amount, 1.20));

            RawEnergies[i] = Clamp(std::pow(Magnitude * FrequencyGain * 7.4, 0.72));
        }

        // Blend adjacent logarithmic bands to suppress isolated spikes and produce a more cohesive layered flow.
        for (size_t i = 0; i < BandCount; ++i)
        {
            const size_t Previous = i     > 0         ? i - 1 : i;
            const size_t Next     = i + 1 < BandCount ? i + 1 : i;

            _Bands[i].Energy = (RawEnergies[Previous] * 0.22) + (RawEnergies[i] * 0.56) + (RawEnergies[Next] * 0.22);
        }
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

                default:
                case EyeCandy::Speed::Normal:
                    return 1.0;

                case EyeCandy::Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        for (size_t i = 0; i < BandCount; ++i)
        {
            auto & Band = _Bands[i];

            Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 12. : 2.8);
            Band.Phase         += deltaTime * SpeedFactor * (0.62 + ((double) i * 0.085));
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::GenerateRibbons() noexcept
    {
        size_t PointCount = (_Quality == EyeCandy::Quality::Low) ? 80u : (_Quality == EyeCandy::Quality::High) ? 192u : 128u;

        _Ribbons.clear();
        _Ribbons.resize(BandCount);

        for (size_t i = 0; i < BandCount; ++i)
        {
            auto & Ribbon = _Ribbons[i];

            const auto & Band = _Bands[i];

            Ribbon.Points.resize(PointCount);

            const double BandAmount = (double) i / (double) (BandCount - 1);
            const double Baseline   = _CenterY + ((BandAmount - 0.5) * _Height * 0.68);
            const double Response   = std::pow(Band.SmoothedEnergy, 0.78);
            const double Amplitude  = _Height * (0.024 + (Response * 0.145));

            for (size_t j = 0; j < PointCount; ++j)
            {
                const double Amount    = (double) j / (double) (PointCount - 1);
                const double Envelope  = std::sin(Amount * Pi);
                const double Primary   = std::sin((Amount * (5.0 + BandAmount * 5.0) * Pi) + Band.Phase);
                const double Secondary = std::sin((Amount * TwoPi) - (Band.Phase * 0.43) + (BandAmount * Pi));

                Ribbon.Points[j] =
                {
                    (float) (Amount * _Width),
                    (float) (Baseline + ((Primary * 0.78 + Secondary * 0.22) * Amplitude * Envelope))
                };
            }

            Ribbon.Color     = Colorize(BandAmount + (_AnimationTime * 0.015));
            Ribbon.Width     = (float) (1.35 + (Response * 5.6));
            Ribbon.Intensity = (float) Clamp(0.24 + (Response * 0.92));
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Decay = std::pow(0.925, deltaTime * 60.);

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderRibbons(ID2D1RenderTarget * renderTarget) noexcept
    {
        for (const auto & Ribbon : _Ribbons)
            DrawSmoothRibbon(renderTarget, Ribbon);
    }

    /// <summary>
    /// Draws one continuous cubic Bezier geometry. Flat end caps and a single path remove the globe-like nodes produced by many round-capped line segments.
    /// </summary>
    void effect_t::DrawSmoothRibbon(ID2D1RenderTarget * renderTarget, const ribbon_t & ribbon) noexcept
    {
        if (ribbon.Points.size() < 2)
            return;

        ComPtr<ID2D1PathGeometry> Geometry;

        if (FAILED(_D2DFactory->CreatePathGeometry(Geometry.GetAddressOf())))
            return;

        ComPtr<ID2D1GeometrySink> Sink;

        if (FAILED(Geometry->Open(Sink.GetAddressOf())))
            return;

        Sink->BeginFigure(ribbon.Points.front(), D2D1_FIGURE_BEGIN_HOLLOW);

        for (size_t i = 0; i + 1 < ribbon.Points.size(); ++i)
        {
            const D2D1_POINT_2F P0 = i > 0 ? ribbon.Points[i - 1] : ribbon.Points[i];
            const D2D1_POINT_2F P1 = ribbon.Points[i];
            const D2D1_POINT_2F P2 = ribbon.Points[i + 1];
            const D2D1_POINT_2F P3 = i + 2 < ribbon.Points.size() ? ribbon.Points[i + 2] : P2;

            Sink->AddBezier(D2D1::BezierSegment
            (
                D2D1::Point2F(P1.x + (P2.x - P0.x) / 6.f, P1.y + (P2.y - P0.y) / 6.f),
                D2D1::Point2F(P2.x - (P3.x - P1.x) / 6.f, P2.y - (P3.y - P1.y) / 6.f), P2)
            );
        }

        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

        if (FAILED(Sink->Close()))
            return;

        SetBrush(ribbon.Color, ribbon.Intensity * 0.10f);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), ribbon.Width + 7.f, _StrokeStyle.Get());

        SetBrush(ribbon.Color, ribbon.Intensity * 0.28f);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), ribbon.Width + 3.f, _StrokeStyle.Get());

        SetBrush(ribbon.Color, ribbon.Intensity);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), ribbon.Width, _StrokeStyle.Get());
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
    D2D1_COLOR_F effect_t::Colorize(double value) const noexcept
    {
        return HSVToColor((float) Fraction(0.50 + (value * 0.42)), 0.82f, 1.f);
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

        hr = renderTarget->CreateSolidColorBrush(Black, _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        // Flat caps are intentional: round caps on every short segment were visible as small globes.
        const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        {
            const D2D1_SIZE_U RequiredPixelSize = D2D1::SizeU(std::max<UINT32>(1u, (UINT32) std::ceil(_Width)), std::max<UINT32>(1u, (UINT32) std::ceil(_Height)));
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
        _RenderTarget = nullptr;

        _StrokeStyle.Reset();
        _Brush.Reset();

        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        __super::DeleteDeviceDependentResources();
    }
}
