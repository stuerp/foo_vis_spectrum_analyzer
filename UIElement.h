
/** $VER: UIElement.h (2024.03.23) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"
#include "ConfigurationDialog.h"
#include "Event.h"

#include "Grid.h"
#include "Graph.h"
#include "Artwork.h"
#include "FrameCounter.h"

#include <vector>

/// <summary>
/// Implements the UIElement and Playback interface.
/// </summary>
class UIElement : public CWindowImpl<UIElement>, private play_callback_impl_base, now_playing_album_art_notify
{
public:
    UIElement();

    UIElement(const UIElement &) = delete;
    UIElement & operator=(const UIElement &) = delete;
    UIElement(UIElement &&) = delete;
    UIElement & operator=(UIElement &&) = delete;

    #pragma region CWindowImpl

    static CWndClassInfo & GetWndClassInfo();

    void OnColorsChanged();

    #pragma endregion

protected:
    /// <summary>
    /// Retrieves the GUID of the element.
    /// </summary>
    static const GUID & GetGUID() noexcept
    {
        static const GUID guid = GUID_UI_ELEMENT;

        return guid;
    }

    virtual LRESULT OnEraseBackground(CDCHandle dc) = 0;
    virtual void OnContextMenu(CWindow wnd, CPoint point);
    virtual void GetColors() noexcept = 0;

    void UpdateState() noexcept;

private:
    #pragma region Render thread

    void OnTimer();

    void ProcessEvents();
    void UpdateSpectrum();
    void Render();

    #pragma region Timer

    void StartTimer() noexcept;
    void StopTimer() noexcept;

    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept;

    #pragma endregion

    #pragma region DirectX

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateArtworkDependentResources();

    HRESULT CreateDeviceSpecificResources();
    void ReleaseDeviceSpecificResources();

    #pragma endregion

    #pragma endregion

    #pragma region UI thread

    #pragma region CWindowImpl

    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect);

    void OnMouseMove(UINT, CPoint);
    void OnMouseLeave();

    LRESULT OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam);

    #pragma endregion

    #pragma region Playback callback methods

    void on_playback_starting(play_control::t_track_command p_command, bool p_paused) { }
    void on_playback_new_track(metadb_handle_ptr p_track);
    void on_playback_stop(play_control::t_stop_reason p_reason);
    void on_playback_seek(double p_time) { }
    void on_playback_pause(bool p_state);
    void on_playback_edited(metadb_handle_ptr p_track) { }
    void on_playback_dynamic_info(const file_info& p_info) { }
    void on_playback_dynamic_info_track(const file_info& p_info) { }
    void on_playback_time(double time);
    void on_volume_change(float p_new_val) { }

    #pragma endregion

    virtual void ToggleFullScreen() noexcept = 0;
    void ToggleFrameCounter() noexcept;
    void ToggleHardwareRendering() noexcept;

    void Configure() noexcept;
    void Resize();

    Graph * GetGraph(const CPoint & pt) noexcept;

    void on_album_art(album_art_data::ptr data);

    #pragma region CWindowImpl

    BEGIN_MSG_MAP_EX(UIElement)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_ERASEBKGND(OnEraseBackground)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_DPICHANGED(OnDPIChanged)

        MSG_WM_MOUSEMOVE(OnMouseMove) // Required for CToolTip
        MSG_WM_MOUSELEAVE(OnMouseLeave) // Required for tracking tooltip

        MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChanged)
    END_MSG_MAP()

    #pragma endregion

    #pragma endregion

protected:
    State _MainState;
    State _ThreadState;

    CriticalSection _CriticalSection;
    ConfigurationDialog _ConfigurationDialog;

    RECT _OldBounds;
    bool _IsFullScreen;
    bool _IsVisible;                // True if the component is visible.

    Event _Event;

private:
    #pragma region Shared

    Artwork _Artwork;

    #pragma endregion

    #pragma region Render thread

    UINT _DPI;

    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;

    visualisation_stream_v2::ptr _VisualisationStream;
    bool _IsFrozen;                 // True if the component should stop rendering the spectrum.

    FrameCounter _FrameCounter;
    Grid _Grid;

    #pragma endregion

    #pragma region UI thread

    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_TOGGLE_FRAME_COUNTER,
        IDM_TOGGLE_HARDWARE_RENDERING,

        IDM_REFRESH_RATE_LIMIT_20,
        IDM_REFRESH_RATE_LIMIT_30,
        IDM_REFRESH_RATE_LIMIT_60,
        IDM_REFRESH_RATE_LIMIT_100,
        IDM_REFRESH_RATE_LIMIT_200,

        IDM_CONFIGURE,
        IDM_FREEZE,

        IDM_PRESET_NAME,
    };

    PTP_TIMER _ThreadPoolTimer;

    CToolTipCtrl _ToolTipControl;

    Graph * _TrackingGraph;
    CToolInfo * _TrackingToolInfo;
    POINT _LastMousePos;
    size_t _LastIndex;

    uint32_t _SampleRate;

    bool _IsConfigurationChanged;   // True when the render thread has changed the configuration (e.g. because a change in artwork).

    fb2k::CCoreDarkModeHooks _DarkMode;
    #pragma endregion
};
