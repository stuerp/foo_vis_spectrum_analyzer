
/** $VER: UIElement.h (2023.12.11) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Configuration.h"
#include "SpectrumAnalyzer.h"
#include "CQTProvider.h"
#include "ConfigurationDialog.h"

#include "FrameCounter.h"
#include "XAxis.h"
#include "YAxis.h"
#include "Spectrum.h"

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

    BEGIN_MSG_MAP_EX(UIElement)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_DPICHANGED(OnDPIChanged)

        MSG_WM_MOUSEMOVE(OnMouseMove) // Required for CToolTip
        MSG_WM_MOUSELEAVE(OnMouseLeave) // Required for tracking tooltip

        MESSAGE_HANDLER_EX(WM_CONFIGURATION_CHANGING, OnConfigurationChanging)
    END_MSG_MAP()

    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    virtual void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect);

    void OnMouseMove(UINT, CPoint);
    void OnMouseLeave();

    LRESULT OnConfigurationChanging(UINT uMsg, WPARAM wParam, LPARAM lParam);
    #pragma endregion

protected:
    /// <summary>
    /// Retrieves the GUID of the element.
    /// </summary>
    static const GUID & GetGUID() noexcept
    {
        static const GUID guid = GUID_UI_ELEMENT_SPECTRUM_ANALYZER;

        return guid;
    }

    void UpdateRefreshRateLimit() noexcept;
    void Log(LogLevel logLevel, const char * format, ...) const noexcept;

private:
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
    void ToggleHardwareRendering() noexcept;

    void Configure() noexcept;
    void SetConfiguration() noexcept;

    void Resize();

    HRESULT RenderFrame();
    HRESULT RenderChunk(const audio_chunk & chunk);
    HRESULT RenderXAxisFreq(FLOAT, FLOAT, FLOAT, FLOAT, double frequency);
    HRESULT RenderXAxis(FLOAT, FLOAT, FLOAT, FLOAT, uint32_t octave);
    HRESULT RenderYAxis();
    HRESULT RenderSpectrum();
    HRESULT RenderFrameCounter();

    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();

    void ApplyAverageSmoothing(double factor);
    void ApplyPeakSmoothing(double factor);

    static double ScaleF(double x, ScalingFunction function, double factor);
    static double DeScaleF(double x, ScalingFunction function, double factor);

    #pragma region DirectX

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceSpecificResources();
    void ReleaseDeviceSpecificResources();

    CComPtr<ID2D1GradientStopCollection> GetGradientStopCollection() const;

    #pragma endregion

//  static DWORD WINAPI TimerMain(LPVOID Parameter);
    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept;

protected:
    Configuration _Configuration;

private:
    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_TOGGLE_HARDWARE_RENDERING,

        IDM_REFRESH_RATE_LIMIT_20,
        IDM_REFRESH_RATE_LIMIT_30,
        IDM_REFRESH_RATE_LIMIT_60,
        IDM_REFRESH_RATE_LIMIT_100,
        IDM_REFRESH_RATE_LIMIT_200,

        IDM_CONFIGURE,
    };

    HANDLE _hMutex;

    CRITICAL_SECTION _Lock;
    PTP_TIMER _ThreadPoolTimer;

    struct TimerData
    {
        HWND hWnd;
        Configuration * Configuration;
    } _TimerData;

    bool _UseFullScreen;

    visualisation_stream_v2::ptr _VisualisationStream;

    #pragma region Rendering

    FrameCounter _FrameCounter;
    XAxis _XAxis;
    YAxis _YAxis;
    Spectrum _Spectrum;

    #pragma endregion

private:
    #pragma region DirectX

    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2dFactory;
    CComPtr<IDWriteFactory> _DirectWriteFactory;

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;

    #pragma endregion

private:
    ConfigurationDialog _ConfigurationDialog;

    CToolTipCtrl _ToolTipControl;
    CToolInfo * _TrackingToolInfo;
    bool _IsTracking;
    POINT _LastMousePos;
    int _LastIndex;

    const WindowFunction * _WindowFunction;
    SpectrumAnalyzer * _SpectrumAnalyzer;
    std::vector<std::complex<double>> _FrequencyCoefficients;

    CQTProvider * _CQT;

    std::vector<FrequencyBand> _FrequencyBands;
    size_t _FFTSize;
    uint32_t _SampleRate;
    double _Bandwidth;

    UINT _DPI;
};
