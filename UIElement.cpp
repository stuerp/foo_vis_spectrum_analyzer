
/** $VER: UIElement.cpp (2024.02.12) P. Stuer **/

#include "UIElement.h"

#include "DirectX.h"
#include "StyleManager.h"

#include "Support.h"
#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
UIElement::UIElement(): _ThreadPoolTimer(), _DPI(), _TrackingToolInfo(), _IsTracking(false), _LastMousePos(), _LastIndex(~0U), _WindowFunction(), _BrownPucketteKernel(), _FFTAnalyzer(), _CQTAnalyzer(), _NumBins(), _SampleRate(44100), _Bandwidth()
{
}

#pragma region User Interface

/// <summary>
/// Gets the window class definition.
/// </summary>
CWndClassInfo & UIElement::GetWndClassInfo()
{
    static ATL::CWndClassInfoW wci =
    {
        {
            sizeof(WNDCLASSEX),
            CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
            StartWindowProc,
            0, 0,
            NULL, // Instance,
            NULL, // Icon
            NULL, // Cursor
            NULL, // Background brush
            NULL, // Menu
            TEXT(STR_WINDOW_CLASS_NAME), // Class name
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
    DirectX::Initialize();

    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
    {
        Log::Write(Log::Level::Critical, "%s: Unable to create DirectX device independent resources: 0x%08X", core_api::get_my_file_name(), hr);

        return -1;
    }

    (void) GetDPI(m_hWnd, _DPI);

    // Create the visualisation stream.
    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, visualisation_manager::KStreamFlagNewFFT);

        _VisualisationStream->request_backlog(0.8); // FIXME: What does this do?
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Critical, "%s: Unable to create visualisation stream. %s.", core_api::get_my_file_name(), ex.what());

        return -1;
    }

    // Register ourselves with the album art notification manager.
    {
        auto AlbumArtNotificationManager = now_playing_album_art_notify_manager::tryGet();

        if (AlbumArtNotificationManager.is_valid())
            AlbumArtNotificationManager->add(this);
    }

    // Create the tooltip control.
    {
        _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

        _ToolTipControl.SetMaxTipWidth(100);
    }

    // Create the timer.
    CreateTimer();

    // Apply the initial configuration.
    SetConfiguration();

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void UIElement::OnDestroy()
{
    StopTimer();

    _CriticalSection.Enter();

    if (_ThreadPoolTimer)
    {
        ::CloseThreadpoolTimer(_ThreadPoolTimer);
        _ThreadPoolTimer = nullptr;
    }

    DeleteResources();

    {
        auto Manager = now_playing_album_art_notify_manager::tryGet();

        if (Manager.is_valid())
            Manager->remove(this);
    }

    _VisualisationStream.release();

    ReleaseDeviceSpecificResources();

    ReleaseDeviceIndependentResources();

    _CriticalSection.Leave();

    DirectX::Terminate();
}

/// <summary>
/// Handles the WM_ERASEBKGND message.
/// </summary>
LRESULT UIElement::OnEraseBackground(CDCHandle hDC)
{
    return 1;
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void UIElement::OnPaint(CDCHandle hDC)
{
    StartTimer();

    ValidateRect(nullptr);
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void UIElement::OnSize(UINT type, CSize size)
{
    if (_RenderTarget == nullptr)
        return;

    _CriticalSection.Enter();

    D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    _RenderTarget->Resize(Size);

    Resize();

    _CriticalSection.Leave();
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
        Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, !_IsFullScreen ? L"Full-Screen Mode" : L"Exit Full-Screen Mode");
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
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _Configuration._RefreshRateLimit = 30;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_60:
            _Configuration._RefreshRateLimit = 60;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_100:
            _Configuration._RefreshRateLimit = 100;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_200:
            _Configuration._RefreshRateLimit = 200;
            StartTimer();
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
        if (pt.x < (LONG) _Graph.GetSpectrum().GetLeft())
            return;

        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };

            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
        
            ::TrackMouseEvent(&tme);
        }

        _LastMousePos = POINT(-1, -1);
        _LastIndex = ~0U;

        _ToolTipControl.TrackActivate(_TrackingToolInfo, TRUE);
        _IsTracking = true;
    }
    else
    {
        if (_TrackingToolInfo && ((pt.x != _LastMousePos.x) || (pt.y != _LastMousePos.y)))
        {
            _LastMousePos = pt;
    
            FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

            size_t Index = (size_t) ::floor(Map(ScaledX, _Graph.GetSpectrum().GetLeft(), _Graph.GetSpectrum().GetRight(), 0., (double) _FrequencyBands.size()));

            if (Index != _LastIndex)
            {
                if (InRange(Index, (size_t) 0U, _FrequencyBands.size() - 1))
                    _TrackingToolInfo->lpszText = _FrequencyBands[Index].Label;

                _ToolTipControl.UpdateTipText(_TrackingToolInfo);

                _LastIndex = Index;
            }

            // Reposition the tooltip.
            ::ClientToScreen(m_hWnd, &pt);

            _ToolTipControl.TrackPosition(pt.x + 10, pt.y - 35);
        }
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
/// Handles the WM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT UIElement::OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
/// Shows the configuration dialog.
/// </summary>
void UIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        _CriticalSection.Enter();

        DialogParameters dp = { m_hWnd, &_Configuration };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);

        _CriticalSection.Leave();
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Sets the current configuration.
/// </summary>
void UIElement::SetConfiguration() noexcept
{
    _CriticalSection.Enter();

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

    #pragma warning (disable: 4061)
    switch (_Configuration._FFTMode)
    {
        default:
            _NumBins = (size_t) (64. * ::exp2((long) _Configuration._FFTMode));
            break;

        case FFTMode::FFTCustom:
            _NumBins = (_Configuration._FFTCustom > 0) ? (size_t) _Configuration._FFTCustom : 64;
            break;

        case FFTMode::FFTDuration:
            _NumBins = (_Configuration._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _Configuration._FFTDuration) / 1000.) : 64;
            break;
    }
    #pragma warning (default: 4061)

    _Bandwidth = (((_Configuration._Transform == Transform::FFT) && (_Configuration._MappingMethod == Mapping::TriangularFilterBank)) || (_Configuration._Transform == Transform::CQT)) ? _Configuration._Bandwidth : 0.5;

    DeleteResources();

    _Configuration._StyleManager.ReleaseDeviceSpecificResources();

    _NewArtworkGradient = true; // Request an update of the artwork gradient.

    _Graph.Initialize(&_Configuration, _FrequencyBands);

    _ToolTipControl.Activate(_Configuration._ShowToolTips);

    Resize();

    _CriticalSection.Leave();
}

