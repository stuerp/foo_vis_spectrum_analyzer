
/** $VER: UIElement.cpp (2025.09.17) P. Stuer **/

#include "pch.h"

#include "UIElement.h"

#include "DirectX.h"
#include "StyleManager.h"

#include "ToneGenerator.h"
#include "Support.h"
#include "Log.h"

#include "PresetManager.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
uielement_t::uielement_t(): _IsFullScreen(false), _IsVisible(true), _IsInitializing(true), _DPI(), _ThreadPoolTimer(), _TrackingGraph(), _TrackingToolInfo(), _LastMousePos(), _LastIndex(~0U)
{
}

#pragma region User Interface

/// <summary>
/// Gets the window class definition.
/// </summary>
CWndClassInfo & uielement_t::GetWndClassInfo()
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
LRESULT uielement_t::OnCreate(LPCREATESTRUCT cs)
{
    DirectX::Initialize();

    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
    {
        Log.AtFatal().Write(STR_COMPONENT_BASENAME " is unable to create DirectX device independent resources: 0x%08X", hr);

        return -1;
    }

    (void) GetDPI(m_hWnd, _DPI);

    // Create the visualisation stream.
    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, visualisation_manager::KStreamFlagNewFFT);

        if (_VisualisationStream.is_valid())
        {
        //  _VisualisationStream->request_backlog(0.8); // FIXME: What does this do?
            _VisualisationStream->set_channel_mode(visualisation_stream_v2::channel_mode_default);
        }
    }
    catch (std::exception & ex)
    {
        Log.AtFatal().Write(STR_COMPONENT_BASENAME " is unable to create visualisation stream: %s.", ex.what());

        return -1;
    }

    // Create the tooltip control.
    CreateToolTipControl();

    // Apply the initial configuration.
    UpdateState();

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void uielement_t::OnDestroy()
{
    StopTimer();

    _CriticalSection.Enter();

    {
        for (auto & Iter : _Grid)
            delete Iter._Graph;

        _Grid.clear();
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
void uielement_t::OnPaint(CDCHandle hDC)
{
//  Log::Write(Log::Level::Trace, "%8d: OnPaint", (uint32_t) ::GetTickCount64());
    StartTimer();

    ValidateRect(nullptr); // Prevent any further WM_PAINT messages.
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void uielement_t::OnSize(UINT type, CSize size)
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
void uielement_t::OnContextMenu(CWindow wnd, CPoint position)
{
    CMenu Menu;
    CMenu RefreshRateLimitMenu;
    CMenu PresetMenu;

    std::vector<std::wstring> PresetNames;

    {
        Menu.CreatePopupMenu();

        Menu.AppendMenu((UINT) MF_STRING, IDM_CONFIGURE, L"Configure");
        Menu.AppendMenu((UINT) MF_SEPARATOR);
        Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, L"Toggle Full-Screen Mode");
        Menu.AppendMenu((UINT) MF_STRING | (_MainState._ShowFrameCounter ? MF_CHECKED : 0), IDM_TOGGLE_FRAME_COUNTER, L"Frame Counter");

        {
            RefreshRateLimitMenu.CreatePopupMenu();

            const size_t RefreshRates[] = { 20, 30, 60, 100, 200 };

            for (size_t i = 0; i < _countof(RefreshRates); ++i)
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_MainState._RefreshRateLimit ==  RefreshRates[i]) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20 + i,
                    pfc::wideFromUTF8(pfc::format(RefreshRates[i], L"Hz")));

            Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, L"Refresh Rate Limit");
        }

        {
            PresetMenu.CreatePopupMenu();

            PresetManager::GetPresetNames(_MainState._PresetsDirectoryPath, PresetNames);

            UINT_PTR i = 0;

            for (auto & PresetName : PresetNames)
            {
                PresetMenu.AppendMenu((UINT) MF_STRING | ((_MainState._ActivePresetName == PresetName) ? MF_CHECKED : 0), IDM_PRESET_NAME + i, PresetName.c_str());
                i++;
            }

            Menu.AppendMenu((UINT) MF_STRING, PresetMenu, L"Presets");
        }

        Menu.AppendMenu((UINT) MF_SEPARATOR);
        Menu.AppendMenu((UINT) MF_STRING | (_IsFrozen ? MF_CHECKED : 0), IDM_FREEZE, (_IsFrozen ? L"Frozen" : L"Freeze"));

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
            _MainState._RefreshRateLimit = 20;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _MainState._RefreshRateLimit = 30;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_60:
            _MainState._RefreshRateLimit = 60;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_100:
            _MainState._RefreshRateLimit = 100;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_200:
            _MainState._RefreshRateLimit = 200;
            StartTimer();
            break;

        case IDM_CONFIGURE:
            Configure();
            break;

        case IDM_FREEZE:
            _IsFrozen = !_IsFrozen;
            break;

        default:
        {
            if (CommandId >= IDM_PRESET_NAME)
            {
                state_t NewState;

                size_t Index = (size_t) CommandId - IDM_PRESET_NAME;

                if (msc::InRange(Index, (size_t) 0, PresetNames.size() - (size_t) 1))
                {
                    PresetManager::Load(_MainState._PresetsDirectoryPath, PresetNames[Index], &NewState);

                    NewState._StyleManager._DominantColor       = _MainState._StyleManager._DominantColor;
                    NewState._StyleManager._UserInterfaceColors = _MainState._StyleManager._UserInterfaceColors;

                    NewState._StyleManager.UpdateCurrentColors();

                    NewState._ActivePresetName = PresetNames[Index];

                    _MainState = NewState;

                    UpdateState();

                    if (_ConfigurationDialog.IsWindow())
                    {
                        _ConfigurationDialog.PostMessageW(UM_CONFIGURATION_CHANGED, CC_PRESET_LOADED); // Must be sent outside the critical section.
                        Log.AtDebug().Write(STR_COMPONENT_BASENAME " notified configuration dialog of configuration change (Preset loaded).");
                    }
                }
            }
        }
    }

    Invalidate();
}

