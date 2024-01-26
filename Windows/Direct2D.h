
/** $VER: Direct2D.h (2024.01.20) P. Stuer **/

#pragma once

#include "framework.h"

#include <vector>
#include <algorithm>

class Direct2D
{
public:
    Direct2D();

    virtual ~Direct2D()
    {
        Factory.Release();
    }

    HRESULT GetDPI(HWND hWnd, UINT & dpi) const;

    HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) const noexcept;
    HRESULT Load(const WCHAR * uri, IWICBitmapSource ** source) const noexcept;

    HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) const noexcept;

    HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1RenderTarget * renderTarget, ID2D1Bitmap ** bitmap) const noexcept;

    HRESULT CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors, std::vector<D2D1_GRADIENT_STOP> & gradientStops) const noexcept;

    void SortColorsByHue(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;
    void SortColorsBySaturation(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;
    void SortColorsByLightness(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;

private:
    static HRESULT GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize);

public:
    CComPtr<ID2D1Factory2> Factory;
};

extern Direct2D _Direct2D;
