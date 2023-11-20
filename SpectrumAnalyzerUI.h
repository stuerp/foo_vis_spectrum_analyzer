
/** $VER: SpectrumAnalyzerUI.h (2023.11.20) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "SpectrumAnalyzer.h"
#include "CQTProvider.h"
#include "ConfigurationDialog.h"

#include "FrameCounter.h"
#include "XAxis.h"
#include "YAxis.h"

#include <vector>

/// <summary>
/// Implements the UIElement and Playback interface.
/// </summary>
class SpectrumAnalyzerUIElement : public ui_element_instance, public CWindowImpl<SpectrumAnalyzerUIElement>, private play_callback_impl_base
{
public:
    SpectrumAnalyzerUIElement() = delete;
    SpectrumAnalyzerUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback);

    SpectrumAnalyzerUIElement(const SpectrumAnalyzerUIElement &) = delete;
    SpectrumAnalyzerUIElement & operator=(const SpectrumAnalyzerUIElement &) = delete;
    SpectrumAnalyzerUIElement(SpectrumAnalyzerUIElement &&) = delete;
    SpectrumAnalyzerUIElement & operator=(SpectrumAnalyzerUIElement &&) = delete;

    // Default User Interface
    #pragma region (ui_element_instance interface)
    static void g_get_name(pfc::string_base & p_out);
    static const char * g_get_description();
    static GUID g_get_guid();
    static GUID g_get_subclass();
    static ui_element_config::ptr g_get_default_configuration();

    void initialize_window(HWND p_parent);
    virtual void set_configuration(ui_element_config::ptr p_data);
    virtual ui_element_config::ptr get_configuration();
    virtual void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);
    #pragma endregion

    #pragma region (CWindowImpl interface)
    static CWndClassInfo & GetWndClassInfo();

    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnTimer(UINT_PTR nIDEvent);
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);

    LRESULT OnConfigurationChanging(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnConfigurationChanged(UINT uMsg, WPARAM wParam, LPARAM lParam);

    BEGIN_MSG_MAP_EX(SpectrumAnalyzerUIElement)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)
        MSG_WM_CONTEXTMENU(OnContextMenu)

        MESSAGE_HANDLER_EX(WM_CONFIGURATION_CHANGING, OnConfigurationChanging)
        MESSAGE_HANDLER_EX(WM_CONFIGURATION_CHANGED, OnConfigurationChanged)
    END_MSG_MAP()
    #pragma endregion

protected:
    ui_element_instance_callback::ptr m_callback;

private:
    #pragma region (Playback callback methods)
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

    void ToggleFullScreen() noexcept;
    void ToggleHardwareRendering() noexcept;
    void UpdateRefreshRateLimit() noexcept;
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

    void GenerateFrequencyBands();
    void GenerateFrequencyBandsFromNotes();
    void GenerateFrequencyBandsOfAveePlayer();

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

private:
    enum
    {
        ID_REFRESH_TIMER = 1
    };

    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_TOGGLE_HARDWARE_RENDERING,
        IDM_REFRESH_RATE_LIMIT_20,
        IDM_REFRESH_RATE_LIMIT_60,
        IDM_REFRESH_RATE_LIMIT_100,
        IDM_REFRESH_RATE_LIMIT_200,
        IDM_CONFIGURE,
    };

    ULONGLONG _LastRefresh;
    DWORD _RefreshInterval;

    bool _UseFullScreen;

    visualisation_stream_v2::ptr _VisualisationStream;

    #pragma region Rendering
    FrameCounter _FrameCounter;
    XAxis _XAxis;
    YAxis _YAxis;
    #pragma endregion

    #pragma region DirectX

    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2dFactory;
    CComPtr<IDWriteFactory> _DirectWriteFactory;

    // Device-specific resources
    D2D1_SIZE_U _ClientSize;
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;

    CComPtr<ID2D1SolidColorBrush> _WhiteBrush;

    CComPtr<ID2D1SolidColorBrush> _BandBackgroundBrush;
    CComPtr<ID2D1LinearGradientBrush> _BandForegroundBrush;

    #pragma endregion

    ConfigurationDialog _ConfigurationDialog;

private:
    SpectrumAnalyzer * _SpectrumAnalyzer;
    std::vector<std::complex<double>> _FrequencyCoefficients;

    CQTProvider * _CQT;

    std::vector<FrequencyBand> _FrequencyBands;
    size_t _FFTSize;
    uint32_t _SampleRate;
    double _Bandwidth;
};
