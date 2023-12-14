
/** $VER: UIElement.cpp (2023.12.14) P. Stuer **/

#include "UIElement.h"

#include "Resources.h"
#include "Gradients.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
UIElement::UIElement(): _TrackingToolInfo()
{
}

#pragma region User Interface

/// <summary>
/// Gets the window class definition.
/// </summary>
CWndClassInfo & UIElement::GetWndClassInfo()
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
        NULL, NULL, IDC_ARROW, TRUE, 0, L""
    };

    return wci;
}

/// <summary>
/// Creates the window.
/// </summary>
LRESULT UIElement::OnCreate(LPCREATESTRUCT cs)
{
    ::InitializeCriticalSection(&_Lock);

    _DPI = GetDpiForWindow(m_hWnd);

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
    _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

    _ToolTipControl.SetMaxTipWidth(100);

    _TrackingToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, nullptr);

    // Create the timer.
    _ThreadPoolTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);

    // Applies the initial configuration.
    SetConfiguration();

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void UIElement::OnDestroy()
{
    ::EnterCriticalSection(&_Lock);

    if (_ThreadPoolTimer)
    {
        ::CloseThreadpoolTimer(_ThreadPoolTimer);
        _ThreadPoolTimer = nullptr;
    }

    if (_WindowFunction)
    {
        delete _WindowFunction;
        _WindowFunction = nullptr;
    }

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

    ::LeaveCriticalSection(&_Lock);
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void UIElement::OnPaint(CDCHandle hDC)
{
    RenderFrame();

    ValidateRect(nullptr);

    UpdateRefreshRateLimit();
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void UIElement::OnSize(UINT type, CSize size)
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
void UIElement::OnContextMenu(CWindow wnd, CPoint position)
{
    CMenu Menu;
    CMenu RefreshRateLimitMenu;

    {
        Menu.CreatePopupMenu();

        Menu.AppendMenu((UINT) MF_STRING, IDM_CONFIGURE, L"Configure");
        Menu.AppendMenu((UINT) MF_SEPARATOR);
        Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, L"Full-Screen Mode");
        Menu.AppendMenu((UINT) MF_STRING | (_Configuration._ShowFrameCounter ? MF_CHECKED : 0), IDM_TOGGLE_FRAME_COUNTER, L"Frame Counter");
//      Menu.AppendMenu((UINT) MF_STRING | (_Configuration._UseHardwareRendering ? MF_CHECKED : 0), IDM_TOGGLE_HARDWARE_RENDERING, L"Hardware Rendering");

        {
            RefreshRateLimitMenu.CreatePopupMenu();

            const size_t RefreshRates[] = { 20, 30, 60, 100, 200 };

            for (size_t i = 0; i < _countof(RefreshRates); ++i)
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  RefreshRates[i]) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20 + i,
                    pfc::wideFromUTF8(pfc::format(RefreshRates[i], L"Hz")));

            Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, L"Refresh Rate Limit");
        }

        Menu.SetMenuDefaultItem(IDM_CONFIGURE);
    }

    int CommandId = Menu.TrackPopupMenu(TPM_RETURNCMD | TPM_VERNEGANIMATION | TPM_RIGHTBUTTON | TPM_NONOTIFY, position.x, position.y, *this);

    switch (CommandId)
    {
        case IDM_TOGGLE_FULLSCREEN:
            ToggleFullScreen();
            break;

        case IDM_TOGGLE_FRAME_COUNTER:
            ToggleFrameCounter();
            break;

        case IDM_TOGGLE_HARDWARE_RENDERING:
            ToggleHardwareRendering();
            break;

        case IDM_REFRESH_RATE_LIMIT_20:
            _Configuration._RefreshRateLimit = 20;
            UpdateRefreshRateLimit();
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _Configuration._RefreshRateLimit = 30;
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

/// <summary>
/// Toggles between panel and full screen mode.
/// </summary>
void UIElement::OnLButtonDblClk(UINT flags, CPoint point)
{
    ToggleFullScreen();
}

/// <summary>
/// Handles a DPI change.
/// </summary>
LRESULT UIElement::OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect)
{
    _DPI = dpiX;

    ReleaseDeviceSpecificResources();

    return 0;
}

