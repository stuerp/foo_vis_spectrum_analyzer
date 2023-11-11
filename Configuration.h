
/** $VER: Configuration.h (2022.11.06) P. Stuer **/

#pragma once

class Config
{
public:
    size_t GetVersion();

    Config();

    void Build(ui_element_config_builder & builder);
    void Parse(ui_element_config_parser & parser);
    void Reset();

    double GetZoomPercentage()
    {
        return (double) _ZoomFactor * 0.01;
    }

    /// <summary>
    /// Gets the duration (in ms) of the window that will be rendered.
    /// </summary>
    double GetWindowDurationInMs()
    {
        return (double) _WindowDuration * 0.001;
    }

    double GetLineStrokeWidth()
    {
        return (double) _LineStrokeWidth * 0.1;
    }

public:
    bool _UseHardwareRendering;
    bool _DoDownMixing;
    bool _UseZeroTrigger;
    bool _DoResampling;
    bool _UseLowQuality;

    size_t _WindowDuration;
    size_t _ZoomFactor;
    size_t _RefreshRateLimit; // in Hz
    size_t _LineStrokeWidth;
};

extern cfg_bool cfg_popup_enabled;
extern cfg_window_placement cfg_popup_window_placement;

enum t_blend_mode
{
    blend_mode_linear,
    blend_mode_clockwise,
    blend_mode_counterclockwise,
};
