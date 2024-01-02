
/** $VER: XAXis.cpp (2023.12.31) P. Stuer - Implements the X axis of a graph. **/

#include "XAxis.h"
#include "DirectX.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void XAxis::Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands)
{
    _Configuration = configuration;

    if (frequencyBands.size() == 0)
        return;

    _Mode = _Configuration->_XAxisMode;

    _LoFrequency = frequencyBands[0].Ctr;
    _HiFrequency = frequencyBands[frequencyBands.size() - 1].Ctr;
    _NumBands = frequencyBands.size();

    _TextColor = _Configuration->_XTextColor;
    _LineColor = _Configuration->_XLineColor;

    _Labels.clear();

    if (frequencyBands.size() == 0)
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
                for (size_t i = 0; i < frequencyBands.size(); i += 10)
                {
                    double Frequency = frequencyBands[i].Ctr;

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

                while (Frequency < frequencyBands.back().Lo)
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
            case XAxisMode::Notes:
            {
                double Note = -57.; // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _Configuration->_Pitch * ::exp2(Note / 12.);

                for (int i = 0; Frequency < frequencyBands.back().Lo; ++i)
                {
                    if (_Mode == XAxisMode::Octaves)
                    {
                        if (Frequency < 1000.)
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                        else
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);
                    }
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    Label lb = { Frequency, Text, 0.f };

                    _Labels.push_back(lb);

                    Note += 12.;
                    Frequency = _Configuration->_Pitch * ::exp2(Note / 12.);
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
void XAxis::Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    if (_Mode == XAxisMode::None)
        return;

    CreateDeviceSpecificResources(renderTarget);

    const FLOAT StrokeWidth = 1.0f;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;

    FLOAT OldTextRight = -_Width;

    for (const Label & Iter : _Labels)
    {
        if (Iter.x < _Bounds.left)
            continue;

        // Draw the vertical grid line.
        {
            _SolidBrush->SetColor(_Configuration->_UseCustomXLineColor ? _LineColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

            renderTarget->DrawLine(D2D1_POINT_2F(Iter.x, 0.f), D2D1_POINT_2F(Iter.x, Height -_Height), _SolidBrush, StrokeWidth, nullptr);
        }

        // Draw the label.
        {
            CComPtr<IDWriteTextLayout> TextLayout;

            HRESULT hr = _DirectX._DirectWrite->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, 1920.f, 1080.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                D2D1_RECT_F TextRect = { Iter.x - (TextMetrics.width / 2.f), Height - _Height, Iter.x + (TextMetrics.width / 2.f), Height };

                if (OldTextRight <= TextRect.left)
                {
                    _SolidBrush->SetColor(_Configuration->_UseCustomXTextColor ? _TextColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _SolidBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                    OldTextRight = TextRect.right;
                }
            }
        }
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT XAxis::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

        hr = _DirectX._DirectWrite->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);            // Center horizontallly
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            _TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectX._DirectWrite->CreateTextLayout(L"9999.9Hz", 6, _TextFormat, 100.f, 100.f, &TextLayout);

        if (SUCCEEDED(hr))
        {
            DWRITE_TEXT_METRICS TextMetrics = { };

            TextLayout->GetMetrics(&TextMetrics);

            // Calculate the height.
            _Width  = TextMetrics.width;
            _Height = 2.f + TextMetrics.height + 2.f;
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
HRESULT XAxis::CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    if (_SolidBrush != nullptr)
        return S_OK;

    HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_SolidBrush);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void XAxis::ReleaseDeviceSpecificResources()
{
    _SolidBrush.Release();
}
