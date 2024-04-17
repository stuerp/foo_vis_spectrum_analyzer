
/** $VER: Graph.cpp (2024.04.15) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#include "framework.h"
#include "Graph.h"

#include "StyleManager.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Graph::Graph()
{
}

/// <summary>
/// Destroys this instance.
/// </summary>
Graph::~Graph()
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Graph::Initialize(State * state, const GraphSettings * settings) noexcept
{
    _State = state;
    _GraphSettings = settings;

    _Description = settings->_Description;

    _Analysis.Initialize(state, settings);

    _Spectrum.Initialize(state, settings, &_Analysis);
    _Spectogram.Initialize(state, settings, &_Analysis);
    _PeakMeter.Initialize(state, settings, &_Analysis);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect) noexcept
{
    const D2D1_RECT_F cr = { rect.left + _GraphSettings->_LPadding, rect.top + _GraphSettings->_TPadding, rect.right - _GraphSettings->_RPadding, rect.bottom - _GraphSettings->_BPadding };

    SetBounds(cr);

    _Spectrum.Move(cr);
    _Spectogram.Move(cr);
    _PeakMeter.Move(cr);
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void Graph::Render(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        RenderBackground(renderTarget, artwork);
        RenderForeground(renderTarget);
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void Graph::Reset()
{
    for (FrequencyBand & fb : _Analysis._FrequencyBands)
        fb.CurValue = 0.;

    for (MeterValue & mv : _Analysis._GaugeValues)
    {
        mv.Peak = mv.RMS = -std::numeric_limits<double>::infinity();
        mv.PeakRender = mv.RMSRender = 0.;
    }

    _Spectrum.Reset();
    _Spectogram.Reset();
    _PeakMeter.Reset();
}

/// <summary>
/// Initializes a structure with the tool area of this graph.
/// </summary>
void Graph::InitToolInfo(HWND hWnd, TTTOOLINFOW & ti) const noexcept
{
    ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, hWnd, (UINT_PTR) hWnd, nullptr, nullptr);

    ::SetRect(&ti.rect, (int) _Bounds.left, (int) _Bounds.top, (int) _Bounds.right, (int) _Bounds.bottom);
}

/// <summary>
/// Gets the tooltip at the specified x or y position.
/// </summary>
bool Graph::GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & index) const noexcept
{
    if ((_State->_VisualizationType == VisualizationType::Bars) || (_State->_VisualizationType == VisualizationType::Curve))
    {
        const D2D1_RECT_F & Bounds = _Spectrum.GetClientBounds();

        const FLOAT Bandwidth = Max(::floor((Bounds.right - Bounds.left) / (FLOAT) _Analysis._FrequencyBands.size()), 2.f);

        const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? Bandwidth * (FLOAT) _Analysis._FrequencyBands.size() : Bounds.right - Bounds.left;

        const FLOAT x1 = Bounds.left + ((Bounds.right - Bounds.left) - SpectrumWidth) / 2.f;
        const FLOAT x2 = x1 + SpectrumWidth;

        if (!InRange(x, x1, x2))
            return false;

        if (_GraphSettings->_FlipHorizontally)
            x = (x2 + x1) - x;

        index = std::clamp((size_t) ::floor(Map(x, x1, x2, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
    if (_State->_VisualizationType == VisualizationType::Spectogram)
    {
        const D2D1_RECT_F & Bounds = _Spectogram.GetClientBounds();

        if (!InRange(y, Bounds.top, Bounds.bottom))
            return false;

        if (!_GraphSettings->_FlipVertically)
            y = (Bounds.bottom + Bounds.top) - y;

        index = std::clamp((size_t) ::floor(Map(y, Bounds.top, Bounds.bottom, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
        return false;

    toolTip = _Analysis._FrequencyBands[index].Label;

    return true;
}

/// <summary>
/// Renders the background.
/// </summary>
void Graph::RenderBackground(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept
{
//  if (_BackgroundStyle->IsEnabled())
        renderTarget->FillRectangle(_Bounds, _BackgroundStyle->_Brush);

    // Render the bitmap if there is one.
    if (_State->_ShowArtworkOnBackground && (_State->_VisualizationType != VisualizationType::PeakMeter) && (artwork.Bitmap() != nullptr))
        artwork.Render(renderTarget, _State->_FitWindow ? _Spectrum.GetBounds() : _Spectrum.GetClientBounds(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void Graph::RenderForeground(ID2D1RenderTarget * renderTarget) noexcept
{
    switch (_State->_VisualizationType)
    {
        case VisualizationType::Bars:
        case VisualizationType::Curve:
        {
            _Spectrum.Render(renderTarget);
            RenderDescription(renderTarget);
            break;
        }

        case VisualizationType::Spectogram:
        {
            _Spectogram.Render(renderTarget);
            RenderDescription(renderTarget);
            break;
        }

        case VisualizationType::PeakMeter:
        {
            _PeakMeter.Render(renderTarget);
            break;
        }
    }
}

/// <summary>
/// Renders the description.
/// </summary>
void Graph::RenderDescription(ID2D1RenderTarget * renderTarget) noexcept
{
    if (_Description.empty())
        return;

    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle->_TextFormat, _Size.width, _Size.height, &TextLayout);

    DWRITE_TEXT_METRICS TextMetrics = { };

    if (SUCCEEDED(hr))
        hr = TextLayout->GetMetrics(&TextMetrics);

    if (SUCCEEDED(hr))
    {
        const FLOAT Inset = 2.f;

        D2D1_RECT_F Rect = { };

        Rect.left   = _Spectrum.GetClientBounds().left + 10.f;
        Rect.top    = _Spectrum.GetClientBounds().top  + 10.f;
        Rect.right  = Rect.left + TextMetrics.width  + (Inset * 2.f);
        Rect.bottom = Rect.top  + TextMetrics.height + (Inset * 2.f);

        if (_DescriptionBackgroundStyle->IsEnabled())
            renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionBackgroundStyle->_Brush);

        if (_DescriptionTextStyle->IsEnabled())
            renderTarget->DrawText(_Description.c_str(), (UINT) _Description.length(), _DescriptionTextStyle->_TextFormat, Rect, _DescriptionTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Graph::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphBackground, renderTarget, _Size, L"", &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionText, renderTarget, _Size, L"", &_DescriptionTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionBackground, renderTarget, _Size, L"", &_DescriptionBackgroundStyle);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Graph::ReleaseDeviceSpecificResources() noexcept
{
    _PeakMeter.ReleaseDeviceSpecificResources();

    _Spectogram.ReleaseDeviceSpecificResources();

    _Spectrum.ReleaseDeviceSpecificResources();

    SafeRelease(&_DescriptionBackgroundStyle);
    SafeRelease(&_DescriptionTextStyle);
    SafeRelease(&_BackgroundStyle);
}
