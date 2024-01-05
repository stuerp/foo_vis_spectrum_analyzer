
/** $VER: WIC.cpp (2024.01.05) P. Stuer **/

#include "WIC.h"

#pragma comment(lib, "windowscodecs")

/// <summary>
/// Initializes a new instance.
/// </summary>
WIC::WIC()
{
    HRESULT hr = ::CoInitialize(nullptr);

    hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_WICFactory));
}

CComPtr<IWICFormatConverter> WIC::Load(const uint8_t * data, size_t size)
{
    CComPtr<IWICStream> Stream;
    CComPtr<IWICBitmapDecoder> Decoder;
    CComPtr<IWICBitmapFrameDecode> Frame;

    HRESULT hr = _WICFactory->CreateStream(&Stream);

    if (SUCCEEDED(hr))
        hr = Stream->InitializeFromMemory((BYTE *) data, (DWORD) size);

    if (SUCCEEDED(hr))
        hr = _WICFactory->CreateDecoderFromStream(Stream, nullptr, WICDecodeMetadataCacheOnLoad, &Decoder);

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, &Frame);

   // Format convert the frame to 32bppPBGRA
    CComPtr<IWICFormatConverter> FormatConverter;

    if (SUCCEEDED(hr))
       hr = _WICFactory->CreateFormatConverter(&FormatConverter);

   if (SUCCEEDED(hr))
       hr = FormatConverter->Initialize(Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

    return FormatConverter;
}

WIC _WIC;
