
/** $VER: Graph.cpp (2026.08.22) P. Stuer - Implements a graph on which the visualizations are rendered. **/

#include "pch.h"

#include "Graph.h"
#include "StyleManager.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#include "Spectrum.h"
#include "Spectrogram.h"
#include "PeakMeter.h"
#include "LevelMeter.h"
#include "Oscilloscope.h"
#include "OscilloscopeXY.h"

#include "BitMeter.h"

#include "Tester.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
graph_t::graph_t()
{
}

/// <summary>
/// Destroys this instance.
/// </summary>
graph_t::~graph_t() noexcept
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void graph_t::Initialize(state_t * state, graph_options_t * graphOptions, bool isFirst, bool isLast) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _IsFirst = isFirst;
    _IsLast = isLast;

    _Description = graphOptions->_Description;

    _Analysis.Initialize(state, graphOptions);

    _Visualization.reset();

    switch (_State->_VisualizationType)
    {
        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
            _Visualization = std::make_unique<spectrum_t>();
            break;

        case VisualizationType::Spectrogram:
            _Visualization = std::make_unique<spectrogram_t>();
            break;

        case VisualizationType::PeakMeter:
            _Visualization = std::make_unique<peak_meter_t>();
            break;

        case VisualizationType::LevelMeter:
            _Visualization = std::make_unique<level_meter_t>();
            break;

        case VisualizationType::Oscilloscope:
            if (!_State->_XYMode)
                _Visualization = std::make_unique<oscilloscope_t>();
            else
                _Visualization = std::make_unique<oscilloscope_xy_t>();
            break;

        case VisualizationType::BitMeter:
            _Visualization = std::make_unique<bit_meter_t>();
            break;

        case VisualizationType::Tester:
        default:
            _Visualization = std::make_unique<tester_t>();
            break;
    }

    _Visualization->Initialize(state, graphOptions, &_Analysis, _IsFirst, _IsLast);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void graph_t::Move(const D2D1_RECT_F & rect) noexcept
{
    const D2D1_RECT_F Rect =
    {
        .left   = rect.left   + _GraphOptions->_LPadding,
        .top    = rect.top    + _GraphOptions->_TPadding,
        .right  = rect.right  - _GraphOptions->_RPadding,
        .bottom = rect.bottom - _GraphOptions->_BPadding
    };

    SetRect(Rect);

    _Visualization->Move(Rect);
}

