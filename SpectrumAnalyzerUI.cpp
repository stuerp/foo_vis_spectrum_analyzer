
/** $VER: SpectrumAnalyzerUI.cpp (2023.12.02) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include <complex>

#include "Configuration.h"
#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#include "Gradients.h"

#pragma hdrstop

/// <summary>
/// Constructor
/// </summary>
SpectrumAnalyzerUIElement::SpectrumAnalyzerUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback) : m_callback(callback), _LastRefresh(0), _RefreshInterval(10)
{
    set_configuration(data);
}

#pragma region User Interface

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
    // Remove the border of the client area.
    {
        LONG_PTR NewStyle = ::GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE);

        NewStyle &= ~WS_EX_STATICEDGE;

        ::SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, NewStyle);
    }

    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
        Log(LogLevel::Critical, "%s: Unable to create Direct2D device independent resources: 0x%08X", core_api::get_my_file_name(), hr);

    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, visualisation_manager::KStreamFlagNewFFT);

        _VisualisationStream->request_backlog(0.8); // FIXME: What does this do?
    }
    catch (std::exception & ex)
    {
        Log(LogLevel::Critical, "%s: Unable to create visualisation stream. %s.", core_api::get_my_file_name(), ex.what());
    }

    // Create the tooltip control.
    {
        _ToolTip.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP);

        // Adds a tracking tool tip.
        {
            auto ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, (LPWSTR) L"");

            GetClientRect(&ti.rect);

            _ToolTip.AddTool(&ti);
        }
    }

    // Applies the initial configuration.
    SetConfiguration();

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

    if (_CQT)
    {
        delete _CQT;
        _CQT = nullptr;
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
    RenderFrame();

    ValidateRect(nullptr);

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
    if (_RenderTarget == nullptr)
        return;

    D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    if (_RenderTarget)
        _RenderTarget->Resize(Size);

    Resize();
}

/// <summary>
/// Handles a context menu selection.
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

/// <summary>
/// Handles a DPI change.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect)
{
    ReleaseDeviceSpecificResources();

    return 0;
}

/// <summary>
/// Handles mouse move messages.
/// </summary>
void SpectrumAnalyzerUIElement::OnMouseMove(UINT, CPoint pt)
{
    if (!_ToolTip.IsWindow())
        return;

    if (_TrackingToolInfo == nullptr)
    {
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };

            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
        
            ::TrackMouseEvent(&tme);
        }

        {
            _TrackingToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, nullptr);

            _ToolTip.SetMaxTipWidth(100);
            _ToolTip.TrackActivate(_TrackingToolInfo, TRUE);
        }

        _LastMousePos = POINT(-1, -1);
        _LastIndex = -1;
    }
    else
    if ((pt.x != _LastMousePos.x) || (pt.y != _LastMousePos.y))
    {
        _LastMousePos = pt;

        int Index = (int) ::floor(Map((FLOAT) pt.x, _Spectrum.GetLeft(), _Spectrum.GetRight(), 0.f, (FLOAT) _FrequencyBands.size()));

        if ((Index != _LastIndex) && InRange(Index, 0, (int) _FrequencyBands.size() - 1))
        {
            _LastIndex = Index;

            _TrackingToolInfo->lpszText = _FrequencyBands[(size_t) Index].Label;

            _ToolTip.SetToolInfo(_TrackingToolInfo);

            ::ClientToScreen(m_hWnd, &pt);

            _ToolTip.TrackPosition(pt.x + 10, pt.y - 20);
        }
    }
}

/// <summary>
/// Turns off the tracking tooltip when the mouse leaves the window.
/// </summary>
void SpectrumAnalyzerUIElement::OnMouseLeave()
{
    _ToolTip.TrackActivate(_TrackingToolInfo, FALSE);

    delete _TrackingToolInfo;
    _TrackingToolInfo = nullptr;
}

/// <summary>
/// Handles a configuration change.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnConfigurationChanging(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SetConfiguration();

    return 0;
}

/// <summary>
/// Called when the configuration dialog is closed by a click on the OK button.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ui_element_config_builder Builder;

    _Configuration.Write(Builder);

    Builder.finish(g_get_guid());

    return 0;
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
    _RefreshInterval = Clamp<DWORD>(1000 / (DWORD) _Configuration._RefreshRateLimit, 5, 1000);
}

/// <summary>
/// Shows the Options dialog.
/// </summary>
void SpectrumAnalyzerUIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        DialogParameters dp = { m_hWnd, &_Configuration };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) == NULL)
            return;

        _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Sets the current configuration.
/// </summary>
void SpectrumAnalyzerUIElement::SetConfiguration() noexcept
{
    _Bandwidth = ((_Configuration._Transform == Transform::CQT) || ((_Configuration._Transform == Transform::FFT) && (_Configuration._MappingMethod == Mapping::TriangularFilterBank))) ? _Configuration._Bandwidth : 0.5;

    // Initialize the frequency bands.
    if (_Configuration._Transform == Transform::FFT)
    {
        switch (_Configuration._FrequencyDistribution)
        {
            default:

            case FrequencyDistribution::Linear:
                GenerateFrequencyBands();
                break;

            case FrequencyDistribution::Octaves:
                GenerateFrequencyBandsFromNotes();
                break;

            case FrequencyDistribution::AveePlayer:
                GenerateFrequencyBandsOfAveePlayer();
                break;
        }
    }
    else
        GenerateFrequencyBandsFromNotes();

    _XAxis.Initialize(&_Configuration, _FrequencyBands);

    _YAxis.Initialize(&_Configuration);

    _Spectrum.Initialize(&_Configuration);

    _Spectrum.SetGradientStops(_Configuration._GradientStops);

    _Spectrum.SetDrawBandBackground(_Configuration._DrawBandBackground);

    _ToolTip.Activate(_Configuration._ShowToolTips);

    // Forces the recreation of the spectrum analyzer.
    if (_SpectrumAnalyzer != nullptr)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    // Forces the recreation of the Constant-Q transform.
    if (_CQT != nullptr)
    {
        delete _CQT;
        _CQT = nullptr;
    }

    Resize();

    InvalidateRect(NULL);
}

/// <summary>
/// Resizes all render targets.
/// </summary>
void SpectrumAnalyzerUIElement::Resize()
{
    if (_RenderTarget == nullptr)
        return;

    D2D1_SIZE_F Size = _RenderTarget->GetSize();

    const FLOAT dw = (_Configuration._YAxisMode != YAxisMode::None) ? _YAxis.GetWidth()  : 0.f;
    const FLOAT dh = (_Configuration._XAxisMode != XAxisMode::None) ? _XAxis.GetHeight() : 0.f;

    _FrameCounter.Resize(Size.width, Size.height);

    {
        D2D1_RECT_F Rect(dw, 0.f, Size.width, Size.height);

        _XAxis.Resize(Rect);
    }

    {
        D2D1_RECT_F Rect(0.f, 0.f, Size.width, Size.height - dh);

        _YAxis.Resize(Rect);
    }

    {
        D2D1_RECT_F Rect(dw, 0.f, Size.width, Size.height - dh);

        _Spectrum.Resize(Rect);

        // Adjust the tracking tool tip.
        {
            auto ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, (LPWSTR) L"");

            SetRect(&ti.rect, (int) Rect.left, (int) Rect.top, (int) Rect.right, (int) Rect.bottom);

            _ToolTip.AddTool(&ti);
        }
    }
}

/// <summary>
/// Writes a message to the console
/// </summary>
void SpectrumAnalyzerUIElement::Log(LogLevel logLevel, const char * format, ...) const noexcept
{
    if (logLevel < _Configuration._LogLevel)
        return;

    va_list va;

    va_start(va, format);

    console::printfv(format, va);

    va_end(va);
}

#pragma endregion

#pragma region Rendering

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderFrame()
{
    _FrameCounter.NewFrame();

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && !(_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _RenderTarget->Clear(_Configuration._BackColor);

        _XAxis.Render(_RenderTarget);

        _YAxis.Render(_RenderTarget);

        // Draw the visualization.
        if (_VisualisationStream.is_valid())
        {
            double PlaybackTime; // in ms

            if (_VisualisationStream->get_absolute_time(PlaybackTime))
            {
                double WindowDuration = _Configuration.GetWindowDurationInMs();

                audio_chunk_impl Chunk;

                if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - WindowDuration / 2., WindowDuration * (_Configuration._UseZeroTrigger ? 2. : 1.)))
                    RenderChunk(Chunk);
            }
        }

        if (_Configuration._LogLevel != LogLevel::None)
            _FrameCounter.Render(_RenderTarget);

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
/// Renders an audio chunk.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderChunk(const audio_chunk & chunk)
{
    HRESULT hr = S_OK;

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    _SampleRate = chunk.get_sample_rate();

    Log(LogLevel::Trace, "%s: Rendering chunk { ChannelCount: %d, ChannelSetup: 0x%08X, SampleRate: %d }.", core_api::get_my_file_name(), ChannelCount, ChannelSetup, _SampleRate);

    // Create the transformer when necessary.
    {
        if (_SpectrumAnalyzer == nullptr)
        {
            #pragma warning (disable: 4061)
            switch (_Configuration._FFTSize)
            {
                default:
                    _FFTSize = (size_t) (64. * ::exp2((long) _Configuration._FFTSize));
                    break;

                case FFTSize::FFTCustom:
                    _FFTSize = (_Configuration._FFTCustom > 0) ? (size_t) _Configuration._FFTCustom : 64;
                    break;

                case FFTSize::FFTDuration:
                    _FFTSize = (_Configuration._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _Configuration._FFTDuration) / 1000.) : 64;
                    break;
            }
            #pragma warning (default: 4061)

            _SpectrumAnalyzer = new SpectrumAnalyzer(&_Configuration, ChannelCount, ChannelSetup, _SampleRate, _FFTSize);

            _FrequencyCoefficients.resize(_FFTSize);
        }

        if (_CQT == nullptr)
            _CQT = new CQTProvider(ChannelCount, ChannelSetup, _SampleRate, 1.0, 1.0, 0.0);
    }

    // Add the samples to the spectrum analyzer or the CQT.
    {
        const audio_sample * Samples = chunk.get_data();

        if (Samples == nullptr)
            return E_FAIL;

        size_t SampleCount = chunk.get_sample_count();

        if (_Configuration._Transform == Transform::FFT)
        {
            _SpectrumAnalyzer->Add(Samples, SampleCount, _Configuration._SelectedChannels);

            _SpectrumAnalyzer->GetFrequencyCoefficients(_FrequencyCoefficients);

            // Get the spectrum from the frequency coefficients.
            if (_Configuration._MappingMethod == Mapping::Standard)
                _SpectrumAnalyzer->GetSpectrum(_FrequencyCoefficients, _FrequencyBands, _SampleRate, _Configuration._SummationMethod);
            else
                _SpectrumAnalyzer->GetSpectrum(_FrequencyCoefficients, _FrequencyBands, _SampleRate);
        }
        else
            _CQT->GetFrequencyBands(Samples, SampleCount, _Configuration._SelectedChannels, _FrequencyBands);
    }

    // Smooth the spectrum.
    {
        switch (_Configuration._SmoothingMethod)
        {
            default:

            case SmoothingMethod::Average:
            {
                ApplyAverageSmoothing(_Configuration._SmoothingFactor);
                break;
            }

            case SmoothingMethod::Peak:
            {
                ApplyPeakSmoothing(_Configuration._SmoothingFactor);
                break;
            }
        }
    }

    // Update the peak indicators.
    {
        if (_Configuration._PeakMode != PeakMode::None)
            _SpectrumAnalyzer->UpdatePeakIndicators(_FrequencyBands);
    }

    _Spectrum.Render(_RenderTarget, _FrequencyBands, (double) _SampleRate, _Configuration);

    return hr;
}

/// <summary>
/// Generates frequency bands.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBands()
{
    const double MinFreq = ScaleF(_Configuration._LoFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);
    const double MaxFreq = ScaleF(_Configuration._HiFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);

    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = DeScaleF(Map((double) i - _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Ctr = DeScaleF(Map((double) i,              0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Hi  = DeScaleF(Map((double) i + _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);

        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBandsFromNotes()
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (_Configuration._Pitch > 0.) ? ::round((::log2(_Configuration._Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = _Configuration._Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NotesGroup = 24. / _Configuration._BandsPerOctave;

    const double LoNote = ::round(_Configuration._MinNote * 2. / NotesGroup);
    const double HiNote = ::round(_Configuration._MaxNote * 2. / NotesGroup);

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - _Bandwidth) * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24,  (i               * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24, ((i + _Bandwidth) * NotesGroup + _Configuration._Transpose)),
        };

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[(int) i % 12], (int) i / 12, fb.Ctr);

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands of AveePlayer.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBandsOfAveePlayer()
{
    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo      = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i - _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Ctr     = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i,              _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Hi      = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i + _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double SpectrumAnalyzerUIElement::ScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::log2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::Mel:
            return ::log2(1.0 + x / 700.0);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * x) / (1960.0 + x) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * x) / (::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * x);

        case ScalingFunction::Cams:
            return ::log2((x / 1000.0 + 0.312) / (x / 1000.0 + 14.675));

        case ScalingFunction::HyperbolicSine:
            return ::asinh(x / ::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return ::pow(x, (1.0 / (11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::exp2(-x / ::exp2(7 + skewFactor * 8));

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Descales the frequency.
/// </summary>
double SpectrumAnalyzerUIElement::DeScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case ScalingFunction::Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case ScalingFunction::AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case ScalingFunction::Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case ScalingFunction::HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case ScalingFunction::NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Smooths the spectrum using averages.
/// </summary>
void SpectrumAnalyzerUIElement::ApplyAverageSmoothing(double factor)
{
    if (factor != 0.0)
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = (::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0) + (::isfinite(Iter.NewValue) ? Iter.NewValue * (1.0 - factor) : 0.0);

    }
    else
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = Iter.NewValue;
    }
}

/// <summary>
/// Smooths the spectrum using peak decay.
/// </summary>
void SpectrumAnalyzerUIElement::ApplyPeakSmoothing(double factor)
{
    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue = Max(::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0, ::isfinite(Iter.NewValue) ? Iter.NewValue : 0.0);
}

#pragma endregion

#pragma region DirectX
/// <summary>
/// Create resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::CreateDeviceIndependentResources()
{
    Log(LogLevel::Trace, "%s: Creating device independent resource", core_api::get_my_file_name());

    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_Direct2dFactory);

    if (SUCCEEDED(hr))
        hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_DirectWriteFactory), reinterpret_cast<IUnknown **>(&_DirectWriteFactory));
    else
        Log(LogLevel::Error, "%s: Unable to create D2D1CreateFactory: 0x%08X.", core_api::get_my_file_name(), hr);

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceIndependentResources(_DirectWriteFactory);

    if (SUCCEEDED(hr))
        hr = _XAxis.CreateDeviceIndependentResources(_DirectWriteFactory);

    if (SUCCEEDED(hr))
        hr = _YAxis.CreateDeviceIndependentResources(_DirectWriteFactory);

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
        CRect rc;

        GetClientRect(rc);

        D2D1_SIZE_U Size = D2D1::SizeU((UINT32) rc.Width(), (UINT32) rc.Height());

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
            _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Resize some elements based on the size of the render target.
        Resize();
    }

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceSpecificResources(_RenderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void SpectrumAnalyzerUIElement::ReleaseDeviceSpecificResources()
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();
    _FrameCounter.ReleaseDeviceSpecificResources();

    _RenderTarget.Release();
}
#pragma endregion

#pragma region ui_element_instance

/// <summary>
/// Retrieves the name of the element.
/// </summary>
void SpectrumAnalyzerUIElement::g_get_name(pfc::string_base & p_out)
{
    p_out = STR_COMPONENT_NAME;
}

/// <summary>
/// Retrieves the description of the element.
/// </summary>
const char * SpectrumAnalyzerUIElement::g_get_description()
{
    return "Spectum analyzer visualization using DirectX";
}

/// <summary>
/// Retrieves the GUID of the element.
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_guid()
{
    static const GUID guid = GUID_UI_ELEMENT_SPECTOGRAM;

    return guid;
}

/// <summary>
/// Retrieves the subclass GUID of the element.
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_subclass()
{
    return ui_element_subclass_playback_visualisation;
}

/// <summary>
/// Retrieves the default configuration of the element.
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::g_get_default_configuration()
{
    Configuration DefaultConfiguration;

    ui_element_config_builder Builder;

    DefaultConfiguration.Write(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// Initializes the element's windows.
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

    try
    {
        _Configuration.Read(Parser);
    }
    catch (exception_io & ex)
    {
        Log(LogLevel::Error, "%s: Exception while reading configuration data: %s", core_api::get_my_file_name(), ex.what());

        _Configuration.Reset();
    }

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
/// Used by host to notify the element about various events. See ui_element_notify_* GUIDs for possible p_what parameter; meaning of other parameters depends on p_what value. Container classes should dispatch all notifications to their children.
/// </summary>
void SpectrumAnalyzerUIElement::notify(const GUID & what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
    if (what == ui_element_notify_colors_changed)
        Invalidate();
}

#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_new_track(metadb_handle_ptr track)
{
    SetConfiguration();

    Invalidate();
}

/// <summary>
/// Playback stopped.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    Invalidate();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_pause(bool)
{
    Invalidate();
}

#pragma endregion

static service_factory_single_t<ui_element_impl_visualisation<SpectrumAnalyzerUIElement>> _Factory;