/// <summary>
/// Handles mouse move messages.
/// </summary>
void UIElement::OnMouseMove(UINT, CPoint pt)
{
    if (!_ToolTipControl.IsWindow())
        return;

    if (!_IsTracking)
    {
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };

            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
        
            ::TrackMouseEvent(&tme);
        }

        _LastMousePos = POINT(-1, -1);
        _LastIndex = -1;

        _ToolTipControl.TrackActivate(_TrackingToolInfo, TRUE);
        _IsTracking = true;
    }
    else
    if ((pt.x != _LastMousePos.x) || (pt.y != _LastMousePos.y))
    {
        _LastMousePos = pt;
    
        FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

        int Index = (int) ::floor(Map(ScaledX, _Spectrum.GetLeft(), _Spectrum.GetRight(), 0.f, (FLOAT) _FrequencyBands.size()));

        if ((Index != _LastIndex) && InRange(Index, 0, (int) _FrequencyBands.size() - 1))
        {
            _LastIndex = Index;

            _TrackingToolInfo->lpszText = _FrequencyBands[(size_t) Index].Label;

            _ToolTipControl.UpdateTipText(_TrackingToolInfo);
        }

        ::ClientToScreen(m_hWnd, &pt);

        _ToolTipControl.TrackPosition(pt.x + 10, pt.y - 35);
    }
}

/// <summary>
/// Turns off the tracking tooltip when the mouse leaves the window.
/// </summary>
void UIElement::OnMouseLeave()
{
    _ToolTipControl.TrackActivate(_TrackingToolInfo, FALSE);
    _IsTracking = false;
}

/// <summary>
/// Handles a configuration change.
/// </summary>
LRESULT UIElement::OnConfigurationChanging(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SetConfiguration();

    return 0;
}

/// <summary>
/// Toggles full screen mode.
/// </summary>
void UIElement::ToggleFullScreen() noexcept
{
}

/// <summary>
/// Toggles the frame counter display.
/// </summary>
void UIElement::ToggleFrameCounter() noexcept
{
    _Configuration._ShowFrameCounter = !_Configuration._ShowFrameCounter;
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void UIElement::ToggleHardwareRendering() noexcept
{
    _Configuration._UseHardwareRendering = !_Configuration._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Updates the refresh rate.
/// </summary>
void UIElement::UpdateRefreshRateLimit() noexcept
{
    if (_ThreadPoolTimer)
    {
        FILETIME DueTime = { };

        ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _Configuration._RefreshRateLimit, 0);
    }
}

/// <summary>
/// Shows the Options dialog.
/// </summary>
void UIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        ::EnterCriticalSection(&_Lock);

        DialogParameters dp = { m_hWnd, &_Configuration };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);

        ::LeaveCriticalSection(&_Lock);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Sets the current configuration.
/// </summary>
void UIElement::SetConfiguration() noexcept
{
    ::EnterCriticalSection(&_Lock);

    _Bandwidth = ((_Configuration._Transform == Transform::CQT) || ((_Configuration._Transform == Transform::FFT) && (_Configuration._MappingMethod == Mapping::TriangularFilterBank))) ? _Configuration._Bandwidth : 0.5;

    // Initialize the frequency bands.
    {
        if (_Configuration._Transform == Transform::FFT)
        {
            switch (_Configuration._FrequencyDistribution)
            {
                default:

                case FrequencyDistribution::Linear:
                    GenerateLinearFrequencyBands();
                    break;

                case FrequencyDistribution::Octaves:
                    GenerateOctaveFrequencyBands();
                    break;

                case FrequencyDistribution::AveePlayer:
                    GenerateAveePlayerFrequencyBands();
                    break;
            }
        }
        else
            GenerateOctaveFrequencyBands();
    }

    // Generate the horizontal color gradient, if required.
    if (_Configuration._HorizontalGradient)
    {
        if (_Configuration._GradientStops.size() > 1)
        {
            size_t j = 0;

            D2D1_COLOR_F Color1 = _Configuration._GradientStops[0].color;
            D2D1_COLOR_F Color2 = _Configuration._GradientStops[1].color;

            float i =  0.f;
            float n = (_Configuration._GradientStops[1].position - _Configuration._GradientStops[0].position) * (float) _FrequencyBands.size();

            for (FrequencyBand & Iter : _FrequencyBands)
            {
                Iter.ForeColor = D2D1::ColorF(Color1.r + ((Color2.r - Color1.r) * i / n), Color1.g + ((Color2.g - Color1.g) * i / n), Color1.b + ((Color2.b - Color1.b) * i / n));
                i++;

                if (i >= n)
                {
                    j++;

                    if (j == _Configuration._GradientStops.size() - 1)
                        break;

                    Color1 = _Configuration._GradientStops[j].color;
                    Color2 = _Configuration._GradientStops[j + 1].color;

                    i = 0.f;
                    n = (_Configuration._GradientStops[j + 1].position - _Configuration._GradientStops[j].position) * (float) _FrequencyBands.size();;
                }
            }
        }
        else
        {
            for (FrequencyBand & Iter : _FrequencyBands)
                Iter.ForeColor = _Configuration._GradientStops[0].color;
        }
    }

    _XAxis.Initialize(&_Configuration, _FrequencyBands);

    _YAxis.Initialize(&_Configuration);

    _Spectrum.Initialize(&_Configuration);

    _ToolTipControl.Activate(_Configuration._ShowToolTips);

    // Forces the recreation of the window function.
    if (_WindowFunction != nullptr)
    {
        delete _WindowFunction;
        _WindowFunction = nullptr;
    }

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

    ::LeaveCriticalSection(&_Lock);

//  InvalidateRect(NULL);
}

