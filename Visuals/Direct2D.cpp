
/** $VER: Direct2D.cpp (2024.01.15) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct2D.h"
#include "WIC.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Direct2D::Direct2D()
{
    Initialize();
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT Direct2D::Initialize()
{
    return ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory);
}

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT Direct2D::GetDPI(HWND hWnd, UINT & dpi) const
{
    dpi = ::GetDpiForWindow(hWnd);

    return S_OK;
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
/// Gets a Direct2D from a WIC source.
/// </summary>
HRESULT Direct2D::CreateBitmap(IWICBitmapSource * source, ID2D1RenderTarget * renderTarget, ID2D1Bitmap ** bitmap) const noexcept
{
    CComPtr<IWICFormatConverter> Converter;

    HRESULT hr = _WIC.Factory->CreateFormatConverter(&Converter);

    if (SUCCEEDED(hr))
        hr = Converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);

    if (SUCCEEDED(hr))
        hr = renderTarget->CreateBitmapFromWicBitmap(Converter, nullptr, bitmap);

    return hr;
}

/// <summary>
/// Gets the data and size of a resource. 
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

Direct2D _Direct2D;
