
/** $VER: DirectX.h (2024.01.01) P. Stuer **/

#pragma once

#include "framework.h"

class DirectX
{
public:
    DirectX();

    HRESULT CreateDeviceIndependentResources();

    HRESULT GetDPI(HWND hWnd, UINT & dpi) const;

public:
    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2D;
    CComPtr<IDWriteFactory> _DirectWrite;
};

extern DirectX _DirectX;
