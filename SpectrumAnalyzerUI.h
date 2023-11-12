
/** $VER: SpectrumAnalyzerUI.h (2023.11.12) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "SpectrumAnalyzer.h"
#include "RingBuffer.h"

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

    BEGIN_MSG_MAP_EX(SpectrumAnalyzerUIElement)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
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

    void ToggleFullScreen();
    void ToggleHardwareRendering();
    void UpdateRefreshRateLimit();

    HRESULT Render();
    HRESULT RenderChunk(const audio_chunk & chunk);
    HRESULT RenderXAxis(FLOAT, FLOAT, FLOAT, FLOAT, uint32_t octave);
    HRESULT RenderYAxis();
    HRESULT RenderBands();
    HRESULT RenderText();

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceSpecificResources();
    void ReleaseDeviceSpecificResources();

private:
    enum
    {
        ID_REFRESH_TIMER = 1
    };

    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_HW_RENDERING_ENABLED,
        IDM_REFRESH_RATE_LIMIT_20,
        IDM_REFRESH_RATE_LIMIT_60,
        IDM_REFRESH_RATE_LIMIT_100,
        IDM_REFRESH_RATE_LIMIT_200,
    };

    Configuration _Configuration;

    ULONGLONG _LastRefresh;
    DWORD _RefreshInterval;

    visualisation_stream_v2::ptr _VisualisationStream;

    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2dFactory;
    CComPtr<IDWriteFactory> _DirectWriteFactory;
    CComPtr<IDWriteTextFormat> _TextFormat;
    CComPtr<IDWriteTextFormat> _LabelTextFormat;

    DWRITE_TEXT_METRICS _LabelTextMetrics;

    // Device-specific resources
    D2D1_HWND_RENDER_TARGET_PROPERTIES _RenderTargetProperties;
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
    CComPtr<ID2D1SolidColorBrush> _StrokeBrush;
    CComPtr<ID2D1SolidColorBrush> _TextBrush;

    CComPtr<ID2D1LinearGradientBrush> _GradientBrush;
 
    RingBuffer<LONGLONG, 16> _Times;

private:
    SpectrumAnalyzer * _SpectrumAnalyzer;
};

const FLOAT YAxisWidth = 30.f;
