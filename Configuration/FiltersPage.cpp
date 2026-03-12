
/** $VER: FiltersPage.cpp (2026.03.08) P. Stuer - Implements a configuration dialog page. **/

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

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_ACOUSTIC_FILTER, "Selects the Weighting filter type that will be applied." },

        { IDC_SLOPE_FN_OFFS, "Slope function offset expressed in sample rate / FFT size in samples" },
        { IDC_SLOPE, "Frequency slope offset" },
        { IDC_SLOPE_OFFS, "Frequency slope in dB per octave" },

        { IDC_EQ_AMT, "Equalization amount" },
        { IDC_EQ_DEPTH, "Equalization offset" },
        { IDC_EQ_OFFS, "Equalization depth" },

        { IDC_WT_AMT, "Weighting amount" },
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

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_SLOPE_FN_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE_FN_OFFS, _State->_SlopeFunctionOffset);

        auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_FN_OFFS_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinSlopeFunctionOffset * 100.), (int) (MaxSlopeFunctionOffset * 100.));
        w.SetPos32((int)(_State->_SlopeFunctionOffset * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_SLOPE)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE, _State->_Slope);

        auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinSlope * 100.), (int) (MaxSlope * 100.));
        w.SetPos32((int)(_State->_Slope* 100.));
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

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_SLOPE_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE_OFFS, _State->_SlopeOffset);

        auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_OFFS_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinSlopeOffset * 100.), (int) (MaxSlopeOffset * 100.));
        w.SetPos32((int)(_State->_SlopeOffset * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     100 }, //     1.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_AMT)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_AMT, _State->_EqualizeAmount);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_AMT_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizeAmount * 100.), (int) (MaxEqualizeAmount * 100.));
        w.SetPos32((int)(_State->_EqualizeAmount * 100.));
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

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_OFFS, _State->_EqualizeOffset);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_OFFS_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizeOffset * 100.), (int) (MaxEqualizeOffset * 100.));
        w.SetPos32((int)(_State->_EqualizeOffset * 100.));
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

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_EQ_DEPTH)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_DEPTH, _State->_EqualizeDepth);

        auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_DEPTH_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinEqualizeDepth * 100.), (int) (MaxEqualizeDepth * 100.));
        w.SetPos32((int)(_State->_EqualizeDepth * 100.));
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
    const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool SupportsFilter = !(IsPeakMeter || IsLevelMeter || IsOscilloscope || IsBitMeter || IsTester);

    GetDlgItem(IDC_ACOUSTIC_FILTER).EnableWindow(SupportsFilter);

    const bool HasFilter = (_State->_WeightingType != WeightingType::None) && SupportsFilter;

    for (const auto & Iter : { IDC_SLOPE_FN_OFFS, IDC_SLOPE_FN_OFFS, IDC_SLOPE, IDC_SLOPE_OFFS, IDC_EQ_AMT, IDC_EQ_OFFS, IDC_EQ_DEPTH, IDC_WT_AMT })
        GetDlgItem(Iter).EnableWindow(HasFilter);
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

        case IDC_SLOPE_FN_OFFS: { ON_EDIT_CHANGE_DOUBLE(SlopeFunctionOffset, IDC_SLOPE_FN_OFFS); break; }
        case IDC_SLOPE:         { ON_EDIT_CHANGE_DOUBLE(Slope, IDC_SLOPE); break; }
        case IDC_SLOPE_OFFS:    { ON_EDIT_CHANGE_DOUBLE(SlopeOffset, IDC_SLOPE_OFFS); break; }
        case IDC_EQ_AMT:        { ON_EDIT_CHANGE_DOUBLE(EqualizeAmount, IDC_EQ_AMT); break; }
        case IDC_EQ_OFFS:       { ON_EDIT_CHANGE_DOUBLE(EqualizeOffset, IDC_EQ_OFFS); break; }
        case IDC_EQ_DEPTH:      { ON_EDIT_CHANGE_DOUBLE(EqualizeDepth, IDC_EQ_DEPTH); break; }
        case IDC_WT_AMT:        { ON_EDIT_CHANGE_DOUBLE(WeightingAmount, IDC_WT_AMT); break; }
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

        case IDC_SLOPE_FN_OFFS: { SetDouble(id, _State->_SlopeFunctionOffset); break; }
        case IDC_SLOPE:         { SetDouble(id, _State->_Slope); break; }
        case IDC_SLOPE_OFFS:    { SetDouble(id, _State->_SlopeOffset); break; }
        case IDC_EQ_AMT:        { SetDouble(id, _State->_EqualizeAmount); break; }
        case IDC_EQ_OFFS:       { SetDouble(id, _State->_EqualizeOffset); break; }
        case IDC_EQ_DEPTH:      { SetDouble(id, _State->_EqualizeDepth); break; }
        case IDC_WT_AMT:        { SetDouble(id, _State->_WeightingAmount); break; }
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

        case IDC_SLOPE_FN_OFFS_SPIN:
        {
            if (!SetProperty(_State->_SlopeFunctionOffset, ClampNewSpinPosition(nmud, MinSlopeFunctionOffset, MaxSlopeFunctionOffset, 100.)))
                return -1;

            SetDouble(IDC_SLOPE_FN_OFFS, _State->_SlopeFunctionOffset);
            break;
        }

        case IDC_SLOPE_SPIN:
        {
            if (!SetProperty(_State->_Slope, ClampNewSpinPosition(nmud, MinSlope, MaxSlope, 100.)))
                return -1;

            SetDouble(IDC_SLOPE, _State->_Slope);
            break;
        }

        case IDC_SLOPE_OFFS_SPIN:
        {
            if (!SetProperty(_State->_SlopeOffset, ClampNewSpinPosition(nmud, MinSlopeOffset, MaxSlopeOffset, 100.)))
                return -1;

            SetDouble(IDC_SLOPE_OFFS, _State->_SlopeOffset);
            break;
        }

        case IDC_EQ_AMT_SPIN:
        {
            if (!SetProperty(_State->_EqualizeAmount, ClampNewSpinPosition(nmud, MinEqualizeAmount, MaxEqualizeAmount, 100.)))
                return -1;

            SetDouble(IDC_EQ_AMT, _State->_EqualizeAmount);
            break;
        }

        case IDC_EQ_OFFS_SPIN:
        {
            if (!SetProperty(_State->_EqualizeOffset, ClampNewSpinPosition(nmud, MinEqualizeOffset, MaxEqualizeOffset, 100.)))
                return -1;

            SetDouble(IDC_EQ_OFFS, _State->_EqualizeOffset);
            break;
        }

        case IDC_EQ_DEPTH_SPIN:
        {
            if (!SetProperty(_State->_EqualizeDepth, ClampNewSpinPosition(nmud, MinEqualizeDepth, MaxEqualizeDepth, 100.)))
                return -1;

            SetDouble(IDC_EQ_DEPTH, _State->_EqualizeDepth);
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
