
/** $VER: Direct2D.h (2024.01.15) P. Stuer **/

#pragma once

#include "framework.h"

class Direct2D
{
public:
    Direct2D();

    HRESULT GetDPI(HWND hWnd, UINT & dpi) const;

    HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) const noexcept;
    HRESULT Load(const WCHAR * uri, IWICBitmapSource ** source) const noexcept;

    HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) const noexcept;

    HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1RenderTarget * renderTarget, ID2D1Bitmap ** bitmap) const noexcept;

private:
    HRESULT Initialize();

    static HRESULT GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize);

public:
    CComPtr<ID2D1Factory> Factory;
};

extern Direct2D _Direct2D;
