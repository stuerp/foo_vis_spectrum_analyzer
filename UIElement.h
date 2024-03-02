
/** $VER: UIElement.h (2024.03.01) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"
#include "ConfigurationDialog.h"

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
    void UpdateState() noexcept;

private:
    #pragma region CWindowImpl

    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect);

    void OnMouseMove(UINT, CPoint);
    void OnMouseLeave();

    LRESULT OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam);

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
    void on_playback_time(double p_time) { }
    void on_volume_change(float p_new_val) { }

    #pragma endregion

    #pragma region Album Art Manager callback methods

    void on_album_art(album_art_data::ptr data);

    #pragma endregion

    virtual void ToggleFullScreen() noexcept;
    void ToggleFrameCounter() noexcept;
    void ToggleHardwareRendering() noexcept;

    void Configure() noexcept;
    void Resize();

    void OnTimer();

    void ProcessPlaybackEvent();
    void UpdateSpectrum();
    void Render();

    Graph * GetGraph(const CPoint & pt) noexcept;

    #pragma region DirectX

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateArtworkDependentResources();

    HRESULT CreateDeviceSpecificResources();
    void ReleaseDeviceSpecificResources();

    #pragma endregion

    #pragma region Timer

    void StartTimer() noexcept;
    void StopTimer() noexcept;

    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept;

    #pragma endregion

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

        MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChange)
    END_MSG_MAP()

    #pragma endregion

protected:
    State _State;
    CriticalSection _CriticalSection;
    RECT _OldBounds;
    bool _IsFullScreen;
    bool _IsVisible;                // True if the component is visible.

private:
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
    };

    PTP_TIMER _ThreadPoolTimer;

    enum class PlaybackEvent
    {
        None = 0,

        NewTrack,
        Stop,
    } _PlaybackEvent;

    ConfigurationDialog _ConfigurationDialog;

    CToolTipCtrl _ToolTipControl;

    Graph * _TrackingGraph;
    CToolInfo * _TrackingToolInfo;
    POINT _LastMousePos;
    size_t _LastIndex;

    uint32_t _SampleRate;

    Artwork _Artwork;
    bool _NewArtwork;               // True when new artwork has arrived.
    bool _NewArtworkColors;       // True when the artwork gradient needs an update (either a new bitmap or new configuration parameters).

    bool _IsConfigurationChanged;   // True when the render thread has changed the configuration (e.g. because a change in artwork).

    #pragma region Render thread

    State _RenderState;

    visualisation_stream_v2::ptr _VisualisationStream;

    FrameCounter _FrameCounter;

    Grid _Grid;
    UINT _DPI;

    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;

    double _OldPlaybackTime;

    bool _IsFrozen;                 // True if the component should stop rendering the spectrum.

    #pragma endregion

};
