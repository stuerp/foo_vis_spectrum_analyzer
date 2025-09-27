
/** $VER: Spectrum.h (2025.09.27) P. Stuer - Represents and renders the spectrum. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Support.h"

#include "Element.h"
#include "Gradients.h"

#include "XAxis.h"
#include "YAxis.h"

#include "Chrono.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the visualisation of the spectrum.
/// </summary>
class spectrum_t : public element_t
{
public:
    spectrum_t() {}

    spectrum_t(const spectrum_t &) = delete;
    spectrum_t & operator=(const spectrum_t &) = delete;
    spectrum_t(spectrum_t &&) = delete;
    spectrum_t & operator=(spectrum_t &&) = delete;

    virtual ~spectrum_t();

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget) noexcept;
    void Reset() noexcept { }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    const D2D1_RECT_F & GetClientBounds() const noexcept { return _ClientBounds; }

private:
    void Resize() noexcept;

    void RenderBars(ID2D1RenderTarget * renderTarget) noexcept;
    void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;
    void RenderRadialBars(ID2D1RenderTarget * renderTarget) noexcept;
    void RenderRadialCurve(ID2D1RenderTarget * renderTarget) noexcept;

    void RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget) const noexcept;

    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

    struct geometry_points_t
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

    HRESULT CreateGeometryPointsFromAmplitude(geometry_points_t & gp, bool usePeak) const noexcept;
    HRESULT CreateRadialGeometryPointsFromAmplitude(geometry_points_t & gp, bool usePeak) const noexcept;
    HRESULT CreateCurve(const geometry_points_t & gp, bool isFilled, ID2D1PathGeometry ** curve) const noexcept;

    HRESULT CreateSegment(FLOAT a1, FLOAT a2, FLOAT r1, FLOAT r2, ID2D1PathGeometry ** segment) const noexcept;

private:
    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    D2D1_RECT_F _ClientBounds;
    D2D1_SIZE_F _ClientSize;

    x_axis_t _XAxis;
    y_axis_t _YAxis;

    chrono_t _Chrono;

    // Device-dependent resources
    CComPtr<ID2D1Bitmap> _OpacityMask;

    style_t * _BarAreaStyle;
    style_t * _BarTopStyle;
    style_t * _BarPeakAreaStyle;
    style_t * _BarPeakTopStyle;
    style_t * _DarkBackgroundStyle;
    style_t * _LightBackgroundStyle;

    style_t * _CurveLineStyle;
    style_t * _CurveAreaStyle;
    style_t * _CurvePeakLineStyle;
    style_t * _CurvePeakAreaStyle;

    style_t * _NyquistMarkerStyle;
};
