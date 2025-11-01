
/** $VER: OscilloscopeXY.cpp (2025.10.25) P. Stuer - Implements an oscilloscope in X-Y mode. **/

#include <pch.h>

#include "OscilloscopeXY.h"
#include "AmplitudeScaler.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
oscilloscope_xy_t::oscilloscope_xy_t()
{
    _XAxisTextStyle = nullptr;
    _YAxisTextStyle = nullptr;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_xy_t::~oscilloscope_xy_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_xy_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void oscilloscope_xy_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void oscilloscope_xy_t::Reset() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void oscilloscope_xy_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    oscilloscope_base_t::Resize();

    // Release resources that are size dependent.
    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->DeleteDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->DeleteDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    _GridCommandList.Release();

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void oscilloscope_xy_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    const auto Translate = D2D1::Matrix3x2F::Translation(_Size.width / 2.f, _Size.height / 2.f);
    const auto Scale     = D2D1::Matrix3x2F::Scale(D2D1::SizeF(_ScaleFactor, _ScaleFactor));

    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    const uint32_t ChunkChannels    = _Analysis->_Chunk.get_channel_config();                   // Mask containing the channels in the audio chunk.
    const uint32_t SelectedChannels = _GraphDescription->_SelectedChannels;                     // Mask containing the channels selected by the user.
    const uint32_t BalanceChannels  = analysis_t::ChannelPairs[(size_t) _State->_ChannelPair];  // Mask containing the channels selected by the user as a channel pair.

    const uint32_t ChannelMask = ChunkChannels & SelectedChannels & BalanceChannels;

    if ((FrameCount >= 2) && (ChannelCount >= 2) && (ChannelMask != 0))
    {
        const audio_sample * Samples = _Analysis->_Chunk.get_data();

        const size_t Channel1 = (size_t) std::countr_zero(ChannelMask);         // Index of the channel 1 sample in the audio chunk.
        const size_t Channel2 = (size_t) (31 - std::countl_zero(ChannelMask));  // Index of the channel 2 sample in the audio chunk.

        CComPtr<ID2D1TransformedGeometry> TransformedGeometry;

        // Create the geometry for the X-Y plot.
        {
            CComPtr<ID2D1PathGeometry> Geometry;

            hr = _Direct2D.Factory->CreatePathGeometry(&Geometry);

            if (SUCCEEDED(hr))
            {
                CComPtr<ID2D1GeometrySink> Sink;

                hr = Geometry->Open(&Sink);

                FLOAT x = (FLOAT) std::clamp(Samples[Channel1] * _State->_XGain, -1., 1.);
                FLOAT y = (FLOAT) std::clamp(Samples[Channel2] * _State->_YGain, -1., 1.);

                Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

                for (size_t i = ChannelCount; i < FrameCount; i += ChannelCount)
                {
                    x = (FLOAT) std::clamp(Samples[i + Channel1] * _State->_XGain, -1., 1.);
                    y = (FLOAT) std::clamp(Samples[i + Channel2] * _State->_YGain, -1., 1.);

                    Sink->AddLine(D2D1::Point2F(x, y));
                }

                Sink->EndFigure(D2D1_FIGURE_END_OPEN);

                hr = Sink->Close();
            }

            if (SUCCEEDED(hr))
                hr = _Direct2D.Factory->CreateTransformedGeometry(Geometry, Scale * Translate, &TransformedGeometry);
        }

        if (SUCCEEDED(hr))
        {
            _DeviceContext->SetTarget(_BackBuffer);
            _DeviceContext->BeginDraw();

            _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            _DeviceContext->DrawGeometry(TransformedGeometry, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);

            hr = _DeviceContext->EndDraw();
        }
    }

    if (SUCCEEDED(hr))
    {
        // Draw the grid.
        {
            deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            deviceContext->SetTransform(Translate);

            deviceContext->DrawImage(_GridCommandList);

            deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        }

        // Draw the back buffer to the window.
        {
            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);

            deviceContext->DrawBitmap(_BackBuffer);

            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        }

        // Add the phosphor afterglow effect before the next pass.
        {
            _DeviceContext->SetTarget(_FrontBuffer);
            _DeviceContext->BeginDraw();

            if (_State->_PhosphorDecay)
            {
                _GaussBlurEffect->SetInput(0, _BackBuffer);

                _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);
                _DeviceContext->DrawImage(_GaussBlurEffect);

                _ColorMatrixEffect->SetInput(0, _BackBuffer);

                _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
                _DeviceContext->DrawImage(_ColorMatrixEffect);
            }
            else
                _DeviceContext->Clear(); // Required for alpha transparency

            hr = _DeviceContext->EndDraw();
        }

        std::swap(_FrontBuffer, _BackBuffer);
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_xy_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = oscilloscope_base_t::CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceIndependentResources() noexcept
{
    oscilloscope_base_t::DeleteDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_xy_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    _ScaleFactor = std::min((_Size.width - 1.f) / 2.f, (_Size.height - 1.f) / 2.f);

    if (SUCCEEDED(hr))
        Resize();

    if (SUCCEEDED(hr))
        hr = oscilloscope_base_t::CreateDeviceSpecificResources(deviceContext);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, _DeviceContext, _Size, L"+0.0", _ScaleFactor, &_XAxisTextStyle);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, _DeviceContext, _Size, L"+0.0", _ScaleFactor, &_YAxisTextStyle);

    if (SUCCEEDED(hr) && (_GridCommandList == nullptr))
        hr = CreateGridCommandList();

    return hr;
}

