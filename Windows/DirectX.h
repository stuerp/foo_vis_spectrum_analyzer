
/** $VER: DirectX.h (2024.01.15) P. Stuer **/

#pragma once

#include "framework.h"

class DirectX
{
public:
    DirectX();

    HRESULT GetDPI(HWND hWnd, UINT & dpi) const;

public:
};

extern DirectX _DirectX;
