
/** $VER: UIElement.cpp (2026.06.08) P. Stuer - UIElement methods that run on the UI thread. **/

#include "pch.h"

#include "UIElement.h"

#include "Support.h"
#include "Log.h"

#include "Error.h"
#include "PresetManager.h"

#include <Constants.h>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
uielement_t::uielement_t(): _IsFullScreen(false), _IsVisible(true), _IsInitializing(true), _hParent(), _DPI(), _DisplayRefreshRate(), _hStopRendering(), _hThread(), _TrackingGraph(), _TrackingToolInfo(), _LastMousePos(), _LastBandIndex(~(size_t) 0)
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
LRESULT uielement_t::OnCreate(LPCREATESTRUCT cs) noexcept
{
    _hParent = ::GetAncestor(m_hWnd, GA_ROOT); // This is usually the main foobar2000 window but it can also be a flowin window.

    ::SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, ::GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT); // Required for alpha transparency

    HRESULT hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
    {
        error_t LastError((DWORD) hr);

        Log.AtFatal().Write(STR_COMPONENT_BASENAME " is unable to create DirectX device independent resources: %s", LastError.Message().c_str());

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
            _VisualisationStream->request_backlog(1.0); // Initialize the backbuffer allowing data requests up to 1s back in time.
            _VisualisationStream->set_channel_mode(visualisation_stream_v2::channel_mode_default);
        }
    }
    catch (std::exception & ex)
    {
        Log.AtFatal().Write(STR_COMPONENT_BASENAME " is unable to create visualisation stream: %s.", ex.what());

        return -1;
    }

    // Get the artwork for the currently playing track (in case we get instantiated when playback is already ongoing).
    {
        metadb_handle_ptr CurrentTrack;

        if (playback_control::get()->get_now_playing(CurrentTrack))
            GetArtwork(CurrentTrack);
    }

    // Create the tooltip control.
    CreateToolTipControl();

    // Apply the initial configuration.
    UpdateState(ConfigurationChanges::All);

    _hStopRendering = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

    StartRenderer();

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void uielement_t::OnDestroy() noexcept
{
    StopRenderer();

    ::CloseHandle(_hStopRendering);

    {
        msc::lock_t Lock(_CriticalSection);

        _Grid.Clear();

        _VisualisationStream.release();

        DeleteDeviceSpecificResources();

        DeleteDeviceIndependentResources();
    }
}