/// <summary>
/// Toggles between panel and full screen mode.
/// </summary>
void uielement_t::OnLButtonDblClk(UINT flags, CPoint point)
{
    ToggleFullScreen();
}

/// <summary>
/// Handles a DPI change.
/// </summary>
LRESULT uielement_t::OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect)
{
    _DPI = dpiX;

    ReleaseDeviceSpecificResources();

    return 0;
}

/// <summary>
/// Resizes all visual elements.
/// </summary>
void uielement_t::Resize()
{
    if (_RenderTarget == nullptr)
        return;

    DeleteTrackingToolTip();

    D2D1_SIZE_F SizeF = _RenderTarget->GetSize(); // Gets the size in DPIs.

    // Reposition the frame counter.
    _FrameCounter.Resize(SizeF.width, SizeF.height);

    // Resize the grid.
    {
        for (auto & Iter : _Grid)
        {
            TTTOOLINFOW ti;

            Iter._Graph->InitToolInfo(m_hWnd, ti);
            _ToolTipControl.DelTool(&ti);
        }

        _CriticalSection.Enter();

        {
            _Grid.Resize(SizeF.width, SizeF.height);

            _ThreadState._StyleManager.ReleaseGradientBrushes();
        }

        _CriticalSection.Leave();

        for (auto & Iter : _Grid)
        {
            TTTOOLINFOW ti;

            Iter._Graph->InitToolInfo(m_hWnd, ti);
            _ToolTipControl.AddTool(&ti);
        }
    }
}

/// <summary>
/// Handles a change of the user interface colors.
/// </summary>
void uielement_t::OnColorsChanged()
{
    GetColors();

    _MainState._StyleManager.UpdateCurrentColors();

    _CriticalSection.Enter();

    _ThreadState._StyleManager._UserInterfaceColors = _MainState._StyleManager._UserInterfaceColors;

    ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);

    // Notify the render thread.
    _Event.Raise(event_t::UserInterfaceColorsChanged);

    _CriticalSection.Leave();

    // Notify the configuration dialog.
    if (_ConfigurationDialog.IsWindow())
    {
        _ConfigurationDialog.PostMessageW(UM_CONFIGURATION_CHANGED, CC_COLORS);
        Log.AtDebug().Write(STR_COMPONENT_BASENAME " notified configuration dialog of configuration change (User interface colors changed).");
    }
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT uielement_t::OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UpdateState();

    return 0;
}

/// <summary>
/// Starts the timer.
/// </summary>
void uielement_t::StartTimer() noexcept
{
    if (_ThreadPoolTimer != nullptr)
        StopTimer();

    _ThreadPoolTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);

    FILETIME DueTime = { };

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _MainState._RefreshRateLimit, 0);
}

/// <summary>
/// Stops the timer.
/// </summary>
void uielement_t::StopTimer() noexcept
{
    if (_ThreadPoolTimer == nullptr)
        return;

    ::SetThreadpoolTimer(_ThreadPoolTimer, nullptr, 0, 0);

    ::WaitForThreadpoolTimerCallbacks(_ThreadPoolTimer, true);

    ::CloseThreadpoolTimer(_ThreadPoolTimer);
    _ThreadPoolTimer = nullptr;
}

