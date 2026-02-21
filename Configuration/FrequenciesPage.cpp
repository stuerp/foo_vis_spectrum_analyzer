
/** $VER: FrequenciesPage.cpp (2026.02.20) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "FrequenciesPage.h"
#include "Support.h"
#include "Log.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL frequencies_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_DISTRIBUTION, "Determines how the frequencies are distributed" },
        { IDC_NUM_BANDS, "Determines how many frequency bands are used" },

        { IDC_LO_FREQUENCY, "Center frequency of the first band" },
        { IDC_HI_FREQUENCY, "Center frequency of the last band" },

        { IDC_MIN_NOTE, "Note that determines the center frequency of the first band" },
        { IDC_MAX_NOTE, "Note that determines the center frequency of the last band" },

        { IDC_BANDS_PER_OCTAVE, "Number of frequency bands per octave" },

        { IDC_PITCH, "Frequency of the tuning pitch" },
        { IDC_TRANSPOSE, "Determines how many semitones the frequencies will be transposed." },

        { IDC_SCALING_FUNCTION, "Determines which function is used to scale the frequencies." },
        { IDC_SKEW_FACTOR, "Affects any adjustable frequency scaling functions like hyperbolic sine and nth root. Higher values mean a more linear spectrum." },
        { IDC_BANDWIDTH, "Distance between the low and high frequency boundaries for each frequency band" },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void frequencies_page_t::InitializeControls() noexcept
{
    {
        auto w = (CComboBox) GetDlgItem(IDC_DISTRIBUTION);

        w.ResetContent();

        for (const auto & x : { L"Linear", L"Octaves", L"AveePlayer" })
            w.AddString(x);

        w.SetCurSel((int) _State->_FrequencyDistribution);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,  5 },
            { 2, 10 },
            { 3, 50 },
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_NUM_BANDS)); _NumericEdits.push_back(ne); SetInteger(IDC_NUM_BANDS, (int64_t) _State->_BandCount);

        auto w = CUpDownCtrl(GetDlgItem(IDC_NUM_BANDS_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32(MinBands, MaxBands);
        w.SetPos32((int) _State->_BandCount);
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

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_LO_FREQUENCY)); _NumericEdits.push_back(ne); SetDouble(IDC_LO_FREQUENCY, _State->_LoFrequency);

            auto w = CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
            w.SetPos32((int)(_State->_LoFrequency * 100.));
        }

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_HI_FREQUENCY)); _NumericEdits.push_back(ne); SetDouble(IDC_HI_FREQUENCY, _State->_HiFrequency);

            auto w = CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
            w.SetPos32((int)(_State->_HiFrequency * 100.));
        }
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_MIN_NOTE)); _NumericEdits.push_back(ne); SetNote(IDC_MIN_NOTE, _State->_MinNote);

        auto w = CUpDownCtrl(GetDlgItem(IDC_MIN_NOTE_SPIN));

        w.SetRange32(MinNote, MaxNote);
        w.SetPos32((int) _State->_MinNote);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_MAX_NOTE)); _NumericEdits.push_back(ne); SetNote(IDC_MAX_NOTE, _State->_MaxNote);

        auto w = CUpDownCtrl(GetDlgItem(IDC_MAX_NOTE_SPIN));

        w.SetRange32(MinNote, MaxNote);
        w.SetPos32((int) _State->_MaxNote);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BANDS_PER_OCTAVE)); _NumericEdits.push_back(ne); SetInteger(IDC_BANDS_PER_OCTAVE, (int64_t) _State->_BandsPerOctave);

        auto w = CUpDownCtrl(GetDlgItem(IDC_BANDS_PER_OCTAVE_SPIN));

        w.SetRange32(MinBandsPerOctave, MaxBandsPerOctave);
        w.SetPos32((int) _State->_BandsPerOctave);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,    50 }, //   0.5
            { 2,   100 }, //   1.0
            { 3,  1000 }, //  10.0
            { 4,  2500 }, //  25.0
            { 5,  5000 }, //  50.0
            { 6, 10000 }, // 100.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_PITCH)); _NumericEdits.push_back(ne); SetDouble(IDC_PITCH, _State->_TuningPitch);

        auto w = CUpDownCtrl(GetDlgItem(IDC_PITCH_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinPitch * 100.), (int) (MaxPitch * 100.));
        w.SetPos32((int) (_State->_TuningPitch * 100.));
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_TRANSPOSE)); _NumericEdits.push_back(ne); SetInteger(IDC_TRANSPOSE, _State->_Transpose);

        auto w = CUpDownCtrl(GetDlgItem(IDC_TRANSPOSE_SPIN));

        w.SetRange32(MinTranspose, MaxTranspose);
        w.SetPos32(_State->_Transpose);
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_SCALING_FUNCTION);

        w.ResetContent();

        for (const auto & x : { L"Linear", L"Logarithmic", L"Shifted Logarithmic", L"Mel", L"Bark", L"Adjustable Bark", L"ERB", L"Cams", L"Hyperbolic Sine", L"Nth Root", L"Negative Exponential", L"Period" })
            w.AddString(x);

        w.SetCurSel((int) _State->_ScalingFunction);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,    1 }, // 0.01
            { 2,    5 }, // 0.05
            { 3,   10 }, // 0.10
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_SKEW_FACTOR)); _NumericEdits.push_back(ne); SetDouble(IDC_SKEW_FACTOR, _State->_SkewFactor);

        auto w = CUpDownCtrl(GetDlgItem(IDC_SKEW_FACTOR_SPIN));

        w.SetAccel(_countof(Accel), Accel);

        w.SetRange32((int) (MinSkewFactor * 100.), (int) (MaxSkewFactor * 100.));
        w.SetPos32((int) (_State->_SkewFactor * 100.));
    }

    {
        UDACCEL Accel[] =
        {
            { 1,   1 }, //  0.1
            { 2,   5 }, //  0.5
            { 3,  10 }, //  1.0
            { 4,  50 }, //  5.0
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BANDWIDTH)); _NumericEdits.push_back(ne); SetDouble(IDC_BANDWIDTH, _State->_Bandwidth, 0, 1);

        auto w = CUpDownCtrl(GetDlgItem(IDC_BANDWIDTH_SPIN));

        w.SetRange32((int) (MinBandwidth * 10.), (int) (MaxBandwidth * 10.));
        w.SetPos32((int) (_State->_Bandwidth * 10.));
        w.SetAccel(_countof(Accel), Accel);
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void frequencies_page_t::UpdateControls() noexcept
{
    const bool IsPeakMeter    = (_State->_VisualizationType == VisualizationType::PeakMeter);
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);

    const bool SupportsFrequencies = !(IsPeakMeter && IsLevelMeter && IsOscilloscope);

    if (SupportsFrequencies)
    {
        const bool IsOctaves = (_State->_FrequencyDistribution == FrequencyDistribution::Octaves);
    //  const bool IsAveePlayer = (_State->_FrequencyDistribution == FrequencyDistribution::AveePlayer);

        GetDlgItem(IDC_NUM_BANDS).EnableWindow(!IsOctaves);
        GetDlgItem(IDC_LO_FREQUENCY).EnableWindow(!IsOctaves);
        GetDlgItem(IDC_HI_FREQUENCY).EnableWindow(!IsOctaves);

    //  GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(!IsOctaves && !IsAveePlayer);
    //  GetDlgItem(IDC_SKEW_FACTOR).EnableWindow(!IsOctaves);

        for (const auto & Iter : { IDC_MIN_NOTE, IDC_MAX_NOTE, IDC_BANDS_PER_OCTAVE, IDC_PITCH, IDC_TRANSPOSE, })
            GetDlgItem(Iter).EnableWindow(IsOctaves);
    }
    else
    {
        for (const auto & Iter :
        {
            IDC_DISTRIBUTION,
            IDC_NUM_BANDS, IDC_NUM_BANDS_SPIN,
            IDC_LO_FREQUENCY, IDC_LO_FREQUENCY_SPIN, IDC_HI_FREQUENCY, IDC_HI_FREQUENCY_SPIN,
            IDC_MIN_NOTE, IDC_MIN_NOTE_SPIN, IDC_MAX_NOTE, IDC_MAX_NOTE_SPIN,
            IDC_BANDS_PER_OCTAVE, IDC_BANDS_PER_OCTAVE_SPIN,
            IDC_PITCH, IDC_PITCH_SPIN,
            IDC_TRANSPOSE, IDC_TRANSPOSE_SPIN,
            IDC_SCALING_FUNCTION,
            IDC_SKEW_FACTOR, IDC_SKEW_FACTOR_SPIN,
            IDC_BANDWIDTH, IDC_BANDWIDTH_SPIN,
        })
            GetDlgItem(Iter).EnableWindow(SupportsFrequencies);
    }
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void frequencies_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void frequencies_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    const auto cb = (CComboBox) w;

    int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_DISTRIBUTION:
        {
            _State->_FrequencyDistribution = (FrequencyDistribution) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_SCALING_FUNCTION:
        {
            _State->_ScalingFunction = (ScalingFunction) SelectedIndex;
            break;
        }

        case IDC_SUMMATION_METHOD:
        {
            _State->_SummationMethod = (SummationMethod) SelectedIndex;
            break;
        }

        case IDC_SMOOTHING_METHOD:
        {
            _State->_SmoothingMethod = (SmoothingMethod) SelectedIndex;
            break;
        }

    }

    if (ChangedSettings != Settings::None)
        ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void frequencies_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    auto ChangedSettings = Settings::All;

    WCHAR Text[MAX_PATH];

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        case IDC_NUM_BANDS:
        {
            if (!SetProperty(_State->_BandCount, (size_t) std::clamp(::_wtoi(Text), MinBands, MaxBands)))
                return;

            break;
        }

        case IDC_LO_FREQUENCY:
        {
            _State->_LoFrequency = std::min(std::clamp(::_wtof(Text), MinFrequency, MaxFrequency), _State->_HiFrequency);

            CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN)).SetPos32((int)(_State->_LoFrequency * 100.));
            break;
        }

        case IDC_HI_FREQUENCY:
        {
            _State->_HiFrequency = std::max(std::clamp(::_wtof(Text), MinFrequency, MaxFrequency), _State->_LoFrequency);

            CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN)).SetPos32((int)(_State->_HiFrequency * 100.));
            break;
        }

        case IDC_PITCH:
        {
            _State->_TuningPitch = std::clamp(::_wtof(Text), MinPitch, MaxPitch);
            break;
        }
    }

    if (ChangedSettings != Settings::None)
        ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void frequencies_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        case IDC_NUM_BANDS:
        {
            SetInteger(id, (int64_t) _State->_BandCount); break;
        }

        case IDC_LO_FREQUENCY:
        {
            SetDouble(id, _State->_LoFrequency); break;
        }

        case IDC_HI_FREQUENCY:
        {
            SetDouble(id, _State->_HiFrequency); break;
        }

        case IDC_PITCH:
        {
            SetDouble(id, _State->_TuningPitch); break;
        }

        case IDC_SKEW_FACTOR:
        {
            SetDouble(id, _State->_SkewFactor); break;
        }

        case IDC_BANDWIDTH:
        {
            SetDouble(id, _State->_Bandwidth, 0, 1); break;
        }
    }

    if (ChangedSettings != Settings::None)
        ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT frequencies_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = Settings::All;

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_NUM_BANDS_SPIN:
        {
            _State->_BandCount = (size_t) ClampNewSpinPosition(nmud, MinBands, MaxBands);
            SetInteger(IDC_NUM_BANDS, (int64_t) _State->_BandCount);
            break;
        }

        case IDC_LO_FREQUENCY_SPIN:
        {
            _State->_LoFrequency = std::min(ClampNewSpinPosition(nmud, MinFrequency, MaxFrequency, 100.), _State->_HiFrequency);
            SetDouble(IDC_LO_FREQUENCY, _State->_LoFrequency);
            break;
        }

        case IDC_HI_FREQUENCY_SPIN:
        {
            _State->_HiFrequency = std::max(ClampNewSpinPosition(nmud, MinFrequency, MaxFrequency, 100.), _State->_LoFrequency);
            SetDouble(IDC_HI_FREQUENCY, _State->_HiFrequency);
            break;
        }

        case IDC_MIN_NOTE_SPIN:
        {
            _State->_MinNote = std::min((uint32_t) ClampNewSpinPosition(nmud, MinNote, MaxNote), _State->_MaxNote);
            SetNote(IDC_MIN_NOTE, _State->_MinNote);
            break;
        }

        case IDC_MAX_NOTE_SPIN:
        {
            _State->_MaxNote = std::max((uint32_t) ClampNewSpinPosition(nmud, MinNote, MaxNote), _State->_MinNote);
            SetNote(IDC_MAX_NOTE, _State->_MaxNote);
            break;
        }

        case IDC_BANDS_PER_OCTAVE_SPIN:
        {
            _State->_BandsPerOctave = (uint32_t) ClampNewSpinPosition(nmud, MinBandsPerOctave, MaxBandsPerOctave);
            break;
        }

        case IDC_PITCH_SPIN:
        {
            _State->_TuningPitch = ClampNewSpinPosition(nmud, MinPitch, MaxPitch, 100.);
            SetDouble(IDC_PITCH, _State->_TuningPitch);
            break;
        }

        case IDC_TRANSPOSE_SPIN:
        {
            _State->_Transpose = ClampNewSpinPosition(nmud, MinTranspose, MaxTranspose);
            break;
        }

        case IDC_SKEW_FACTOR_SPIN:
        {
            _State->_SkewFactor = ClampNewSpinPosition(nmud, MinSkewFactor, MaxSkewFactor, 100.);
            SetDouble(IDC_SKEW_FACTOR, _State->_SkewFactor);
            break;
        }

        case IDC_BANDWIDTH_SPIN:
        {
            _State->_Bandwidth = ClampNewSpinPosition(nmud, MinBandwidth, MaxBandwidth, 10.);
            SetDouble(IDC_BANDWIDTH, _State->_Bandwidth, 0, 1);
            break;
        }
    }

    if (ChangedSettings != Settings::None)
        ConfigurationChanged(ChangedSettings);

    return 0;
}
