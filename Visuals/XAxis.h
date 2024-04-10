
/** $VER: XAxis.h (2024.04.08) P. Stuer - Implements the X axis of a graph. **/

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
class XAxis : public Element
{
public:
    XAxis() : _BandCount(), _LoFrequency(), _HiFrequency() { }

    XAxis(const XAxis &) = delete;
    XAxis & operator=(const XAxis &) = delete;
    XAxis(XAxis &&) = delete;
    XAxis & operator=(XAxis &&) = delete;

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget);
    void Reset() { }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetHeight() const
    {
        return _TextStyle->_TextHeight;
    }

private:
    void Resize() noexcept;

private:
    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    struct Label
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

    std::vector<Label> _Labels;

    Style * _LineStyle;
    Style * _TextStyle;
};
