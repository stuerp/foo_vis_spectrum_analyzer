
/** $VER: Tester.h (2025.10.21) P. Stuer - Implements a minimal visualization for testing purposes. **/

#pragma once

#include <pch.h>

#include "Element.h"

class tester_t : public element_t
{
public:
    tester_t();

    tester_t(const tester_t &) = delete;
    tester_t & operator=(const tester_t &) = delete;
    tester_t(tester_t &&) = delete;
    tester_t & operator=(tester_t &&) = delete;

    virtual ~tester_t();

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;
    void Release() noexcept override final;

    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

private:
    D2D1_POINT_2F _p1;
    D2D1_POINT_2F _p2;
    FLOAT _d;

    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
};
