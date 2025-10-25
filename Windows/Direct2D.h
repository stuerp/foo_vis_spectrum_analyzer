
/** $VER: Direct2D.h (2025.10.20) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>
#include <d2d1helper.h>
#include <atlbase.h>

#include "WIC.h"
#include "Gradients.h"

class Direct2D
{
public:
    Direct2D() { };

    HRESULT Initialize();
    void Terminate() noexcept;

    HRESULT GetRefreshRate(_In_ IDXGIDevice1 * dxgiDevice, _Out_ double & refreshRate) const noexcept;

    HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, _Out_ IWICBitmapSource ** source) const noexcept;
    HRESULT Load(const WCHAR * uri, _Out_ IWICBitmapSource ** source) const noexcept;

    HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, _Out_ IWICBitmapScaler ** scaler) const noexcept;

    HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1DeviceContext * deviceContext, _Out_ ID2D1Bitmap ** bitmap) const noexcept;

    HRESULT CreateGradientStops(_In_ const std::vector<D2D1_COLOR_F> & colors, _Out_ std::vector<D2D1_GRADIENT_STOP> & gradientStops) const noexcept;
    HRESULT CreateGradientBrush(_In_ ID2D1DeviceContext * deviceContext, _In_ const gradient_stops_t & gradientStops, _In_ const D2D1_SIZE_F & size, _In_ bool isHorizontal, _Out_ ID2D1LinearGradientBrush ** gradientBrush) const noexcept;
    HRESULT CreateRadialGradientBrush(_In_ ID2D1DeviceContext * deviceContext, _In_ const gradient_stops_t & gradientStops, _In_ const D2D1_POINT_2F & center, _In_ const D2D1_POINT_2F & offset, _In_ FLOAT rx, _In_ FLOAT ry, _In_ FLOAT rOffset, _Out_ ID2D1RadialGradientBrush ** gradientBrush) const noexcept;

private:
    static HRESULT GetResource(const WCHAR * resourceName, const WCHAR * resourceType, _Out_ void ** resourceData, _Out_ DWORD * resourceSize);

public:
    CComPtr<ID2D1Factory2> Factory;
};

extern Direct2D _Direct2D;
