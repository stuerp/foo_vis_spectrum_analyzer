
/** $VER: Configuration.h (2023.11.13) P. Stuer **/

#pragma once

/// <summary>
/// Represents the configuration of the spectrum analyzer.
/// </summary>
class Configuration
{
public:
    Configuration();

    Configuration(const Configuration &) = delete;
    Configuration & operator=(const Configuration &) = delete;
    Configuration(Configuration &&) = delete;
    Configuration & operator=(Configuration &&) = delete;

    virtual ~Configuration() { }

    void Reset() noexcept;

    void Read(ui_element_config_parser & parser);
    void Write(ui_element_config_builder & builder) const;

    /// <summary>
    /// Gets the duration (in ms) of the window that will be rendered.
    /// </summary>
    double GetWindowDurationInMs() const noexcept
    {
        return (double) _WindowDuration * 0.001;
    }

public:
    RECT _OptionsRect;

    bool _UseHardwareRendering;
    bool _UseZeroTrigger;
    bool _UseAntialiasing;

    size_t _WindowDuration;
    size_t _RefreshRateLimit; // in Hz

private:
    const size_t _Version = 2;
};

extern cfg_bool cfg_popup_enabled;
extern cfg_window_placement cfg_popup_window_placement;