/// <summary>
/// Resizes all render targets.
/// </summary>
void UIElement::Resize()
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
            if (_TrackingToolInfo != nullptr)
            {
                _ToolTipControl.DelTool(_TrackingToolInfo);

                delete _TrackingToolInfo;
            }

            _TrackingToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, nullptr);

            ::SetRect(&_TrackingToolInfo->rect, (int) Rect.left, (int) Rect.top, (int) Rect.right, (int) Rect.bottom);

            _ToolTipControl.AddTool(_TrackingToolInfo);
        }
    }
}

/// <summary>
/// Writes a message to the console
/// </summary>
void UIElement::Log(LogLevel logLevel, const char * format, ...) const noexcept
{
    if (logLevel < _Configuration._LogLevel)
        return;

    va_list va;

    va_start(va, format);

    console::printfv(format, va);

    va_end(va);
}

/// <summary>
/// Handles a timer tick.
/// </summary>
void CALLBACK UIElement::TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept
{
    ((UIElement *) context)->RenderFrame();
}

#pragma endregion

#pragma region Rendering

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT UIElement::RenderFrame()
{
    if (!TryEnterCriticalSection(&_Lock))
        return E_ABORT;

    _FrameCounter.NewFrame();

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && !(_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        {
            _RenderTarget->Clear(_Configuration._UseCustomBackColor ? _Configuration._BackColor : _Configuration._DefBackColor);

            _XAxis.Render(_RenderTarget);

            _YAxis.Render(_RenderTarget);

            // Draw the spectrum.
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

            if (_Configuration._ShowFrameCounter)
                _FrameCounter.Render(_RenderTarget);
        }

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }

    ::LeaveCriticalSection(&_Lock);

    return hr;
}

/// <summary>
/// Renders an audio chunk.
/// </summary>
HRESULT UIElement::RenderChunk(const audio_chunk & chunk)
{
    HRESULT hr = S_OK;

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    _SampleRate = chunk.get_sample_rate();

    Log(LogLevel::Trace, "%s: Rendering chunk { ChannelCount: %d, ChannelSetup: 0x%08X, SampleRate: %d }.", core_api::get_my_file_name(), ChannelCount, ChannelSetup, _SampleRate);

    // Create the transformer when necessary.
    {
        if (_WindowFunction == nullptr)
            _WindowFunction = WindowFunction::Create(_Configuration._WindowFunction, _Configuration._WindowParameter, _Configuration._WindowSkew, _Configuration._Truncate);

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

            _SpectrumAnalyzer = new SpectrumAnalyzer(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, _FFTSize, &_Configuration);

            _FrequencyCoefficients.resize(_FFTSize);
        }

        if (_CQT == nullptr)
            _CQT = new CQTProvider(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, 1.0, 1.0, 0.0);
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
void UIElement::GenerateLinearFrequencyBands()
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

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void UIElement::GenerateOctaveFrequencyBands()
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

        int n = (int) i % 12;

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], (int) i / 12, fb.Ctr);

        fb.BackColor = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10) ? _Configuration._DarkBandColor : _Configuration._LiteBandColor;

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands of AveePlayer.
/// </summary>
void UIElement::GenerateAveePlayerFrequencyBands()
{
    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i - _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Ctr = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i,              _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Hi  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i + _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double UIElement::ScaleF(double x, ScalingFunction function, double skewFactor)
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
double UIElement::DeScaleF(double x, ScalingFunction function, double skewFactor)
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
void UIElement::ApplyAverageSmoothing(double factor)
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
void UIElement::ApplyPeakSmoothing(double factor)
{
    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue = Max(::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0, ::isfinite(Iter.NewValue) ? Iter.NewValue : 0.0);
}

#pragma endregion

#pragma region DirectX
/// <summary>
/// Create resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT UIElement::CreateDeviceIndependentResources()
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
HRESULT UIElement::CreateDeviceSpecificResources()
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
void UIElement::ReleaseDeviceSpecificResources()
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();
    _FrameCounter.ReleaseDeviceSpecificResources();

    _RenderTarget.Release();
}
#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void UIElement::on_playback_new_track(metadb_handle_ptr track)
{
    SetConfiguration();

    Invalidate();
}

/// <summary>
/// Playback stopped.
/// </summary>
void UIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    Invalidate();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void UIElement::on_playback_pause(bool)
{
    Invalidate();
}

#pragma endregion
