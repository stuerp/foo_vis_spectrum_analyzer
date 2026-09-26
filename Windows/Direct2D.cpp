
/** $VER: Direct2D.cpp (2026.09.26) P. Stuer **/

#include "pch.h"

#include "Direct2D.h"

#include <d2d1helper.h>
#include "WIC.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Gets the refresh rate of the current display.
/// </summary>
HRESULT Direct2D::GetRefreshRate(IDXGIDevice1 * dxgiDevice, double & refreshRate) noexcept
{
    refreshRate = 0.;

    if (dxgiDevice == nullptr)
        return E_POINTER;

    // Get the current desktop resolution for mode matching.
    DEVMODE DisplaySettings = { .dmSize = sizeof(DEVMODE) };

    if (!::EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &DisplaySettings))
        return HRESULT_FROM_WIN32(::GetLastError());

    ComPtr<IDXGIAdapter> DXGIAdapter;

    HRESULT hr = dxgiDevice->GetAdapter(DXGIAdapter.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<IDXGIOutput> DXGIOutput;

    // Primary output (index 0)
    hr = DXGIAdapter->EnumOutputs(0, DXGIOutput.GetAddressOf());

    if (FAILED(hr))
        return hr;

    // Find the closest matching mode (includes current refresh rate).
    const DXGI_MODE_DESC TargetMode =
    {
        .Width            = DisplaySettings.dmPelsWidth,
        .Height           = DisplaySettings.dmPelsHeight,
        .Format           = DXGI_FORMAT_R8G8B8A8_UNORM,  // Common format
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling          = DXGI_MODE_SCALING_UNSPECIFIED
    };

    DXGI_MODE_DESC MatchedMode = { };

    hr = DXGIOutput->FindClosestMatchingMode(&TargetMode, &MatchedMode, nullptr);

    if (SUCCEEDED(hr))
    {
        refreshRate = static_cast<double>(MatchedMode.RefreshRate.Numerator) / MatchedMode.RefreshRate.Denominator;

        return hr;
    }

    // Fallback: Enumerate all modes and return the refresh rate of the first exact resolution match.

    UINT Flags = 0;  // Use 0 for current mode matching
    UINT ModeCount = 0;

    hr = DXGIOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, Flags, &ModeCount, nullptr);

    if (FAILED(hr) || (ModeCount == 0))
        return hr;

    std::vector<DXGI_MODE_DESC> Modes(ModeCount);

    hr = DXGIOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, Flags, &ModeCount, Modes.data());

    if (FAILED(hr))
        return hr;

    for (const auto & Mode : Modes)
    {
        if (Mode.Width == TargetMode.Width && Mode.Height == TargetMode.Height)
        {
            refreshRate = static_cast<double>(Mode.RefreshRate.Numerator) / Mode.RefreshRate.Denominator;
            break;
        }
    }

    return hr;
}

