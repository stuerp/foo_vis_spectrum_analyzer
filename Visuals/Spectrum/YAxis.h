
/** $VER: YAxis.h (2024.04.08) P. Stuer - Implements the Y axis of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Element.h"
#include "State.h"
#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the Y axis of a graph.
/// </summary>
#pragma warning(disable: 4820)
class y_axis_t : public element_t
{
public:
    y_axis_t() { }

    y_axis_t(const y_axis_t &) = delete;
    y_axis_t & operator=(const y_axis_t &) = delete;
    y_axis_t(y_axis_t &&) = delete;
    y_axis_t & operator=(y_axis_t &&) = delete;

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget);
    void Reset() { }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle->_Width;
    }

private:
    void Resize() noexcept;

private:
    bool _FlipVertically;

    struct label_t
    {
        std::wstring Text;
        double Amplitude;
        bool IsHidden;

        D2D1_POINT_2F PointL;
        D2D1_POINT_2F PointR;

        D2D1_RECT_F RectL;
        D2D1_RECT_F RectR;
    };

    std::vector<label_t> _Labels;

    style_t * _LineStyle;
    style_t * _TextStyle;
};
