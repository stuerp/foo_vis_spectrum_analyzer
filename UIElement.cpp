
/** $VER: UIElement.cpp (2024.03.26) P. Stuer **/

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
UIElement::UIElement(): _IsVisible(true), _IsStartingUp(true), _DPI(), _ThreadPoolTimer(), _TrackingGraph(), _TrackingToolInfo(), _LastMousePos(), _LastIndex(~0U), _SampleRate(44100)
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
        Log::Write(Log::Level::Critical, "%s: Unable to create visualisation stream: %s.", core_api::get_my_file_name(), ex.what());

        return -1;
    }

    // Register ourselves with the album art notification manager.
    {
        auto AlbumArtNotificationManager = now_playing_album_art_notify_manager::tryGet();

        if (AlbumArtNotificationManager.is_valid())
            AlbumArtNotificationManager->add(this);
    }

    // Get the artwork data from the album art.
    if (_MainState._ArtworkFilePath.empty())
    {
        auto aanm = now_playing_album_art_notify_manager::get();

        if (aanm != nullptr)
        {
            album_art_data_ptr aad = aanm->current();

            if (aad.is_valid())
                hr = _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
        }
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
                PresetMenu.AppendMenu((UINT) MF_STRING, IDM_PRESET_NAME + i, PresetName.c_str());
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
                State NewState;

                size_t Index = (size_t) CommandId - IDM_PRESET_NAME;

                if (InRange(Index, (size_t) 0, PresetNames.size() - (size_t) 1))
                {
                    PresetManager::Load(_MainState._PresetsDirectoryPath, PresetNames[Index], &NewState);

                    NewState._StyleManager._DominantColor       = _MainState._StyleManager._DominantColor;
                    NewState._StyleManager._UserInterfaceColors = _MainState._StyleManager._UserInterfaceColors;

                    NewState._StyleManager.UpdateCurrentColors();

                    _MainState = NewState;

                    UpdateState();
                }
            }
        }
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
/// Resizes all visual elements.
/// </summary>
void UIElement::Resize()
{
    if (_RenderTarget == nullptr)
        return;

    DeleteTrackingToolTip();

    D2D1_SIZE_F Size = _RenderTarget->GetSize();

    // Reposition the frame counter.
    _FrameCounter.Resize(Size.width, Size.height);

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
            _Grid.Resize(Size.width, Size.height);

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
void UIElement::OnColorsChanged()
{
    GetColors();

    _MainState._StyleManager.UpdateCurrentColors();

    _CriticalSection.Enter();

    _ThreadState._StyleManager._UserInterfaceColors = _MainState._StyleManager._UserInterfaceColors;

    ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);

    // Notify the render thread.
    _Event.Raise(Event::UserInterfaceColorsChanged);

    _CriticalSection.Leave();

    // Notify the configuration dialog.
    if (_ConfigurationDialog.IsWindow())
        _ConfigurationDialog.PostMessageW(UM_CONFIGURATION_CHANGED, CC_COLORS);
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT UIElement::OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UpdateState();

    return 0;
}

/// <summary>
/// Toggles the frame counter display.
/// </summary>
void UIElement::ToggleFrameCounter() noexcept
{
    _MainState._ShowFrameCounter = !_MainState._ShowFrameCounter;
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void UIElement::ToggleHardwareRendering() noexcept
{
    _MainState._UseHardwareRendering = !_MainState._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Shows the configuration dialog.
/// </summary>
void UIElement::Configure() noexcept
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
void UIElement::UpdateState() noexcept
{
//  Log::Write(Log::Level::Trace, "%08X: UpdateState", GetTickCount64());

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
        #pragma warning (disable: 4061)
        switch (_MainState._FFTMode)
        {
            default:
                _MainState._BinCount = (size_t) (64. * ::exp2((long) _MainState._FFTMode));
                break;

            case FFTMode::FFTCustom:
                _MainState._BinCount = (_MainState._FFTCustom > 0) ? (size_t) _MainState._FFTCustom : 64;
                break;

            case FFTMode::FFTDuration:
                _MainState._BinCount = (_MainState._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _MainState._FFTDuration) / 1000.) : 64;
                break;
        }
        #pragma warning (default: 4061)

        _ToneGenerator.Initialize(440., 1., 0., _MainState._BinCount);

        _ThreadState = _MainState;

        _ThreadState._StyleManager.ReleaseDeviceSpecificResources();

        // Create the graphs.
        {
            for (auto & Iter : _Grid)
                delete Iter._Graph;

            _Grid.clear();

            _Grid.Initialize(_ThreadState._GridRowCount, _ThreadState._GridColumnCount);

            for (const auto & Iter : _ThreadState._GraphSettings)
            {
                auto * g = new Graph();

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

    _Artwork.RequestColorUpdate();

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
    _Event.Raise(Event::PlaybackStartedNewTrack);
/*
    // Load the album art of the current playing track.
    {
        static_api_ptr_t<playback_control> PlaybackControl;
        metadb_handle_ptr Track;

        if (PlaybackControl->get_now_playing(Track))
            LoadAlbumArt(Track, fb2k::noAbort);
    }
*/
    UpdateState();

    // Get the sample rate from the track because the spectrum analyzer requires it. The next opportunity is to get it from the audio chunk but that is too late.
    // Also, set the sample rate after the FFT size to prevent the render thread from getting wrong results.
    _SampleRate = (uint32_t) track->get_info_ref()->info().info_get_int("samplerate");

    if (_SampleRate == 0)
        _SampleRate = 44100;

    // Use the script from the configuration to load the album art.
    if (track.is_valid() && !_MainState._ArtworkFilePath.empty())
    {
        titleformat_object::ptr Script;

        bool Success = titleformat_compiler::get()->compile(Script, pfc::utf8FromWide(_MainState._ArtworkFilePath.c_str()));

        pfc::string Result;

        if (Success && Script.is_valid() && track->format_title(0, Result, Script, 0))
            _Artwork.Initialize(pfc::wideFromUTF8(Result).c_str());
    }
}

/// <summary>
/// Playback stopped.
/// </summary>
void UIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    _Event.Raise(Event::PlaybackStopped);

    _SampleRate = 44100;
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void UIElement::on_playback_pause(bool)
{
}

/// <summary>
/// Called every second, for time display.
/// </summary>
void UIElement::on_playback_time(double time)
{
    _ThreadState._TrackTime = time;
}

#pragma endregion

#pragma region now_playing_album_art_notify

void UIElement::on_album_art(album_art_data::ptr aad)
{
    // The script in the configuration takes precedence over the album art supplied by the track.
    if (!_MainState._ArtworkFilePath.empty())
        return;

    if (aad.is_valid())
        _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
}

#pragma endregion

/// <summary>
/// 
/// </summary>
void UIElement::LoadAlbumArt(const metadb_handle_ptr & track, abort_callback & abort)
{
    static_api_ptr_t<album_art_manager_v2> aam;

    auto Extractor = aam->open(pfc::list_single_ref_t(track), pfc::list_single_ref_t(album_art_ids::cover_front), abort);

    try
    {
        auto aad = Extractor->query(album_art_ids::cover_front, abort);

        if (aad.is_valid())
            _Artwork.Initialize((uint8_t *) aad->data(), aad->size());
    }
    catch (const exception_album_art_not_found &)
    {
        return;
    }
    catch (const exception_aborted &)
    {
        throw;
    }
    catch (...)
    {
        return;
    }
}
