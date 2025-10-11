
/** $VER: Direct2D.cpp (2024.05.03) P. Stuer **/

#include "pch.h"
#include "Direct2D.h"

#include <libmsc.h>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#ifndef THIS_HINSTANCE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE ((HINSTANCE) &__ImageBase)
#endif

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Direct2D::Initialize()
{
#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_INFORMATION };
#else
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE };
#endif

    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, &Factory);

    if (!SUCCEEDED(hr))
        throw msc::win32_exception("Unable to create Direct2D factory.", (DWORD) hr);

    return hr;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void Direct2D::Terminate()
{
    Factory.Release();
}

/// <summary>
/// Loads a bitmap source from the application resources.
/// </summary>
HRESULT Direct2D::Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) const noexcept
{
    void * Data = nullptr;
    DWORD Size;

    HRESULT hr = GetResource(resourceName, resourceType, &Data, &Size);

    CComPtr<IWICStream> Stream;

    if (SUCCEEDED(hr))
        hr = _WIC.Factory->CreateStream(&Stream);

    if (SUCCEEDED(hr))
        hr = Stream->InitializeFromMemory((BYTE *) Data, Size);

    CComPtr<IWICBitmapDecoder> Decoder;

    if (SUCCEEDED(hr))
        hr = _WIC.Factory->CreateDecoderFromStream(Stream, nullptr, WICDecodeMetadataCacheOnLoad, &Decoder);

    IWICBitmapFrameDecode * Frame = nullptr;

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, &Frame);

    if (SUCCEEDED(hr))
        *source = Frame;

    return hr;
}

/// <summary>
/// Loads a bitmap source from the specified file path.
/// </summary>
HRESULT Direct2D::Load(const WCHAR * uri, IWICBitmapSource ** source) const noexcept
{
    CComPtr <IWICBitmapDecoder> Decoder;

    HRESULT hr = _WIC.Factory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &Decoder);

    IWICBitmapFrameDecode * Frame = nullptr;

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, &Frame);

    if (SUCCEEDED(hr))
        *source = Frame;

    return hr;
}

/// <summary>
/// Gets a scaler that changes the width and the height of the bitmap source.
/// </summary>
HRESULT Direct2D::CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) const noexcept
{
    HRESULT hr = _WIC.Factory->CreateBitmapScaler(scaler);

    if (SUCCEEDED(hr))
    {
        // Fit big images.
        FLOAT HScalar = (width  > maxWidth)  ? (FLOAT) maxWidth  / (FLOAT) width  : 1.f;
        FLOAT VScalar = (height > maxHeight) ? (FLOAT) maxHeight / (FLOAT) height : 1.f;

        FLOAT Scalar = (std::min)(HScalar, VScalar);

        width  = (UINT) ((FLOAT) width  * Scalar);
        height = (UINT) ((FLOAT) height * Scalar);

        hr = (*scaler)->Initialize(source, width, height, WICBitmapInterpolationModeCubic);
    }

    return hr;
}

/// <summary>
/// Gets a Direct2D bitmap from a WIC bitmap source.
/// </summary>
HRESULT Direct2D::CreateBitmap(IWICBitmapSource * source, ID2D1DeviceContext * deviceContext, ID2D1Bitmap ** bitmap) const noexcept
{
    CComPtr<IWICFormatConverter> Converter;

    HRESULT hr = _WIC.Factory->CreateFormatConverter(&Converter);

    if (SUCCEEDED(hr))
        hr = Converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);

    if (SUCCEEDED(hr))
        hr = deviceContext->CreateBitmapFromWicBitmap(Converter, nullptr, bitmap);

    return hr;
}

/// <summary>
/// Gets the data and size of a Win32 resource.
/// </summary>
HRESULT Direct2D::GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize)
{
    HRSRC imageResHandle = ::FindResourceW(THIS_HINSTANCE, resourceName, resourceType);

    if (imageResHandle == NULL)
        return E_FAIL;

    HGLOBAL imageResDataHandle = ::LoadResource(THIS_HINSTANCE, imageResHandle);

    if (imageResDataHandle == NULL)
        return E_FAIL;

    *resourceData = ::LockResource(imageResDataHandle);

    if (*resourceData)
        *resourceSize = ::SizeofResource(THIS_HINSTANCE, imageResHandle);

    return S_OK;
}

/// <summary>
/// Creates a gradient stops vector from a color vector.
/// </summary>
HRESULT Direct2D::CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors, std::vector<D2D1_GRADIENT_STOP> & gradientStops) const noexcept
{
    gradientStops.clear();

    if (colors.empty())
        return S_OK;

    gradientStops.push_back({ 0.f, colors[0] });

    for (size_t i = 1; i < colors.size(); ++i)
        gradientStops.push_back({ (FLOAT) i / (FLOAT) (colors.size() - 1), colors[i] });

    return S_OK;
}

/// <summary>
/// Creates a gradient brush.
/// </summary>
HRESULT Direct2D::CreateGradientBrush(ID2D1DeviceContext * deviceContext, const gradient_stops_t & gradientStops, const D2D1_SIZE_F & size, bool isHorizontal, ID2D1LinearGradientBrush ** gradientBrush) const noexcept
{
    if (gradientStops.empty())
        return E_FAIL;

    gradient_stops_t gs = gradientStops;

    // Because the graph is always rendered in a (0,0) top-left coordinate system, the gradient brush has to be created upside-down to compensate for a vertical flip during rendering.
    std::reverse(gs.begin(), gs.end());

    for (auto & x : gs)
        x.position = 1.f - x.position;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = deviceContext->CreateGradientStopCollection(gs.data(), (UINT32) gs.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_POINT_2F Start = isHorizontal ? D2D1::Point2F(       0.f, 0.f) : D2D1::Point2F(0.f, 0.f);
        D2D1_POINT_2F End   = isHorizontal ? D2D1::Point2F(size.width, 0.f) : D2D1::Point2F(0.f, size.height);

        hr = deviceContext->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), D2D1::BrushProperties(), Collection, gradientBrush);
    }

    return hr;
}

/// <summary>
/// Creates a radial gradient brush.
/// </summary>
HRESULT Direct2D::CreateRadialGradientBrush(ID2D1DeviceContext * deviceContext, const gradient_stops_t & gradientStops, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, ID2D1RadialGradientBrush ** gradientBrush) const noexcept
{
    if (gradientStops.empty())
        return E_FAIL;

    gradient_stops_t gs = gradientStops;

    // Recalculate the stop offsets to take into account the inner radius.
    if (rOffset != 0.f)
    {
        for (auto & x : gs)
            x.position = rOffset + ((1.f - rOffset) * x.position);
    }

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = deviceContext->CreateGradientStopCollection(gs.data(), (UINT32) gs.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
        hr = deviceContext->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(center, offset, rx, ry), Collection, gradientBrush);

    return hr;
}

Direct2D _Direct2D;
