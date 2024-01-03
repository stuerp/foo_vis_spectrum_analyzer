
/** $VER: UIElement.h (2024.01.03) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "ConfigurationDialog.h"

#include "FrameCounter.h"
#include "Graph.h"
#include "XAxis.h"
#include "YAxis.h"
#include "Spectrum.h"

#include "Analyzers\FFTAnalyzer.h"
#include "Analyzers\CQTAnalyzer.h"

#include <vector>
#include <complex>

/// <summary>
/// Implements the UIElement and Playback interface.
/// </summary>
class UIElement : public CWindowImpl<UIElement>, private play_callback_impl_base
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

    virtual void OnContextMenu(CWindow wnd, CPoint point);

private:
    #pragma region CWindowImpl

    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnPaint(CDCHandle dc);
    LRESULT OnEraseBackground(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect);

    void OnMouseMove(UINT, CPoint);
    void OnMouseLeave();

    LRESULT OnConfigurationChanging(UINT uMsg, WPARAM wParam, LPARAM lParam);

    BEGIN_MSG_MAP_EX(UIElement)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBackground)
        MSG_WM_SIZE(OnSize)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_DPICHANGED(OnDPIChanged)

        MSG_WM_MOUSEMOVE(OnMouseMove) // Required for CToolTip
        MSG_WM_MOUSELEAVE(OnMouseLeave) // Required for tracking tooltip

        MESSAGE_HANDLER_EX(WM_CONFIGURATION_CHANGING, OnConfigurationChanging)
    END_MSG_MAP()

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

    virtual void ToggleFullScreen() noexcept;
    void ToggleFrameCounter() noexcept;
    void ToggleHardwareRendering() noexcept;

    void Configure() noexcept;
    void SetConfiguration() noexcept;

    void Resize();

    void RenderFrame();

    void ProcessAudioChunk(const audio_chunk & chunk) noexcept;
    void GetAnalyzer(const audio_chunk & chunk) noexcept;
    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();

    void ApplyAverageSmoothing(double factor);
    void ApplyPeakSmoothing(double factor);

    static double ScaleF(double x, ScalingFunction function, double factor);
    static double DeScaleF(double x, ScalingFunction function, double factor);

    #pragma region DirectX

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateDeviceSpecificResources();
    void ReleaseDeviceSpecificResources();

    #pragma endregion

    #pragma region Timer

    void CreateTimer() noexcept;
    void StartTimer() const noexcept;
    void StopTimer() const noexcept;

    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept;

    #pragma endregion

protected:
    Configuration _Configuration;

private:
    CComPtr<ID2D1GradientStopCollection> GetGradientStopCollection() const;

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
    };

    CRITICAL_SECTION _Lock;
    PTP_TIMER _ThreadPoolTimer;

    visualisation_stream_v2::ptr _VisualisationStream;

    #pragma region Rendering

    FrameCounter _FrameCounter;
    Graph _Graph;

    #pragma endregion

private:
    #pragma region DirectX

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;

    #pragma endregion

private:
    ConfigurationDialog _ConfigurationDialog;

    CToolTipCtrl _ToolTipControl;
    CToolInfo * _TrackingToolInfo;
    bool _IsTracking;
    POINT _LastMousePos;
    size_t _LastIndex;

    double _OldPlaybackTime;

    const WindowFunction * _WindowFunction;
    FFTAnalyzer * _FFTAnalyzer;
    std::vector<std::complex<double>> _FrequencyCoefficients;

    CQTAnalyzer * _CQTAnalyzer;

    std::vector<FrequencyBand> _FrequencyBands;
    size_t _FFTSize;
    uint32_t _SampleRate;
    double _Bandwidth;

    UINT _DPI;
};
