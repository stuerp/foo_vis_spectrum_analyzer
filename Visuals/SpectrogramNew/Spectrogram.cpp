
/** $VER: Spectrogram.cpp (2025.12.14) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "pch.h"

#include "Spectrogram.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
spectrogram_t::spectrogram_t()
{
    _SpectrogramStyle = nullptr;

    _TimeLineStyle = nullptr;
    _TimeTextStyle = nullptr;

    _FreqLineStyle = nullptr;
    _FreqTextStyle = nullptr;

    _NyquistMarkerStyle = nullptr;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
spectrogram_t::~spectrogram_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void spectrogram_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _Settings = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void spectrogram_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Renders the spectrum analysis as a spectrogram.
/// </summary>
void spectrogram_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    // Update the back buffer.
    if (SUCCEEDED(hr))
        hr = UpdateBackBuffer(deviceContext);

    if (SUCCEEDED(hr))
    {
        SetTransform(deviceContext, _BackBufferRect);

        deviceContext->DrawBitmap(_BackBuffer);

        ResetTransform(deviceContext);
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void spectrogram_t::Reset() noexcept
{
    _SpectrumIndex = 0;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT spectrogram_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        OnResize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Spectrogram, deviceContext, _Size, L"", 1.f, &_SpectrogramStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, deviceContext, _Size, L"", 1.f, &_TimeLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, deviceContext, _Size, L"00:00", 1.f, &_TimeTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_FreqLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"99.9fk", 1.f, &_FreqTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::NyquistMarker, deviceContext, _Size, L"", 1.f, &_NyquistMarkerStyle);

    if (SUCCEEDED(hr) && (_DeviceContext == nullptr))
    {
        CComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(&D2DDevice);

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_DeviceContext);
    }

    if (SUCCEEDED(hr) && (_BackBuffer == nullptr))
    {
        const D2D1_BITMAP_PROPERTIES1 BitmapProperties = D2D1::BitmapProperties1
        (
            D2D1_BITMAP_OPTIONS_TARGET,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED) // Required for alpha transparency. Otherwise use D2D1_ALPHA_MODE_IGNORE.
        );

        hr = deviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_BackBuffer);

        if (SUCCEEDED(hr))
        {
            _BackBufferRect = { 0.f, 0.f, _Size.width, _Size.height };

            _DeviceContext->SetTarget(_BackBuffer);
            _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Don't use D2D1_ANTIALIAS_MODE_PER_PRIMITIVE: it causes ghost images in the final output.

            _DeviceContext->BeginDraw();
            _DeviceContext->Clear(); // Make the bitmap completely transparent.
            _DeviceContext->EndDraw();

            _Spectra.resize((size_t) _Size.width);
        }
    }

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void spectrogram_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    _BackBuffer.Release();
    _DeviceContext.Release();

    if (_NyquistMarkerStyle)
    {
        _NyquistMarkerStyle->DeleteDeviceSpecificResources();
        _NyquistMarkerStyle = nullptr;
    }

    if (_FreqTextStyle)
    {
        _FreqTextStyle->DeleteDeviceSpecificResources();
        _FreqTextStyle = nullptr;
    }

    if (_FreqLineStyle)
    {
        _FreqLineStyle->DeleteDeviceSpecificResources();
        _FreqLineStyle = nullptr;
    }

    if (_TimeTextStyle)
    {
        _TimeTextStyle->DeleteDeviceSpecificResources();
        _TimeTextStyle = nullptr;
    }

    if (_TimeLineStyle)
    {
        _TimeLineStyle->DeleteDeviceSpecificResources();
        _TimeLineStyle = nullptr;
    }

    if (_SpectrogramStyle)
    {
        _SpectrogramStyle->DeleteDeviceSpecificResources();
        _SpectrogramStyle = nullptr;
    }
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void spectrogram_t::OnResize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    // Release resources that are size dependent.
    _BackBuffer.Release();
    _DeviceContext.Release();

    _IsResized = false;
}

/// <summary>
/// Updates the back buffer.
/// </summary>
HRESULT spectrogram_t::UpdateBackBuffer(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_Analysis->_FrequencyBands.empty())
        return S_OK;

    FLOAT x = (FLOAT) _SpectrumIndex;

    D2D1_RECT_F r = D2D1::RectF(x, 0.f, x + 1.f, 0.f);
    FLOAT & y1 = r.top;
    FLOAT & y2 = r.bottom;

    _DeviceContext->BeginDraw();

    for (; y1 < _Size.height; ++y1)
    {
//      const size_t i = msc::Map(y1, 0.f, _Size.height - 1, (size_t) 0, _Analysis->_FrequencyBands.size() - 1);
        const size_t i = msc::Map(y1, 0.f, _Size.height, (size_t) 0, _Analysis->_FrequencyBands.size());
        const auto fb = _Analysis->_FrequencyBands[i];

        _SpectrogramStyle->SetBrushColor(fb.CurValue);

        if ((fb.Lo >= _Analysis->_NyquistFrequency) && _State->_SuppressMirrorImage)
            break;

        y2 = y1 + 1.f;

        _DeviceContext->FillRectangle(r, _SpectrogramStyle->_Brush);
    }

    HRESULT hr = _DeviceContext->EndDraw();

    // Do some bookkeeping.
    _Spectra[_SpectrumIndex++] = _Analysis->_FrequencyBands;

    if (_SpectrumIndex >= (size_t) _Size.width)
        _SpectrumIndex = 0;

    return hr;
}
