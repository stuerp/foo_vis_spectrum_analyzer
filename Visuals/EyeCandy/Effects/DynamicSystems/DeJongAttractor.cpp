
/** $VER: DeJongAttractor.cpp (2026.11.15) https://paulbourke.net/fractals/peterdejong/ */

#include <pch.h>

#include "DeJongAttractor.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <format>

namespace DeJongAttractor
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"De Jong Attractor, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        ClearDensity();

        _Point = { 0.1, 0.1 };
        _Exposure = 1.;
        _AccumulationRemainder = 0.;

        __super::Reset();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = Clamp(deltaTime);

        Process(chunk, deltaTime);
        Update(deltaTime);

        {
            RenderBackground(renderTarget);

            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _Stride);

            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);
        }
    }

    /// <summary>
    /// 
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

        _AnimationTime += deltaTime * SpeedFactor;

        {
            _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak,        _Peak,        deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.0);
            _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS,         _RMS,         deltaTime, (_RMS  > _SmoothedRMS)  ? 16. : 4.0);

            _SmoothedStereoWidth = ExponentialSmoothing(_SmoothedStereoWidth, _StereoWidth, deltaTime, 5.5);
            _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0);
        }

        UpdateCurveTheme(deltaTime);

        UpdateAttractor(deltaTime);
        UpdateRaster();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAttractor(double deltaTime) noexcept
    {
        // Each render pass represents a fresh density sample. CopyFromMemory() replaces the complete bitmap later, but the source histogram must also be cleared or density from earlier passes remains visible indefinitely.
        std::fill(_Densities.begin(), _Densities.end(), 0.f);

        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return 0.60;

                default:
                case Speed::Normal:
                    return 1.00;

                case Speed::Fast:
                    return 1.65;
            }
        }();

        const double IterationsPerSecond = [&]() noexcept
        {
            switch (_Quality)
            {
                case Quality::Low:
                    return 180'000.;

                default:
                case Quality::Medium:
                    return 420'000.;

                case Quality::High:
                    return 900'000.;
            }
        }();

        const double Loudness         = Clamp(_Loudness);

        const double BassLevel        = Clamp(_BassLevel);
        const double MidLevel         = Clamp(_MidLevel);
        const double TrebleLevel      = Clamp(_TrebleLevel);

        const double SpectralFlux     = Clamp(_SpectralFlux);
        const double SpectralFlatness = Clamp(_SpectralFlatness);

        const double StereoWidth      = Clamp(_StereoWidth * 0.5);
        const double PhaseEnergy      = Clamp(_PhaseEnergy);
        const double Balance          = std::clamp(_StereoBalance, -1., 1.);

        // Loud passages receive more samples, while spectral attacks temporarily increase the density of the rendered curve.
        const double AudioIterationFactor = 0.82 + (Loudness * 0.38) + (SpectralFlux * 0.55);

        _AccumulationRemainder += IterationsPerSecond * AudioIterationFactor * deltaTime * SpeedFactor;

        const size_t IterationCount = (size_t) _AccumulationRemainder;

        _AccumulationRemainder -= (double) IterationCount;

        // Loudness controls the general density. Bass and spectral flux produce stronger, immediately visible deposits on beats and attacks.
        const double Deposit = 0.48 + (Loudness * 1.15) + (BassLevel * 0.85) + (SpectralFlux * 1.65) + (TrebleLevel * 0.20) + (SpectralFlatness * 0.15);

        // Bass expands and contracts the attractor. Spectral attacks add a shorter secondary pulse.
        const double Scale = 1. + (BassLevel * 0.115) + (SpectralFlux * 0.075);

        // Midrange and phase energy rotate the rendered coordinates without disturbing the recursive De Jong state.
        const double Rotation = (_AnimationTime * (0.018 + (MidLevel * 0.035))) + ((PhaseEnergy - 0.5) * 0.22);

        const double Cosine = std::cos(Rotation);
        const double Sine   = std::sin(Rotation);

        // Stereo width opens the visualization horizontally. Balance shifts the visible attractor toward the dominant channel.
        const double HorizontalScale = Scale   * (0.92 + (StereoWidth * 0.24));
        const double VerticalScale   = Scale   * (1.04 - (StereoWidth * 0.08));
        const double HorizontalShift = Balance * (0.08 + (StereoWidth * 0.06));

        for (size_t i = 0; i < IterationCount; ++i)
        {
            const auto Iterate = [&](point_t point) -> point_t
            {
                return
                {
                    std::sin(_Parameters.A * point.Y) - std::cos(_Parameters.B * point.X),
                    std::sin(_Parameters.C * point.X) - std::cos(_Parameters.D * point.Y)
                };
            };

            _Point = Iterate(_Point);

            if (std::isfinite(_Point.X) && std::isfinite(_Point.Y))
            {
                const double RotatedX = (_Point.X * Cosine) - (_Point.Y * Sine);
                const double RotatedY = (_Point.X * Sine)   + (_Point.Y * Cosine);

                const point_t RenderPoint =
                {
                    (RotatedX * HorizontalScale) + HorizontalShift,
                     RotatedY * VerticalScale
                };

                AccumulatePoint(RenderPoint, Deposit);
            }
            else
            {
                _Point = { 0.1, 0.1 };
            }
        }
    }
