
/** $VER: Spectrum.h (2024.08.16) P. Stuer - Represents and renders the spectrum. **/

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

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget);
    void Reset() { }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    const D2D1_RECT_F & GetClientBounds() const noexcept { return _ClientBounds; }

private:
    void Resize() noexcept;

    void RenderBars(ID2D1RenderTarget * renderTarget);
    void RenderCurve(ID2D1RenderTarget * renderTarget);
    void RenderRadialBars(ID2D1RenderTarget * renderTarget);
    void RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget) const noexcept;

    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget);

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

    HRESULT CreateGeometryPointsFromAmplitude(GeometryPoints & gp, bool usePeak) const;
    HRESULT CreateCurve(const GeometryPoints & gp, bool isFilled, ID2D1PathGeometry ** curve) const noexcept;

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

    style_t * _BarArea;
    style_t * _BarTop;
    style_t * _PeakArea;
    style_t * _PeakTop;
    style_t * _DarkBackground;
    style_t * _LightBackground;

    style_t * _CurveLine;
    style_t * _CurveArea;
    style_t * _CurvePeakLine;
    style_t * _CurvePeakArea;

    style_t * _NyquistMarker;
};
