
/** $VER: Spectrum.h (2024.01.31) P. Stuer - Represents and renders the spectrum. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "Configuration.h"

#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the visualisation of the spectrum.
/// </summary>
class Spectrum
{
public:
    Spectrum() : _Configuration() {}

    Spectrum(const Spectrum &) = delete;
    Spectrum & operator=(const Spectrum &) = delete;
    Spectrum(Spectrum &&) = delete;
    Spectrum & operator=(Spectrum &&) = delete;

    void Initialize(const Configuration * configuration);

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }
    FLOAT GetLeft() const { return _Bounds.left; }
    FLOAT GetRight() const { return _Bounds.right; }

private:
    HRESULT CreateGradientBrush(ID2D1RenderTarget * renderTarget);
    HRESULT CreatePatternBrush(ID2D1RenderTarget * renderTarget);

    struct GeometryPoints
    {
        std::vector<D2D1_POINT_2F> p0; // Determines how many knots will be used to calculate control points.
        std::vector<D2D1_POINT_2F> p1;
        std::vector<D2D1_POINT_2F> p2;

        void Clear()
        {
            p0.clear();
            p1.clear();
            p2.clear();
        }
    };

    HRESULT CreateGeometryPointsFromAmplitude(const std::vector<FrequencyBand> & frequencyBands, double sampleRate, bool usePeak, GeometryPoints & gp);
    HRESULT CreateCurve(const GeometryPoints & gp, bool isFilled, ID2D1PathGeometry ** curve) const noexcept;

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops);

    void RenderBars(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);
    void RenderCurve(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);

private:
    const Configuration * _Configuration;

    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    D2D1_RECT_F _Bounds;

    // Device-dependent resources
    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    CComPtr<ID2D1SolidColorBrush> _BackBrush;
    CComPtr<ID2D1SolidColorBrush> _WhiteBrush;

    CComPtr<ID2D1LinearGradientBrush> _GradientBrush;
    CComPtr<ID2D1BitmapBrush> _PatternBrush;

    std::vector<D2D1_GRADIENT_STOP> _GradientStops;
 };