/*
    /// <summary>
    /// 
    /// </summary>
    effect_t::point_t effect_t::Iterate(point_t point, const parameters_t & parameters) noexcept
    {
        return
        {
            std::sin(parameters.A * point.Y) - std::cos(parameters.B * point.X),
            std::sin(parameters.C * point.X) - std::cos(parameters.D * point.Y)
        };
    }

*/
    /// <summary>
    /// 
    /// </summary>
    void effect_t::AccumulatePoint(point_t point, double amount) noexcept
    {
        // Each De Jong coordinate is a sine minus a cosine, so both axes are bounded by [-2, 2]. A small margin prevents edge clipping.
        constexpr double Minimum = -2.05;
        constexpr double Maximum =  2.05;
        constexpr double Scale = 1. / (Maximum - Minimum);

        const double U =      (point.X - Minimum) * Scale;
        const double V = 1. - (point.Y - Minimum) * Scale;

        if ((U < 0.) || (U >= 1.) || (V < 0.) || (V >= 1.))
            return;

        const double PixelX = U * (double) (_RasterWidth  - 1);
        const double PixelY = V * (double) (_RasterHeight - 1);

        const UINT32 X0 = (UINT32) PixelX;
        const UINT32 Y0 = (UINT32) PixelY;

        const UINT32 X1 = std::min(X0 + 1, _RasterWidth  - 1);
        const UINT32 Y1 = std::min(Y0 + 1, _RasterHeight - 1);

        const double FractionX = PixelX - (double) X0;
        const double FractionY = PixelY - (double) Y0;

        const double InverseX = 1. - FractionX;
        const double InverseY = 1. - FractionY;

        _Densities[(size_t) Y0 * _RasterWidth + X0] += amount * InverseX  * InverseY;
        _Densities[(size_t) Y0 * _RasterWidth + X1] += amount * FractionX * InverseY;
        _Densities[(size_t) Y1 * _RasterWidth + X0] += amount * InverseX  * FractionY;
        _Densities[(size_t) Y1 * _RasterWidth + X1] += amount * FractionX * FractionY;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateRaster() noexcept
    {
        // The maximum-density reduction is intentionally kept sequential. It performs very little work per element and is normally limited by memory bandwidth, so dispatching another parallel pass can cost more than it saves.
        double MaximumDensity = 0.;

        for (const double Value : _Densities)
            MaximumDensity = std::max(MaximumDensity, Value);

        const double Loudness         = Clamp(_Loudness);
        const double Peak             = Clamp(_Peak);
        const double BassLevel        = Clamp(_BassLevel);
        const double MidLevel         = Clamp(_MidLevel);
        const double TrebleLevel      = Clamp(_TrebleLevel);
        const double SpectralFlux     = Clamp(_SpectralFlux);
        const double SpectralCentroid = Clamp(_SpectralCentroid);
        const double SpectralFlatness = Clamp(_SpectralFlatness);
        const double StereoWidth      = Clamp(_StereoWidth * 0.5);
        const double PhaseEnergy      = Clamp(_PhaseEnergy);
        const double Correlation      = std::clamp(_Correlation, -1., 1.);
        const double Balance          = std::clamp(_StereoBalance, -1., 1.);

        // Strong attacks temporarily raise the target exposure so drum hits and new notes flash through the existing density distribution.
        const double ExposureResponse = 1. + (Loudness * 0.20) + (Peak * 0.12) + (SpectralFlux * 0.42);

        const double TargetExposure = (MaximumDensity > 0.) ? ExposureResponse / std::log1p(MaximumDensity) : 1.;

        // Use a faster attack than release. The visualization responds immediately to transients without flickering during quieter passages.
        const double ExposureSpeed = (TargetExposure > _Exposure) ? 14. : 4.;

        _Exposure = ExponentialSmoothing(_Exposure, TargetExposure, 1. / 60., ExposureSpeed);

        // Snapshot all frame-wide values before entering the parallel section. Each worker only reads these values and writes its own output pixel.
        const double Exposure      = _Exposure;
        const double AnimationTime = _AnimationTime;

        const double BrightnessFactor = ((size_t) _Brightness == 0) ? 0.72 : ((size_t) _Brightness >= 2) ? 1.32 : 1.;

        // Loudness establishes the sustained brightness, while peak and spectral flux provide clearly visible transient response.
        const double AudioBrightness = 0.58 + (Loudness * 0.68) + (Peak * 0.22) + (SpectralFlux * 0.62) + (BassLevel * 0.18) + (TrebleLevel * 0.12);

        // Bass emphasizes dense central structures. Treble and spectral flatness reveal weaker outer filaments.
        const double DensityExponent = std::clamp(0.70 - (TrebleLevel * 0.16) - (SpectralFlatness * 0.08) + (BassLevel * 0.07), 0.42, 0.78);

        // Centroid controls the main palette location. Frequency-band balance, stereo width and phase provide additional motion.
        const double ColorOffset = (SpectralCentroid * 0.34) + (TrebleLevel * 0.18) - (BassLevel * 0.10) + (MidLevel * 0.05) + (StereoWidth * 0.06) + (PhaseEnergy * 0.05) + (Balance * 0.04);

        const double ColorScale = 0.72 + (SpectralFlatness * 0.18) + (TrebleLevel * 0.12);

        // Negative or weak correlation produces more transparent detail, making wide and phase-complex material appear less solid.
        const double StereoOpacity = 0.92 + (StereoWidth * 0.13) + ((1. - ((Correlation + 1.) * 0.5)) * 0.10);

        const auto ToByte = [](double Value) noexcept -> uint32_t
        {
            return (uint32_t) std::lround(Clamp(Value) * 255.);
        };

        const auto Rasterize = [this, Exposure, AnimationTime, BrightnessFactor, AudioBrightness, DensityExponent, ColorOffset, ColorScale, StereoOpacity, SpectralFlux, ToByte](double Density) noexcept -> uint32_t
        {
            const double Normalized = Clamp(std::log1p(Density) * Exposure);
            const double Shaped = std::pow(Normalized, DensityExponent);

            // Spectral attacks move the palette immediately, while the animation term provides continuous motion between events.
            const double ColorPosition = (Shaped * ColorScale) + ColorOffset + (SpectralFlux * 0.14) + (AnimationTime * 0.025);

            const D2D1_COLOR_F Color = GetCurveColor(ColorPosition);

            const double Opacity = Clamp(Shaped * BrightnessFactor * AudioBrightness * StereoOpacity);

            const uint32_t Alpha = ToByte(0.85 + (double) Color.a * Opacity);
            const uint32_t Red   = ToByte((double) Color.r * (double) Color.a * Opacity);
            const uint32_t Green = ToByte((double) Color.g * (double) Color.a * Opacity);
            const uint32_t Blue  = ToByte((double) Color.b * (double) Color.a * Opacity);

            return (Alpha << 24) | (Red   << 16) | (Green << 8)  | Blue;
        };

        // Parallel algorithm dispatch has measurable overhead for small rasters. Use the sequential path until there is enough work to amortize it.
        constexpr size_t ParallelThreshold = 128u * 1024u;

        if (_Densities.size() >= ParallelThreshold)
        {
            std::transform(std::execution::par, _Densities.cbegin(), _Densities.cend(), _Pixels.begin(), Rasterize);
        }
        else
        {
            std::transform(_Densities.cbegin(), _Densities.cend(), _Pixels.begin(), Rasterize);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::ClearDensity() noexcept
    {
        std::fill(_Pixels.begin(), _Pixels.end(), 0u);
        std::fill(_Densities.begin(), _Densities.end(), 0.f);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        const UINT32 RequiredWidth  = std::max<UINT32>(1u, (UINT32) std::ceil(_Width));
        const UINT32 RequiredHeight = std::max<UINT32>(1u, (UINT32) std::ceil(_Height));

        if ((_RenderTarget == renderTarget) && _BackBuffer && (_RasterWidth == RequiredWidth) && (_RasterHeight == RequiredHeight))
            return S_OK;

        DeleteDeviceDependentResources();

        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), USER_DEFAULT_SCREEN_DPI, USER_DEFAULT_SCREEN_DPI);

        hr = renderTarget->CreateBitmap(D2D1::SizeU(RequiredWidth, RequiredHeight), nullptr, 0, Properties, _BackBuffer.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _RasterWidth  = RequiredWidth;
        _RasterHeight = RequiredHeight;

        _Stride = _RasterWidth * BytesPerPixel;

        _Pixels.assign((size_t) RequiredWidth * RequiredHeight, 0u);
        _Densities.assign((size_t) RequiredWidth * RequiredHeight, 0.f);

        _Point = { 0.1, 0.1 };

        _Exposure              = 1.;
        _AccumulationRemainder = 0.;

        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _BackBuffer.Reset();

        _RasterWidth  = 0;
        _RasterHeight = 0;

        _RenderTarget = nullptr;
    }
}
