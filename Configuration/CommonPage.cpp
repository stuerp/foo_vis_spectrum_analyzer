
/** $VER: CommonPage.cpp (2026.03.08) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "CommonPage.h"
#include "Support.h"
#include "Log.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL common_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_SMOOTHING_METHOD, "Determines how the spectrum coefficients and the peak meter values are smoothed." },
        { IDC_SMOOTHING_FACTOR, "Determines the strength of the smoothing." },

        { IDC_SHOW_TOOLTIPS, "Enable the check box to see a tooltip with the center frequency and when appropriate, the name of the note, of the frequency band." },
        { IDC_SUPPRESS_MIRROR_IMAGE, "Prevents the mirror image of the spectrum (anything above the Nyquist frequency) from being rendered." },
        { IDC_VISUALIZE_DURING_PAUSE, "Continue visualization when playback is paused." },

        // Artwork
        { IDC_NUM_ARTWORK_COLORS, "Max. number of colors to select from the artwork. The colors can be used in a dynamic gradient." },
        { IDC_LIGHTNESS_THRESHOLD, "Determines when a color is considered light. Expressed as a percentage of whiteness." },
        { IDC_COLOR_ORDER, "Determines how to sort the colors selected from the artwork." },

        { IDC_ARTWORK_BACKGROUND, "Renders artwork on the graph background." },
        { IDC_ARTWORK_TYPE, "Specifies which artwork will be shown on the graph background." },

        { IDC_FIT_MODE, "Determines how over- and undersized artwork is rendered." },
        { IDC_FIT_WINDOW, "Use the component window size instead of the client area of the graph to fit the artwork." },

        { IDC_ARTWORK_OPACITY, "Determines the opacity of the artwork when displayed." },
        { IDC_ARTWORK_FILE_PATH, "A fully-qualified file path or a foobar2000 script that returns the file path of an image to display on the graph background" },

        // Component
        { IDC_LOG_LEVEL, "Sets the verbosity of the log information that gets written to the console." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void common_page_t::InitializeControls() noexcept
{
    {
        auto w = (CComboBox) GetDlgItem(IDC_SMOOTHING_METHOD);

        w.ResetContent();

        for (const auto & x : { L"None", L"Average", L"Peak" })
            w.AddString(x);

        w.SetCurSel((int) _State->_SmoothingMethod);

        SetDouble(IDC_SMOOTHING_FACTOR, _State->_SmoothingFactor, 0, 2);
    }

    {
        SendDlgItemMessageW(IDC_SHOW_TOOLTIPS, BM_SETCHECK, _State->_ShowToolTipsAlways);
        SendDlgItemMessageW(IDC_SUPPRESS_MIRROR_IMAGE, BM_SETCHECK, _State->_SuppressMirrorImage);
        SendDlgItemMessageW(IDC_VISUALIZE_DURING_PAUSE, BM_SETCHECK, _State->_VisualizeDuringPause);
    }

    // Artwork
    {
        SendDlgItemMessageW(IDC_ARTWORK_BACKGROUND, BM_SETCHECK, _State->_ShowArtworkOnBackground);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_FIT_MODE);

        w.ResetContent();

        for (const auto & x : { L"Free", L"Fit big", L"Fit width", L"Fit height", L"Fill" })
            w.AddString(x);

        w.SetCurSel((int) _State->_FitMode);
    }
    {
        SendDlgItemMessageW(IDC_FIT_WINDOW, BM_SETCHECK, _State->_FitWindow);
    }
    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_ARTWORK_OPACITY)); _NumericEdits.push_back(ne); SetInteger(IDC_ARTWORK_OPACITY, (int64_t) (_State->_ArtworkOpacity * 100.f));

        auto w = CUpDownCtrl(GetDlgItem(IDC_ARTWORK_OPACITY_SPIN));

        w.SetRange32((int) (MinArtworkOpacity * 100.f), (int) (MaxArtworkOpacity * 100.f));
        w.SetPos32((int) (_State->_ArtworkOpacity * 100.f));
        w.SetAccel(_countof(Accel), Accel);
    }
    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  2 },
            { 3,  8 },
            { 4, 16 },
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_NUM_ARTWORK_COLORS)); _NumericEdits.push_back(ne); SetInteger(IDC_NUM_ARTWORK_COLORS, _State->_NumArtworkColors);

        auto w = CUpDownCtrl(GetDlgItem(IDC_NUM_ARTWORK_COLORS_SPIN));

        w.SetRange32((int) (MinArtworkColors), (int) (MaxArtworkColors));
        w.SetPos32((int) (_State->_NumArtworkColors));
        w.SetAccel(_countof(Accel), Accel);
    }
    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_LIGHTNESS_THRESHOLD)); _NumericEdits.push_back(ne); SetInteger(IDC_LIGHTNESS_THRESHOLD, (int64_t) (_State->_LightnessThreshold * 100.f));

        auto w = CUpDownCtrl(GetDlgItem(IDC_LIGHTNESS_THRESHOLD_SPIN));

        w.SetRange32((int) (MinLightnessThreshold * 100.f), (int) (MaxLightnessThreshold * 100.f));
        w.SetPos32((int) (_State->_LightnessThreshold * 100.f));
        w.SetAccel(_countof(Accel), Accel);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_ORDER);

        w.ResetContent();

        for (const auto & x : { L"None", L"Increasing hue", L"Decreasing hue", L"Increasing lightness", L"Decreasing lightness", L"Increasing saturation", L"Decreasing saturation" })
            w.AddString(x);

        w.SetCurSel((int) _State->_ColorOrder);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_ARTWORK_TYPE);

        w.ResetContent();

        for (const auto & x : { L"Front", L"Back", L"Disc", L"Icon", L"Artist" })
            w.AddString(x);

        w.SetCurSel((int) _State->_ArtworkType);
    }
    {
        GetDlgItem(IDC_ARTWORK_FILE_PATH).SetWindowTextW(_State->_ArtworkFilePath.c_str());
    }

    // Component
    {
        auto w = (CComboBox) GetDlgItem(IDC_LOG_LEVEL);

        w.ResetContent();

        int i = -1;

        for (const auto & Text : { L"Never", L"Fatal", L"Error", L"Warn", L"Info", L"Debug", L"Trace", L"Always", })
        {
            w.AddString(Text);

            if (++i == CfgLogLevel)
                w.SetCurSel((int) i);
        }
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void common_page_t::UpdateControls() noexcept
{
    const bool IsPeakMeter    = (_State->_VisualizationType == VisualizationType::PeakMeter);
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);
    const bool IsBitMeter     = (_State->_VisualizationType == VisualizationType::BitMeter);
    const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool SupportsFFT = !(IsPeakMeter || IsLevelMeter || IsOscilloscope || IsBitMeter || IsTester);

    for (const auto ID : { IDC_SMOOTHING_METHOD, IDC_SHOW_TOOLTIPS, IDC_SUPPRESS_MIRROR_IMAGE })
        GetDlgItem(ID).EnableWindow(SupportsFFT);

    // Smoothing
    GetDlgItem(IDC_SMOOTHING_FACTOR).EnableWindow(SupportsFFT && (_State->_SmoothingMethod != SmoothingMethod::None));

    // Artwork
    const bool SupportsArtworkOnBackground = !(IsPeakMeter || IsLevelMeter || IsOscilloscope);

    GetDlgItem(IDC_ARTWORK_BACKGROUND).EnableWindow(SupportsArtworkOnBackground);

    for (const auto ID :
    {
        IDC_FIT_MODE, IDC_FIT_WINDOW,
        IDC_ARTWORK_OPACITY, IDC_ARTWORK_OPACITY_SPIN,
        IDC_ARTWORK_FILE_PATH
    })
        GetDlgItem(ID).EnableWindow(SupportsArtworkOnBackground && _State->_ShowArtworkOnBackground);
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void common_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void common_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
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

        case IDC_SMOOTHING_METHOD:
        {
            _State->_SmoothingMethod = (SmoothingMethod) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_ORDER:
        {
            _State->_ColorOrder = (ColorOrder) SelectedIndex;
            break;
        }

        case IDC_ARTWORK_TYPE:
        {
            _State->_ArtworkType = (ArtworkType) SelectedIndex;
            break;
        }

        case IDC_FIT_MODE:
        {
            _State->_FitMode = (FitMode) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_LOG_LEVEL:
        {
            CfgLogLevel = (int64_t) SelectedIndex;

            Log.SetLevel((LogLevel) CfgLogLevel.get());

            return;
        }

    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void common_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
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

        // Spectrum Smoothing
        case IDC_SMOOTHING_FACTOR:
        {
            if (!SetProperty(_State->_SmoothingFactor, std::clamp(::_wtof(Text), MinSmoothingFactor, MaxSmoothingFactor)))
                return;

            break;
        }

        // Artwork
        case IDC_NUM_ARTWORK_COLORS:
        {
            if (!SetProperty(_State->_NumArtworkColors, std::clamp((uint32_t) ::_wtoi(Text), MinArtworkColors, MaxArtworkColors)))
                return;

            break;
        }

        case IDC_LIGHTNESS_THRESHOLD:
        {
            if (!SetProperty(_State->_LightnessThreshold, (FLOAT) std::clamp(::_wtof(Text) / 100.f, MinLightnessThreshold, MaxLightnessThreshold)))
                return;

            break;
        }

        // Artwork Background
        case IDC_ARTWORK_OPACITY:
        {
            if (!SetProperty(_State->_ArtworkOpacity, (FLOAT) std::clamp(::_wtof(Text) / 100.f, MinArtworkOpacity, MaxArtworkOpacity)))
                return;

            break;
        }

        case IDC_ARTWORK_FILE_PATH:
        {
            _State->_ArtworkFilePath = Text;
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void common_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        // Spectrum smoothing
        case IDC_SMOOTHING_FACTOR:
        {
            SetDouble(id, _State->_SmoothingFactor, 0, 2);
            break;
        }

        // Artwork
        case IDC_NUM_ARTWORK_COLORS:
        {
            SetInteger(id, _State->_NumArtworkColors); break;
        }

        case IDC_LIGHTNESS_THRESHOLD:
        {
            SetInteger(id, (int64_t) (_State->_LightnessThreshold * 100.f)); break;
        }

        // Artwork Background
        case IDC_ARTWORK_OPACITY:
        {
            SetInteger(id, (int64_t) (_State->_ArtworkOpacity * 100.f)); break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void common_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        case IDC_SHOW_TOOLTIPS:
        {
            _State->_ShowToolTipsAlways = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            ChangedSettings = ConfigurationChanges::RenderLoop;
            break;
        }

        case IDC_SUPPRESS_MIRROR_IMAGE:
        {
            _State->_SuppressMirrorImage = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_VISUALIZE_DURING_PAUSE:
        {
            _State->_VisualizeDuringPause = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            ChangedSettings = ConfigurationChanges::RenderLoop;
            break;
        }

        case IDC_ARTWORK_BACKGROUND:
        {
            _State->_ShowArtworkOnBackground = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();

            ChangedSettings = ConfigurationChanges::RenderLoop;
            break;
        }

        case IDC_FIT_WINDOW:
        {
            _State->_FitWindow = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT common_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = ConfigurationChanges::All;

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_NUM_ARTWORK_COLORS_SPIN:
        {
            if (!SetProperty(_State->_NumArtworkColors, (uint32_t) ClampNewSpinPosition(nmud, MinArtworkColors, MaxArtworkColors)))
                return -1;

            SetInteger(IDC_NUM_ARTWORK_COLORS, _State->_NumArtworkColors);
            break;
        }

        case IDC_LIGHTNESS_THRESHOLD_SPIN:
        {
            if (!SetProperty(_State->_LightnessThreshold, (FLOAT) ClampNewSpinPosition(nmud, MinLightnessThreshold, MaxLightnessThreshold, 100.)))
                return -1;

            SetInteger(IDC_LIGHTNESS_THRESHOLD, (int64_t) (_State->_LightnessThreshold * 100.f));
            break;
        }

        case IDC_ARTWORK_OPACITY_SPIN:
        {
            if (!SetProperty(_State->_ArtworkOpacity, (FLOAT) ClampNewSpinPosition(nmud, MinArtworkOpacity, MaxArtworkOpacity, 100.)))
                return -1;

            SetInteger(IDC_ARTWORK_OPACITY, (int64_t) (_State->_ArtworkOpacity * 100.f));
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}
