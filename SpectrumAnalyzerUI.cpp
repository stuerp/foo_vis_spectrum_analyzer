
/** $VER: SpectrumAnalyzerUI.cpp (2023.11.19) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include <complex>

#include "Configuration.h"
#include "Support.h"
#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#pragma hdrstop

static bool _IsPlaying = false;

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
    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
        Log(LogLevel::Critical, "%s: Unable to create Direct2D device independent resources: 0x%08X", core_api::get_my_file_name(), hr);

    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, visualisation_manager::KStreamFlagNewFFT);

        _VisualisationStream->request_backlog(0.0); // FIXME: What does this do?
    }
    catch (std::exception & ex)
    {
        Log(LogLevel::Critical, "%s: Unable to create visualisation stream. %s.", core_api::get_my_file_name(), ex.what());
    }

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

    _ClientSize = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    Resize();
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

/// <summary>
/// Handles a configuration change.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SetConfiguration();

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
        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) m_hWnd) == NULL)
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
    // Initialize the bands.
    switch (_Configuration._FrequencyDistribution)
    {
        default:

        case FrequencyDistribution::Frequencies:
            GenerateFrequencyBands();
            break;

        case FrequencyDistribution::Octaves:
            GenerateFrequencyBandsFromNotes();
            break;

        case FrequencyDistribution::AveePlayer:
            GenerateFrequencyBandsOfAveePlayer();
            break;
    }

    _XAxis.Initialize(_YAxis.GetWidth(), 0.f, (FLOAT) _ClientSize.width, (FLOAT) _ClientSize.height, _FrequencyBands, _Configuration._XAxisMode);

    _YAxis.Initialize(0.f, 0.f, (FLOAT) _ClientSize.width, (FLOAT) _ClientSize.height - _XAxis.GetHeight());

    // Forces the recreation of the brush.
    _BandForegroundBrush.Release();

    // Forces the recreation of the spectrum analyzer.
    if (_SpectrumAnalyzer != nullptr)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    InvalidateRect(NULL);
}

/// <summary>
/// Resizes all canvases.
/// </summary>
void SpectrumAnalyzerUIElement::Resize()
{
    if (_RenderTarget)
        _RenderTarget->Resize(_ClientSize);

    _FrameCounter.Initialize((FLOAT) _ClientSize.width, (FLOAT) _ClientSize.height);

    _XAxis.Initialize(_YAxis.GetWidth(), 0.f, (FLOAT) _ClientSize.width, (FLOAT) _ClientSize.height, _FrequencyBands, _Configuration._XAxisMode);

    _YAxis.Initialize(0.f, 0.f, (FLOAT) _ClientSize.width, (FLOAT) _ClientSize.height - _XAxis.GetHeight());
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

    if (SUCCEEDED(hr))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        // Draw the background, x-axis and y-axis.
        _RenderTarget->Clear(_Configuration._BackgroundColor);

        _XAxis.Render(_RenderTarget);

        _YAxis.Render(_RenderTarget);

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

                    if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - WindowDuration / 2., WindowDuration * (_Configuration._UseZeroTrigger ? 2. : 1.)))
                        RenderChunk(Chunk);
                }
            }
            else
                Log(LogLevel::Trace, "%s: Visualisation stream is invalid.", core_api::get_my_file_name());

            if (_Configuration._LogLevel != LogLevel::None)
                _FrameCounter.Render(_RenderTarget);
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
/// Renders an audio chunk.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderChunk(const audio_chunk & chunk)
{
    HRESULT hr = S_OK;

    uint32_t ChannelCount = chunk.get_channel_count();

    _SampleRate = chunk.get_sample_rate();

    Log(LogLevel::Trace, "%s: Rendering chunk { ChannelCount: %d, SampleRate: %d }.", core_api::get_my_file_name(), ChannelCount, _SampleRate);

    // Create the spectrum analyzer if necessary.
    if (_SpectrumAnalyzer == nullptr)
    {
        #pragma warning (disable: 4061)
        switch (_Configuration._Transform)
        {
            default:
                _FFTSize = (size_t) (64. * ::exp2((long) _Configuration._Transform));
                break;

            case Transform::FFTCustom:
                _FFTSize = (_Configuration._FFTCustom > 0.) ? (size_t) _Configuration._FFTCustom : 64;
                break;

            case Transform::FFTDuration:
                _FFTSize = (_Configuration._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _Configuration._FFTDuration) / 1000.) : 64;
                break;
        }
        #pragma warning (default: 4061)

        _SpectrumAnalyzer = new SpectrumAnalyzer(ChannelCount, _FFTSize, _SampleRate);
    }

    // Add the samples to the spectrum analyzer.
    {
        const audio_sample * Samples = chunk.get_data();

        if (Samples == nullptr)
            return E_FAIL;

        size_t SampleCount = chunk.get_sample_count();

        _SpectrumAnalyzer->Add(Samples, SampleCount);
    }

    {
        // Get the frequency coefficients.
        std::vector<std::complex<double>> FrequencyCoefficients(_FFTSize, 0.0); // FIXME: Don't reallocate every time.

        _SpectrumAnalyzer->GetFrequencyCoefficients(FrequencyCoefficients);

        // Get the spectrum from the frequency coefficients.
        _SpectrumAnalyzer->GetSpectrum(FrequencyCoefficients, _FrequencyBands, _SampleRate, _Configuration._SummationMethod);

        // Smooth the spectrum.
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

        // Update the peak indicators.
        if (_Configuration._PeakMode != PeakMode::None)
            _SpectrumAnalyzer->UpdatePeakIndicators(_FrequencyBands);
    }

    hr = RenderSpectrum();

    return hr;
}

/// <summary>
/// Renders the spectrum.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderSpectrum()
{
    Log(LogLevel::Trace, "%s: Rendering %d bands.", core_api::get_my_file_name(), _FrequencyBands.size());

    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    const FLOAT Width = (FLOAT) _ClientSize.width - _YAxis.GetWidth();

    const FLOAT BandWidth = Max((Width / (FLOAT) _FrequencyBands.size()), 1.f);

    FLOAT x1 = _YAxis.GetWidth() + (Width - ((FLOAT) _FrequencyBands.size() * BandWidth)) / 2.f;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = PaddingY;
    const FLOAT y2 = (FLOAT) _ClientSize.height - _XAxis.GetHeight();

    for (const FrequencyBand & Iter : _FrequencyBands)
    {
        if (Iter.Hi > ((double) _SampleRate / 2.)) // Don't render anything above the Nyquist frequency.
            break;

        D2D1_RECT_F Rect = { x1, y1, x2 - PaddingX, y2 - PaddingY };

        // Draw the background.
        if (_Configuration._DrawBandBackground)
            _RenderTarget->FillRectangle(Rect, _BandBackgroundBrush);

        // Draw the foreground.
        if (Iter.CurValue > 0.0)
        {
            Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * ScaleA(Iter.CurValue))), y1, Rect.bottom);

            _RenderTarget->FillRectangle(Rect, _BandForegroundBrush);
        }

        // Draw the peak indicator.
        if ((_Configuration._PeakMode != PeakMode::None) && (Iter.Peak > 0.))
        {
            Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * Iter.Peak)), y1, Rect.bottom);
            Rect.bottom = Rect.top + 1.f;

            ID2D1Brush * Brush = (_Configuration._PeakMode != PeakMode::FadeOut) ? (ID2D1Brush *) _BandForegroundBrush : (ID2D1Brush *) _WhiteBrush;

            if (_Configuration._PeakMode == PeakMode::FadeOut)
                Brush->SetOpacity((FLOAT) Iter.Opacity);

            _RenderTarget->FillRectangle(Rect, Brush);

            if (_Configuration._PeakMode == PeakMode::FadeOut)
                Brush->SetOpacity(1.f);
        }

        x1  = x2;
        x2 += BandWidth;
    }

    return S_OK;
}

/// <summary>
/// Generates frequency bands.
/// </summary>
void SpectrumAnalyzerUIElement::GenerateFrequencyBands()
{
    const double MinFreq = ScaleF(_Configuration._MinFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);
    const double MaxFreq = ScaleF(_Configuration._MaxFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);

    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = DeScaleF(Map((double) i - _Configuration._Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Ctr = DeScaleF(Map((double) i,                             0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Hi  = DeScaleF(Map((double) i + _Configuration._Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
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

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - _Configuration._Bandwidth) * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24,  (i                              * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24, ((i + _Configuration._Bandwidth) * NotesGroup + _Configuration._Transpose)),
        };

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

        Iter.Lo      = LogSpace(_Configuration._MinFrequency, _Configuration._MaxFrequency, (double) i - _Configuration._Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Ctr     = LogSpace(_Configuration._MinFrequency, _Configuration._MaxFrequency, (double) i,                             _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Hi      = LogSpace(_Configuration._MinFrequency, _Configuration._MaxFrequency, (double) i + _Configuration._Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.LoBound = LogSpace(_Configuration._MinFrequency, _Configuration._MaxFrequency, (double) i - 0.5,                       _Configuration._NumBands - 1, _Configuration._SkewFactor); // FIXME: Why 0.5 and not 64.0 / 2?
        Iter.HiBound = LogSpace(_Configuration._MinFrequency, _Configuration._MaxFrequency, (double) i + 0.5,                       _Configuration._NumBands - 1, _Configuration._SkewFactor); // FIXME: Why 0.5 and not 64.0 / 2?
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
/// Applies a time smoothing factor.
/// </summary>
void SpectrumAnalyzerUIElement::ApplyAverageSmoothing(double factor)
{
    if (factor != 0.0)
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = (!::isnan(Iter.CurValue) ? Iter.CurValue * factor : 0.0) + (!::isnan(Iter.NewValue) ? Iter.NewValue * (1.0 - factor) : 0.0);
    }
    else
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = Iter.NewValue;
    }
}

/// <summary>
/// Calculates the peak decay.
/// </summary>
void SpectrumAnalyzerUIElement::ApplyPeakSmoothing(double factor)
{
    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue = Max(!::isnan(Iter.CurValue) ? Iter.CurValue * factor : 0.0, !::isnan(Iter.NewValue) ? Iter.NewValue : 0.0);
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

    hr = _FrameCounter.CreateDeviceIndependentResources(_DirectWriteFactory);

    hr = _XAxis.CreateDeviceIndependentResources(_DirectWriteFactory);

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
        CRect ClientRect;

        GetClientRect(ClientRect);

        _ClientSize = D2D1::SizeU((UINT32) ClientRect.Width(), (UINT32) ClientRect.Height());

        Resize();

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, _ClientSize);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);
    }

    // Create the brushes.
    if (SUCCEEDED(hr) && (_WhiteBrush == nullptr))
    {
        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_WhiteBrush);
    }

    if (SUCCEEDED(hr) && (_BandBackgroundBrush == nullptr))
    {
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
/*
        t_ui_color BackColor = m_callback->query_std_color(ui_color_background);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(BackColor) / 255.0f, GetGValue(BackColor) / 255.0f, GetBValue(BackColor) / 255.0f), &_BackBrush);

        t_ui_color TextColor = m_callback->query_std_color(ui_color_text);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(TextColor) / 255.0f, GetGValue(TextColor) / 255.0f, GetBValue(TextColor) / 255.0f), &_TextBrush);
*/
        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(.2f, .2f, .2f, .7f), &_BandBackgroundBrush); // #1E90FF, 0.3f
    }

    if (SUCCEEDED(hr) && (_BandForegroundBrush == nullptr))
    {
        CComPtr<ID2D1GradientStopCollection> Collection = GetGradientStopCollection();

        if (SUCCEEDED(hr))
        {
            D2D1_SIZE_F Size = _RenderTarget->GetSize();

            hr = _RenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_BandForegroundBrush);
        }
    }

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr))
        hr = _XAxis.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr))
        hr = _YAxis.CreateDeviceSpecificResources(_RenderTarget);

    return hr;
}

