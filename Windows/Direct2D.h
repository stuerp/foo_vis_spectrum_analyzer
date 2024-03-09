
/** $VER: Direct2D.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>
#include <d2d1helper.h>
#include <atlbase.h>

#include <vector>
#include <algorithm>

#include "WIC.h"
#include "Gradients.h"

class Direct2D
{
public:
    Direct2D() { };

    HRESULT Initialize();
    void Terminate();

    HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) const noexcept;
    HRESULT Load(const WCHAR * uri, IWICBitmapSource ** source) const noexcept;

    HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) const noexcept;

    HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1RenderTarget * renderTarget, ID2D1Bitmap ** bitmap) const noexcept;

    HRESULT CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors, std::vector<D2D1_GRADIENT_STOP> & gradientStops) const noexcept;
    HRESULT CreateGradientBrush(ID2D1RenderTarget * renderTarget, const GradientStops & gradientStops, const D2D1_SIZE_F & size, bool isHorizontal, ID2D1LinearGradientBrush ** gradientBrush) const noexcept;

    void SortColorsByHue(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;
    void SortColorsBySaturation(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;
    void SortColorsByLightness(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept;

private:
    static HRESULT GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize);

public:
    CComPtr<ID2D1Factory2> Factory;
};

extern Direct2D _Direct2D;
