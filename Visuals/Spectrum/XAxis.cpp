
/** $VER: XAXis.cpp (2025.09.24) P. Stuer - Implements the X axis of a graph. **/

#include "pch.h"
#include "XAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void x_axis_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    _Labels.clear();

    const frequency_bands_t & fb = _Analysis->_FrequencyBands;

    if (fb.empty())
        return;

    _BandCount = fb.size();

    _LoFrequency = fb.front().Ctr;
    _HiFrequency = fb.back().Ctr;

    // Precalculate the labels.
    {
        WCHAR Text[32] = { };

        switch (settings->_XAxisMode)
        {
            case XAxisMode::None:
                break;

            default:

            case XAxisMode::Bands:
            {
                for (size_t i = 0; i < _BandCount; i += 10)
                {
                    double Frequency = fb[i].Ctr;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    label_t lb = { Text, Frequency };

                    _Labels.push_back(lb);
                }
                break;
            }

            case XAxisMode::Decades:
            {
                double Frequency = 0.;
                int i = 1;
                int j = 10;

                while (Frequency < fb.back().Lo)
                {
                    Frequency = j * i;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    label_t lb = { Text, Frequency };

                    _Labels.push_back(lb);

                    if (++i == 10)
                    {
                        i = 1;
                        j *= 10;
                    }
                }
                break;
            }

            case XAxisMode::Octaves:
            {
                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_TuningPitch * ::exp2(Note / 12.); // Frequency of C0

                for (int i = 0; Frequency < fb.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    label_t lb = { Text, Frequency };

                    _Labels.push_back(lb);

                    Note += 12.;
                    Frequency = _State->_TuningPitch * ::exp2(Note / 12.);
                }
                break;
            }

            case XAxisMode::Notes:
            {
                static const char Name[] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
                static const int Step[] = { 2, 2, 1, 2, 2, 2, 1 };

                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_TuningPitch * ::exp2(Note / 12.); // Frequency of C0

                int j = 0;

                while (Frequency < fb.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    label_t lb = { Text, Frequency, j != 0 };

                    _Labels.push_back(lb);

                    Note += Step[j];
                    Frequency = _State->_TuningPitch * ::exp2(Note / 12.);

                    if (j < 6) j++; else j = 0;
                }
                break;
            }
        }
    }
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void x_axis_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void x_axis_t::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    FLOAT t = _Size.width / (FLOAT) _BandCount;

    // Use the full width of the graph?
    if (_GraphSettings->_HorizontalAlignment != HorizontalAlignment::Fit)
        t = ::floor(t);

    // Calculate the position of the labels.
    const FLOAT BarWidth = std::max(t, 2.f); // In DIP
    const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? BarWidth * (FLOAT) _BandCount : _Size.width;
    const FLOAT HOffset = GetHOffset(_GraphSettings->_HorizontalAlignment, _Size.width - SpectrumWidth);

    const FLOAT x1 = !_GraphSettings->_FlipHorizontally ? _Bounds.left  + HOffset : _Bounds.right - HOffset;
    const FLOAT x2 = !_GraphSettings->_FlipHorizontally ? _Bounds.right - HOffset : _Bounds.left  + HOffset;

    const FLOAT yt = _Bounds.top    + (_GraphSettings->_XAxisTop    ? _TextStyle->_Height : 0.f); // Top axis
    const FLOAT yb = _Bounds.bottom - (_GraphSettings->_XAxisBottom ? _TextStyle->_Height : 0.f); // Bottom axis

    const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    // Calculate the rectangles of the labels.
    for (label_t & Iter : _Labels)
    {
        const FLOAT dx = msc::Map(ScaleF(Iter.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, SpectrumWidth);

        const FLOAT x = !_GraphSettings->_FlipHorizontally ? (x1 + dx) : (x1 - dx);

        Iter.PointT = D2D1_POINT_2F(x, yt);
        Iter.PointB = D2D1_POINT_2F(x, yb);

        {
            CComPtr<IDWriteTextLayout> TextLayout;

            HRESULT hr = _DirectWrite.Factory->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, _Size.width, _Size.height, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                Iter.RectT = { x - (TextMetrics.width / 2.f), _Bounds.top, x + (TextMetrics.width / 2.f), yt };

                // Make sure the label is completely visible.
                if (!_GraphSettings->_FlipHorizontally)
                {
                    if (Iter.RectT.left < x1)
                    {
                        Iter.RectT.left  = x1;
                        Iter.RectT.right = x1 + TextMetrics.width;
                    }
                    else
                    if (Iter.RectT.right > x2)
                    {
                        Iter.RectT.left  = x2 - TextMetrics.width;
                        Iter.RectT.right = x2;
                    }
                }
                else
                {
                    if (Iter.RectT.right > x1)
                    {
                        Iter.RectT.left  = x1 - TextMetrics.width;
                        Iter.RectT.right = x1;
                    }
                    else
                    if (Iter.RectT.left < x2)
                    {
                        Iter.RectT.left  = x2;
                        Iter.RectT.right = x2 + TextMetrics.width;
                    }
                }

                Iter.RectB = { Iter.RectT.left,               yb,          Iter.RectT.right,              _Bounds.bottom };
            }
        }

        // Labels outside the bounds are always hidden.
        Iter.IsHidden = !msc::InRange(x, _Bounds.left, _Bounds.right);
    }

    if (_Labels.size() > 2)
    {
        const bool NotesMode = (_GraphSettings->_XAxisMode == XAxisMode::Notes);

        const label_t * LastLabel = nullptr;

        // Determine which labels should be hidden.
        for (size_t i = 1; i < _Labels.size() - 1; ++i)
        {
            if (LastLabel && !(LastLabel->IsHidden) && IsOverlappingHorizontally(_Labels[i].RectB, LastLabel->RectB))
                _Labels[i].IsHidden = !NotesMode || (NotesMode && !_Labels[i + 1].IsDimmed);
            else
            if (!_Labels[i + 1].IsHidden && NotesMode && IsOverlappingHorizontally(_Labels[i].RectB, _Labels[i + 1].RectB))
                _Labels[i].IsHidden = !_Labels[i + 1].IsDimmed;
            else
                LastLabel = &_Labels[i];
        }
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void x_axis_t::Render(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    FLOAT Opacity = _TextStyle->_Brush->GetOpacity();

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    for (const label_t & Iter : _Labels)
    {
        // Draw the vertical grid line.
        if (_LineStyle->IsEnabled())
            renderTarget->DrawLine(Iter.PointT, Iter.PointB, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

        // Draw the text.
        if (!Iter.IsHidden && _TextStyle->IsEnabled() && (_GraphSettings->_XAxisMode != XAxisMode::None))
        {
            _TextStyle->_Brush->SetOpacity(Iter.IsDimmed && (_GraphSettings->_XAxisMode == XAxisMode::Notes) ? Opacity * .5f : Opacity);

            if (_GraphSettings->_XAxisTop)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectT, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if (_GraphSettings->_XAxisBottom)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectB, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    _TextStyle->_Brush->SetOpacity(Opacity);
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT x_axis_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, renderTarget, _Size, L"", &_LineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, _Size, L"999.9k", &_TextStyle);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void x_axis_t::ReleaseDeviceSpecificResources() noexcept
{
    SafeRelease(&_TextStyle);
    SafeRelease(&_LineStyle);
}

#pragma endregion
