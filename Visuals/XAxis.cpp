
/** $VER: XAXis.cpp (2024.02.25) P. Stuer - Implements the X axis of a graph. **/

#include "XAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void XAxis::Initialize(State * state, const GraphSettings * settings, const FrequencyBands & frequencyBands) noexcept
{
    _State = state;
    _GraphSettings = settings;

    CreateDeviceIndependentResources();

    _Labels.clear();

    if (frequencyBands.size() == 0)
        return;

    _BandCount = frequencyBands.size();

    _LoFrequency = frequencyBands[0].Ctr;
    _HiFrequency = frequencyBands[_BandCount - 1].Ctr;

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
                for (size_t i = 0; i < frequencyBands.size(); i += 10)
                {
                    double Frequency = frequencyBands[i].Ctr;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                    Label lb = { Frequency, Text };

                    _Labels.push_back(lb);
                }
                break;
            }

            case XAxisMode::Decades:
            {
                double Frequency = 0.;
                int i = 1;
                int j = 10;

                while (Frequency < frequencyBands.back().Lo)
                {
                    Frequency = j * i;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                    Label lb = { Frequency, Text };

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

                for (int i = 0; Frequency < frequencyBands.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    Label lb = { Frequency, Text };

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

                while (Frequency < frequencyBands.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    Label lb = { Frequency, Text, j != 0 };

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
    _Bounds = rect;

    const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    // Calculate the position of the labels based on the width.
    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT BandWidth = Max((Width / (FLOAT) _BandCount), 1.f); // In pixels

    const FLOAT StartX = !_GraphSettings->_FlipHorizontally ? _Bounds.left + (BandWidth / 2.f) : _Bounds.right - (BandWidth / 2.f);

    const FLOAT yt = _Bounds.top    + (_GraphSettings->_XAxisTop    ? _Height : 0.f);  // Top axis
    const FLOAT yb = _Bounds.bottom - (_GraphSettings->_XAxisBottom ? _Height : 0.f);  // Bottom axis

    for (Label & Iter : _Labels)
    {
        const FLOAT dx = Map(ScaleF(Iter.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, Width - BandWidth);
        const FLOAT x = !_GraphSettings->_FlipHorizontally ? StartX + dx : StartX - dx;

        // Don't generate any labels outside the bounds.
        if (!InRange(x, _Bounds.left, _Bounds.right))
            continue;

        Iter.PointT = D2D1_POINT_2F(x, yt);
        Iter.PointB = D2D1_POINT_2F(x, yb);

        {
            CComPtr<IDWriteTextLayout> TextLayout;

            HRESULT hr = _DirectWrite.Factory->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, Width, Height, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                Iter.RectT = { x - (TextMetrics.width / 2.f), _Bounds.top, x + (TextMetrics.width / 2.f), yt };
                Iter.RectB = { Iter.RectT.left,               yb,          Iter.RectT.right,              _Bounds.bottom };
            }
        }
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void XAxis::Render(ID2D1RenderTarget * renderTarget)
{
    if ((_GraphSettings->_XAxisMode == XAxisMode::None) || (!_GraphSettings->_XAxisTop && !_GraphSettings->_XAxisBottom))
        return;

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    D2D1_RECT_F OldRect = {  };
    FLOAT Opacity = _TextStyle->_Brush->GetOpacity();

    for (const Label & Iter : _Labels)
    {
        // Draw the vertical grid line.
        if (_LineStyle->_ColorSource != ColorSource::None)
            renderTarget->DrawLine(Iter.PointT, Iter.PointB, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

        if (_TextStyle->_ColorSource != ColorSource::None)
        {
            _TextStyle->_Brush->SetOpacity(Iter.IsDimmed ? Opacity * .5f : Opacity);

            // Prevent overdraw of the labels.
            if (!InRange(Iter.RectB.left, OldRect.left, OldRect.right) && !InRange(Iter.RectB.right, OldRect.left, OldRect.right))
            {
                // Draw the labels.
                if (_GraphSettings->_XAxisTop)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, Iter.RectT, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_XAxisBottom)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, Iter.RectB, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                OldRect = Iter.RectB;
            }
        }
    }

    _TextStyle->_Brush->SetOpacity(Opacity);
}

#pragma region DirectX

/// <summary>
/// Creates resources which are not bound to any D3D device.
/// </summary>
HRESULT XAxis::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    if (_TextFormat == 0)
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);            // Center horizontallly
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            _TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(L"9999.9Hz", 6, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                // Calculate the height.
                _Width  = TextMetrics.width;
                _Height = 2.f + TextMetrics.height + 2.f;
            }
        }
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void XAxis::ReleaseDeviceIndependentResources()
{
    _TextFormat.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT XAxis::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    const D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
    {
        if (_LineStyle == nullptr)
            _LineStyle = _State->_StyleManager.GetStyle(VisualElement::XAxisLine);

        if (_LineStyle && (_LineStyle->_Brush == nullptr))
            hr = _LineStyle->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_TextStyle == nullptr)
            _TextStyle = _State->_StyleManager.GetStyle(VisualElement::XAxisText);

        if (_TextStyle && (_TextStyle->_Brush == nullptr))
            hr = _TextStyle->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
        renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE); // https://learn.microsoft.com/en-us/windows/win32/direct2d/improving-direct2d-performance

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void XAxis::ReleaseDeviceSpecificResources()
{
    _TextStyle = nullptr;
    _LineStyle = nullptr;

    for (const auto & Iter : { VisualElement::XAxisLine, VisualElement::XAxisText })
    {
        Style * style = _State->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }
}

#pragma endregion
