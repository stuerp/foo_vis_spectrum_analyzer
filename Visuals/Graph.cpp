
/** $VER: Graph.cpp (2026.10.11) P. Stuer - Implements a graph on which the visualizations are rendered. **/

#include "pch.h"

#include "Graph.h"
#include "StyleManager.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Configures this instance.
/// </summary>
void graph_t::Configure(graph_options_t * graphOptions, bool isFirst, bool isLast) noexcept
{
    _GraphOptions = graphOptions;
    _IsFirst = isFirst;
    _IsLast = isLast;

    _Description = graphOptions->_Description;

    // Configure the analysis.
    _Analysis.Configure(&_State, graphOptions);

    // Configure the visualization.
    _Visualization.reset();

    _Visualization = visualization_t::Create(_State);

    _Visualization->Configure(&_State, graphOptions, &_Analysis, _IsFirst, _IsLast, _RenderContext.D3DDevice, _RenderContext.D3DDeviceContext);
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

    InitializeMetrics(Rect);

    _Visualization->Move(Rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void graph_t::Reset() noexcept
{
    _Visualization->Reset();

    DeleteDeviceSpecificResources();
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
void graph_t::Render(artwork_t & artwork) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(_RenderContext.D2DDeviceContext);

    if (FAILED(hr))
        return;

    RenderBackground(artwork);

    {
        // FIXME
        _RenderContext.D2DDeviceContext->EndDraw();

        _Visualization->Render3D(_RenderContext);

        // FIXME
        _RenderContext.D2DDeviceContext->BeginDraw();
    }

    _Visualization->Render(_RenderContext.D2DDeviceContext);

    RenderDescription();

    // Erase the copy of the chunk.
    _Analysis._Chunk.reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void graph_t::Initialize() noexcept
{
    _Analysis.Reset();
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
    if ((_State._VisualizationType == VisualizationType::Bars) || (_State._VisualizationType == VisualizationType::Curve))
    {
        const rect_t & cr = (const rect_t &) _Visualization->GetClientRect();

        FLOAT t = cr.Width() / (FLOAT) _Analysis._FrequencyBands.size();

        // Allow non-integer bar widths?
        if (_GraphOptions->_HorizontalAlignment != HorizontalAlignment::Fit)
            t = ::floor(t);

        const FLOAT BarWidth = std::max(t, 2.f);
        const FLOAT SpectrumWidth = (_State._VisualizationType == VisualizationType::Bars) ? BarWidth * (FLOAT) _Analysis._FrequencyBands.size() : cr.Width();
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
    if (_State._VisualizationType == VisualizationType::Spectrogram)
    {
        const D2D1_RECT_F & cr = _Visualization->GetClientRect();

        if (_State._IsHorizontalSpectrogram)
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
void graph_t::RenderBackground(artwork_t & artwork) noexcept
{
    if (_BackgroundStyle.IsEnabled())
        _RenderContext.D2DDeviceContext->FillRectangle(_Rect, _BackgroundStyle._Brush.Get());

    if (!_State._ShowArtworkOnBackground)
        return;

    if ((_State._VisualizationType == VisualizationType::PeakMeter) || (_State._VisualizationType == VisualizationType::LevelMeter) || (_State._VisualizationType == VisualizationType::Oscilloscope))
        return;

    if (artwork.Bitmap() == nullptr)
        return;

    artwork.Render(_RenderContext, _State._FitWindow ? _Visualization->GetRect() : _Visualization->GetClientRect(), &_State);
}

/// <summary>
/// Renders the description.
/// </summary>
void graph_t::RenderDescription() noexcept
{
    if ((_State._VisualizationType == VisualizationType::PeakMeter) || (_State._VisualizationType == VisualizationType::LevelMeter))
        return;

    if (_Description.empty())
        return;

    ComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = DirectWriteFactory::Get()->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle._TextFormat.Get(), _Size.width, _Size.height, TextLayout.GetAddressOf());

    if (FAILED(hr))
        return;

    DWRITE_TEXT_METRICS TextMetrics = { };

    hr = TextLayout->GetMetrics(&TextMetrics);

    if (FAILED(hr))
        return;

    constexpr FLOAT Inset = 2.f;

    D2D1_RECT_F Rect =
    {
        .left = _Visualization->GetClientRect().left + 10.f,
        .top  = _Visualization->GetClientRect().top  + 10.f,
    };

    Rect.right  = Rect.left + TextMetrics.width  + (Inset * 2.f);
    Rect.bottom = Rect.top  + TextMetrics.height + (Inset * 2.f);

    if (_DescriptionBackgroundStyle.IsEnabled())
        _RenderContext.D2DDeviceContext->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionBackgroundStyle._Brush.Get());

    if (_DescriptionTextStyle.IsEnabled())
        _RenderContext.D2DDeviceContext->DrawText(_Description.c_str(), (UINT) _Description.length(), _DescriptionTextStyle._TextFormat.Get(), Rect, _DescriptionTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT graph_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State._SizeHasChanged)
        DeleteDeviceSpecificResources();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _DebugBrush.GetAddressOf());
#endif

    auto & StyleManager = _GraphOptions->_UseLocalStyles ? _GraphOptions->_StyleManager : _State._StyleManager;

    const auto InitializeStyle = [this, deviceContext, & StyleManager](auto & style, VisualElement visualElement, const wchar_t * text) noexcept -> HRESULT
    {
        if (style._Brush != nullptr)
            return S_OK;

        style = *StyleManager.GetStyle(visualElement);

        style.SetColor(&_State);

        return style.CreateDeviceSpecificResources(deviceContext, _Size, text, 1.f);
    };

    hr = InitializeStyle(_BackgroundStyle, VisualElement::GraphBackground, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_DescriptionTextStyle, VisualElement::GraphDescriptionText, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_DescriptionBackgroundStyle, VisualElement::GraphDescriptionBackground, L"");

    if (FAILED(hr))
        return hr;

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void graph_t::DeleteDeviceSpecificResources() noexcept
{
    _DescriptionBackgroundStyle.DeleteDeviceSpecificResources();
    _DescriptionTextStyle.DeleteDeviceSpecificResources();
    _BackgroundStyle.DeleteDeviceSpecificResources();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Forwards the configuration change event to the visualization.
/// </summary>
void graph_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (_Visualization.get() != nullptr)
        _Visualization->OnConfigurationChanged(configurationChanges);
}