/// <summary>
/// Deletes some analysis resources.
/// </summary>
void UIElement::DeleteResources()
{
    // Forces the recreation of the Brown-Puckette window function.
    if (_BrownPucketteKernel != nullptr)
    {
        delete _BrownPucketteKernel;
        _BrownPucketteKernel = nullptr;
    }

    // Forces the recreation of the window function.
    if (_WindowFunction != nullptr)
    {
        delete _WindowFunction;
        _WindowFunction = nullptr;
    }

    // Forces the recreation of the spectrum analyzer.
    if (_FFTAnalyzer != nullptr)
    {
        delete _FFTAnalyzer;
        _FFTAnalyzer = nullptr;
    }

    // Forces the recreation of the Constant-Q transform.
    if (_CQTAnalyzer != nullptr)
    {
        delete _CQTAnalyzer;
        _CQTAnalyzer = nullptr;
    }

    // Forces the recreation of the Sliding Window Infinite Fourier transform.
    if (_SWIFTAnalyzer != nullptr)
    {
        delete _SWIFTAnalyzer;
        _SWIFTAnalyzer = nullptr;
    }
}

/// <summary>
/// Resizes all render targets.
/// </summary>
void UIElement::Resize()
{
    if (_RenderTarget == nullptr)
        return;

    D2D1_SIZE_F Size = _RenderTarget->GetSize();

    // Reposition the frame counter.
    _FrameCounter.Resize(Size.width, Size.height);

    // Resize the graph area.
    const D2D1_RECT_F Bounds(0.f, 0.f, Size.width, Size.height);

    _Graph.Move(Bounds);

    // Adjust the tracking tool tip.
    {
        if (_TrackingToolInfo != nullptr)
        {
            _ToolTipControl.DelTool(_TrackingToolInfo);

            delete _TrackingToolInfo;
        }

        _TrackingToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, nullptr);

        ::SetRect(&_TrackingToolInfo->rect, (int) Bounds.left, (int) Bounds.top, (int) Bounds.right, (int) Bounds.bottom);

        _ToolTipControl.AddTool(_TrackingToolInfo);
    }
}

#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void UIElement::on_playback_new_track(metadb_handle_ptr track)
{
    _PlaybackEvent = PlaybackEvent::NewTrack;

    SetConfiguration();

    // Get the sample rate from the track because the spectrum analyzer requires it. The next opportunity is to get it from the audio chunk but that is too late.
    // Also, set the sample rate after the FFT size to prevent the render thread from getting wrong results.
    _SampleRate = (uint32_t) track->get_info_ref()->info().info_get_int("samplerate");

    if (_SampleRate == 0)
        _SampleRate = 44100;

    // Use the script from the configuration to load the album art.
    if (track.is_valid() && !_Configuration._ArtworkFilePath.isEmpty())
    {
        titleformat_object::ptr Script;

        bool Success = titleformat_compiler::get()->compile(Script, _Configuration._ArtworkFilePath.c_str());

        pfc::string Result;

        if (Success && Script.is_valid() && track->format_title(0, Result, Script, 0))
        {
            _Artwork.Initialize(pfc::wideFromUTF8(Result).c_str());
            _NewArtwork = true;
        }
    }
}

/// <summary>
/// Playback stopped.
/// </summary>
void UIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    _PlaybackEvent = PlaybackEvent::Stop;

    _SampleRate = 44100;

    _Artwork.Release();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void UIElement::on_playback_pause(bool)
{
}

#pragma endregion

#pragma region now_playing_album_art_notify

void UIElement::on_album_art(album_art_data::ptr aad)
{
    // The script in the configuration takes precedence over the album art supplied by the track.
    if (!_Configuration._ArtworkFilePath.isEmpty())
        return;

    _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
    _NewArtwork = true;
}

#pragma endregion
