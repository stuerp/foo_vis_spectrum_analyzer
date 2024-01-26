
/** $VER: Direct2D.cpp (2024.01.21) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct2D.h"
#include "WIC.h"

#include "COMException.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#pragma hdrstop

struct ColorHSL
{
    FLOAT H;
    FLOAT S;
    FLOAT L;
};

ColorHSL RGB2HSL(const D2D1_COLOR_F & rgb);
D2D1_COLOR_F HSL2RGB(const ColorHSL & hsl);
FLOAT EvalHSL(FLOAT x, FLOAT y, FLOAT z);

/// <summary>
/// Initializes a new instance.
/// </summary>
Direct2D::Direct2D()
{
#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE /*D2D1_DEBUG_LEVEL_INFORMATION*/ }; // FIXME: Debug complains about a dangling reference to the Direct2D factory when exiting foobar2000. No clue where the leak is.
#else
    D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE };
#endif

    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, &Factory);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create Direct2D factory.");
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

/// <summary>
/// Creates a gradient stops vector from a color vector.
/// </summary>
HRESULT Direct2D::CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors, std::vector<D2D1_GRADIENT_STOP> & gradientStops) const noexcept
{
    gradientStops.clear();

    if (colors.size() == 0)
        return S_OK;

    gradientStops.push_back({ 0.f, colors[0] });

    for (size_t i = 1; i < colors.size(); ++i)
        gradientStops.push_back({ (FLOAT) i / (FLOAT) (colors.size() - 1), colors[i] });

    return S_OK;
}

/// <summary>
/// Sorts a color vector by hue.
/// </summary>
void Direct2D::SortColorsByHue(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by saturation.
/// </summary>
void Direct2D::SortColorsBySaturation(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by lightness.
/// </summary>
void Direct2D::SortColorsByLightness(std::vector<D2D1_COLOR_F> & colors, bool ascending) const noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        return false;
    });
}

/// <summary>
/// Converts a RGB color to HSL.
/// </summary>
ColorHSL RGB2HSL(const D2D1_COLOR_F & rgb)
{
    FLOAT MaxC = (std::max)((std::max)(rgb.r, rgb.g), rgb.b);
    FLOAT MinC = (std::min)((std::min)(rgb.r, rgb.g), rgb.b);
  
    ColorHSL HSL = {};

    if (MaxC != MinC)
    {
        FLOAT d = MaxC - MinC;
   
        if (MaxC == rgb.r)
            HSL.H = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.f : 0.f);
        else
        if (MaxC == rgb.g)
            HSL.H = (rgb.b - rgb.r) / d + 2.f;
        else
        if (MaxC == rgb.b)
            HSL.H = (rgb.r - rgb.g) / d + 4.f;
    
        HSL.H /= 6.f;
        HSL.S = (HSL.L > 0.5f) ? d / (2.f - MaxC - MinC) : d / (MaxC + MinC);
        HSL.L = (MaxC + MinC) / 2.f;
    }

    return HSL;
}

/// <summary>
/// Converts a HSL color to RGB.
/// </summary>
D2D1_COLOR_F HSL2RGB(const ColorHSL & hsl)
{
    if (hsl.S == 0)
        return D2D1::ColorF(hsl.L, hsl.L, hsl.L); // Achromatic

    FLOAT q = (hsl.L < 0.5f) ? hsl.L * (1.f + hsl.S) : hsl.L + hsl.S - hsl.L * hsl.S;
    FLOAT p = 2.f * hsl.L - q;

    return D2D1::ColorF(EvalHSL(p, q, hsl.H + 1.f / 3.f), EvalHSL(p, q, hsl.H), EvalHSL(p, q, hsl.H - 1.f / 3.f));
}

FLOAT EvalHSL(FLOAT x, FLOAT y, FLOAT z)
{
    if (z < 0) 
        z += 1.f;

    if (z > 1) 
        z -= 1.f;

    if (z < 1.f / 6.f) 
        return x + (y - x) * 6.f * z;

    if (z < 1. / 2.f) 
        return y;

    if (z < 2.f / 3.f)
        return x + (y - x) * (2.f / 3.f - z) * 6.f;
    
    return x;
}

Direct2D _Direct2D;