/// <summary>
/// Toggles the frame counter display.
/// </summary>
void uielement_t::ToggleFrameCounter() noexcept
{
    _MainState._ShowFrameCounter = !_MainState._ShowFrameCounter;
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void uielement_t::ToggleHardwareRendering() noexcept
{
    _MainState._UseHardwareRendering = !_MainState._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Shows the configuration dialog.
/// </summary>
void uielement_t::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        DialogParameters dp = { m_hWnd, &_MainState };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Updates the state.
/// </summary>
void uielement_t::UpdateState() noexcept
{
//  Log::Write(Log::Level::Trace, "%8d: UpdateState", (uint32_t) ::GetTickCount64());

    {
        DeleteTrackingToolTip();

        for (auto & Iter : _Grid)
        {
            TTTOOLINFOW ti;

            Iter._Graph->InitToolInfo(m_hWnd, ti);
            _ToolTipControl.DelTool(&ti);
        }
    }

    _CriticalSection.Enter();

    {
        _ThreadState = _MainState;

        _ThreadState._SampleRate = 0;
        _ThreadState._StyleManager.ReleaseDeviceSpecificResources();

        // Create the graphs.
        {
            for (auto & Iter : _Grid)
                delete Iter._Graph;

            _Grid.clear();

            _Grid.Initialize(_ThreadState._GridRowCount, _ThreadState._GridColumnCount);

            for (const auto & Iter : _ThreadState._GraphSettings)
            {
                auto * g = new graph_t();

                g->Initialize(&_ThreadState, &Iter);

                _Grid.push_back({ g, Iter._HRatio, Iter._VRatio });
            }
        }
    }

    _CriticalSection.Leave();

    {
        for (auto & Iter : _Grid)
        {
            TTTOOLINFOW ti;

            Iter._Graph->InitToolInfo(m_hWnd, ti);
            _ToolTipControl.AddTool(&ti);
        }

        _ToolTipControl.Activate(_ThreadState._ShowToolTips);
    }

    Resize();
}

/// <summary>
/// Gets the graph that contains the specified point.
/// </summary>
graph_t * uielement_t::GetGraph(const CPoint & pt) noexcept
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
void uielement_t::on_playback_new_track(metadb_handle_ptr track)
{
    _Event.Raise(event_t::PlaybackStartedNewTrack);

    UpdateState();

    // Always get the album art in case the user enables the _ShowArtworkOnBackground setting while playing a track.
    if (track.is_valid())
    {
        if (!_MainState._ArtworkFilePath.empty())
            GetAlbumArtFromScript(track, fb2k::noAbort);
        else
            GetAlbumArtFromTrack(track, fb2k::noAbort);
    }
}

/// <summary>
/// Playback stopped.
/// </summary>
void uielement_t::on_playback_stop(play_control::t_stop_reason reason)
{
    _Event.Raise(event_t::PlaybackStopped);

    _MainState._SampleRate = 0;
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void uielement_t::on_playback_pause(bool)
{
}

/// <summary>
/// Called every second, for time display.
/// </summary>
void uielement_t::on_playback_time(double time)
{
    _ThreadState._TrackTime = time;
}

#pragma endregion

/// <summary>
/// Gets the album art from the specified track.
/// </summary>
void uielement_t::GetAlbumArtFromTrack(const metadb_handle_ptr & track, abort_callback & abort)
{
    static_api_ptr_t<album_art_manager_v2> aam;

    auto Extractor = aam->open(pfc::list_single_ref_t(track), pfc::list_single_ref_t(album_art_ids::cover_front), abort);

    if (!Extractor.is_valid())
        return;

    try
    {
        auto aad = Extractor->query(album_art_ids::cover_front, abort);

        if (aad.is_valid())
            _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
    }
    catch (const std::exception & e) // exception_aborted, exception_album_art_not_found
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to get album art from playing track: %s", e.what());
    }
}

/// <summary>
/// Gets the album art from a script.
/// </summary>
void uielement_t::GetAlbumArtFromScript(const metadb_handle_ptr & track, abort_callback & abort)
{
    titleformat_object::ptr Script;

    bool Success = titleformat_compiler::get()->compile(Script, pfc::utf8FromWide(_MainState._ArtworkFilePath.c_str()));

    pfc::string Result;

    if (Success && Script.is_valid() && track->format_title(0, Result, Script, 0))
        _Artwork.Initialize(pfc::wideFromUTF8(Result).c_str());
    else
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to get album art from script.");
}