/// <summary>
/// Deletes the device specific resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceSpecificResources() noexcept
{
    _GridCommandList.Release();

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->DeleteDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->DeleteDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    oscilloscope_base_t::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates a command list to render the grid and the X and Y axis labels.
/// This is created in a -1 .. 1 axis setup and scaled up as necessary.
/// </summary>
HRESULT oscilloscope_xy_t::CreateGridCommandList() noexcept
{
    HRESULT hr = S_OK;

    const auto ScaleTransform = D2D1::Matrix3x2F::Scale(D2D1::SizeF(_ScaleFactor, _ScaleFactor));

    // Create a command list that will store the grid pattern and the axes.
    if (SUCCEEDED(hr))
        hr = _DeviceContext->CreateCommandList(&_GridCommandList);

    if (SUCCEEDED(hr))
    {
        WCHAR Text[6] = { };

        _DeviceContext->SetTarget(_GridCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetTransform(ScaleTransform);

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

        // Draw the X-axis, Y-axis and the center label.
        {
            D2D1_RECT_F TextRect = { -1.f, 0.01f, 1.f, 1.f };

            if (_GraphDescription->HasXAxis() || _GraphDescription->HasYAxis())
            {
                _XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                _XAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                _DeviceContext->DrawText(L"0.0", 3, _XAxisTextStyle->_TextFormat, TextRect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphDescription->HasXAxis())
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  0.f), D2D1::Point2F(1.f, 0.f), _XAxisLineStyle->_Brush, 1.f, _AxisStrokeStyle);

            if (_GraphDescription->HasYAxis())
                _DeviceContext->DrawLine(D2D1::Point2F( 0.f, -1.f), D2D1::Point2F(0.f, 1.f), _YAxisLineStyle->_Brush, 1.f, _AxisStrokeStyle);
        }

        _XAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        for (FLOAT x = .2f; x < 1.01f; x += .2f)
        {
            // Draw the vertical grid line.
            if (_VerticalGridLineStyle->IsEnabled())
            {
                _DeviceContext->DrawLine(D2D1::Point2F( x, -1.f), D2D1::Point2F( x, 1.f), _VerticalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);
                _DeviceContext->DrawLine(D2D1::Point2F(-x, -1.f), D2D1::Point2F(-x, 1.f), _VerticalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);
            }

            if (_GraphDescription->HasXAxis())
            {
                // Draw the negative X label.
                _XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                D2D1_RECT_F TextRect = { -x + 0.01f, 0.01f, 0.f, 1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%-.1f", -x);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle->_TextFormat, TextRect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                // Draw the positive X label.
                _XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                TextRect = { 0.f, 0.01f, x - 0.01f, 1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", x);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle->_TextFormat, TextRect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!_GraphDescription->HasXAxis())
            _DeviceContext->DrawLine(D2D1::Point2F(0.f, -1.f), D2D1::Point2F(0.f, 1.f), _VerticalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);

        _YAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

        for (FLOAT y = .2f; y < 1.01f; y += .2f)
        {
            // Draw the horizontal grid line.
            if (_HorizontalGridLineStyle->IsEnabled())
            {
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  y), D2D1::Point2F(1.f,  y), _HorizontalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f, -y), D2D1::Point2F(1.f, -y), _HorizontalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);
            }

            if (_GraphDescription->HasYAxis())
            {
                // Draw the negative y label.
                _YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                D2D1_RECT_F TextRect = { 0.01f, y - 0.01f, 1.f, -1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%-.1f", -y);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _YAxisTextStyle->_TextFormat, TextRect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                // Draw the positive y label.
                _YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                TextRect = { 0.01f, -y + 0.01f, 1.f, 0.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", y);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _YAxisTextStyle->_TextFormat, TextRect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!_GraphDescription->HasYAxis())
            _DeviceContext->DrawLine(D2D1::Point2F(-1.f, 0.f), D2D1::Point2F(1.f, 0.f), _HorizontalGridLineStyle->_Brush, 1.f, _AxisStrokeStyle);

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _GridCommandList->Close();

    return hr;
}
