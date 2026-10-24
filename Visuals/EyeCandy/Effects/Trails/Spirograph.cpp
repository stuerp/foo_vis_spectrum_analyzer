
/** $VER: Spirograph.cpp (2026.10.23) */

#include <pch.h>

#include "Spirograph.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spirograph
{
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Spirograph, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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
    /// Calculates logarithmic spectral bands, a broadband rhythm impulse and a chromatic tonality center.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels)
    {
        for (auto & Band : _Bands)
            Band.Energy = 0.;

        _PitchClasses.fill(0.);

        _Rhythm = 0.;

        if (chunk.is_empty())
            return;

        const size_t ChannelCount = chunk.get_channel_count();
        const size_t FrameCount   = chunk.get_sample_count();

        if ((channels[0] >= ChannelCount) || (channels[1] >= ChannelCount) || (FrameCount < 16))
            return;

        const audio_sample * Data = chunk.get_data();

        const size_t AnalysisCount = std::min<size_t>(FrameCount, _Quality == EyeCandy::Quality::High ? 1024 : 512);
        const size_t FrameStep     = std::max<size_t>(1, FrameCount / AnalysisCount);

        std::vector<double> Samples(AnalysisCount);

        {
            const double WindowScale = TwoPi / (double) (AnalysisCount - 1);

            double MeanSquare = 0.;
            double Difference = 0.;

            double Previous = 0.;

            for (size_t i = 0; i < AnalysisCount; ++i)
            {
                const size_t Frame = std::min(i * FrameStep, FrameCount - 1);

                // Convert to mono.
                const double Mono   = 0.5 * (ClampAudio(Data[Frame * ChannelCount + channels[0]]) + ClampAudio(Data[Frame * ChannelCount + channels[1]]));

                // Apply a Hann Window.
                Samples[i] = Mono * (0.5 - (std::cos((double) i * WindowScale) * 0.5));

                MeanSquare += Mono * Mono;
                Difference += std::abs(Mono - Previous); // Sample-to-sample difference tends to be larger for percussion, transients, noisy or bright signals or rapidly varying waveforms

                Previous = Mono;
            }

            _Rhythm = Clamp((std::sqrt(MeanSquare / (double) AnalysisCount) * 4.8) + (Difference / (double) AnalysisCount * 2.2));
        }

        {
            constexpr double MinFrequency = 40.; // Lowest analyzed frequency (Low end of most musical bass content)

            const double MaxFrequency = std::min(16'500., _SampleRate * 0.50); // 40 Hz to 16.5 kHz

            std::array<double, BandCount> RawEnergies {};

            for (size_t i = 0; i < BandCount; ++i)
            {
                const double Amount          = ((double) i + 0.5) / (double) BandCount;
                const double CenterFrequency = MinFrequency * std::pow(MaxFrequency / MinFrequency, Amount);
                const double AngularStep     = TwoPi * CenterFrequency / _SampleRate;

                // Evaluate one point of the discrete Fourier transform.
                double Real      = 0.;
                double Imaginary = 0.;

                {
                    for (size_t j = 0; j < AnalysisCount; ++j)
                    {
                        const double Angle = AngularStep * (double) j;

                        Real      += Samples[j] * std::cos(Angle);
                        Imaginary -= Samples[j] * std::sin(Angle);
                    }
                }

                const double Magnitude = 2. * std::sqrt((Real * Real) + (Imaginary * Imaginary)) / (double) AnalysisCount;

                // Apply perceptual frequency compensation.
                const double FrequencyGain = 1.65 + (4.1 * std::pow(Amount, 1.18));

                RawEnergies[i] = Clamp(std::pow(Magnitude * FrequencyGain * 7., 0.74)); // Boost quiet spectral components to give them more visual presence

                const double MIDINote = 69. + (12. * std::log2(std::max(CenterFrequency, 1.) / 440.));

                // Determine the pitch class (e.g. A2, A3, A4, A5...)
                const size_t PitchClass = (size_t) (((int) std::lround(MIDINote) % 12 + 12) % 12);

                _PitchClasses[PitchClass] += RawEnergies[i];
            }

            // Apply a three-point smoothing filter to the neighboring bands.
            for (size_t i = 0; i < BandCount; ++i)
            {
                const size_t PreviousBand = (i > 0)             ? i - 1 : i;
                const size_t NextBand     = (i + 1 < BandCount) ? i + 1 : i;

                _Bands[i].Energy = (RawEnergies[PreviousBand] * 0.20) + (RawEnergies[i] * 0.60) + (RawEnergies[NextBand] * 0.20);
            }

            // Determine the strongest pitch class.
            const auto Strongest = std::max_element(_PitchClasses.begin(), _PitchClasses.end());

            _Tonality = Strongest != _PitchClasses.end() ? (double) std::distance(_PitchClasses.begin(), Strongest) / 12. : 0.;
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
                case Speed::Slow:
                    return 0.55;

                default:
                case Speed::Normal:
                    return 1.0;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        for (auto & Band : _Bands)
            Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 11. : 2.6);

        _SmoothedRhythm   = ExponentialSmoothing(_SmoothedRhythm, _Rhythm, deltaTime, (_Rhythm > _SmoothedRhythm) ? 15. : 3.2);
        _RhythmPulse      = std::max(_SmoothedRhythm, _RhythmPulse * std::pow(0.86, deltaTime * 60.));
        _SmoothedTonality = ExponentialSmoothing(_SmoothedTonality, _Tonality, deltaTime, 2.2);

        _Rotation += (deltaTime * SpeedFactor) * (0.34 + (_SmoothedRhythm * 1.15));
    }

    /// <summary>
    /// Generates nested hypotrochoid and epitrochoid curves. Rhythm expands the geometry while tonality changes rotational ratios and color.
    /// </summary>
    void effect_t::GenerateCurves() noexcept
    {
        size_t PointCount = (_Quality == EyeCandy::Quality::Low) ? 240u : (_Quality == EyeCandy::Quality::High) ? 720 : 480;

        _Curves.resize(CurveCount);

        const double Extent = std::min(_Width, _Height) * 0.43;

        for (size_t i = 0; i < CurveCount; ++i)
        {
            auto & Curve = _Curves[i];

            Curve.Points.resize(PointCount + 1);

            const double Layer       = (double) i / (double) (CurveCount - 1);
            const double BandEnergy  = _Bands[(i * 2) % BandCount].SmoothedEnergy;
            const double Response    = std::pow(BandEnergy, 0.76);
            const double MajorRadius = Extent * (0.34 + (Layer * 0.60) + (_RhythmPulse * 0.10));
            const double Ratio       = 2.0 + (double) i + std::round(_SmoothedTonality * 5.0);
            const double MinorRadius = MajorRadius / Ratio;
            const double PenDistance = MinorRadius * (0.72 + (Response * 1.55));
            const bool Inward        = (i & 1u) == 0u;
            const double Turns       = 2.0 + Ratio;
            const double Direction   = Inward ? 1. : -1.;

            for (size_t j = 0; j <= PointCount; ++j)
            {
                const double Amount           = (double) j / (double) PointCount;
                const double Angle            = Amount * TwoPi * Turns;
                const double RadiusDifference = Inward ? MajorRadius - MinorRadius : MajorRadius + MinorRadius;
                const double RotationAngle    = Angle + (_Rotation * Direction * (0.46 + Layer * 0.52));
                const double DetailAngle      = (RadiusDifference / MinorRadius) * Angle - (_Rotation * Direction);
                const double RhythmWarp       = 1. + (_RhythmPulse * 0.08 * std::sin((Angle * 2.) + (double) i));

                const double X = RadiusDifference * std::cos(RotationAngle) + Direction * PenDistance * std::cos(DetailAngle);
                const double Y = RadiusDifference * std::sin(RotationAngle) - Direction * PenDistance * std::sin(DetailAngle);

                Curve.Points[j] = D2D1::Point2F((float) (_CenterX + (X * RhythmWarp)), (float) (_CenterY + (Y * RhythmWarp)));
            }

            Curve.Color     = Colorize(_SmoothedTonality + (Layer * 0.27) + (_AnimationTime * 0.008));
            Curve.Width     = (float) (0.85 + (Response * 3.8) + (_RhythmPulse * 1.2));
            Curve.Intensity = (float) Clamp(0.18 + (Response * 0.72) + (_RhythmPulse * 0.24));
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Decay = std::pow(0.935, deltaTime * 60.);

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

            Sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(P1.x + (P2.x - P0.x) / 6.f, P1.y + (P2.y - P0.y) / 6.f),
                D2D1::Point2F(P2.x - (P3.x - P1.x) / 6.f, P2.y - (P3.y - P1.y) / 6.f), P2));
        }

        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

        if (FAILED(Sink->Close()))
            return;

        SetBrush(curve.Color, curve.Intensity * 0.08f);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width + 8.f, _StrokeStyle.Get());

        SetBrush(curve.Color, curve.Intensity * 0.26f);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width + 3.f, _StrokeStyle.Get());

        SetBrush(curve.Color, curve.Intensity);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), curve.Width, _StrokeStyle.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::SetBrush(uint32_t color, float opacity) noexcept
    {
        _Brush->SetColor(D2D1::ColorF(color));
        _Brush->SetOpacity(Clamp(opacity));
    }

    /// <summary>
    ///
    /// </summary>
    uint32_t effect_t::Colorize(double value) const noexcept
    {
        return HSVToBGRA((float) Fraction(0.54 + (value * 0.68)), 0.84f, 1.f);
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

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        // Flat caps prevent bright beads where the densely sampled nested curves overlap.
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
