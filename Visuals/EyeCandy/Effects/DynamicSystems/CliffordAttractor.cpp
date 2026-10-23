
/** $VER: CliffordAttractor.cpp (2026.10.17) https://paulbourke.net/fractals/clifford/ */

#include <pch.h>

#include "CliffordAttractor.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace CliffordAttractor
{
    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0.0, 0.1);

        AnalyzeAudio(chunk);
        Update(deltaTime);

        UpdateAttractor(deltaTime);
        UpdateBitmap();

        RenderBackground(renderTarget);

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
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
    void effect_t::ClearDensity() noexcept
    {
        std::fill(_Densities.begin(), _Densities.end(), 0.f);
        std::fill(_Pixels.begin(), _Pixels.end(), 0u);
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
                case EyeCandy::Speed::Slow: return 0.55;
                case EyeCandy::Speed::Fast: return 1.75;
                case EyeCandy::Speed::Normal:
                default: return 1.0;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.);
        _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS, _RMS, deltaTime, (_RMS > _SmoothedRMS) ? 16. : 4.);
        _SmoothedWidth       = ExponentialSmoothing(_SmoothedWidth, _StereoWidth, deltaTime, 5.5);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0);

        UpdateCurveTheme(deltaTime);
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point_t effect_t::Iterate(point_t point, const parameters_t & parameters) noexcept
    {
        return
        {
            std::sin(parameters.A * point.Y) + parameters.C * std::cos(parameters.A * point.X),
            std::sin(parameters.B * point.X) + parameters.D * std::cos(parameters.B * point.Y)
        };
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAttractor(double deltaTime) noexcept
    {
        if (_Densities.empty() || (_BitmapWidth < 2) || (_BitmapHeight < 2))
            return;

        // Each render pass represents a fresh density sample. CopyFromMemory()
        // replaces the complete bitmap later, but the source histogram must also
        // be cleared or density from earlier passes remains visible indefinitely.
        std::fill(_Densities.begin(), _Densities.end(), 0.f);

        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case EyeCandy::Speed::Slow: return 0.60;
                case EyeCandy::Speed::Fast: return 1.65;
                case EyeCandy::Speed::Normal:
                default: return 1.0;
            }
        }();

        const double IterationsPerSecond = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low: return 180000.;
                case EyeCandy::Quality::High: return 900000.;
                case EyeCandy::Quality::Medium:
                default: return 420000.;
            }
        }();

        _AccumulationRemainder += IterationsPerSecond * SpeedFactor * deltaTime;

        const size_t IterationCount = (size_t) _AccumulationRemainder;
        _AccumulationRemainder -= (double) IterationCount;

        const double Deposit = (0.75 + std::clamp(_SmoothedRMS, 0., 1.) * 1.25);

        for (size_t i = 0; i < IterationCount; ++i)
        {
            _Point = Iterate(_Point, _Parameters);

            if (std::isfinite(_Point.X) && std::isfinite(_Point.Y))
                AccumulatePoint(_Point, Deposit);
            else
                _Point = { 0.1, 0.1 };
        }

    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AccumulatePoint(point_t point, double amount) noexcept
    {
        // Clifford coordinates are bounded by the sums of the sine/cosine coefficients.
        // A fixed symmetric view avoids camera drift and preserves the genuine 2D attractor.
        constexpr double Minimum = -2.25;
        constexpr double Maximum =  2.25;
        constexpr double Scale = 1. / (Maximum - Minimum);

        const double U = (point.X - Minimum) * Scale;
        const double V = 1. - (point.Y - Minimum) * Scale;

        if ((U < 0.) || (U >= 1.) || (V < 0.) || (V >= 1.))
            return;

        const double PixelX = U * (double) (_BitmapWidth - 1);
        const double PixelY = V * (double) (_BitmapHeight - 1);

        const UINT32 X0 = (UINT32) PixelX;
        const UINT32 Y0 = (UINT32) PixelY;
        const UINT32 X1 = std::min(X0 + 1, _BitmapWidth - 1);
        const UINT32 Y1 = std::min(Y0 + 1, _BitmapHeight - 1);

        const double FractionX = PixelX - (double) X0;
        const double FractionY = PixelY - (double) Y0;

        const double InverseX = 1. - FractionX;
        const double InverseY = 1. - FractionY;

        _Densities[(size_t) Y0 * _BitmapWidth + X0] += amount * InverseX  * InverseY;
        _Densities[(size_t) Y0 * _BitmapWidth + X1] += amount * FractionX * InverseY;
        _Densities[(size_t) Y1 * _BitmapWidth + X0] += amount * InverseX  * FractionY;
        _Densities[(size_t) Y1 * _BitmapWidth + X1] += amount * FractionX * FractionY;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateBitmap() noexcept
    {
        if (!_BackBuffer || _Densities.empty() || (_Pixels.size() != _Densities.size()))
            return;

        double MaximumDensity = 0.;

        for (const double Value : _Densities)
            MaximumDensity = std::max(MaximumDensity, Value);

        const double TargetExposure = (MaximumDensity > 0.) ? 1. / std::log1p((double) MaximumDensity) : 1.;

        _Exposure = ExponentialSmoothing(_Exposure, TargetExposure, 1. / 60., 3.0);

        const double BrightnessFactor = ((size_t) _Brightness == 0) ? 0.72 : ((size_t) _Brightness >= 2) ? 1.32 : 1.0;

        const double AudioBrightness = 0.88 + (std::clamp(_SmoothedRMS, 0.0, 1.0) * 0.35) + (std::clamp(_SmoothedPeak, 0.0, 1.0) * 0.12);

        for (size_t i = 0; i < _Densities.size(); ++i)
        {
            const double Normalized = std::clamp(std::log1p((double) _Densities[i]) * _Exposure, 0., 1.);
            const double Shaped     = std::pow(Normalized, 0.62);

            const float Opacity = (float) std::clamp(Shaped * BrightnessFactor * AudioBrightness, 0., 1.);
            const uint32_t Color = GetCurveColor(Shaped * 0.82 + _AnimationTime * 0.025);

            _Pixels[i] = PremultiplyColor(Color, Opacity);
        }

        const D2D1_RECT_U Destination = D2D1::RectU(0, 0, _BitmapWidth, _BitmapHeight);
        const UINT32 Pitch = _BitmapWidth * (UINT32) sizeof(uint32_t);

        if (FAILED(_BackBuffer->CopyFromMemory(&Destination, _Pixels.data(), Pitch)))
            DeleteDeviceDependentResources();
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::PremultiplyColor(uint32_t color, float opacity) noexcept
    {
        assert((opacity >= 0.f) && (opacity <= 1.f));

        const uint32_t R = (color >> 16) & 0xFFu;
        const uint32_t G = (color >>  8) & 0xFFu;
        const uint32_t B = color         & 0xFFu;
        const uint32_t A = (uint32_t) std::lround(opacity * 255.f);

        const uint32_t PremultipliedRed   = (R * A + 127u) / 255u;
        const uint32_t PremultipliedGreen = (G * A + 127u) / 255u;
        const uint32_t PremultipliedBlue  = (B * A + 127u) / 255u;

        return (A << 24) | (PremultipliedRed << 16) | (PremultipliedGreen << 8) | PremultipliedBlue;
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

        if ((_RenderTarget == renderTarget) && _BackBuffer && (_BitmapWidth == RequiredWidth) && (_BitmapHeight == RequiredHeight))
            return S_OK;

        DeleteDeviceDependentResources();

        const D2D1_BITMAP_PROPERTIES Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.f, 96.f);

        hr = renderTarget->CreateBitmap(D2D1::SizeU(RequiredWidth, RequiredHeight), nullptr, 0, Properties, _BackBuffer.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _BitmapWidth  = RequiredWidth;
        _BitmapHeight = RequiredHeight;

        _Densities.assign((size_t) RequiredWidth * RequiredHeight, 0.);
        _Pixels.assign((size_t) RequiredWidth * RequiredHeight, 0u);

        _Point = { 0.1, 0.1 };
        _Exposure              = 1.;
        _AccumulationRemainder = 0.;

        _RenderTarget = renderTarget;

        return S_OK;
    }

    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _BackBuffer.Reset();

        _BitmapWidth = 0;
        _BitmapHeight = 0;

        _RenderTarget = nullptr;
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Clifford Attractor, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }
}
