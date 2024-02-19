
/** $VER: UIElement.cpp (2024.02.17) P. Stuer **/

#include "UIElement.h"

#include "DirectX.h"
#include "StyleManager.h"

#include "ToneGenerator.h"
#include "Support.h"
#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
UIElement::UIElement(): _ThreadPoolTimer(), _TrackingGraph(), _TrackingToolInfo(), _LastMousePos(), _LastIndex(~0U), _SampleRate(44100), _DPI()
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

//      _VisualisationStream->request_backlog(0.8); // FIXME: What does this do?
        _VisualisationStream->set_channel_mode(visualisation_stream_v2::channel_mode_default);

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

    // Apply the initial configuration.
    UpdateState();

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void UIElement::OnDestroy()
{
    StopTimer();

    _CriticalSection.Enter();

    {
        for (auto & Iter : _Grid)
            delete Iter._Graph;

        _Grid.clear();
    }

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
/// Handles the WM_PAINT message.
/// </summary>
void UIElement::OnPaint(CDCHandle hDC)
{
//  Log::Write(Log::Level::Trace, "%08X: OnPaint", GetTickCount64());

    StartTimer();

    ValidateRect(nullptr); // Prevent any further WM_PAINT messages.
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
        Menu.AppendMenu((UINT) MF_STRING | (_State._ShowFrameCounter ? MF_CHECKED : 0), IDM_TOGGLE_FRAME_COUNTER, L"Frame Counter");

        {
            RefreshRateLimitMenu.CreatePopupMenu();

            const size_t RefreshRates[] = { 20, 30, 60, 100, 200 };

            for (size_t i = 0; i < _countof(RefreshRates); ++i)
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_State._RefreshRateLimit ==  RefreshRates[i]) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20 + i,
                    pfc::wideFromUTF8(pfc::format(RefreshRates[i], L"Hz")));

            Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, L"Refresh Rate Limit");
        }

        Menu.AppendMenu((UINT) MF_SEPARATOR);
        Menu.AppendMenu((UINT) MF_STRING | (_IsFrozen ? MF_CHECKED : 0), IDM_FREEZE, L"Frozen");

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
            _State._RefreshRateLimit = 20;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _State._RefreshRateLimit = 30;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_60:
            _State._RefreshRateLimit = 60;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_100:
            _State._RefreshRateLimit = 100;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_200:
            _State._RefreshRateLimit = 200;
            StartTimer();
            break;

        case IDM_CONFIGURE:
            Configure();
            break;

        case IDM_FREEZE:
            _IsFrozen = !_IsFrozen;
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

    if (_TrackingGraph == nullptr)
    {
        _TrackingGraph = GetGraph(pt);

        if (_TrackingGraph == nullptr)
            return;

        // Tell Windows we want to know when the mouse leaves this window.
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };

            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
        
            ::TrackMouseEvent(&tme);
        }

        _LastMousePos = POINT(-1, -1);
        _LastIndex = ~0U;

        _TrackingToolInfo = _TrackingGraph->GetToolInfo(m_hWnd);

        if (_TrackingToolInfo != nullptr)
            _ToolTipControl.TrackActivate(_TrackingToolInfo, TRUE);
    }
    else
    {
        if (_TrackingGraph && (pt != _LastMousePos))
        {
            _LastMousePos = pt;

            FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

            std::wstring ToolTip;
            size_t Index;

            if (_TrackingGraph->GetToolTip(ScaledX, ToolTip, Index))
            {
                if (Index != _LastIndex)
                {
                    _TrackingToolInfo->lpszText = (LPWSTR) ToolTip.c_str();

                    _ToolTipControl.UpdateTipText(_TrackingToolInfo);

                    _LastIndex = Index;
                }

                // Reposition the tooltip.
                ::ClientToScreen(m_hWnd, &pt);

                _ToolTipControl.TrackPosition(pt.x + 10, pt.y - 35);
            }
            else
            {
                _ToolTipControl.TrackActivate(_TrackingToolInfo, FALSE);

                delete _TrackingToolInfo;
                _TrackingToolInfo = nullptr;

                _TrackingGraph = nullptr;
            }
        }
    }
}