/// <summary>
/// Loads a bitmap source from the application resources.
/// </summary>
HRESULT Direct2D::Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) noexcept
{
    void * Data = nullptr;
    DWORD Size;

    HRESULT hr = GetResource(resourceName, resourceType, &Data, &Size);

    if (FAILED(hr))
        return hr;

    ComPtr<IWICStream> Stream;

    hr = WICFactory::Get()->CreateStream(Stream.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = Stream->InitializeFromMemory((BYTE *) Data, Size);

    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapDecoder> Decoder;

    hr = WICFactory::Get()->CreateDecoderFromStream(Stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, Decoder.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapFrameDecode> Frame;

    hr = Decoder->GetFrame(0, Frame.GetAddressOf());

    if (FAILED(hr))
        return hr;

    *source = Frame.Detach();

    return hr;
}

/// <summary>
/// Loads a bitmap source from the specified file path.
/// </summary>
HRESULT Direct2D::Load(const WCHAR * uri, IWICBitmapSource ** source) noexcept
{
    ComPtr<IWICBitmapDecoder> Decoder;

    HRESULT hr = WICFactory::Get()->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, Decoder.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapFrameDecode> Frame;

    hr = Decoder->GetFrame(0, Frame.GetAddressOf());

    if (FAILED(hr))
        return hr;

    *source = Frame.Detach();

    return hr;
}

/// <summary>
/// Gets a scaler that changes the width and the height of the bitmap source.
/// </summary>
HRESULT Direct2D::CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) noexcept
{
    HRESULT hr = WICFactory::Get()->CreateBitmapScaler(scaler);

    if (FAILED(hr))
        return hr;

    // Fit big images.
    FLOAT HScalar = (width  > maxWidth)  ? (FLOAT) maxWidth  / (FLOAT) width  : 1.f;
    FLOAT VScalar = (height > maxHeight) ? (FLOAT) maxHeight / (FLOAT) height : 1.f;

    FLOAT Scalar = (std::min)(HScalar, VScalar);

    width  = (UINT) ((FLOAT) width  * Scalar);
    height = (UINT) ((FLOAT) height * Scalar);

    hr = (*scaler)->Initialize(source, width, height, WICBitmapInterpolationModeCubic);

    return hr;
}

/// <summary>
/// Gets a Direct2D bitmap from a WIC bitmap source.
/// </summary>
HRESULT Direct2D::CreateBitmap(IWICBitmapSource * source, ID2D1DeviceContext * deviceContext, ID2D1Bitmap ** bitmap) noexcept
{
    ComPtr<IWICFormatConverter> Converter;

    HRESULT hr = WICFactory::Get()->CreateFormatConverter(Converter.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = Converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);

    if (FAILED(hr))
        return hr;

    hr = deviceContext->CreateBitmapFromWicBitmap(Converter.Get(), nullptr, bitmap);

    return hr;
}

/// <summary>
/// Gets the data and size of a Win32 resource.
/// </summary>
HRESULT Direct2D::GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize) noexcept
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
HRESULT Direct2D::CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors, std::vector<D2D1_GRADIENT_STOP> & gradientStops) noexcept
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
HRESULT Direct2D::CreateGradientBrush(ID2D1DeviceContext * deviceContext, const gradient_stops_t & gradientStops, const D2D1_SIZE_F & size, bool isHorizontal, ID2D1LinearGradientBrush ** gradientBrush) noexcept
{
    if (gradientStops.empty())
        return E_FAIL;

    gradient_stops_t gs = gradientStops;

    // Because the graph is always rendered in a (0,0) top-left coordinate system, the gradient brush has to be created upside-down to compensate for a vertical flip during rendering.
    std::reverse(gs.begin(), gs.end());

    for (auto & x : gs)
        x.position = 1.f - x.position;

    ComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = deviceContext->CreateGradientStopCollection(gs.data(), (UINT32) gs.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, Collection.GetAddressOf());

    if (FAILED(hr))
        return hr;

    D2D1_POINT_2F Start = isHorizontal ? D2D1::Point2F(       0.f, 0.f) : D2D1::Point2F(0.f, 0.f);
    D2D1_POINT_2F End   = isHorizontal ? D2D1::Point2F(size.width, 0.f) : D2D1::Point2F(0.f, size.height);

    hr = deviceContext->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), D2D1::BrushProperties(), Collection.Get(), gradientBrush);

    return hr;
}

/// <summary>
/// Creates a radial gradient brush.
/// </summary>
HRESULT Direct2D::CreateRadialGradientBrush(ID2D1DeviceContext * deviceContext, const gradient_stops_t & gradientStops, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, ID2D1RadialGradientBrush ** gradientBrush) noexcept
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

    ComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = deviceContext->CreateGradientStopCollection(gs.data(), (UINT32) gs.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, Collection.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = deviceContext->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(center, offset, rx, ry), Collection.Get(), gradientBrush);

    return hr;
}
