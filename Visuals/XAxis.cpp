
/** $VER: XAXis.cpp (2024.04.11) P. Stuer - Implements the X axis of a graph. **/

#include "framework.h"
#include "XAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void XAxis::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    _Labels.clear();

    const FrequencyBands & fb = _Analysis->_FrequencyBands;

    if (fb.size() == 0)
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

                    Label lb = { Text, Frequency };

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

                    Label lb = { Text, Frequency };

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
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                for (int i = 0; Frequency < fb.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    Label lb = { Text, Frequency };

                    _Labels.push_back(lb);

                    Note += 12.;
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);
                }
                break;
            }

            case XAxisMode::Notes:
            {
                static const char Name[] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
                static const int Step[] = { 2, 2, 1, 2, 2, 2, 1 };

                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                int j = 0;

                while (Frequency < fb.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    Label lb = { Text, Frequency, j != 0 };

                    _Labels.push_back(lb);

                    Note += Step[j];
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);

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
void XAxis::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void XAxis::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    // Calculate the position of the labels.
    const FLOAT BandWidth = Max(::floor(_Size.width / (FLOAT) _BandCount), 2.f); // In pixels

    const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? BandWidth * (FLOAT) _BandCount : _Size.width;

    const FLOAT xl = !_GraphSettings->_FlipHorizontally ? _Bounds.left + ((_Size.width - SpectrumWidth) / 2.f) + (BandWidth / 2.f) : _Bounds.right - ((_Size.width - SpectrumWidth) / 2.f) - (BandWidth / 2.f);

    const FLOAT yt = _Bounds.top    + (_GraphSettings->_XAxisTop    ? _TextStyle->_Height : 0.f); // Top axis
    const FLOAT yb = _Bounds.bottom - (_GraphSettings->_XAxisBottom ? _TextStyle->_Height : 0.f); // Bottom axis

    const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    // Calculate the rectangles of the labels.
    for (Label & Iter : _Labels)
    {
        const FLOAT dx = Map(ScaleF(Iter.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, SpectrumWidth);

        const FLOAT x = !_GraphSettings->_FlipHorizontally ? xl + dx : xl - dx;

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
                Iter.RectB = { Iter.RectT.left,               yb,          Iter.RectT.right,              _Bounds.bottom };
            }
        }

        // Labels outside the bounds are always hidden.
        Iter.IsHidden = !InRange(x, _Bounds.left, _Bounds.right);
    }

    if (_Labels.size() > 2)
    {
        #define NotesMode (_GraphSettings->_XAxisMode == XAxisMode::Notes)

        const Label * LastLabel = nullptr;

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
void XAxis::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    FLOAT Opacity = _TextStyle->_Brush->GetOpacity();

    for (const Label & Iter : _Labels)
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
HRESULT XAxis::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
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
void XAxis::ReleaseDeviceSpecificResources()
{
    SafeRelease(&_TextStyle);
    SafeRelease(&_LineStyle);
}

#pragma endregion
