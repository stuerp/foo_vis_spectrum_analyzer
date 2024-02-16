
/** $VER: XAXis.cpp (2024.02.16) P. Stuer - Implements the X axis of a graph. **/

#include "XAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void XAxis::Initialize(State * state, Analyses & analyses) noexcept
{
    _State = state;

    CreateDeviceIndependentResources();

    _Labels.clear();

    if (analyses[0]->_FrequencyBands.size() == 0)
        return;

    _Mode = _State->_XAxisMode;

    _NumBands = analyses[0]->_FrequencyBands.size();

    _LoFrequency = analyses[0]->_FrequencyBands[0].Ctr;
    _HiFrequency = analyses[0]->_FrequencyBands[_NumBands - 1].Ctr;

    if (analyses[0]->_FrequencyBands.size() == 0)
        return;

    // Precalculate the labels.
    {
        WCHAR Text[32] = { };

        switch (_Mode)
        {
            case XAxisMode::None:
                break;

            default:

            case XAxisMode::Bands:
            {
                for (size_t i = 0; i < analyses[0]->_FrequencyBands.size(); i += 10)
                {
                    double Frequency = analyses[0]->_FrequencyBands[i].Ctr;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                    Label lb = { Frequency, Text, 0.f };

                    _Labels.push_back(lb);
                }
                break;
            }

            case XAxisMode::Decades:
            {
                double Frequency = 0.;
                int i = 1;
                int j = 10;

                while (Frequency < analyses[0]->_FrequencyBands.back().Lo)
                {
                    Frequency = j * i;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                    Label lb = { Frequency, Text, 0.f };

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

                for (int i = 0; Frequency < analyses[0]->_FrequencyBands.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    Label lb = { Frequency, Text, 0.f };

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

                while (Frequency < analyses[0]->_FrequencyBands.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    Label lb = { Frequency, Text, 0.f, j != 0 };

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

    // Calculate the position of the labels based on the width.
    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT BandWidth = Max((Width / (FLOAT) _NumBands), 1.f);
    const FLOAT x = _Bounds.left + (BandWidth / 2.f);

    for (Label & Iter : _Labels)
    {
        FLOAT dx = Map(log2(Iter.Frequency), ::log2(_LoFrequency), ::log2(_HiFrequency), 0.f, Width - BandWidth);

        Iter.x = x + dx;
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void XAxis::Render(ID2D1RenderTarget * renderTarget)
{
    if (_Mode == XAxisMode::None)
        return;

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT yt = _Bounds.top    + (_State->_XAxisTop    ? _Height : 0.f);  // Top axis
    const FLOAT yb = _Bounds.bottom - (_State->_XAxisBottom ? _Height : 0.f);  // Bottom axis

    FLOAT OldTextRight = -_Width;

    Style * LineStyle = _State->_StyleManager.GetStyle(VisualElement::XAxisLine);
    Style * TextStyle = _State->_StyleManager.GetStyle(VisualElement::XAxisText);

    FLOAT Opacity = TextStyle->_Brush->GetOpacity();

    for (const Label & Iter : _Labels)
    {
        if (Iter.x < _Bounds.left)
            continue;

        // Draw the vertical grid line.
        renderTarget->DrawLine(D2D1_POINT_2F(Iter.x, yt), D2D1_POINT_2F(Iter.x, yb), LineStyle->_Brush, LineStyle->_Thickness, nullptr);

        // Draw the label.
        if (Iter.Text.empty())
            continue;

        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectWrite.Factory->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, 1920.f, 1080.f, &TextLayout);

        if (SUCCEEDED(hr))
        {
            DWRITE_TEXT_METRICS TextMetrics = { };

            TextLayout->GetMetrics(&TextMetrics);

            D2D1_RECT_F TextRect = { Iter.x - (TextMetrics.width / 2.f), yb, Iter.x + (TextMetrics.width / 2.f), yb + _Height };

            TextStyle->_Brush->SetOpacity(Iter.IsDimmed ? Opacity * .5f : Opacity);

            if ((OldTextRight <= TextRect.left) && (TextRect.left < _Bounds.right))
            {
                if (_State->_XAxisBottom)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                if (_State->_XAxisTop)
                {
                    TextRect.top    = _Bounds.top;
                    TextRect.bottom = _Bounds.top + _Height;

                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
                }

                OldTextRight = TextRect.right;
            }
        }
    }

    TextStyle->_Brush->SetOpacity(Opacity);
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

    if (SUCCEEDED(hr))
    {
        for (const auto & Iter : { VisualElement::XAxisLine, VisualElement::XAxisText })
        {
            Style * style = _State->_StyleManager.GetStyle(Iter);

            if (style->_Brush == nullptr)
                hr = style->CreateDeviceSpecificResources(renderTarget);

            if (!SUCCEEDED(hr))
                break;
        }
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
    for (const auto & Iter : { VisualElement::XAxisLine, VisualElement::XAxisText })
    {
        Style * style = _State->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }
}

#pragma endregion
