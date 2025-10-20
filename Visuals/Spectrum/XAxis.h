
/** $VER: XAxis.h (2025.09.24) P. Stuer - Implements the X axis of a graph. **/

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
/// Implements the X axis of a graph.
/// </summary>
#pragma warning(disable: 4820)
class x_axis_t : public element_t
{
public:
    x_axis_t() : _BandCount(), _LoFrequency(), _HiFrequency() { }

    x_axis_t(const x_axis_t &) = delete;
    x_axis_t & operator=(const x_axis_t &) = delete;
    x_axis_t(x_axis_t &&) = delete;
    x_axis_t & operator=(x_axis_t &&) = delete;

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1DeviceContext * deviceContext) noexcept;
    void Reset() noexcept { }

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle->_Height;
    }

private:
    void Resize() noexcept;

private:
    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    struct label_t
    {
        std::wstring Text;
        double Frequency;
        bool IsDimmed;
        bool IsHidden;

        D2D1_POINT_2F PointT;
        D2D1_POINT_2F PointB;

        D2D1_RECT_F RectT;
        D2D1_RECT_F RectB;
    };

    std::vector<label_t> _Labels;

    style_t * _LineStyle;
    style_t * _TextStyle;
};