/// <summary>
/// Turns off the tracking tooltip when the mouse leaves the window.
/// </summary>
void UIElement::OnMouseLeave()
{
    _ToolTipControl.TrackActivate(_TrackingToolInfo, FALSE);

    delete _TrackingToolInfo;
    _TrackingToolInfo = nullptr;

    _TrackingGraph = nullptr;
}

/// <summary>
/// Handles the WM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT UIElement::OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UpdateState();

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
    _State._ShowFrameCounter = !_State._ShowFrameCounter;
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void UIElement::ToggleHardwareRendering() noexcept
{
    _State._UseHardwareRendering = !_State._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Shows the configuration dialog.
/// </summary>
void UIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        DialogParameters dp = { m_hWnd, &_State };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Updates the state.
/// </summary>
void UIElement::UpdateState() noexcept
{
//  Log::Write(Log::Level::Trace, "%08X: UpdateState", GetTickCount64());

    _CriticalSection.Enter();

    #pragma warning (disable: 4061)
    switch (_State._FFTMode)
    {
        default:
            _State._BinCount = (size_t) (64. * ::exp2((long) _State._FFTMode));
            break;

        case FFTMode::FFTCustom:
            _State._BinCount = (_State._FFTCustom > 0) ? (size_t) _State._FFTCustom : 64;
            break;

        case FFTMode::FFTDuration:
            _State._BinCount = (_State._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _State._FFTDuration) / 1000.) : 64;
            break;
    }
    #pragma warning (default: 4061)

    _ToneGenerator.Initialize(440., 1., 0., _State._BinCount);

    _RenderState = _State;

    _RenderState._StyleManager.ReleaseDeviceSpecificResources();

    // Create the graphs.
    {
        for (auto & Iter : _Grid)
            delete Iter._Graph;

        _Grid.clear();

        _Grid.Initialize(_RenderState._GridRowCount, _RenderState._GridColumnCount);

        for (const auto & Iter : _State._GraphSettings)
        {
            auto * g = new Graph();

            g->Initialize(&_RenderState, Iter);

            _Grid.push_back({ g, Iter._HRatio, Iter._VRatio });
        }
    }

    _CriticalSection.Leave();

    _NewArtworkGradient = true; // Request an update of the artwork gradient.

    _ToolTipControl.Activate(_RenderState._ShowToolTips);

    Resize();
}

/// <summary>
/// Gets the graph that contains the specified point.
/// </summary>
Graph * UIElement::GetGraph(const CPoint & pt) noexcept
{
    for (auto & Iter : _Grid)
    {
        if (Iter._Graph->ContainsPoint(pt))
            return Iter._Graph;
    }

    return nullptr;
}

#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void UIElement::on_playback_new_track(metadb_handle_ptr track)
{
    _PlaybackEvent = PlaybackEvent::NewTrack;

    UpdateState();

    // Get the sample rate from the track because the spectrum analyzer requires it. The next opportunity is to get it from the audio chunk but that is too late.
    // Also, set the sample rate after the FFT size to prevent the render thread from getting wrong results.
    _SampleRate = (uint32_t) track->get_info_ref()->info().info_get_int("samplerate");

    if (_SampleRate == 0)
        _SampleRate = 44100;

    // Use the script from the configuration to load the album art.
    if (track.is_valid() && !_State._ArtworkFilePath.isEmpty())
    {
        titleformat_object::ptr Script;

        bool Success = titleformat_compiler::get()->compile(Script, _State._ArtworkFilePath.c_str());

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
    if (!_State._ArtworkFilePath.isEmpty())
        return;

    _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
    _NewArtwork = true;
}

#pragma endregion