/// <summary>
/// Processes an audio chunk.
/// </summary>
void graph_t::Process(const audio_chunk & chunk) noexcept
{
    _Analysis.Process(chunk); // Delegate it to the analysis.
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void graph_t::Render(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept
{
    if (_State->_RecreateStyles)
        DeleteDeviceSpecificResources();

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    RenderBackground(deviceContext, artwork);
    RenderForeground(deviceContext);
}

/// <summary>
/// Resets this instance.
/// </summary>
void graph_t::Reset() noexcept
{
    _Analysis.Reset();

    _Visualization->Reset();
}

/// <summary>
/// Releases this instance.
/// </summary>
void graph_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes a structure with the tool area of this graph.
/// </summary>
void graph_t::InitToolInfo(HWND hWnd, TTTOOLINFOW & ti) const noexcept
{
    ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, hWnd, (UINT_PTR) hWnd, nullptr, nullptr);

    ::SetRect(&ti.rect, (int) _Rect.left, (int) _Rect.top, (int) _Rect.right, (int) _Rect.bottom);
}

/// <summary>
/// Gets the tooltip and frequency band index at the specified x or y position.
/// </summary>
bool graph_t::GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & bandIndex) const noexcept
{
    if ((_State->_VisualizationType == VisualizationType::Bars) || (_State->_VisualizationType == VisualizationType::Curve))
    {
        const rect_t & cr = (const rect_t &) _Visualization->GetClientRect();

        FLOAT t = cr.Width() / (FLOAT) _Analysis._FrequencyBands.size();

        // Allow non-integer bar widths?
        if (_GraphOptions->_HorizontalAlignment != HorizontalAlignment::Fit)
            t = ::floor(t);

        const FLOAT BarWidth = std::max(t, 2.f);
        const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? BarWidth * (FLOAT) _Analysis._FrequencyBands.size() : cr.Width();
        const FLOAT HOffset = GetHOffset(_GraphOptions->_HorizontalAlignment, cr.Width() - SpectrumWidth);

        const FLOAT x1 = cr.x1 + HOffset;
        const FLOAT x2 = x1 + SpectrumWidth;

        if (!msc::InRange(x, x1, x2))
            return false;

        if (_GraphOptions->_FlipHorizontally)
            x = (x2 + x1) - x;

        bandIndex = std::clamp((size_t) ::floor(msc::Map(x, x1, x2, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
    if (_State->_VisualizationType == VisualizationType::Spectrogram)
    {
        const D2D1_RECT_F & cr = _Visualization->GetClientRect();

        if (_State->_IsHorizontalSpectrogram)
        {
            if (!msc::InRange(y, cr.top, cr.bottom))
                return false;

            if (!_GraphOptions->_FlipVertically)
                y = (cr.bottom + cr.top) - y;

            bandIndex = std::clamp((size_t) ::floor(msc::Map(y, cr.top, cr.bottom, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
        }
        else
        {
            if (!msc::InRange(x, cr.left, cr.right))
                return false;

            bandIndex = std::clamp((size_t) ::floor(msc::Map(x, cr.left, cr.right, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
        }
    }
    else
        return false; // No tooltip available.

    toolTip = _Analysis._FrequencyBands[bandIndex].Label;

    return true;
}

/// <summary>
/// Renders the background.
/// </summary>
void graph_t::RenderBackground(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept
{
    if (_BackgroundStyle.IsEnabled())
        deviceContext->FillRectangle(_Rect, _BackgroundStyle._Brush);

    if (!_State->_ShowArtworkOnBackground)
        return;

    if ((_State->_VisualizationType == VisualizationType::PeakMeter) || (_State->_VisualizationType == VisualizationType::LevelMeter) || (_State->_VisualizationType == VisualizationType::Oscilloscope))
        return;

    if (artwork.Bitmap() == nullptr)
        return;

    artwork.Render(deviceContext, _State->_FitWindow ? _Visualization->GetRect() : _Visualization->GetClientRect(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void graph_t::RenderForeground(ID2D1DeviceContext * deviceContext) noexcept
{
    _Visualization->Render(deviceContext);

    if ((_State->_VisualizationType == VisualizationType::PeakMeter) || (_State->_VisualizationType == VisualizationType::LevelMeter))
        return;

    RenderDescription(deviceContext);
}

/// <summary>
/// Renders the description.
/// </summary>
void graph_t::RenderDescription(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_Description.empty())
        return;

    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle._TextFormat, _Size.width, _Size.height, &TextLayout);

    DWRITE_TEXT_METRICS TextMetrics = { };

    if (!SUCCEEDED(hr))
        return;

    hr = TextLayout->GetMetrics(&TextMetrics);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT Inset = 2.f;

    D2D1_RECT_F Rect =
    {
        .left = _Visualization->GetClientRect().left + 10.f,
        .top  = _Visualization->GetClientRect().top  + 10.f,
    };

    Rect.right  = Rect.left + TextMetrics.width  + (Inset * 2.f);
    Rect.bottom = Rect.top  + TextMetrics.height + (Inset * 2.f);

    if (_DescriptionBackgroundStyle.IsEnabled())
        deviceContext->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionBackgroundStyle._Brush);

    if (_DescriptionTextStyle.IsEnabled())
        deviceContext->DrawText(_Description.c_str(), (UINT) _Description.length(), _DescriptionTextStyle._TextFormat, Rect, _DescriptionTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT graph_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    auto & StyleManager = _GraphOptions->_UseLocalStyles ? _GraphOptions->_StyleManager : _State->_StyleManager;

    if (_BackgroundStyle._Brush == nullptr)
    {
        _BackgroundStyle = *StyleManager.GetStyle(VisualElement::GraphBackground);

        _BackgroundStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BackgroundStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_DescriptionTextStyle._Brush == nullptr)
    {
        _DescriptionTextStyle = *StyleManager.GetStyle(VisualElement::GraphDescriptionText);

        _DescriptionTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _DescriptionTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_DescriptionBackgroundStyle._Brush == nullptr)
    {
        _DescriptionBackgroundStyle = *StyleManager.GetStyle(VisualElement::GraphDescriptionBackground);

        _DescriptionBackgroundStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _DescriptionBackgroundStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif
    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void graph_t::DeleteDeviceSpecificResources() noexcept
{
    _Visualization->Release();

    _DescriptionBackgroundStyle.DeleteDeviceSpecificResources();
    _DescriptionTextStyle.DeleteDeviceSpecificResources();
    _BackgroundStyle.DeleteDeviceSpecificResources();

#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}

/// <summary>
/// Forwards the configuration change event to the visualization.
/// </summary>
void graph_t::OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept
{
    if (_Visualization.get() != nullptr)
        _Visualization->OnConfigurationChange(configurationChanges);
}
