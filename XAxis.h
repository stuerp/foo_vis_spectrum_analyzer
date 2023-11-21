
/** $VER: XAxis.h (2023.11.21) P. Stuer - Represents and renders the X axis. **/

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
    XAxis() : _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _LabelHeight(30.f), _Mode(), _ClientWidth(), _ClientHeight(), _TextHeight() { }

    XAxis(const XAxis &) = delete;
    XAxis & operator=(const XAxis &) = delete;
    XAxis(XAxis &&) = delete;
    XAxis & operator=(XAxis &&) = delete;

    FLOAT GetHeight() const { return _LabelHeight; }

    struct Label
    {
        std::wstring Text;
        FLOAT x;
    };

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    void Initialize(FLOAT x, FLOAT y, FLOAT width, FLOAT height, const std::vector<FrequencyBand> & bands, XAxisMode xAxisMode)
    {
        _X = x;
        _Y = y;

        _ClientWidth = width;
        _ClientHeight = height;

        _Mode = xAxisMode;

        _Labels.clear();

        if (bands.size() == 0)
            return;

        // Precalculate the labels and their position.
        {
            WCHAR Text[32] = { };

            const FLOAT BandWidth = Max((_ClientWidth / (FLOAT) bands.size()), 1.f);

            switch (xAxisMode)
            {
                case XAxisMode::None:
                    break;

                default:

                case XAxisMode::Bands:
                {
                    for (size_t i = 0; i < bands.size(); i += 10)
                    {
                        double Frequency = bands[i].Ctr;

                        if (Frequency < 1000.)
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", Frequency);
                        else
                            ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", Frequency / 1000.);

                        x = (FLOAT) Map(::log2(Frequency), ::log2(bands[0].Ctr), ::log2(bands[bands.size() - 1].Ctr), _X + (BandWidth / 2.f), width - _X);

                        Label lb = { Text, x };

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

                        x = (FLOAT) Map(::log2(Frequency), ::log2(bands[0].Ctr), ::log2(bands[bands.size() - 1].Ctr), _X + (BandWidth / 2.f), width - _X);

                        Label lb = { Text, x };

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

                        x = (FLOAT) Map(::log2(Frequency), ::log2(bands[0].Ctr), ::log2(bands[bands.size() - 1].Ctr), _X + (BandWidth / 2.f), width - _X);

                        Label lb = { Text, x };

                        _Labels.push_back(lb);
                    }
                    break;
                }

                case XAxisMode::Notes:
                {
                    double Note = -57.; // Frequency of C0 (57 semi-tones lower than A4 at 440Hz)

                    for (int i = 0; i < 13; ++i)
                    {
                        ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                        double Frequency = 440. * ::exp2(Note / 12.); // Frequency of C0 (57 semi-tones lower than A4 at 440Hz)

                        x = (FLOAT) Map(::log2(Frequency), ::log2(bands[0].Ctr), ::log2(bands[bands.size() - 1].Ctr), _X + (BandWidth / 2.f), width - _X);

                        Label lb = { Text, x };

                        _Labels.push_back(lb);

                        Note += 12.;
                    }
                    break;
                }
            }
        }
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        if (_Mode == XAxisMode::None)
            return S_OK;

        const FLOAT StrokeWidth = 1.0f;

        for (const Label & Iter : _Labels)
        {
            if (InInterval(Iter.x, _X, _ClientWidth))
            {
                // Draw the horizontal grid line.
                {
                    _Brush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                    renderTarget->DrawLine(D2D1_POINT_2F(Iter.x, 0.f), D2D1_POINT_2F(Iter.x, _ClientHeight -_LabelHeight), _Brush, StrokeWidth, nullptr);
                }

                // Draw the label.
                {
                    D2D1_RECT_F TextRect = { Iter.x - 30.f, _ClientHeight - _LabelHeight, Iter.x + 30.f, _ClientHeight };

                    _Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
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

        if (SUCCEEDED(hr))
        {
            static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

            hr = directWriteFactory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<IDWriteTextLayout> TextLayout;

            hr = directWriteFactory->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);            // Center horizontallly
                _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
                _TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _TextHeight = TextMetrics.height;

                // Calculate the height
                _LabelHeight = 2.f + _TextHeight + 2.f;
            }
//          else
//              Log(LogLevel::Critical, "%s: Unable to create X axis TextLayout: 0x%08X.", core_api::get_my_file_name(), hr);
        }
//      else
//          Log(LogLevel::Critical, "%s: Unable to create X axis TextFormat: 0x%08X.", core_api::get_my_file_name(), hr);

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
    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.
    FLOAT _LabelHeight;  // Determines the max. width of the label.

    XAxisMode _Mode;

    // Parent-dependent parameters
    FLOAT _X;
    FLOAT _Y;

    FLOAT _ClientWidth;
    FLOAT _ClientHeight;

    std::vector<Label> _Labels;

    // Device-independent resources
    CComPtr<IDWriteTextFormat> _TextFormat;
    FLOAT _TextHeight;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _Brush;
};
