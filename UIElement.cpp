
/** $VER: UIElement.cpp (2025.10.20) P. Stuer - UIElement methods that run on the UI thread. **/

#include "pch.h"

#include "UIElement.h"

#include "DirectX.h"
#include "StyleManager.h"

#include "Support.h"
#include "Log.h"

#include "Error.h"
#include "PresetManager.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
uielement_t::uielement_t(): _IsFullScreen(false), _IsVisible(true), _IsInitializing(true), _DPI(), _DisplayRefreshRate(), _ThreadPoolTimer(), _TrackingGraph(), _TrackingToolInfo(), _LastMousePos(), _LastBandIndex(~0U)
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
        {
            GUID ArtworkGUID = GetArtworkTypeGUID(_UIThread._ArtworkType);

            if (_UIThread._ArtworkFilePath.empty())
                GetArtworkFromTrack(CurrentTrack, fb2k::noAbort);
            else
                GetArtworkFromScript(CurrentTrack, fb2k::noAbort);
        }
    }

    // Create the tooltip control.
    CreateToolTipControl();

    // Apply the initial configuration.
    UpdateState(Settings::All);

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

    DeleteDeviceSpecificResources();

    DeleteDeviceIndependentResources();

    _CriticalSection.Leave();

    DirectX::Terminate();
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void uielement_t::OnPaint(CDCHandle hDC)
{
    StartTimer();

    ValidateRect(nullptr); // Prevent any further WM_PAINT messages.
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void uielement_t::OnSize(UINT type, CSize size)
{
    if (_DeviceContext == nullptr)
        return;

    _CriticalSection.Enter();

    D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    // Remove the bitmap from the device context.
    _DeviceContext->SetTarget(nullptr);

    // Resize the swap chain.
    HRESULT hr = _SwapChain->ResizeBuffers(0, Size.width, Size.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

    // Get a surface from the swap chain.
    {
        CComPtr<IDXGISurface> Surface;

        if (SUCCEEDED(hr))
            hr = _SwapChain->GetBuffer(0, IID_PPV_ARGS(&Surface));

        // Create a bitmap pointing to the surface.
        CComPtr<ID2D1Bitmap1> Bitmap;

        if (SUCCEEDED(hr))
        {
            const UINT DPI = ::GetDpiForWindow(m_hWnd);

            D2D1_BITMAP_PROPERTIES1 Properties = D2D1::BitmapProperties1
            (
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                (FLOAT) DPI, (FLOAT) DPI
            );

            hr = _DeviceContext->CreateBitmapFromDxgiSurface(Surface, &Properties, &Bitmap);
        }

        // Set bitmap back onto device context.
        if (SUCCEEDED(hr))
            _DeviceContext->SetTarget(Bitmap);
    }

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
        Menu.AppendMenu((UINT) MF_STRING | (_UIThread._ShowFrameCounter ? MF_CHECKED : 0), IDM_TOGGLE_FRAME_COUNTER, L"Frame Counter");

        {
            RefreshRateLimitMenu.CreatePopupMenu();

            const size_t RefreshRates[] = { 20, 30, 60, 100, 200 };

            for (size_t i = 0; i < _countof(RefreshRates); ++i)
                RefreshRateLimitMenu.AppendMenu
                (
                    (UINT) MF_STRING | ((_UIThread._RefreshRateLimit ==  RefreshRates[i]) ? MF_CHECKED : 0),
                    IDM_REFRESH_RATE_LIMIT_20 + i,
                    pfc::wideFromUTF8(pfc::format(RefreshRates[i], L"Hz"))
                );

            Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, L"Refresh Rate Limit");
        }

        {
            PresetMenu.CreatePopupMenu();

            PresetManager::GetPresetNames(_UIThread._PresetsDirectoryPath, PresetNames);

            UINT_PTR i = 0;

            for (auto & PresetName : PresetNames)
            {
                PresetMenu.AppendMenu((UINT) MF_STRING | ((_UIThread._ActivePresetName == PresetName) ? MF_CHECKED : 0), IDM_PRESET_NAME + i, PresetName.c_str());
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
            _UIThread._RefreshRateLimit = 20;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_30:
            _UIThread._RefreshRateLimit = 30;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_60:
            _UIThread._RefreshRateLimit =60;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_100:
            _UIThread._RefreshRateLimit =100;
            StartTimer();
            break;

        case IDM_REFRESH_RATE_LIMIT_200:
            _UIThread._RefreshRateLimit =200;
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
                    PresetManager::Load(_UIThread._PresetsDirectoryPath, PresetNames[Index], &NewState);

                    NewState._StyleManager.DominantColor       = _UIThread._StyleManager.DominantColor;
                    NewState._StyleManager.UserInterfaceColors = _UIThread._StyleManager.UserInterfaceColors;

                    NewState._StyleManager.UpdateCurrentColors();

                    NewState._ActivePresetName = PresetNames[Index];

                    _UIThread = NewState;

                    UpdateState(Settings::All);

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
void uielement_t::OnLButtonDown(UINT flags, CPoint point)
{
    if (_RenderThread._ShowToolTipsAlways)
        return; // Already showing tooltips.

    _ToolTipControl.Activate(true);

    DeleteTrackingToolTip();

    _RenderThread._ShowToolTipsNow = true;

    OnMouseMove(flags, point);
}

/// <summary>
/// Handles a left mousebutton up message.
/// </summary>
void uielement_t::OnLButtonUp(UINT flags, CPoint point)
{
    if (_RenderThread._ShowToolTipsAlways)
        return; // Already showing tooltips.

    _ToolTipControl.Activate(false);

    DeleteTrackingToolTip();

    _RenderThread._ShowToolTipsNow = false;
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
        for (auto & Iter : _Grid)
        {
            TTTOOLINFOW ti;

            Iter._Graph->InitToolInfo(m_hWnd, ti);
            _ToolTipControl.DelTool(&ti);
        }

        _CriticalSection.Enter();

        {
            _Grid.Resize(SizeF.width, SizeF.height);

            _RenderThread._StyleManager.DeleteGradientBrushes();
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

    _UIThread._StyleManager.UpdateCurrentColors();

    _CriticalSection.Enter();

    _RenderThread._StyleManager.UserInterfaceColors = _UIThread._StyleManager.UserInterfaceColors;

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
    UpdateState((Settings) wParam);

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

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _UIThread._RefreshRateLimit, 0);
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
    _UIThread._ShowFrameCounter = !_UIThread._ShowFrameCounter;
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void uielement_t::ToggleHardwareRendering() noexcept
{
    _UIThread._UseHardwareRendering = !_UIThread._UseHardwareRendering;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Shows the configuration dialog.
/// </summary>
void uielement_t::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        DialogParameters dp = { m_hWnd, &_UIThread };

        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &dp) != NULL)
            _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
}

/// <summary>
/// Updates the state.
/// </summary>
void uielement_t::UpdateState(Settings settings) noexcept
{
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
        _RenderThread = _UIThread;

        _RenderThread._SampleRate = 0;
        _RenderThread._StyleManager.DeleteDeviceSpecificResources();

        // Recreate the resources that depend on the artwork.
        CreateArtworkDependentResources();

        // Create the graphs.
        {
            for (auto & Iter : _Grid)
                delete Iter._Graph;

            _Grid.clear();

            _Grid.Initialize(_RenderThread._GridRowCount, _RenderThread._GridColumnCount);

            for (const auto & Iter : _RenderThread._GraphSettings)
            {
                auto * g = new graph_t();

                g->Initialize(&_RenderThread, &Iter, nullptr);

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

        _ToolTipControl.Activate(_RenderThread._ShowToolTipsAlways);
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
    UpdateState(Settings::All);

    // Always get the album art in case the user enables the _ShowArtworkOnBackground setting while playing a track.
    if (track.is_valid())
    {
        if (_UIThread._ArtworkFilePath.empty())
            GetArtworkFromTrack(track, fb2k::noAbort);
        else
            GetArtworkFromScript(track, fb2k::noAbort);
    }

    // Notify the render thread.
    _Event.Raise(event_t::PlaybackStartedNewTrack);
}

/// <summary>
/// Playback stopped.
/// </summary>
void uielement_t::on_playback_stop(play_control::t_stop_reason reason)
{
    _Artwork.DeleteWICResources();

    _UIThread._SampleRate = 0;

    // Notify the render thread.
    _Event.Raise(event_t::PlaybackStopped);
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
    _RenderThread._TrackTime = time;
}

#pragma endregion

/// <summary>
/// Gets the album art from the specified track.
/// </summary>
bool uielement_t::GetArtworkFromTrack(const metadb_handle_ptr & track, abort_callback & abort) noexcept
{
    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is getting artwork for the playing track.");

    GUID ArtworkGUID = GetArtworkTypeGUID(_UIThread._ArtworkType);

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
/// Gets the artwork from a script.
/// </summary>
bool uielement_t::GetArtworkFromScript(const metadb_handle_ptr & track, abort_callback & abort) noexcept
{
    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is getting artwork from script \"%s\".", pfc::utf8FromWide(_UIThread._ArtworkFilePath.c_str()).c_str());

    titleformat_object::ptr Script;

    if (!titleformat_compiler::get()->compile(Script, pfc::utf8FromWide(_UIThread._ArtworkFilePath.c_str())))
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