/// <summary>
/// Gets a collection of colors to create a gradient brush with.
/// </summary>
CComPtr<ID2D1GradientStopCollection> SpectrumAnalyzerUIElement::GetGradientStopCollection() const
{
    const D2D1_GRADIENT_STOP GradientStopsSolid[] =
    {
        { 1.f, D2D1::ColorF(0x1E90FF, 1.f) },
    };

    // Prism / foo_musical_spectrum
    const D2D1_GRADIENT_STOP GradientStopsPrism1[] =
    {
        { 0.f / 5.f, D2D1::ColorF(0xFD0000, 1.f) },
        { 1.f / 5.f, D2D1::ColorF(0xFF8000, 1.f) },
        { 2.f / 5.f, D2D1::ColorF(0xFFFF01, 1.f) },
        { 3.f / 5.f, D2D1::ColorF(0x7EFF77, 1.f) },
        { 4.f / 5.f, D2D1::ColorF(0x0193A2, 1.f) },
        { 5.f / 5.f, D2D1::ColorF(0x002161, 1.f) },
    };

    // Prism 2
    const D2D1_GRADIENT_STOP GradientStopsPrism2[] =
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

    // Prism 3
    const D2D1_GRADIENT_STOP GradientStopsPrism3[] =
    {
        { 0.f / 4.f, D2D1::ColorF(0xFF0000, 1.f) }, // hsl(  0, 100%, 50%)
        { 1.f / 4.f, D2D1::ColorF(0xFFFF00, 1.f) }, // hsl( 60, 100%, 50%)
        { 2.f / 4.f, D2D1::ColorF(0x00FF00, 1.f) }, // hsl(120, 100%, 50%)
        { 3.f / 4.f, D2D1::ColorF(0x00FFFF, 1.f) }, // hsl(180, 100%, 50%)
        { 4.f / 4.f, D2D1::ColorF(0x0000FF, 1.f) }, // hsl(240, 100%, 50%)
    };

    // foobar2000
    const D2D1_GRADIENT_STOP GradientStopsFB2K[] =
    {
        { 0.f / 1.f, D2D1::ColorF(0x0066CC, 1.f) }, 
        { 1.f / 1.f, D2D1::ColorF(0x000000, 1.f) },
    };

    // foobar2000 Dark Mode
    const D2D1_GRADIENT_STOP GradientStopsFB2KDarkMode[] =
    {
        { 0.f / 1.f, D2D1::ColorF(0x0080FF, 1.f) },
        { 1.f / 1.f, D2D1::ColorF(0xFFFFFF, 1.f) },
    };

    const D2D1_GRADIENT_STOP * GradientStops = nullptr;
    UINT32 GradientStopCount = 0;

    switch (_Configuration._ColorScheme)
    {
        default:

        case ColorScheme::Solid:
        {
            GradientStops = GradientStopsSolid;
            GradientStopCount = _countof(GradientStopsSolid);
            break;
        }

        case ColorScheme::Custom:
        {
            // FIXME
            GradientStops = GradientStopsSolid;
            GradientStopCount = _countof(GradientStopsSolid);
            break;
        }

        case ColorScheme::Prism1:
        {
            GradientStops = GradientStopsPrism1;
            GradientStopCount = _countof(GradientStopsPrism1);
            break;
        }

        case ColorScheme::Prism2:
        {
            GradientStops = GradientStopsPrism2;
            GradientStopCount = _countof(GradientStopsPrism2);
            break;
        }

        case ColorScheme::Prism3:
        {
            GradientStops = GradientStopsPrism3;
            GradientStopCount = _countof(GradientStopsPrism3);
            break;
        }

        case ColorScheme::foobar2000:
        {
            GradientStops = GradientStopsFB2K;
            GradientStopCount = _countof(GradientStopsFB2K);
            break;
        }

        case ColorScheme::foobar2000DarkMode:
        {
            GradientStops = GradientStopsFB2KDarkMode;
            GradientStopCount = _countof(GradientStopsFB2KDarkMode);
            break;
        }
    };

    CComPtr<ID2D1GradientStopCollection> Collection;

    _RenderTarget->CreateGradientStopCollection(GradientStops, GradientStopCount, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    return Collection;
}

/// <summary>
/// Release the device specific resources.
/// </summary>
void SpectrumAnalyzerUIElement::ReleaseDeviceSpecificResources()
{
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();
    _FrameCounter.ReleaseDeviceSpecificResources();

    _BandForegroundBrush.Release();
    _BandBackgroundBrush.Release();

    _WhiteBrush.Release();

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
/// Used by host to notify the element about various events. See ui_element_notify_* GUIDs for possible p_what parameter; meaning of other parameters depends on p_what value. Container classes should dispatch all notifications to their children.
/// </summary>
void SpectrumAnalyzerUIElement::notify(const GUID & what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
    if (what == ui_element_notify_colors_changed)
    {
        _BandBackgroundBrush.Release();

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
    SetConfiguration();

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
