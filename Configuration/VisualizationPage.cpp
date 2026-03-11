
/** $VER: VisualizationPage.cpp (2026.03.10) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "VisualizationPage.h"
#include "Support.h"
#include "Log.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL visualization_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_VISUALIZATION, "Selects the type of visualization." },

        { IDC_PEAK_MODE, "Determines how to display the peak values." },
        { IDC_HOLD_TIME, "Determines how long the peak values are held before they decay." },
        { IDC_ACCELERATION, "Determines the accelaration of the peak value decay." },

        { IDC_LED_MODE, "Renders the spectrum bars and peak meters as LEDs." },
        { IDC_LED_SIZE, "Specifies the size of a LED in pixels." },
        { IDC_LED_GAP, "Specifies the gap between the LEDs in pixels." },
        { IDC_LED_INTEGRAL_SIZE, "Renders the LEDs as full blocks." },

        { IDC_INNER_RADIUS, "Sets the inner radius as a percentage of the smallest side of the graph area." },
        { IDC_OUTER_RADIUS, "Sets the outer radius as a percentage of the smallest side of the graph area." },
        { IDC_ANGULAR_VELOCITY, "Sets the angular velocity of the rotation in degrees per second. Positive values result in clockwise rotation; negative values in anti-clockwise rotation." },

        { IDC_SCROLLING_SPECTROGRAM, "Activates scrolling of the spectrogram." },
        { IDC_HORIZONTAL_SPECTROGRAM, "Renders the spectrogram horizontally." },
        { IDC_SPECTRUM_BAR_METRICS, "Uses the same rounding algorithm as when displaying spectrum bars. This makes it easier to align a vertical spectrogram with a spectrum bar visualization." },

        { IDC_HORIZONTAL_PEAK_METER, "Renders the peak meter horizontally." },
        { IDC_RMS_PLUS_3, "Enables RMS readings compliant with IEC 61606:1997 / AES17-1998 standard (RMS +3)." },
        { IDC_CENTER_SCALE, "Renders a scale between the meter bars" },
        { IDC_SCALE_LINES, "Renders a scale line on the background of the meter bars" },
        { IDC_RMS_WINDOW, "Specifies the duration of each RMS measurement." },
        { IDC_BAR_GAP, "Specifies the gap between the peak meter bars (in pixels)." },
        { IDC_MAX_BAR_SIZE, "Specifies the max. size of a meter bar (in pixels). Use 0 to remove constraint." },

        { IDC_HORIZONTAL_LEVEL_METER, "Renders the level meter horizontally." },

        { IDC_XY_MODE, "Enables X-Y mode." },
        { IDC_X_GAIN, "Specifies the gain applied to the X signal." },
        { IDC_Y_GAIN, "Specifies the gain applied to the Y signal." },
        { IDC_ROTATION, "Specifies the rotation angle of the signal in degrees." },
        { IDC_PHOSPHOR_DECAY, "Enables phosphor decay effect simulation of analog oscilloscopes." },
        { IDC_BLUR_SIGMA, "Specifies the number of pixels for the Gaussian blur. Higher values increase the blurring." },
        { IDC_DECAY_FACTOR, "Specifies the color fade speed. Lower values cause a faster decay." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void visualization_page_t::InitializeControls() noexcept
{
    // Visualization Type
    {
        const WCHAR * Names[] =
        {
            L"Bars", L"Curve", L"Spectrogram", L"Peak / RMS", L"Balance / Correlation", L"Radial Bars", L"Radial Curve", L"Oscilloscope", L"Bit Meter",
        #ifdef _DEBUG
            L"Tester"
        #endif
        };

        auto w = (CComboBox) GetDlgItem(IDC_VISUALIZATION);

        w.ResetContent();

        for (const auto & x : Names)
            w.AddString(x);

        w.SetCurSel((int) _State->_VisualizationType);
    }

    // Peak Indicators
    {
        auto w = (CComboBox) GetDlgItem(IDC_PEAK_MODE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Classic", L"Gravity", L"AIMP", L"Fade Out", L"Fading AIMP" })
            w.AddString(x);

        w.SetCurSel((int) _State->_PeakMode);

        SetDouble(IDC_HOLD_TIME, _State->_HoldTime, 0, 1);
        SetDouble(IDC_ACCELERATION, _State->_Acceleration, 0, 1);
    }

    // LEDs
    {
        SendDlgItemMessageW(IDC_LED_MODE, BM_SETCHECK, _State->_LEDMode);

        SetDouble(IDC_LED_SIZE, _State->_LEDLight, 0, 0);
        SetDouble(IDC_LED_GAP, _State->_LEDGap, 0, 0);

        SendDlgItemMessageW(IDC_LED_INTEGRAL_SIZE, BM_SETCHECK, _State->_LEDIntegralSize);
    }

    // Radial Bars / Radial Curve
    {
        SetDouble(IDC_INNER_RADIUS, _State->_InnerRadius * 100., 0, 1);
        SetDouble(IDC_OUTER_RADIUS, _State->_OuterRadius * 100., 0, 1);
        SetDouble(IDC_ANGULAR_VELOCITY, _State->_AngularVelocity, 0, 1);
    }

    // Spectrogram
    {
        SendDlgItemMessageW(IDC_SCROLLING_SPECTROGRAM, BM_SETCHECK, _State->_ScrollingSpectrogram);
        SendDlgItemMessageW(IDC_HORIZONTAL_SPECTROGRAM, BM_SETCHECK, _State->_HorizontalSpectrogram);
        SendDlgItemMessageW(IDC_SPECTRUM_BAR_METRICS, BM_SETCHECK, _State->_UseSpectrumBarMetrics);
    }

    // Peak Meter
    {
        SendDlgItemMessageW(IDC_HORIZONTAL_PEAK_METER, BM_SETCHECK, _State->_IsHorizontalPeakMeter);
        SendDlgItemMessageW(IDC_RMS_PLUS_3, BM_SETCHECK, _State->_RMSPlus3);
        SendDlgItemMessageW(IDC_CENTER_SCALE, BM_SETCHECK, _State->_HasCenterScale);
        SendDlgItemMessageW(IDC_SCALE_LINES, BM_SETCHECK, _State->_HasScaleLines);

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_RMS_WINDOW)); _NumericEdits.push_back(ne);

            auto w = CUpDownCtrl(GetDlgItem(IDC_RMS_WINDOW_SPIN));

            UDACCEL Accel[] =
            {
                { 1,  100 }, // 100ms
                { 2,  500 }, // 500ms
                { 3, 1000 }, // 1s
            };

            w.SetRange32((int) (MinRMSWindow * 1000.f), (int) (MaxRMSWindow * 1000.f));
            w.SetPos32(0);
            w.SetAccel(_countof(Accel), Accel);

            SetDouble(IDC_RMS_WINDOW, _State->_RMSWindow, 0, 3);
            ((CUpDownCtrl) GetDlgItem(IDC_RMS_WINDOW_SPIN)).SetPos32((int) (_State->_RMSWindow * 1000.));
        }
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BAR_GAP)); _NumericEdits.push_back(ne);

            SetInteger(IDC_BAR_GAP, (int64_t) _State->_BarGap);
        }
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_MAX_BAR_SIZE)); _NumericEdits.push_back(ne);

            SetInteger(IDC_MAX_BAR_SIZE, (int64_t) _State->_MaxBarSize);
        }
    }

    // Level Meter
    {
        SendDlgItemMessageW(IDC_HORIZONTAL_LEVEL_METER, BM_SETCHECK, _State->_HorizontalLevelMeter);
    }

    // Oscilloscope
    {
        SendDlgItemMessageW(IDC_XY_MODE, BM_SETCHECK, _State->_XYMode);

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_X_GAIN)); _NumericEdits.push_back(ne); SetDouble(IDC_X_GAIN, _State->_XGain);
        }
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_Y_GAIN)); _NumericEdits.push_back(ne); SetDouble(IDC_Y_GAIN, _State->_YGain);
        }
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_ROTATION)); _NumericEdits.push_back(ne); SetDouble(IDC_ROTATION, _State->_Rotation);
        }

        SendDlgItemMessageW(IDC_PHOSPHOR_DECAY, BM_SETCHECK, _State->_PhosphorDecay);
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BLUR_SIGMA)); _NumericEdits.push_back(ne); SetDouble(IDC_BLUR_SIGMA, _State->_BlurSigma);
        }
        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_DECAY_FACTOR)); _NumericEdits.push_back(ne); SetDouble(IDC_DECAY_FACTOR, _State->_DecayFactor);
        }
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void visualization_page_t::UpdateControls() noexcept
{
    const bool IsBars         = (_State->_VisualizationType == VisualizationType::Bars);
    const bool IsCurve        = (_State->_VisualizationType == VisualizationType::Curve);
    const bool IsRadialBars   = (_State->_VisualizationType == VisualizationType::RadialBars);
    const bool IsRadialCurve  = (_State->_VisualizationType == VisualizationType::RadialCurve);

    const bool IsSpectrogram  = (_State->_VisualizationType == VisualizationType::Spectrogram);
    const bool IsPeakMeter    = (_State->_VisualizationType == VisualizationType::PeakMeter);
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);
//  const bool IsBitMeter     = (_State->_VisualizationType == VisualizationType::BitMeter);

//  const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool HasPeaks = (IsBars || IsCurve || IsRadialBars || IsRadialCurve);

    const bool HasLEDs = IsBars || IsPeakMeter || IsLevelMeter;
 
    const bool IsRadial = IsRadialBars || IsRadialCurve;

    // Peak Indicators
    GetDlgItem(IDC_PEAK_MODE).EnableWindow(HasPeaks);

    GetDlgItem(IDC_HOLD_TIME).EnableWindow(HasPeaks && (_State->_PeakMode != PeakMode::None));
    GetDlgItem(IDC_ACCELERATION).EnableWindow(HasPeaks && (_State->_PeakMode != PeakMode::None));

    // LEDs
    GetDlgItem(IDC_LED_MODE).EnableWindow(HasLEDs);

    GetDlgItem(IDC_LED_SIZE).EnableWindow(HasLEDs && _State->_LEDMode);
    GetDlgItem(IDC_LED_GAP).EnableWindow(HasLEDs && _State->_LEDMode);
    GetDlgItem(IDC_LED_INTEGRAL_SIZE).EnableWindow(HasLEDs && _State->_LEDMode);

    // Radial Bars / Radial Curve
    GetDlgItem(IDC_INNER_RADIUS).EnableWindow(IsRadial);
    GetDlgItem(IDC_OUTER_RADIUS).EnableWindow(IsRadial);
    GetDlgItem(IDC_ANGULAR_VELOCITY).EnableWindow(IsRadial);

    // Spectrogram
    GetDlgItem(IDC_SCROLLING_SPECTROGRAM).EnableWindow(IsSpectrogram);
    GetDlgItem(IDC_HORIZONTAL_SPECTROGRAM).EnableWindow(IsSpectrogram);
    GetDlgItem(IDC_SPECTRUM_BAR_METRICS).EnableWindow(IsSpectrogram && !_State->_HorizontalSpectrogram);

    // Peak Meter
    GetDlgItem(IDC_HORIZONTAL_PEAK_METER).EnableWindow(IsPeakMeter);
    GetDlgItem(IDC_RMS_PLUS_3).EnableWindow(IsPeakMeter);
    GetDlgItem(IDC_CENTER_SCALE).EnableWindow(IsPeakMeter);
    GetDlgItem(IDC_SCALE_LINES).EnableWindow(IsPeakMeter);

    GetDlgItem(IDC_RMS_WINDOW).EnableWindow(IsPeakMeter);
    GetDlgItem(IDC_BAR_GAP).EnableWindow(IsPeakMeter);
    GetDlgItem(IDC_MAX_BAR_SIZE).EnableWindow(IsPeakMeter);

    // Level Meter
    GetDlgItem(IDC_HORIZONTAL_LEVEL_METER).EnableWindow(IsLevelMeter);

    // Oscilloscope
    GetDlgItem(IDC_XY_MODE).EnableWindow(IsOscilloscope);

    GetDlgItem(IDC_X_GAIN).EnableWindow(IsOscilloscope && _State->_XYMode);
    GetDlgItem(IDC_Y_GAIN).EnableWindow(IsOscilloscope);    // Available in both modes.
    GetDlgItem(IDC_ROTATION).EnableWindow(IsOscilloscope && _State->_XYMode);

    GetDlgItem(IDC_PHOSPHOR_DECAY).EnableWindow(IsOscilloscope);

    GetDlgItem(IDC_BLUR_SIGMA).EnableWindow(IsOscilloscope & _State->_PhosphorDecay);
    GetDlgItem(IDC_DECAY_FACTOR).EnableWindow(IsOscilloscope & _State->_PhosphorDecay);
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void visualization_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void visualization_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    auto cb = (CComboBox) w;

    const int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_VISUALIZATION:
        {
            _State->_VisualizationType = (VisualizationType) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_PEAK_MODE:
        {
            _State->_PeakMode = (PeakMode) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_CHANNEL_PAIRS:
        {
            _State->_ChannelPair = (ChannelPair) SelectedIndex;

            UpdateControls();
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void visualization_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    auto ChangedSettings = Settings::All;

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        // Peak indicator
        case IDC_HOLD_TIME:
        {
            if (!SetProperty(_State->_HoldTime, std::clamp(::_wtof(Text), MinHoldTime, MaxHoldTime)))
                return;

            break;
        }

        case IDC_ACCELERATION:
        {
            if (!SetProperty(_State->_Acceleration, std::clamp(::_wtof(Text), MinAcceleration, MaxAcceleration)))
                return;

            break;
        }

        // LEDs
        case IDC_LED_SIZE:
        {
            if (!SetProperty(_State->_LEDLight, std::clamp((FLOAT) ::_wtof(Text), MinLEDSize, MaxLEDSize)))
                return;

            break;
        }

        case IDC_LED_GAP:
        {
            if (!SetProperty(_State->_LEDGap, std::clamp((FLOAT) ::_wtof(Text), MinLEDGap, MaxLEDGap)))
                return;

            break;
        }

        // Radial Bars / Radial Curve
        case IDC_INNER_RADIUS:
        {
            if (!SetProperty(_State->_InnerRadius, (FLOAT) std::clamp(::_wtof(Text), 0., 100.) / 100.f))
                return;

            break;
        }

        case IDC_OUTER_RADIUS:
        {
            if (!SetProperty(_State->_OuterRadius, (FLOAT) std::clamp(::_wtof(Text), 0., 100.) / 100.f))
                return;

            break;
        }

        case IDC_ANGULAR_VELOCITY:
        {
            if (!SetProperty(_State->_AngularVelocity, (FLOAT) std::clamp(::_wtof(Text), -360., 360.)))
                return;

            break;
        }

        // Peak Meter
        case IDC_RMS_WINDOW:
        {
            if (!SetProperty(_State->_RMSWindow, std::clamp(::_wtof(Text), MinRMSWindow, MaxRMSWindow)))
                return;

            break;
        }

        case IDC_BAR_GAP:
        {
            if (!SetProperty(_State->_BarGap, std::clamp((FLOAT) ::_wtof(Text), MinBarGap, MaxBarGap)))
                return;

            break;
        }

        case IDC_MAX_BAR_SIZE:
        {
            if (!SetProperty(_State->_MaxBarSize, std::clamp((FLOAT) ::_wtof(Text), MinBarSize, MaxBarSize)))
                return;

            break;
        }

        // Oscilloscope
        case IDC_X_GAIN:
        {
            if (!SetProperty(_State->_XGain, std::clamp(::_wtof(Text), MinXGain, MaxXGain)))
                return;

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_Y_GAIN:
        {
            if (!SetProperty(_State->_YGain, std::clamp(::_wtof(Text), MinYGain, MaxYGain)))
                return;
            
            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_ROTATION:
        {
            if (!SetProperty(_State->_Rotation, std::clamp((FLOAT) ::_wtof(Text), MinRotation, MaxRotation)))
                return;

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_BLUR_SIGMA:
        {
            if (!SetProperty(_State->_BlurSigma, std::clamp((FLOAT) ::_wtof(Text), MinBlurSigma, MaxBlurSigma)))
                return;

            ChangedSettings = Settings::PhosphorEffect;
            break;
        }

        case IDC_DECAY_FACTOR:
        {
            if (!SetProperty(_State->_DecayFactor, std::clamp((FLOAT) ::_wtof(Text), MinDecayFactor, MaxDecayFactor)))
                return;

            ChangedSettings = Settings::PhosphorEffect;
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void visualization_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        // Peak indicator
        case IDC_HOLD_TIME:
        {
            SetDouble(id, _State->_HoldTime, 0, 1);
            break;
        }

        case IDC_ACCELERATION:
        {
            SetDouble(id, _State->_Acceleration, 0, 1);
            break;
        }

        // LEDs
        case IDC_LED_SIZE:
        {
            SetDouble(id, _State->_LEDLight, 0, 0);
            break;
        }

        case IDC_LED_GAP:
        {
            SetDouble(id, _State->_LEDGap, 0, 0);
            break;
        }

        // Radial Bars
        case IDC_INNER_RADIUS:
        {
            SetDouble(id, _State->_InnerRadius * 100., 0, 1);
            break;
        }

        case IDC_OUTER_RADIUS:
        {
            SetDouble(id, _State->_OuterRadius * 100., 0, 1);
            break;
        }

        case IDC_ANGULAR_VELOCITY:
        {
            SetDouble(id, _State->_AngularVelocity, 0, 1);
            break;
        }

        // Peak Meter
        case IDC_RMS_WINDOW:
        {
            SetDouble(id, _State->_RMSWindow, 0, 3);
            break;
        }

        case IDC_BAR_GAP:
        {
            SetInteger(id, (int64_t) _State->_BarGap);
            break;
        }

        case IDC_MAX_BAR_SIZE:
        {
            SetInteger(id, (int64_t) _State->_MaxBarSize);
            break;
        }

        // Oscilloscope
        case IDC_X_GAIN:
        {
            SetDouble(id, _State->_XGain, 0, 2);

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_Y_GAIN:
        {
            SetDouble(id, _State->_YGain, 0, 2);

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_ROTATION:
        {
            SetDouble(id, _State->_Rotation, 0, 2);

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_BLUR_SIGMA:
        {
            SetDouble(id, _State->_BlurSigma, 0, 2);

            ChangedSettings = Settings::PhosphorEffect;
            break;
        }

        case IDC_DECAY_FACTOR:
        {
            SetDouble(id, _State->_DecayFactor, 0, 2);

            ChangedSettings = Settings::PhosphorEffect;
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void visualization_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        case IDC_LED_MODE:
        {
            _State->_LEDMode = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();
            break;
        }

        case IDC_LED_INTEGRAL_SIZE:
        {
            _State->_LEDIntegralSize = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SCROLLING_SPECTROGRAM:
        {
            _State->_ScrollingSpectrogram = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_HORIZONTAL_SPECTROGRAM:
        {
            _State->_HorizontalSpectrogram = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();
            break;
        }

        case IDC_SPECTRUM_BAR_METRICS:
        {
            _State->_UseSpectrumBarMetrics = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_HORIZONTAL_PEAK_METER:
        {
            _State->_IsHorizontalPeakMeter = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_RMS_PLUS_3:
        {
            _State->_RMSPlus3 = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_CENTER_SCALE:
        {
            _State->_HasCenterScale = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SCALE_LINES:
        {
            _State->_HasScaleLines = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_HORIZONTAL_LEVEL_METER:
        {
            _State->_HorizontalLevelMeter = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_XY_MODE:
        {
            _State->_XYMode = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();

            ChangedSettings = Settings::Oscilloscope;
            break;
        }

        case IDC_PHOSPHOR_DECAY:
        {
            _State->_PhosphorDecay = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();

            ChangedSettings = Settings::PhosphorEffect;
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT visualization_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = Settings::All;

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_RMS_WINDOW_SPIN:
        {
            if (!SetProperty(_State->_RMSWindow, ClampNewSpinPosition(nmud, MinRMSWindow, MaxRMSWindow, 1000.)))
                return -1;

            SetDouble(IDC_RMS_WINDOW, _State->_RMSWindow, 0, 3);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}
