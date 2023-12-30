
/** $VER: DirectX.h (2023.12.30) P. Stuer **/

#pragma once

#include "framework.h"

class DirectX
{
public:
    HRESULT CreateDeviceIndependentResources();

    HRESULT GetDPI(HWND hWnd, UINT & dpi);

public:
    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2D;
    CComPtr<IDWriteFactory> _DirectWrite;
};

extern DirectX _DirectX;
