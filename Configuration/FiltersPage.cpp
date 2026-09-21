
/** $VER: FiltersPage.cpp (2026.09.20) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "FiltersPage.h"
#include "Support.h"
#include "Log.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL filters_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    static const std::unordered_map<int, const char *> Tips =
    {
        { IDC_ACOUSTIC_FILTER,  "Selects the Weighting function that will be applied." },

        { IDC_FREQ_SHIFT,       "Shifts the frequency at which the complete weighting curve is evaluated. Expressed in spectrum-bin units that are converted to Hz using the current sample rate and bin count." },

        { IDC_FREQ_TILT,        "Adjusts the spectrum by the specified number of dB per octave. Positive values emphasize higher frequencies; negative values emphasize lower frequencies." },
        { IDC_FREQ_TILT_PIVOT,  "Frequency at which the tilt adjustment is 0 dB." },

        { IDC_EQ_AMT,           "Controls the strength of the equalization curve. Zero disables equalization; positive values increase the effect." },
        { IDC_EQ_DEPTH,         "Adjusts the scale and shape of the equalization curve." },
        { IDC_EQ_OFFS,          "Moves the equalization curve along the frequency axis. Higher values shift its features toward higher frequencies." },

        { IDC_WT_AMT,           "Sets how strongly the selected acoustic weighting curve is applied. Zero disables weighting; one applies the full curve." },

        { IDC_CROSSOVER_MODE,   "Selects the mode of the crossover filter." },

        { IDC_LOW_BAND,         "Specifies the end of the low frequency band of the crossover filter." },
        { IDC_HIGH_BAND,        "Specifies the start of the high frequency band of the crossover filter." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void filters_page_t::InitializeControls() noexcept
{
    {
        auto w = (CComboBox) GetDlgItem(IDC_ACOUSTIC_FILTER);

        w.ResetContent();

        for (const auto & x : { L"None", L"A-weighting", L"B-weighting", L"C-weighting", L"D-weighting", L"M-weighting (ITU-R 468)" })
            w.AddString(x);

        w.SetCurSel((int) _State->_WeightingType);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_FREQ_SHIFT)); _NumericEdits.push_back(ne); SetDouble(IDC_FREQ_SHIFT, _State->_FrequencyShift);

        auto w = CUpDownCtrl(GetDlgItem(IDC_FREQ_SHIFT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinFrequencyShift * 100.), (int) (MaxFrequencyShift * 100.));
        w.SetPos32((int)(_State->_FrequencyShift * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_FREQ_TILT)); _NumericEdits.push_back(ne); SetDouble(IDC_FREQ_TILT, _State->_FrequencyTilt);

        auto w = CUpDownCtrl(GetDlgItem(IDC_FREQ_TILT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinFrequencyTilt * 100.), (int) (MaxFrequencyTilt * 100.));
        w.SetPos32((int)(_State->_FrequencyTilt* 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
            { 2,    1000 }, //    10.0
            { 3,    2500 }, //    25.0
            { 4,    5000 }, //    50.0
            { 5,   10000 }, //   100.0
            { 6,  100000 }, //  1000.0
            { 7, 1000000 }, // 10000.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_FREQ_TILT_PIVOT)); _NumericEdits.push_back(ne); SetDouble(IDC_FREQ_TILT_PIVOT, _State->_FrequencyTiltPivot);

        auto w = CUpDownCtrl(GetDlgItem(IDC_FREQ_TILT_PIVOT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinFrequencyTiltPivot * 100.), (int) (MaxFrequencyTiltPivot * 100.));
        w.SetPos32((int)(_State->_FrequencyTiltPivot * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_AMT)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_AMT, _State->_EqualizationAmount);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_AMT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizationAmount * 100.), (int) (MaxEqualizationAmount * 100.));
        w.SetPos32((int)(_State->_EqualizationAmount * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
            { 2,    1000 }, //    10.0
            { 3,    2500 }, //    25.0
            { 4,    5000 }, //    50.0
            { 5,   10000 }, //   100.0
            { 6,  100000 }, //  1000.0
            { 7, 1000000 }, // 10000.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_OFFS, _State->_EqualizationFreqScale);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_OFFS_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizationFreqScale * 100.), (int) (MaxEqualizationFreqScale * 100.));
        w.SetPos32((int)(_State->_EqualizationFreqScale * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
            { 2,    1000 }, //    10.0
            { 3,    2500 }, //    25.0
            { 4,    5000 }, //    50.0
            { 5,   10000 }, //   100.0
            { 6,  100000 }, //  1000.0
            { 7, 1000000 }, // 10000.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_DEPTH)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_DEPTH, _State->_EqualizationDepth);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_DEPTH_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizationDepth * 100.), (int) (MaxEqualizationDepth * 100.));
        w.SetPos32((int)(_State->_EqualizationDepth * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,  1 }, // 0.01
            { 2,  5 }, // 0.05
            { 3, 10 }, // 0.10
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_WT_AMT)); _NumericEdits.push_back(ne); SetDouble(IDC_WT_AMT, _State->_WeightingAmount);

        auto w = CUpDownCtrl(GetDlgItem(IDC_WT_AMT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinWeightingAmount * 100.), (int) (MaxWeightingAmount * 100.));
        w.SetPos32((int)(_State->_WeightingAmount * 100.));
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_CROSSOVER_MODE);

        w.ResetContent();

        for (const auto & x : { L"None", L"1st order", L"4-th order Linkwitz-Riley", })
            w.AddString(x);

        w.SetCurSel((int) _State->_CrossoverMode);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_LOW_BAND)); _NumericEdits.push_back(ne); SetDouble(IDC_LOW_BAND, _State->_LowBand);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_HIGH_BAND)); _NumericEdits.push_back(ne); SetDouble(IDC_HIGH_BAND, _State->_HighBand);
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void filters_page_t::UpdateControls() noexcept
{
    const bool IsPeakMeter    = (_State->_VisualizationType == VisualizationType::PeakMeter);
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);
    const bool IsBitMeter     = (_State->_VisualizationType == VisualizationType::BitMeter);
    const bool IsGoniometer   = (_State->_VisualizationType == VisualizationType::Goniometer);
    const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool SupportsWeighingFilter = !(IsPeakMeter || IsLevelMeter || IsOscilloscope || IsBitMeter || IsTester);

    {
        GetDlgItem(IDC_ACOUSTIC_FILTER).EnableWindow(SupportsWeighingFilter);

        const bool HasFilter = (_State->_WeightingType != WeightingType::None) && SupportsWeighingFilter;

        for (const auto & Iter : { IDC_FREQ_SHIFT, IDC_FREQ_SHIFT, IDC_FREQ_TILT, IDC_FREQ_TILT_PIVOT, IDC_EQ_AMT, IDC_EQ_OFFS, IDC_EQ_DEPTH, IDC_WT_AMT })
            GetDlgItem(Iter).EnableWindow(HasFilter);
    }

    {
        GetDlgItem(IDC_CROSSOVER_MODE).EnableWindow(IsGoniometer);

        const bool HasCrossover = (_State->_CrossoverMode != crossover_filter_t::Mode::None);

        GetDlgItem(IDC_LOW_BAND) .EnableWindow(IsGoniometer && HasCrossover);
        GetDlgItem(IDC_HIGH_BAND).EnableWindow(IsGoniometer && HasCrossover);
    }
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void filters_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void filters_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    const auto cb = (CComboBox) w;

    const int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_ACOUSTIC_FILTER:
        {
            _State->_WeightingType = (WeightingType) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_CROSSOVER_MODE:
        {
            _State->_CrossoverMode = (crossover_filter_t::Mode) SelectedIndex;

            UpdateControls();
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void filters_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        #define ON_EDIT_CHANGE_DOUBLE(x,y) _State->_##x = std::clamp(::_wtof(Text), Min##x, Max##x); CUpDownCtrl(GetDlgItem(y)).SetPos32((int)(_State->_##x * 100.));

        case IDC_FREQ_SHIFT:        { ON_EDIT_CHANGE_DOUBLE(FrequencyShift,         IDC_FREQ_SHIFT); break; }

        case IDC_FREQ_TILT:         { ON_EDIT_CHANGE_DOUBLE(FrequencyTilt,          IDC_FREQ_TILT); break; }
        case IDC_FREQ_TILT_PIVOT:   { ON_EDIT_CHANGE_DOUBLE(FrequencyTiltPivot,     IDC_FREQ_TILT_PIVOT); break; }

        case IDC_EQ_AMT:            { ON_EDIT_CHANGE_DOUBLE(EqualizationAmount,     IDC_EQ_AMT); break; }
        case IDC_EQ_OFFS:           { ON_EDIT_CHANGE_DOUBLE(EqualizationFreqScale,  IDC_EQ_OFFS); break; }
        case IDC_EQ_DEPTH:          { ON_EDIT_CHANGE_DOUBLE(EqualizationDepth,      IDC_EQ_DEPTH); break; }

        case IDC_WT_AMT:            { ON_EDIT_CHANGE_DOUBLE(WeightingAmount,        IDC_WT_AMT); break; }

        case IDC_LOW_BAND:          { ON_EDIT_CHANGE_DOUBLE(LowBand,                IDC_LOW_BAND); break; }
        case IDC_HIGH_BAND:         { ON_EDIT_CHANGE_DOUBLE(HighBand,               IDC_HIGH_BAND); break; }

        #undef ON_EDIT_CHANGE_DOUBLE
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void filters_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        case IDC_FREQ_SHIFT:        { SetDouble(id, _State->_FrequencyShift); break; }
        case IDC_FREQ_TILT:         { SetDouble(id, _State->_FrequencyTilt); break; }
        case IDC_FREQ_TILT_PIVOT:   { SetDouble(id, _State->_FrequencyTiltPivot); break; }
        case IDC_EQ_AMT:            { SetDouble(id, _State->_EqualizationAmount); break; }
        case IDC_EQ_OFFS:           { SetDouble(id, _State->_EqualizationFreqScale); break; }
        case IDC_EQ_DEPTH:          { SetDouble(id, _State->_EqualizationDepth); break; }
        case IDC_WT_AMT:            { SetDouble(id, _State->_WeightingAmount); break; }

        case IDC_LOW_BAND:          { SetDouble(id, _State->_LowBand); break; }
        case IDC_HIGH_BAND:         { SetDouble(id, _State->_HighBand); break; }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT filters_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = ConfigurationChanges::All;

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_FREQ_SHIFT_SPIN:
        {
            if (!SetProperty(_State->_FrequencyShift, ClampNewSpinPosition(nmud, MinFrequencyShift, MaxFrequencyShift, 100.)))
                return -1;

            SetDouble(IDC_FREQ_SHIFT, _State->_FrequencyShift);
            break;
        }

        case IDC_FREQ_TILT_SPIN:
        {
            if (!SetProperty(_State->_FrequencyTilt, ClampNewSpinPosition(nmud, MinFrequencyTilt, MaxFrequencyTilt, 100.)))
                return -1;

            SetDouble(IDC_FREQ_TILT, _State->_FrequencyTilt);
            break;
        }

        case IDC_FREQ_TILT_PIVOT_SPIN:
        {
            if (!SetProperty(_State->_FrequencyTiltPivot, ClampNewSpinPosition(nmud, MinFrequencyTiltPivot, MaxFrequencyTiltPivot, 100.)))
                return -1;

            SetDouble(IDC_FREQ_TILT_PIVOT, _State->_FrequencyTiltPivot);
            break;
        }

        case IDC_EQ_AMT_SPIN:
        {
            if (!SetProperty(_State->_EqualizationAmount, ClampNewSpinPosition(nmud, MinEqualizationAmount, MaxEqualizationAmount, 100.)))
                return -1;

            SetDouble(IDC_EQ_AMT, _State->_EqualizationAmount);
            break;
        }

        case IDC_EQ_OFFS_SPIN:
        {
            if (!SetProperty(_State->_EqualizationFreqScale, ClampNewSpinPosition(nmud, MinEqualizationFreqScale, MaxEqualizationFreqScale, 100.)))
                return -1;

            SetDouble(IDC_EQ_OFFS, _State->_EqualizationFreqScale);
            break;
        }

        case IDC_EQ_DEPTH_SPIN:
        {
            if (!SetProperty(_State->_EqualizationDepth, ClampNewSpinPosition(nmud, MinEqualizationDepth, MaxEqualizationDepth, 100.)))
                return -1;

            SetDouble(IDC_EQ_DEPTH, _State->_EqualizationDepth);
            break;
        }

        case IDC_WT_AMT_SPIN:
        {
            if (!SetProperty(_State->_WeightingAmount, ClampNewSpinPosition(nmud, MinWeightingAmount, MaxWeightingAmount, 100.)))
                return -1;

            SetDouble(IDC_WT_AMT, _State->_WeightingAmount);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}
