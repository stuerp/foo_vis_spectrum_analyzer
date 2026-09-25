
/** $VER: UIElement.h (2026.09.25) P. Stuer **/

#pragma once

#include "pch.h"

#include "State.h"
#include "ConfigurationDialog.h"
#include "Event.h"

#include "Grid.h"
#include "Graph.h"
#include "Artwork.h"
#include "FrameCounter.h"

/// <summary>
/// Implements the UIElement and Playback interface.
/// </summary>
class uielement_t : public CWindowImpl<uielement_t>, private play_callback_impl_base
{
public:
    uielement_t() = default;

    uielement_t(const uielement_t &) = delete;
    uielement_t & operator=(const uielement_t &) = delete;
    uielement_t(uielement_t &&) = delete;
    uielement_t & operator=(uielement_t &&) = delete;

    virtual ~uielement_t() = default;

    #pragma region CWindowImpl

    static CWndClassInfo & GetWndClassInfo();

    void OnColorsChanged() noexcept;

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

    virtual void OnContextMenu(CWindow wnd, CPoint point) noexcept;

    virtual void GetColors() noexcept = 0;
    virtual void ToggleFullScreen() noexcept = 0; // Handled by DUIElement and CUIElement

    void UpdateState(ConfigurationChanges settings) noexcept;

private:
    // These methods (must) run on the main foobar2000 UI thread.
    #pragma region UI thread

    #pragma region CWindowImpl

    LRESULT OnCreate(LPCREATESTRUCT cs) noexcept;
    void OnDestroy() noexcept;
    LRESULT OnEraseBackground(CDCHandle dc) noexcept;
    void OnPaint(CDCHandle dc) noexcept;
    void OnSize(UINT nType, CSize size) noexcept;

    void OnLButtonDown(UINT nFlags, CPoint point) noexcept;
    void OnLButtonUp(UINT nFlags, CPoint point) noexcept;
    void OnLButtonDblClk(UINT nFlags, CPoint point) noexcept;
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect) noexcept;

    void OnMouseMove(UINT, CPoint) noexcept;
    void OnMouseLeave() noexcept;

    LRESULT OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

    #pragma endregion

    #pragma region play_callback methods

    void on_playback_starting(play_control::t_track_command command, bool isPaused) { }
    void on_playback_new_track(metadb_handle_ptr track);
    void on_playback_stop(play_control::t_stop_reason reason);
    void on_playback_seek(double time) { }
    void on_playback_pause(bool state);
    void on_playback_edited(metadb_handle_ptr track) { }
    void on_playback_dynamic_info(const file_info&  info) { }
    void on_playback_dynamic_info_track(const file_info & info) { }
    void on_playback_time(double time);
    void on_volume_change(float newValue) { }

    #pragma endregion

    void StartRenderer() noexcept;
    void StopRenderer() noexcept;

    void ToggleFrameCounter() noexcept;

    void Configure() noexcept;
    void Resize();

    // Tool Tips
    void CreateToolTipControl() noexcept;
    void DeleteTrackingToolTip() noexcept;

    void AddTools() noexcept;
    void RemoveTools() noexcept;

    graph_t * GetGraph(const CPoint & pt) noexcept;

    bool GetArtwork(const metadb_handle_ptr & track) noexcept;
    bool GetArtworkFromTrack(const metadb_handle_ptr & track, abort_callback & abort) noexcept;
    bool GetArtworkFromScript(const metadb_handle_ptr & track, abort_callback & abort) noexcept;

    static GUID GetArtworkTypeGUID(ArtworkType artworkType) noexcept;

    #pragma region CWindowImpl

    BEGIN_MSG_MAP_EX(uielement_t)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_ERASEBKGND(OnEraseBackground)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)

        MSG_WM_CONTEXTMENU(OnContextMenu)

        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_DPICHANGED(OnDPIChanged)

        MSG_WM_MOUSEMOVE(OnMouseMove)   // Required for CToolTip
        MSG_WM_MOUSELEAVE(OnMouseLeave) // Required for tracking tooltip

        MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChanged)
    END_MSG_MAP()

    #pragma endregion

    #pragma endregion

    // These methods run on the render thread.
    #pragma region Render thread

    void RenderThreadProc() noexcept;

    void ProcessEvents() noexcept;
    void Render() noexcept;
    void ProcessAudio() noexcept;
    void Animate(int64_t now) noexcept;

    void InitializeSampleRateDependentParameters(const audio_chunk_impl & chunk) noexcept;

    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources() noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT ResizeSwapChain(UINT width, UINT height) noexcept;
    HRESULT CreateBackBuffer() noexcept;
    HRESULT CreateArtworkDependentResources() noexcept;

    #pragma endregion

#ifdef _DEBUG
    void RenderDebug() noexcept;
#endif

protected:
    state_t _UIState;
    state_t _RenderState;

    msc::critical_section_t _CriticalSection;
    configuration_dialog_t _ConfigurationDialog;
    configuration_dialog_t _NewConfigurationDialog;

    RECT _OldRect { };

    bool _IsFullScreen { false };
    bool _IsVisible { true };       // True if the component is visible.
    bool _IsInitializing { true };

    event_t _Event;

private:
    #pragma region Accessed by UI and render thread

    HWND _hParent { nullptr };

    artwork_t _Artwork;

    #pragma endregion

    #pragma region UI thread

    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_TOGGLE_FRAME_COUNTER,

        IDM_REFRESH_RATE_LIMIT = 1000,

        IDM_CONFIGURE = 2000,
        IDM_FREEZE,

        IDM_PRESET_NAME,
    };

    HANDLE _hThread { nullptr };
    HANDLE _hStopRendering { nullptr };

    CToolTipCtrl _ToolTipControl;

    graph_t * _TrackingGraph { nullptr };
    TTTOOLINFOW _TrackingToolInfo { };
    POINT _LastMousePos { };
    size_t _LastBandIndex { ~(size_t) 0 };

    bool _IsConfigurationChanged { false }; // True when the render thread has changed the configuration (e.g. because a change in artwork).

    fb2k::CCoreDarkModeHooks _DarkMode;

    #pragma endregion

    #pragma region Render thread

    UINT _DPI { 96 };
    double _DisplayRefreshRate = { 0. };

#ifdef _DEBUG
    static constexpr int64_t RefreshRates[] = { 1, 5, 20, 30, 60, 100, 200 };
#else
    static constexpr int64_t RefreshRates[] = { 20, 30, 60, 100, 200 };
#endif

    // Device-independent resources.
    ComPtr<IDXGIFactory2> _DXGIFactory;

    ComPtr<IDWriteTextFormat> _TextFormat;

    // Device-dependent resources.
    ComPtr<ID3D11Device> _D3DDevice;
    ComPtr<ID3D11DeviceContext> _D3DDeviceContext;

    ComPtr<IDCompositionDevice> _DCompositionDevice;

    ComPtr<IDXGISwapChain1> _SwapChain;

    ComPtr<ID2D1Device> _D2DDevice;
    ComPtr<ID2D1DeviceContext> _DeviceContext;

    ComPtr<IDCompositionVisual>  _CompositionVisual;
    ComPtr<IDCompositionTarget>  _CompositionTarget;

    ComPtr<ID2D1Bitmap1> _BackBuffer;

#ifdef _DEBUG
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    visualisation_stream_v2::ptr _VisualisationStream;
    bool _IsFrozen { false };   // True if the component should stop rendering.

    frame_counter_t _FrameCounter;
    grid_t _Grid;

    #pragma endregion
};