/// <summary>
/// Handles the WM_ERASEBKGND message.
/// </summary>
LRESULT uielement_t::OnEraseBackground(CDCHandle hDC) noexcept
{
    return 1; // Prevent GDI from erasing the background. Required for alpha transparency.
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void uielement_t::OnPaint(CDCHandle hDC) noexcept
{
    ValidateRect(nullptr); // Prevent any further WM_PAINT messages.
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void uielement_t::OnSize(UINT type, CSize size) noexcept
{
    if ((_DeviceContext == nullptr) || (size.cx == 0) || (size.cy == 0))
        return;

    msc::lock_t Lock(_CriticalSection);

    // Remove the bitmap from the device context.
    _DeviceContext->SetTarget(nullptr);

    // Release the bitmap so that the swap chain can be resized.
    _BackBuffer.Release();

    // Resize the swap chain.
    HRESULT hr = _SwapChain->ResizeBuffers(0, (UINT) size.cx, (UINT) size.cy, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

    // Recreate the back buffer.
    if (SUCCEEDED(hr))
        hr = CreateBackBuffer();

    // Initialize the target buffer of the device context.
    if (SUCCEEDED(hr))
        _DeviceContext->SetTarget(_BackBuffer);

    Resize();

//  ::InvalidateRect(m_hWnd, nullptr, FALSE); // Force a repaint.
}

/// <summary>
/// Handles a context menu selection.
/// </summary>
void uielement_t::OnContextMenu(CWindow wnd, CPoint position) noexcept
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
        Menu.AppendMenu((UINT) MF_STRING | (_UIState._ShowFrameCounter ? MF_CHECKED : 0), IDM_TOGGLE_FRAME_COUNTER, L"Frame Counter");

        {
            RefreshRateLimitMenu.CreatePopupMenu();

            const int64_t RefreshRates[] = { 20, 30, 60, 100, 200 };

            for (size_t i = 0; i < _countof(RefreshRates); ++i)
                RefreshRateLimitMenu.AppendMenu
                (
                    (UINT) MF_STRING | ((_UIState._RefreshRateLimit ==  RefreshRates[i]) ? MF_CHECKED : 0),
                    IDM_REFRESH_RATE_LIMIT_20 + i,
                    pfc::wideFromUTF8(pfc::format(RefreshRates[i], L"Hz"))
                );

            Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, L"Refresh Rate Limit");
        }

        {
            PresetMenu.CreatePopupMenu();

            PresetManager::GetPresetNames(_UIState._PresetsDirectoryPath, PresetNames);

            UINT_PTR i = 0;

            for (auto & PresetName : PresetNames)
            {
                PresetMenu.AppendMenu((UINT) MF_STRING | ((_UIState._ActivePresetName == PresetName) ? MF_CHECKED : 0), IDM_PRESET_NAME + i, PresetName.c_str());
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

        case IDM_REFRESH_RATE_LIMIT_20:
            _UIState._RefreshRateLimit =
            _RenderState._RefreshRateLimit = 20; // Near-atomic
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _UIState._RefreshRateLimit =
            _RenderState._RefreshRateLimit = 30; // Near-atomic
            break;

        case IDM_REFRESH_RATE_LIMIT_60:
            _UIState._RefreshRateLimit =
            _RenderState._RefreshRateLimit = 60; // Near-atomic
            break;

        case IDM_REFRESH_RATE_LIMIT_100:
            _UIState._RefreshRateLimit =
            _RenderState._RefreshRateLimit = 100; // Near-atomic
            break;

        case IDM_REFRESH_RATE_LIMIT_200:
            _UIState._RefreshRateLimit =
            _RenderState._RefreshRateLimit = 200; // Near-atomic
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
                    PresetManager::Load(_UIState._PresetsDirectoryPath, PresetNames[Index], &NewState);

                    NewState._ArtworkDominantColor = _UIState._ArtworkDominantColor;
                    NewState._UserInterfaceColors  = _UIState._UserInterfaceColors;

                    NewState._ActivePresetName = PresetNames[Index];

                    _UIState = NewState;

                    _UIState._RecreateStyles = true;

                    UpdateState(ConfigurationChanges::All);

                    // Notify the configuration dialog.
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
/// Handles a left mousebutton down message.
/// </summary>
void uielement_t::OnLButtonDown(UINT flags, CPoint point) noexcept
{
   if (_UIState._ShowToolTipsAlways)
        return; // Already showing tooltips.

    _ToolTipControl.Activate(true);

    DeleteTrackingToolTip();

    _UIState._ShowToolTipsNow = true;

    OnMouseMove(flags, point);
}

/// <summary>
/// Handles a left mousebutton up message.
/// </summary>
void uielement_t::OnLButtonUp(UINT flags, CPoint point) noexcept
{
    if (_UIState._ShowToolTipsAlways)
        return; // Already showing tooltips.

    _ToolTipControl.Activate(false);

    DeleteTrackingToolTip();

    _UIState._ShowToolTipsNow = false;
}

/// <summary>
/// Toggles between panel and full screen mode.
/// </summary>
void uielement_t::OnLButtonDblClk(UINT flags, CPoint point) noexcept
{
    ToggleFullScreen();
}

/// <summary>
/// Handles a DPI change.
/// </summary>
LRESULT uielement_t::OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect) noexcept
{
    _DPI = dpiX;

    DeleteDeviceSpecificResources();

    return 0;
}

/// <summary>
/// Resizes all visual elements.
/// </summary>
void uielement_t::Resize()
{
    if (_DeviceContext == nullptr)
        return;

    DeleteTrackingToolTip();

    D2D1_SIZE_F SizeF = _DeviceContext->GetSize(); // Gets the size in DPIs.

    // Reposition the frame counter.
    _FrameCounter.Resize(SizeF.width, SizeF.height);

    // Resize the grid.
    {
        DeleteTools();

        {
            msc::lock_t Lock(_CriticalSection);

            _Grid.Resize(SizeF.width, SizeF.height);

            _RenderState._RecreateStyles = true;
        }

        AddTools();
    }
}

/// <summary>
/// Handles a change of the user interface colors.
/// </summary>
void uielement_t::OnColorsChanged() noexcept
{
    GetColors();

    _UIState._RecreateStyles = true;

    {
        msc::lock_t Lock(_CriticalSection);

        _RenderState._UserInterfaceColors = _UIState._UserInterfaceColors;

        ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);

        // Notify the render thread.
        _Event.Raise(event_t::UserInterfaceColorsChanged);
    }

    // Notify the configuration dialog about the changed UI colors.
    if (_ConfigurationDialog.IsWindow())
    {
        _ConfigurationDialog.PostMessageW(UM_CONFIGURATION_CHANGED, CC_COLORS); // Must be sent outside the critical section.

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " notified configuration dialog of configuration change (User interface colors).");
    }
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message from the configuration dialog.
/// </summary>
LRESULT uielement_t::OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    UpdateState((ConfigurationChanges) wParam);

    return 0;
}

/// <summary>
/// Starts the render thread.
/// </summary>
void uielement_t::StartRenderer() noexcept
{
    assert(_hThread == NULL);

    _ThreadId = 0;

    _hThread = ::CreateThread(nullptr, 0, CallRenderThreadProc, this, 0, &_ThreadId);
}

/// <summary>
/// Render thread procedure.
/// </summary>
DWORD WINAPI uielement_t::CallRenderThreadProc(LPVOID context) noexcept
{
    ((uielement_t *) context)->RenderThreadProc();

    return 0;
}

/// <summary>
/// Stops the render thread.
/// </summary>
void uielement_t::StopRenderer() noexcept
{
    if (_hThread == NULL)
        return;

    ::SetEvent(_hStopRendering);

    ::WaitForSingleObject(_hThread, INFINITE);

    ::CloseHandle(_hThread);
    _hThread = NULL;
}

/// <summary>
/// Toggles the frame counter display.
/// </summary>
void uielement_t::ToggleFrameCounter() noexcept
{
    _UIState._ShowFrameCounter = !_UIState._ShowFrameCounter;
}

/// <summary>
/// Shows the configuration dialog.
/// </summary>
void uielement_t::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        dialog_parameters_t dp = { m_hWnd, &_UIState };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Updates the state.
/// </summary>
void uielement_t::UpdateState(ConfigurationChanges configurationChanges) noexcept
{
    if (configurationChanges == ConfigurationChanges::All)
    {
        DeleteTrackingToolTip();

        DeleteTools();
    }

    {
        msc::lock_t Lock(_CriticalSection);

        _RenderState = _UIState; // Copies only the settings that are relevant for rendering.

        switch (configurationChanges)
        {
            case ConfigurationChanges::All:
            {
                _RenderState._SampleRate = 0;
                _RenderState._RecreateStyles = true;

                // Recreate the resources that depend on the artwork.
                CreateArtworkDependentResources();

                // Create the graphs.
                {
                    const bool OverlapGraphs = _RenderState._OverlapGraphs && ((_RenderState._VisualizationType == VisualizationType::Bars) || (_RenderState._VisualizationType == VisualizationType::Curve) || (_RenderState._VisualizationType == VisualizationType::RadialBars) || (_RenderState._VisualizationType == VisualizationType::RadialCurve));

                    _Grid.Clear();

                    _Grid.Initialize(_RenderState._GridRowCount, _RenderState._GridColumnCount, _RenderState._VerticalLayout, OverlapGraphs);

                    size_t i = 0;

                    for (auto & GraphOptions : _RenderState._GraphOptions)
                    {
                        auto * Graph = new graph_t();

                        const bool IsFirst = !OverlapGraphs || (OverlapGraphs && (i == 0));
                        const bool IsLast  = !OverlapGraphs || (OverlapGraphs && (i == _RenderState._GraphOptions.size() - 1));

                        Graph->Initialize(&_RenderState, &GraphOptions, IsFirst, IsLast);

                        _Grid.push_back(Graph);
                        ++i;
                    }
                }
                break;
            }
            
            case ConfigurationChanges::Layout:
            {
                for (auto & Item : _Grid)
                    Item->OnConfigurationChange(configurationChanges);
                break;
            }

            case ConfigurationChanges::UserInterfaceColors:
            {
                _RenderState._UserInterfaceColors = _UIState._UserInterfaceColors;
                _RenderState._RecreateStyles = true;
                break;
            }

            case ConfigurationChanges::None:
            case ConfigurationChanges::RenderLoop:
            case ConfigurationChanges::RefreshRate:
            case ConfigurationChanges::Oscilloscope:
            case ConfigurationChanges::Artwork:
            default:
                break;
        }
    }

    if (configurationChanges == ConfigurationChanges::All)
    {
        Resize();

        AddTools();

        _ToolTipControl.Activate(_RenderState._ShowToolTipsAlways);
    }
}

/// <summary>
/// Gets the graph that contains the specified point.
/// </summary>
graph_t * uielement_t::GetGraph(const CPoint & pt) noexcept
{
    for (auto & Item : _Grid)
    {
        if (Item->ContainsPoint(pt))
            return Item;
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
    // Always get the album art in case the user enables the _ShowArtworkOnBackground setting while playing a track.
    if (track.is_valid())
        GetArtwork(track);

    // Notify the render thread.
    _Event.Raise(event_t::PlaybackNewTrack);
}

/// <summary>
/// Playback stopped.
/// </summary>
void uielement_t::on_playback_stop(play_control::t_stop_reason reason)
{
    _Artwork.DeleteWICResources();

    _UIState._SampleRate = 0;

    // Notify the render thread.
    _Event.Raise(event_t::PlaybackStopped);
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void uielement_t::on_playback_pause(bool state)
{
    // Notify the render thread.
    _Event.Raise(state ? event_t::PlaybackPaused : event_t::PlaybackResumed);
}

/// <summary>
/// Called every second, for time display.
/// </summary>
void uielement_t::on_playback_time(double time)
{
    _RenderState._TrackTime = time; // Near-atomic
}

#pragma endregion

#pragma region Artwork

/// <summary>
/// Gets the artwork for the specified track.
/// </summary>
bool uielement_t::GetArtwork(const metadb_handle_ptr & track) noexcept
{
    _Artwork.DeleteWICResources();

    if (_UIState._ArtworkFilePath.empty())
        return GetArtworkFromTrack(track, fb2k::noAbort);
    else
        return GetArtworkFromScript(track, fb2k::noAbort);
}

/// <summary>
/// Gets the artwork for the specified track.
/// </summary>
bool uielement_t::GetArtworkFromTrack(const metadb_handle_ptr & track, abort_callback & abort) noexcept
{
    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is getting artwork for the playing track.");

    const GUID ArtworkGUID = GetArtworkTypeGUID(_UIState._ArtworkType);

    static_api_ptr_t<album_art_manager_v2> ArtworkManager;

    auto ArtworkExtractor = ArtworkManager->open(pfc::list_single_ref_t(track), pfc::list_single_ref_t(ArtworkGUID), abort);

    if (!ArtworkExtractor.is_valid())
        return false;

    try
    {
        album_art_data::ptr ArtworkData;

        if (!ArtworkExtractor->query(ArtworkGUID, ArtworkData, abort))
        {
            auto StubExtractor = ArtworkManager->open_stub(abort);

            if (!StubExtractor.is_valid())
                return false;

            if (!StubExtractor->query(ArtworkGUID, ArtworkData, abort))
                return false;
        }

        if (ArtworkData.is_valid())
            _Artwork.CreateWICResources((uint8_t *) ArtworkData->data(), ArtworkData->size());
    }
    catch (const std::exception & e) // exception_aborted, exception_album_art_not_found
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " failed to get artwork for the playing track: %s", e.what());
    }

    return true;
}

/// <summary>
/// Gets the artwork for the specified track from a script.
/// </summary>
bool uielement_t::GetArtworkFromScript(const metadb_handle_ptr & track, abort_callback & abort) noexcept
{
    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is getting artwork from script \"%s\".", pfc::utf8FromWide(_UIState._ArtworkFilePath.c_str()).c_str());

    titleformat_object::ptr Script;

    if (!titleformat_compiler::get()->compile(Script, pfc::utf8FromWide(_UIState._ArtworkFilePath.c_str())))
        return false;

    if (!Script.is_valid())
        return false;

    pfc::string Result;

    if (!track->format_title(0, Result, Script, 0))
        return false;

    _Artwork.CreateWICResources(pfc::wideFromUTF8(Result).c_str());

    return true;
}

/// <summary>
/// Returns the API GUID for the specified artwork type.
/// </summary>
GUID uielement_t::GetArtworkTypeGUID(ArtworkType artworkType) noexcept
{
    switch (artworkType)
    {
        default:
        case ArtworkType::Front:  return album_art_ids::cover_front;
        case ArtworkType::Back:   return album_art_ids::cover_back;
        case ArtworkType::Disc:   return album_art_ids::disc;
        case ArtworkType::Icon:   return album_art_ids::icon;
        case ArtworkType::Artist: return album_art_ids::artist;
    }
}

#pragma endregion
