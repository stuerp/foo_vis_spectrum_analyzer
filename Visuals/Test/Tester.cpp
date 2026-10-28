
/** $VER: Tester.cpp (2026.10.28) P. Stuer - Implements a minimal visualization for testing purposes. **/

#include <pch.h>

#include "Tester.h"

#pragma hdrstop

namespace Test
{

/// <summary>
/// Destroys this instance.
/// </summary>
tester_t::~tester_t()
{
    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void tester_t::Configure(graph_options_t * options, audio_processor_t * processor, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _GraphOptions = options;
    _Processor    = (processor_t *) processor;

    Reset();

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void tester_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);
}

/// <summary>
/// Renders this instance.
/// </summary>
void tester_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    double Gain = 1.;

    // Axes
    {
        deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        _Brush->SetColor(D2D1::ColorF(RGB(87, 123, 197), 1.0f));

        deviceContext->DrawLine({ 0, _Size.height / 2 }, { _Size.width, _Size.height / 2 }, _Brush.Get());

        _Brush->SetColor(D2D1::ColorF(RGB(87, 123, 197), 0.5f));

        deviceContext->DrawLine({ 0, _Size.height * 0.25f }, { _Size.width, _Size.height * 0.25f }, _Brush.Get());
        deviceContext->DrawLine({ 0, _Size.height * 0.75f }, { _Size.width, _Size.height * 0.75f }, _Brush.Get());
    }

    // Waveform
    {
        deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        const size_t FrameCount   = _Processor->_Chunk.get_sample_count();     // get_sample_count() actually returns the number of frames.
        const size_t ChannelCount = _Processor->_Chunk.get_channel_count();

        const auto dx = _Size.width / (FLOAT) FrameCount;
        const auto h2 = _Size.height / 2.f;
        const auto Scale = h2 * (0.5 * Gain);

        D2D1_POINT_2F p1 = { };
        D2D1_POINT_2F p2 = { };

        {
            _Brush->SetColor(D2D1::ColorF(RGB(134, 171, 241), .5f));

            const audio_sample * const Frames   = _Processor->_Chunk.get_data();
            const audio_sample * const FrameEnd = Frames + (FrameCount * ChannelCount);

            float x = 0.f;

            for (const auto * Frame = Frames; Frame < FrameEnd; Frame += ChannelCount, x += dx)
            {
                const auto y = h2 - (float) (Frame[0] * Scale);

                if (Frame == Frames)
                    p1 = { x, y };
                else
                {
                    p2 = { x, y };

                    deviceContext->DrawLine(p1, p2, _Brush.Get(), 3.f);

                    p1 = p2;
                }
            }
        }

        if (ChannelCount > 1)
        {
            _Brush->SetColor(D2D1::ColorF(RGB(213, 238, 251), 1.f));

            const audio_sample * const Frames   = _Processor->_Chunk.get_data();
            const audio_sample * const FrameEnd = Frames + (FrameCount * ChannelCount);

            float x = 0.f;

            for (const auto * Frame = Frames; Frame < FrameEnd; Frame += ChannelCount, x += dx)
            {
                const auto y = h2 - (float) (Frame[1] * Scale);

                if (Frame == Frames)
                    p1 = { x, y };
                else
                {
                    p2 = { x, y };

                    deviceContext->DrawLine(p1, p2, _Brush.Get(), 3.f);

                    p1 = p2;
                }
            }
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void tester_t::Reset() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT tester_t::CreateDeviceIndependentResources() noexcept
{
    return S_OK;
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
    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_Brush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _Brush.GetAddressOf());
#endif

    if (_State._DiscardResources)
    {
        DeleteColorDependentResources();

        Initialize();

        hr = CreateColorDependentResources(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    if (_State._DiscardResources)
    {
        DeleteSizeDependentResources();

        Initialize();

        hr = CreateSizeDependentResources(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void tester_t::DeleteDeviceSpecificResources() noexcept
{
    DeleteColorDependentResources();
    DeleteSizeDependentResources();

#ifdef _DEBUG
    _Brush.Reset();
#endif
}

/// <summary>
/// Creates the resources that depend on the size of the render target.
/// </summary>
HRESULT tester_t::CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    return S_OK;
}

/// <summary>
/// Deletes the resources that depend on the size of the render target.
/// </summary>
void tester_t::DeleteSizeDependentResources() noexcept
{
}

/// <summary>
/// Creates the resources that depend on the color palette.
/// </summary>
HRESULT tester_t::CreateColorDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    return S_OK;
}

/// <summary>
/// Deletes the resources that depend on the color palette.
/// </summary>
void tester_t::DeleteColorDependentResources() noexcept
{
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void tester_t::Initialize() noexcept
{
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void tester_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        return;

    _State._DiscardResources = true;
}

}
