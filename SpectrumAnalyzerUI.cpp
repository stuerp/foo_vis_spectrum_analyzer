
/** $VER: SpectrumAnalyzerUI.cpp (2023.11.14) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#include <complex>

#include "Configuration.h"

#pragma hdrstop

static bool _IsPlaying = false;

/// <summary>
/// Constructor
/// </summary>
SpectrumAnalyzerUIElement::SpectrumAnalyzerUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback) : m_callback(callback), _LastRefresh(0), _RefreshInterval(10)
{
    set_configuration(data);
}

#pragma region CWindowImpl

/// <summary>
/// Gets the window class definition.
/// </summary>
CWndClassInfo & SpectrumAnalyzerUIElement::GetWndClassInfo()
{
    static ATL::CWndClassInfo wci =
    {
        {
            sizeof(WNDCLASSEX),
            CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
            StartWindowProc,
            0, 0,
            NULL, // Instance,
            NULL, // Icon
            NULL, // Cursor
            (HBRUSH) NULL, // Background
            NULL, // Menu
            TEXT(STR_SPECTOGRAM_WINDOW_CLASS), // Class name
            NULL // Small Icon
        },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };

    return wci;
}

/// <summary>
/// Creates the window.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnCreate(LPCREATESTRUCT cs)
{
    HRESULT hr = S_OK;

    hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Unable to create Direct2D device independent resources.";
    }

    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, 0);

        _VisualisationStream->request_backlog(0.8);
    }
    catch (std::exception & ex)
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Exception while creating visualisation stream. " << ex;
    }

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void SpectrumAnalyzerUIElement::OnDestroy()
{
    if (_SpectrumAnalyzer)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    _VisualisationStream.release();

    ReleaseDeviceSpecificResources();

    _Direct2dFactory.Release();
}

/// <summary>
/// Handles the WM_TIMER message.
/// </summary>
void SpectrumAnalyzerUIElement::OnTimer(UINT_PTR timerID)
{
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void SpectrumAnalyzerUIElement::OnPaint(CDCHandle hDC)
{
    Render();
    ValidateRect(nullptr);

    if (!_IsPlaying)
        return;

    ULONGLONG Now = ::GetTickCount64(); // in ms, with a resolution of the system timer, which is typically in the range of 10ms to 16ms.

    if (_VisualisationStream.is_valid())
    {
        ULONGLONG NextRefresh = _LastRefresh + _RefreshInterval;

        if (NextRefresh < Now)
            NextRefresh = Now;

        // Schedule the next refresh. Limited to USER_TIMER_MINIMUM (10ms / 100Hz).
        SetTimer(ID_REFRESH_TIMER, (UINT)(NextRefresh - Now));
    }

    _LastRefresh = Now;
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void SpectrumAnalyzerUIElement::OnSize(UINT type, CSize size)
{
    if (!_RenderTarget)
        return;

    const D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    _RenderTarget->Resize(Size);

    _RenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::OnContextMenu(CWindow wnd, CPoint point)
{
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
    }
    else
    {
        CMenu Menu;
        CMenu RefreshRateLimitMenu;

        {
            Menu.CreatePopupMenu();
            Menu.AppendMenu((UINT) MF_STRING, IDM_CONFIGURE, TEXT("Configure"));
            Menu.AppendMenu((UINT) MF_SEPARATOR);
            Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Full-Screen Mode"));
//          Menu.AppendMenu((UINT) MF_STRING | (_Configuration._UseHardwareRendering ? MF_CHECKED : 0), IDM_TOGGLE_HARDWARE_RENDERING, TEXT("Hardware Rendering"));

            {
                RefreshRateLimitMenu.CreatePopupMenu();
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  20) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20,  TEXT("20 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  60) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_60,  TEXT("60 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 100) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_100, TEXT("100 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 200) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_200, TEXT("200 Hz"));

                Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, TEXT("Refresh Rate Limit"));
            }

            Menu.SetMenuDefaultItem(IDM_CONFIGURE);
        }

        int CommandId = Menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

        switch (CommandId)
        {
            case IDM_TOGGLE_FULLSCREEN:
                ToggleFullScreen();
                break;

            case IDM_TOGGLE_HARDWARE_RENDERING:
                ToggleHardwareRendering();
                break;

            case IDM_REFRESH_RATE_LIMIT_20:
                _Configuration._RefreshRateLimit = 20;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_60:
                _Configuration._RefreshRateLimit = 60;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_100:
                _Configuration._RefreshRateLimit = 100;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_200:
                _Configuration._RefreshRateLimit = 200;
                UpdateRefreshRateLimit();
                break;

            case IDM_CONFIGURE:
                Configure();
                break;
        }

        Invalidate();
    }
}

/// <summary>
/// Toggles between panel and full screen mode.
/// </summary>
void SpectrumAnalyzerUIElement::OnLButtonDblClk(UINT flags, CPoint point)
{
    ToggleFullScreen();
}

#pragma endregion

/// <summary>
/// Toggles full screen mode.
/// </summary>
void SpectrumAnalyzerUIElement::ToggleFullScreen() noexcept
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void SpectrumAnalyzerUIElement::ToggleHardwareRendering() noexcept
{
    _Configuration._UseHardwareRendering = !_Configuration._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Updates the refresh rate.
/// </summary>
void SpectrumAnalyzerUIElement::UpdateRefreshRateLimit() noexcept
{
    _RefreshInterval = pfc::clip_t<DWORD>(1000 / (DWORD) _Configuration._RefreshRateLimit, 5, 1000);
}

/// <summary>
/// Shows the Options dialog.
/// </summary>
void SpectrumAnalyzerUIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &_Configuration) == NULL)
            return;

        _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::Render()
{
    // Frame counter
    {
        LARGE_INTEGER Time;

        ::QueryPerformanceCounter(&Time);

        _Times.Add(Time.QuadPart);
    }

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the background.
        {
            t_ui_color BackgroundColor = m_callback->query_std_color(ui_color_background);

            //FIXME: Should be dynamic
            BackgroundColor = RGB(0, 0, 0);

            _RenderTarget->Clear(D2D1::ColorF(GetRValue(BackgroundColor) / 255.0f, GetGValue(BackgroundColor) / 255.0f, GetBValue(BackgroundColor) / 255.0f));
        }

        if (_IsPlaying)
        {
            // Draw the visualization.
            if (_VisualisationStream.is_valid())
            {
                double PlaybackTime; // in ms

                if (_VisualisationStream->get_absolute_time(PlaybackTime))
                {
                    double WindowDuration = _Configuration.GetWindowDurationInMs();

                    audio_chunk_impl Chunk;

                    if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - WindowDuration / 2, WindowDuration * (_Configuration._UseZeroTrigger ? 2 : 1)))
                        RenderChunk(Chunk);
                }
            }

            // Draw the text.
//          hr = RenderText();
        }

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }

    return hr;
}

/// <summary>
/// Render an audio chunk.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderChunk(const audio_chunk & chunk)
{
    HRESULT hr = S_OK;

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t SampleRate   = chunk.get_sample_rate();

    // Create the spectrum analyzer if necessary.
    if (_SpectrumAnalyzer == nullptr)
        _SpectrumAnalyzer = new SpectrumAnalyzer<double>(ChannelCount, _Configuration._FFTSize, SampleRate);

    // Add the samples to the spectrum analyzer.
    {
        const audio_sample * Samples = chunk.get_data();

        if (Samples == nullptr)
            return E_FAIL;

        size_t SampleCount  = chunk.get_sample_count();

        _SpectrumAnalyzer->Add(Samples, SampleCount);
    }

    // Initialize the bands.
    switch (_Configuration._FrequencyDistribution)
    {
        default:

        case Frequencies:
            GenerateFrequencyBands();
            break;

        case Octaves:
            GenerateFrequencyBandsFromNotes(SampleRate);
            break;

        case AveePlayer:
//          generateAveePlayerFreqs(_Configuration._numBands, _Configuration._minFreq, _Configuration._maxFreq, _Configuration._hzLinearFactor, _Configuration._bandwidth);
            break;
    }

    _Spectrum.resize(_FrequencyBands.size());
    std::fill(_Spectrum.begin(), _Spectrum.end(), 0.0);

    _CurrentSpectrum.resize(_FrequencyBands.size());
    std::fill(_CurrentSpectrum.begin(), _CurrentSpectrum.end(), 0.0);

    {
        // Get the frequency data.
        std::vector<double> FreqData((size_t) _Configuration._FFTSize, 0.0);

        _SpectrumAnalyzer->GetFrequencyData(&FreqData[0], FreqData.size());

        // Calculate the spectrum 
        _SpectrumAnalyzer->calcSpectrum(FreqData, _FrequencyBands, _Configuration.interpSize, _Configuration._SummationMethod, false, _Configuration.smoothInterp, _Configuration.smoothSlope, SampleRate, _Spectrum);

        switch (_Configuration._SmoothingMethod)
        {
            case MethodAverage:
                calcSmoothingTimeConstant(_CurrentSpectrum, _Spectrum, _Configuration._SmoothingConstant);
                break;

            case MethodPeak:
                calcPeakDecay(_CurrentSpectrum, _Spectrum, _Configuration._SmoothingConstant);
                break;
        
            default:
                _CurrentSpectrum = _Spectrum;
        }
    }

    hr = RenderBands();

    return hr;
}

/// <summary>
/// Renders the bands.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderBands()
{
    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    FLOAT Width = (FLOAT) _RenderTargetProperties.pixelSize.width - YAxisWidth;

    FLOAT BandWidth = Max((Width / (FLOAT) _CurrentSpectrum.size()), 1.f);

    FLOAT x1 = YAxisWidth + (Width - ((FLOAT) _CurrentSpectrum.size() * BandWidth)) / 2.f;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = PaddingY;
    const FLOAT y2 = (FLOAT) _RenderTargetProperties.pixelSize.height - _LabelTextMetrics.height;

    RenderYAxis();

    const double FrequenciesDecades[] = { 10., 20., 30., 40., 50., 60., 70., 80., 90., 100., 200., 300., 400., 500., 600., 700., 800., 900., 1000., 2000., 3000., 4000., 5000., 6000., 7000., 8000., 9000., 10000., 20000. };
    const double FrequenciesOctaves[] = { 31., 63.5, 125., 250., 500., 1000., 2000., 4000., 8000., 16000. };

    uint32_t Note = _Configuration.MinNote;
    uint32_t i = 0;
    uint32_t j = 0;

    if (_Configuration._XAxisMode == XAxisMode::Decades)
    {
        for (; j < _countof(FrequenciesDecades); ++j)
        {
            if (_FrequencyBands[0].lo <= FrequenciesDecades[j])
                break;
        }
    }
    else
    if (_Configuration._XAxisMode == XAxisMode::OctavesX)
    {
        for (; j < _countof(FrequenciesOctaves); ++j)
        {
            if (_FrequencyBands[0].lo <= FrequenciesOctaves[j])
                break;
        }
    }

    for (const auto Iter : _CurrentSpectrum)
    {
        D2D1_RECT_F Rect = { x1, y1, x2 - PaddingX, y2 - PaddingY };

        switch (_Configuration._XAxisMode)
        {
            default:

            case XAxisMode::Bands:
            {
                if (i % 10 == 0)
                {
                    RenderXAxisFreq(x1 + BandWidth / 2.f - 20.f, y2, x1 + BandWidth / 2.f + 20.f, y2 + _LabelTextMetrics.height, _FrequencyBands[i].ctr);

                    // Draw the vertical grid line.
                    {
                        _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                        _RenderTarget->DrawLine(D2D1_POINT_2F(x1 + BandWidth / 2.f, y1), D2D1_POINT_2F(x1 + BandWidth / 2.f, y2), _TextBrush, 1.f, nullptr);
                    }
                }
                break;
            }

            case XAxisMode::Decades:
            {
                if ((_FrequencyBands[i].lo <= FrequenciesDecades[j]) && (FrequenciesDecades[j] <= _FrequencyBands[i].hi) && (j < _countof(FrequenciesDecades)))
                {
                    RenderXAxisFreq(x1 + BandWidth / 2.f - 20.f, y2, x1 + BandWidth / 2.f + 20.f, y2 + _LabelTextMetrics.height, FrequenciesDecades[j]);
                    ++j;

                    // Draw the vertical grid line.
                    {
                        _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                        _RenderTarget->DrawLine(D2D1_POINT_2F(x1 + BandWidth / 2.f, y1), D2D1_POINT_2F(x1 + BandWidth / 2.f, y2), _TextBrush, 1.f, nullptr);
                    }
                }
                break;
            }

            case XAxisMode::OctavesX:
            {
                if ((_FrequencyBands[i].lo <= FrequenciesOctaves[j]) && (FrequenciesOctaves[j] <= _FrequencyBands[i].hi) && (j < _countof(FrequenciesOctaves)))
                {
                    RenderXAxisFreq(x1 + BandWidth / 2.f - 20.f, y2, x1 + BandWidth / 2.f + 20.f, y2 + _LabelTextMetrics.height, FrequenciesOctaves[j]);
                    ++j;

                    // Draw the vertical grid line.
                    {
                        _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                        _RenderTarget->DrawLine(D2D1_POINT_2F(x1 + BandWidth / 2.f, y1), D2D1_POINT_2F(x1 + BandWidth / 2.f, y2), _TextBrush, 1.f, nullptr);
                    }
                }
                break;
            }

            case XAxisMode::Notes:
            {
                if (Note % 12 == 0)
                {
                    RenderXAxis(x1, y2, x2, y2 + _LabelTextMetrics.height, Note / 12u);

                    // Draw the vertical grid line.
                    {
                        _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                        _RenderTarget->DrawLine(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x1, y2), _TextBrush, 1.f, nullptr);
                    }
                }

                Note++;
                break;
            }
        }

#ifdef Original
        // Draw the background.
        {
            _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 0.3f)); // #1E90FF
            _RenderTarget->FillRectangle(Rect, _TextBrush);
        }

        // Draw the foreground.
        if (Iter > 0.0)
        {
            Rect.top = Max((FLOAT)(y2 - ((y2 - y1) * ascale(Iter))), y1);

            _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 1.0f)); // #1E90FF
            _RenderTarget->FillRectangle(Rect, _TextBrush);
        }
#endif
        // Draw the foreground.
        if (Iter > 0.0)
        {
            Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * ScaleA(Iter))), y1, y2);

            _RenderTarget->FillRectangle(Rect, _GradientBrush);
        }

        x1  = x2;
        x2 += BandWidth;

        ++i;
    }

    return S_OK;
}

/// <summary>
/// Render an X axis label.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderXAxisFreq(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, double frequency)
{
    _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // Draw the label.
    {
        WCHAR Text[16] = { };

        if (frequency < 1000.0)
            ::StringCchPrintfW(Text, _countof(Text), L"%.1fHz", frequency);
        else
            ::StringCchPrintfW(Text, _countof(Text), L"%.1fkHz", frequency / 1000);

        D2D1_RECT_F TextRect = { x1, y1, x2, y2 };

//      _RenderTarget->FillRectangle(&TextRect, _GradientBrush);

        _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

        _LabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _LabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        _LabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
        _RenderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _LabelTextFormat, TextRect, _TextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        #pragma warning(default: 6385)
    }

    return S_OK;
}

/// <summary>
/// Render an X axis label.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderXAxis(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, uint32_t octave)
{
    _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // Draw the label.
    {
        WCHAR Text[16] = { };

        ::StringCchPrintfW(Text, _countof(Text), L"C%d", octave);

        D2D1_RECT_F TextRect = { x1, y1, x2, y2 };

        _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

        _LabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _LabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        _LabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
        _RenderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _LabelTextFormat, TextRect, _TextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        #pragma warning(default: 6385)
    }

    return S_OK;
}

/// <summary>
/// Renders the Y axis.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderYAxis()
{
    const double Amplitudes[] = { 0, -6, -12, -18, -24, -30, -36, -42, -48, -54, -60, -66, -72, -78, -84, -90 }; // FIXME: Should be based on MindB and MaxdB

    const FLOAT Width  = (FLOAT) _RenderTargetProperties.pixelSize.width;
    const FLOAT Height = (FLOAT) _RenderTargetProperties.pixelSize.height - _LabelTextMetrics.height;

    const FLOAT StrokeWidth = 1.0f;

    for (size_t i = 0; i < _countof(Amplitudes); ++i)
    {
        FLOAT y = (FLOAT) Map(ScaleA(ToMagnitude(Amplitudes[i])), 0.0, 1.0, Height, 0.0);

        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the horizontal grid line.
        {
            _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

            _RenderTarget->DrawLine(D2D1_POINT_2F(YAxisWidth, y), D2D1_POINT_2F(Width, y), _TextBrush, StrokeWidth, nullptr);
        }

        // Draw the label.
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitudes[i]);

            FLOAT LineHeight = _LabelTextMetrics.height;

            D2D1_RECT_F TextRect = { 0.f, y - LineHeight / 2, YAxisWidth - 2.f, y + LineHeight / 2 };

            _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

            _LabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _LabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
            _RenderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _LabelTextFormat, TextRect, _TextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            #pragma warning(default: 6385)
        }
    }

    return S_OK;
}

/// <summary>
/// Renders some information about the visulizer.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderText()
{
    float FPS = 0.0f;

    {
        LARGE_INTEGER Frequency;

        ::QueryPerformanceFrequency(&Frequency);

        FPS = (float)((_Times.GetCount() - 1) * Frequency.QuadPart) / (float) (_Times.GetLast() - _Times.GetFirst());
    }

    HRESULT hr = S_OK;

    WCHAR Text[512] = { };

    switch (_Configuration._FrequencyDistribution)
    {
        case FrequencyDistribution::Frequencies:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"%uHz - %uHz, %u bands, FFT %u, FPS: %.2f\n",
                    _Configuration.MinFrequency, _Configuration.MaxFrequency, (uint32_t) _Configuration.NumBands, _Configuration._FFTSize, FPS);
            break;

        case FrequencyDistribution::Octaves:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"Note %u - %u, %u bands per octave, Tuning %.2fHz, FFT %u, FPS: %.2f\n",
                    _Configuration.MinNote, _Configuration.MaxNote, (uint32_t) _Configuration.BandsPerOctave, _Configuration._Pitch, _Configuration._FFTSize, FPS);
            break;

        case FrequencyDistribution::AveePlayer:
            break;
    }

    if (SUCCEEDED(hr))
    {
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        static const D2D1_RECT_F TextRect = { 4.f, 4.f, 768.f, 50.f };
        static const float TextRectInset = 10.f;

        {
            _TextBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

            _RenderTarget->FillRoundedRectangle(D2D1::RoundedRect(TextRect, TextRectInset, TextRectInset), _TextBrush);
        }

        {
            _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

            #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
            _RenderTarget->DrawText
            (
                Text,
                (UINT) ::wcsnlen(Text, _countof(Text)),
                _TextFormat,
                D2D1::RectF(TextRect.left + TextRectInset, TextRect.top + TextRectInset, TextRect.right - TextRectInset, TextRect.bottom - TextRectInset),
                _TextBrush,
                D2D1_DRAW_TEXT_OPTIONS_NONE
            );
            #pragma warning(default: 6385)
        }
    }

    return hr;
}

/// <summary>
/// Generates frequency bands.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBands()
{
    const double MinFreq = ScaleF(_Configuration.MinFrequency, _Configuration.ScalingFunction, _Configuration.SkewFactor);
    const double MaxFreq = ScaleF(_Configuration.MaxFrequency, _Configuration.ScalingFunction, _Configuration.SkewFactor);

    _FrequencyBands.resize(_Configuration.NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.lo  = DeScaleF(Map((double) i - _Configuration.Bandwidth, 0., (double)(_Configuration.NumBands - 1), MinFreq, MaxFreq), _Configuration.ScalingFunction, _Configuration.SkewFactor);
        Iter.ctr = DeScaleF(Map((double) i,                            0., (double)(_Configuration.NumBands - 1), MinFreq, MaxFreq), _Configuration.ScalingFunction, _Configuration.SkewFactor);
        Iter.hi  = DeScaleF(Map((double) i + _Configuration.Bandwidth, 0., (double)(_Configuration.NumBands - 1), MinFreq, MaxFreq), _Configuration.ScalingFunction, _Configuration.SkewFactor);
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBandsFromNotes(uint32_t sampleRate)
{
    const double Root24 = ::exp2(1. / 24.);
    const double NyquistFrequency = (double) sampleRate / 2.;

    const double Pitch = (_Configuration._Pitch > 0.0) ? ::round((::log2(_Configuration._Pitch) - 4.) * 12.) * 2. : 00;
    const double C0 = _Configuration._Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NotesGroup = 24. / _Configuration.BandsPerOctave;

    const double LoNote = ::round(_Configuration.MinNote * 2. / NotesGroup);
    const double HiNote = ::round(_Configuration.MaxNote * 2. / NotesGroup);

    _FrequencyBands.clear();

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - _Configuration.Bandwidth) * NotesGroup + _Configuration.Detune)),
            C0 * ::pow(Root24,  (i                             * NotesGroup + _Configuration.Detune)),
            C0 * ::pow(Root24, ((i + _Configuration.Bandwidth) * NotesGroup + _Configuration.Detune)),
        };

        _FrequencyBands.push_back((fb.ctr < NyquistFrequency) ? fb : FrequencyBand(NyquistFrequency, NyquistFrequency, NyquistFrequency));
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double SpectrumAnalyzerUIElement::ScaleF(double x, ScalingFunctions function, double skewFactor)
{
    switch (function)
    {
        default:

        case Linear:
            return x;

        case Logarithmic:
            return ::log2(x);

        case ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + x);

        case Mel:
            return ::log2(1.0 + x / 700.0);

        case Bark: // "Critical bands"
            return (26.81 * x) / (1960.0 + x) - 0.53;

        case AdjustableBark:
            return (26.81 * x) / (::pow(10, skewFactor * 4.0) + x);

        case ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * x);

        case Cams:
            return ::log2((x / 1000.0 + 0.312) / (x / 1000.0 + 14.675));

        case HyperbolicSine:
            return ::asinh(x / ::pow(10, skewFactor * 4));

        case NthRoot:
            return ::pow(x, (1.0 / (11.0 - skewFactor * 10.0)));

        case NegativeExponential:
            return -::exp2(-x / ::exp2(7 + skewFactor * 8));

        case Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Descales the frequency.
/// </summary>
double SpectrumAnalyzerUIElement::DeScaleF(double x, ScalingFunctions function, double skewFactor)
{
    switch (function)
    {
        default:

        case Linear:
            return x;

        case Logarithmic:
            return ::exp2(x);

        case ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Scales the amplitude.
/// </summary>
double SpectrumAnalyzerUIElement::ScaleA(double x) const
{
    if (_Configuration.UseDecibels)
        return Map(ToDecibel(x), _Configuration.MinDecibels, _Configuration.MaxDecibels, 0.0, 1.0);

    double Exponent = 1.0 / _Configuration.Gamma;

    return Map(::pow(x, Exponent), _Configuration._UseAbsolute ? 0.0 : ::pow(ToMagnitude(_Configuration.MinDecibels), Exponent), ::pow(ToMagnitude(_Configuration.MaxDecibels), Exponent), 0.0, 1.0);
}

/// <summary>
/// Applies a time smoothing factor.
/// </summary>
void SpectrumAnalyzerUIElement::calcSmoothingTimeConstant(std::vector<double> & dst, const std::vector<double> & src, double factor)
{
    if (factor != 0.0)
    {
        for (size_t i = 0; i < src.size(); ++i)
            dst[i] = (!::isnan(dst[i]) ? dst[i] * factor : 0.0) + (!::isnan(src[i]) ? src[i] * (1.0 - factor) : 0.0);
    }
    else
        dst = src;
}

void SpectrumAnalyzerUIElement::calcPeakDecay(std::vector<double> & dst, const std::vector<double> & src, double factor)
{
    for (size_t i = 0; i < src.size(); ++i)
        dst[i] = Max(!::isnan(dst[i]) ? dst[i] * factor : 0.0, !::isnan(src[i]) ? src[i] : 0.0);
}

#pragma region DirectX
/// <summary>
/// Create resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::CreateDeviceIndependentResources()
{
    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_Direct2dFactory);

    if (SUCCEEDED(hr))
        hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_DirectWriteFactory), reinterpret_cast<IUnknown **>(&_DirectWriteFactory));

    if (SUCCEEDED(hr))
    {
        static const PCWSTR FontFamilyName = L"Calibri";
        static const FLOAT FontSize = 20.0f; // In DIP

        hr = _DirectWriteFactory->CreateTextFormat(FontFamilyName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
    }

    if (SUCCEEDED(hr))
    {
        static const PCWSTR FontFamilyName = L"Segoe UI";
        static const FLOAT FontSize = ToDIPs(6.0f); // In DIP

        hr = _DirectWriteFactory->CreateTextFormat(FontFamilyName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_LabelTextFormat);
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectWriteFactory->CreateTextLayout(L"-90dB", 1, _LabelTextFormat, 100, 100, &TextLayout);

        if (SUCCEEDED(hr))
            TextLayout->GetMetrics(&_LabelTextMetrics);
    }

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT SpectrumAnalyzerUIElement::CreateDeviceSpecificResources()
{
    if (_Direct2dFactory == nullptr)
        return E_FAIL;

    HRESULT hr = S_OK;

    // Create the render target.
    if (_RenderTarget == nullptr)
    {
        CRect ClientRect;

        GetClientRect(ClientRect);

        D2D1_SIZE_U ClientSize = D2D1::SizeU((UINT32) ClientRect.Width(), (UINT32) ClientRect.Height());

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);

        _RenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, ClientSize);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, _RenderTargetProperties, &_RenderTarget);
    }

    // Create the brushes.
    if (SUCCEEDED(hr) && (_StrokeBrush == nullptr))
    {
        t_ui_color TextColor = m_callback->query_std_color(ui_color_text);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(TextColor) / 255.0f, GetGValue(TextColor) / 255.0f, GetBValue(TextColor) / 255.0f), &_StrokeBrush);
    }

    if (SUCCEEDED(hr) && (_GradientBrush == nullptr))
    {
        CComPtr<ID2D1GradientStopCollection> Collection;

        // Prism / foo_musical_spectrum
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 5.f, D2D1::ColorF(0xFD0000, 1.f) },
            { 1.f / 5.f, D2D1::ColorF(0xFF8000, 1.f) },
            { 2.f / 5.f, D2D1::ColorF(0xFFFF01, 1.f) },
            { 3.f / 5.f, D2D1::ColorF(0x7EFF77, 1.f) },
            { 4.f / 5.f, D2D1::ColorF(0x0193A2, 1.f) },
            { 5.f / 5.f, D2D1::ColorF(0x002161, 1.f) },
        };
/*
        // Prism 2
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 9.f, D2D1::ColorF(0xAA3355, 1.f) },
            { 1.f / 9.f, D2D1::ColorF(0xCC6666, 1.f) },
            { 2.f / 9.f, D2D1::ColorF(0xEE9944, 1.f) },
            { 3.f / 9.f, D2D1::ColorF(0xEEDD00, 1.f) },
            { 4.f / 9.f, D2D1::ColorF(0x99DD55, 1.f) },
            { 5.f / 9.f, D2D1::ColorF(0x44DD88, 1.f) },
            { 6.f / 9.f, D2D1::ColorF(0x22CCBB, 1.f) },
            { 7.f / 9.f, D2D1::ColorF(0x00BBCC, 1.f) },
            { 8.f / 9.f, D2D1::ColorF(0x0099CC, 1.f) },
            { 9.f / 9.f, D2D1::ColorF(0x3366BB, 1.f) },
        };
*//*
        // Prism 3
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 4.f, D2D1::ColorF(0xFF0000, 1.f) }, // hsl(  0, 100%, 50%)
            { 1.f / 4.f, D2D1::ColorF(0xFFFF00, 1.f) }, // hsl( 60, 100%, 50%)
            { 2.f / 4.f, D2D1::ColorF(0x00FF00, 1.f) }, // hsl(120, 100%, 50%)
            { 3.f / 4.f, D2D1::ColorF(0x00FFFF, 1.f) }, // hsl(180, 100%, 50%)
            { 4.f / 4.f, D2D1::ColorF(0x0000FF, 1.f) }, // hsl(240, 100%, 50%)
        };
*//*
        // foobar2000 Dark Mode
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 1.f, D2D1::ColorF(0x0080FF, 1.f) },
            { 1.f / 1.f, D2D1::ColorF(0xFFFFFF, 1.f) },
        };
*//*
        // foobar2000
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 1.f, D2D1::ColorF(0x0066CC, 1.f) }, 
            { 1.f / 1.f, D2D1::ColorF(0x000000, 1.f) },
        };
*/
        hr = _RenderTarget->CreateGradientStopCollection(GradientStops, _countof(GradientStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

        if (SUCCEEDED(hr))
        {
            D2D1_SIZE_F Size = _RenderTarget->GetSize();

            hr = _RenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_GradientBrush);
        }
    }

    if (SUCCEEDED(hr) && (_TextBrush == nullptr))
    {
        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_TextBrush);
    }

    return hr;
}

