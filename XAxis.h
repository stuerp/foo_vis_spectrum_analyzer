
/** $VER: XAxis.h (2023.11.29) P. Stuer - Represents and renders the X axis. **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "Math.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the X axis.
/// </summary>
class XAxis
{
public:
    XAxis() : _Mode(), _LoFrequency(), _HiFrequency(), _NumBands(), _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Rect(), _Height(30.f) { }

    XAxis(const XAxis &) = delete;
    XAxis & operator=(const XAxis &) = delete;
    XAxis(XAxis &&) = delete;
    XAxis & operator=(XAxis &&) = delete;

    struct Label
    {
        double Frequency;
        std::wstring Text;
        FLOAT x;
    };

    /// <summary>
    /// Gets the height of the axis.
    /// </summary>
    FLOAT GetHeight() const { return _Height; }

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    void Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands)
    {
        if (frequencyBands.size() == 0)
            return;

        _Mode = configuration->_XAxisMode;

        _LoFrequency = frequencyBands[0].Ctr;
        _HiFrequency = frequencyBands[frequencyBands.size() - 1].Ctr;
        _NumBands = frequencyBands.size();

        _TextColor = configuration->_XTextColor;
        _LineColor = configuration->_XLineColor;

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
                    const double FrequenciesDecades[] = { 10., 20., 30., 40., 50., 60., 70., 80., 90., 100., 200., 300., 400., 500., 600., 700., 800., 900., 1000., 2000., 3000., 4000., 5000., 6000., 7000., 8000., 9000., 10000., 20000. };

                    for (size_t i = 0; i < _countof(FrequenciesDecades); ++i)
                    {
                        double Frequency = FrequenciesDecades[i];

                        if (Frequency < 1000.)
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                        else
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                        Label lb = { Frequency, Text, 0.f };

                        _Labels.push_back(lb);
                    }
                    break;
                }

                case XAxisMode::Octaves:
                {
                    const double FrequenciesOctaves[] = { 32.7, 65.41, 130.81, 261.63, 523.25, 1108.73, 2093.00, 4186.01, 8372.02, 16744.04, 33488.07 };

                    for (size_t i = 0; i < _countof(FrequenciesOctaves); ++i)
                    {
                        double Frequency = FrequenciesOctaves[i];

                        if (Frequency < 1000.)
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                        else
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                        Label lb = { Frequency, Text, 0.f };

                        _Labels.push_back(lb);
                    }
                    break;
                }

                case XAxisMode::Notes:
                {
                    double Note = -57.; // Frequency of C0 (57 semi-tones lower than A4 at 440Hz)

                    for (int i = 0; i < 12; ++i)
                    {
                        ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                        double Frequency = 440. * ::exp2(Note / 12.); // Frequency of C0 (57 semi-tones lower than A4 at 440Hz)

                        Label lb = { Frequency, Text, 0.f };

                        _Labels.push_back(lb);

                        Note += 12.;
                    }
                    break;
                }
            }
        }
    }

    /// <summary>
    /// Resizes the axis.
    /// </summary>
    void Resize(const D2D1_RECT_F & rect)
    {
        _Rect = rect;

        // Calculate the position of the labels based on the width.
        const FLOAT Width = _Rect.right - _Rect.left;
        const FLOAT BandWidth = Max((Width / (FLOAT) _NumBands), 1.f);
        const FLOAT x = _Rect.left + (BandWidth / 2.f);

        for (Label & Iter : _Labels)
        {
            FLOAT dx = Map(log2(Iter.Frequency), ::log2(_LoFrequency), ::log2(_HiFrequency), 0.f, Width - BandWidth);

            Iter.x = x + dx;
        }
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        if (_Mode == XAxisMode::None)
            return S_OK;

        CreateDeviceSpecificResources(renderTarget);

        const FLOAT StrokeWidth = 1.0f;
        const FLOAT Height = _Rect.bottom - _Rect.top;

        FLOAT OldTextRight = -_Width;

        for (const Label & Iter : _Labels)
        {
            if (Iter.x < _Rect.left)
                continue;

            // Draw the vertical grid line.
            {
                _Brush->SetColor(_LineColor);

                renderTarget->DrawLine(D2D1_POINT_2F(Iter.x, 0.f), D2D1_POINT_2F(Iter.x, Height -_Height), _Brush, StrokeWidth, nullptr);
            }

            // Draw the label.
            {
                CComPtr<IDWriteTextLayout> TextLayout;

                HRESULT hr = _DirectWriteFactory->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, 1920.f, 1080.f, &TextLayout);

                if (SUCCEEDED(hr))
                {
                    DWRITE_TEXT_METRICS TextMetrics = { };

                    TextLayout->GetMetrics(&TextMetrics);

                    D2D1_RECT_F TextRect = { Iter.x - (TextMetrics.width / 2.f), Height - _Height, Iter.x + (TextMetrics.width / 2.f), Height };

                    if (OldTextRight <= TextRect.left)
                    {
                        _Brush->SetColor(_TextColor);

                        renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                        OldTextRight = TextRect.right;
                    }
                }
            }
        }

        return S_OK;
    }

    /// <summary>
    /// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
    /// </summary>
    HRESULT CreateDeviceIndependentResources(CComPtr<IDWriteFactory> & directWriteFactory)
    {
        HRESULT hr = S_OK;

        _DirectWriteFactory = directWriteFactory;

        if (SUCCEEDED(hr))
        {
            static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

            hr = directWriteFactory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

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

            hr = directWriteFactory->CreateTextLayout(L"9999.9Hz", 6, _TextFormat, 100.f, 100.f, &TextLayout);

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
    /// Creates resources which are bound to a particular D3D device.
    /// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
    /// </summary>
    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        if (_Brush != nullptr)
            return S_OK;

        HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_Brush);

        return hr;
    }

    /// <summary>
    /// Releases the device specific resources.
    /// </summary>
    void ReleaseDeviceSpecificResources()
    {
        _Brush.Release();
    }

private:
    XAxisMode _Mode;

    double _LoFrequency;
    double _HiFrequency;
    size_t _NumBands;

    D2D1_COLOR_F _TextColor;
    D2D1_COLOR_F _LineColor;
    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    std::vector<Label> _Labels;

    // Device-independent resources
    D2D1_RECT_F _Rect;
    FLOAT _Width;       // Width of a label
    FLOAT _Height;      // Height of the X axis area (Font size-dependent).

    CComPtr<IDWriteFactory> _DirectWriteFactory;
    CComPtr<IDWriteTextFormat> _TextFormat;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _Brush;
};
