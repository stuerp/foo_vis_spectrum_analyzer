
/** $VER: DXGI.h (2024.01.28) P. Stuer **/

#pragma once

#include "framework.h"

class DXGI
{
public:
    DXGI() { };

    HRESULT Initialize();
    void Terminate();

    HRESULT CreateSwapChain(IDXGIDevice * dxgiDevice, UINT width, UINT height, IDXGISwapChain1 ** swapChain) const noexcept;

public:
    CComPtr<IDXGIFactory2> Factory;
};

extern DXGI _DXGI;