/// <summary>
/// Release the device specific resources.
/// </summary>
void SpectrumAnalyzerUIElement::ReleaseDeviceSpecificResources()
{
    _TextBrush.Release();
    _GradientBrush.Release();
    _StrokeBrush.Release();
    _RenderTarget.Release();
}
#pragma endregion

#pragma region ui_element_instance

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::g_get_name(pfc::string_base & p_out)
{
    p_out = STR_COMPONENT_NAME;
}

/// <summary>
/// 
/// </summary>
const char * SpectrumAnalyzerUIElement::g_get_description()
{
    return "Spectogram visualization using Direct2D";
}

/// <summary>
/// 
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_guid()
{
    static const GUID guid = GUID_UI_ELEMENT_SPECTOGRAM;

    return guid;
}

/// <summary>
/// 
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_subclass()
{
    return ui_element_subclass_playback_visualisation;
}

/// <summary>
/// 
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::g_get_default_configuration()
{
    Configuration Config;

    ui_element_config_builder Builder;

    Config.Write(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::initialize_window(HWND p_parent)
{
    this->Create(p_parent, nullptr, nullptr, 0, WS_EX_STATICEDGE);
}

/// <summary>
/// Alters element's current configuration. Specified ui_element_config's GUID must be the same as this element's GUID.
/// </summary>
void SpectrumAnalyzerUIElement::set_configuration(ui_element_config::ptr data)
{
    ui_element_config_parser Parser(data);

    _Configuration.Read(Parser);

    UpdateRefreshRateLimit();
}

/// <summary>
/// Retrieves element's current configuration. Returned object's GUID must be set to your element's GUID so your element can be re-instantiated with stored settings.
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::get_configuration()
{
    ui_element_config_builder Builder;

    _Configuration.Write(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::notify(const GUID & what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
    if (what == ui_element_notify_colors_changed)
    {
        _StrokeBrush.Release();

        Invalidate();
    }
}

#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_new_track(metadb_handle_ptr track)
{
    _Configuration.Read();

    // Make sure the spectrum analyzer is recreated. The audio chunks may have another configuration than the ones from the previous track.
    if (_SpectrumAnalyzer != nullptr)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    _IsPlaying = true;

    Invalidate();
}

/// <summary>
/// Playback stopped.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    _IsPlaying = false;

    Invalidate();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_pause(bool)
{
    _IsPlaying = !_IsPlaying;

    Invalidate();
}

#pragma endregion

static service_factory_single_t<ui_element_impl_visualisation<SpectrumAnalyzerUIElement>> _Factory;
