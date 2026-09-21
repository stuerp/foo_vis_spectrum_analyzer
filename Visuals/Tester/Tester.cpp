
/** $VER: Tester.cpp (2026.06.24) P. Stuer - Implements a minimal visualization for testing purposes. **/

#include <pch.h>

#include "Tester.h"

#include <cmath>
#include <random>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
tester_t::tester_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
tester_t::~tester_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void tester_t::Configure(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device> d3dDevice, CComPtr<ID3D11DeviceContext> d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void tester_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void tester_t::Reset() noexcept
{
    if (!_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    _ForceElementToResize = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void tester_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void tester_t::Resize() noexcept
{
    if (!_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    _ForceElementToResize = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void tester_t::Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    double Gain = 1.;

    // Axes
    {
        deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        _Brush->SetColor(D2D1::ColorF(RGB(87, 123, 197), 1.0f));

        deviceContext->DrawLine({ 0, _Size.height / 2 }, { _Size.width, _Size.height / 2 }, _Brush);

        _Brush->SetColor(D2D1::ColorF(RGB(87, 123, 197), 0.5f));

        deviceContext->DrawLine({ 0, _Size.height * 0.25f }, { _Size.width, _Size.height * 0.25f }, _Brush);
        deviceContext->DrawLine({ 0, _Size.height * 0.75f }, { _Size.width, _Size.height * 0.75f }, _Brush);
    }

    // Waveform
    {
        deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        const audio_sample * Frames = _Analysis->_Chunk.get_data();

        const size_t FrameCount   = _Analysis->_Chunk.get_sample_count();     // get_sample_count() actually returns the number of frames.
        const size_t ChannelCount = _Analysis->_Chunk.get_channel_count();

        const auto dx = _Size.width / (FLOAT) FrameCount;
        const auto h2 = _Size.height / 2.f;
        const auto Scale = (h2 * 0.85) * (0.5 + Gain);

        D2D1_POINT_2F p1 = { };
        D2D1_POINT_2F p2 = { };

        _Brush->SetColor(D2D1::ColorF(RGB(134, 171, 241), .5f));

        for (size_t i = 0; i < FrameCount * ChannelCount; i += ChannelCount)
        {
            const auto x = (FLOAT) ((FLOAT) i * dx);
            const auto y = (FLOAT) h2 - (FLOAT) (Frames[i] * Scale); // FIX ME: Only the first channel is used.

            if (i == 0)
                p1 = { x, y };
            else
            {
                p2 = { x, y };

                deviceContext->DrawLine(p1, p2, _Brush, 3.f);

                p1 = p2;
            }
        }

        _Brush->SetColor(D2D1::ColorF(RGB(213, 238, 251), 1.f));

        for (size_t i = 0; i < FrameCount * ChannelCount; i += ChannelCount)
        {
            const auto x = (FLOAT) ((FLOAT) i * dx);
            const auto y = (FLOAT) h2 - (FLOAT) (Frames[i] * Scale); // FIX ME: Only the first channel is used.

            if (i == 0)
                p1 = { x, y };
            else
            {
                p2 = { x, y };

                deviceContext->DrawLine(p1, p2, _Brush, 1.f);

                p1 = p2;
            }
        }
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT tester_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void tester_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT tester_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_RecreateStyles)
        DeleteDeviceSpecificResources();

    if ((_Size.width <= 0.f) || _Size.height <= 0.f)
        return E_INVALIDARG;

    Resize();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_Brush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_Brush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void tester_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _Brush.Release();
#endif
}
