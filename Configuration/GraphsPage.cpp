
/** $VER: GraphsPage.cpp (2026.03.11) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "GraphsPage.h"
#include "Support.h"
#include "Log.h"

// Display names for the audio_chunk channel bits.
static const WCHAR * const ChannelNames[] =
{
    L"Front Left", L"Front Right",
    L"Front Center",
    L"Low Frequency Effects",
    L"Back Left", L"Back Right",
    L"Front Left of Center", L"Front Right of Center",
    L"Back Center",
    L"Side Left", L"Side Right",
    L"Top Center",
    L"Top Front Left", L"Top Front Center", L"Top Front Right",
    L"Top Back Left", L"Top Back Center", L"Top Back Right",
};

/// <summary>
/// Initializes the page.
/// </summary>
BOOL graphs_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_GRAPH_SETTINGS, "Shows the list of graphs." },

        { IDC_ADD_GRAPH, "Adds a graph." },
        { IDC_REMOVE_GRAPH, "Removes the selected graph." },

        { IDC_VERTICAL_LAYOUT, "Enable to stack the graphs vertically instead of horizontally." },

        { IDC_GRAPH_DESCRIPTION, "Describes the configuration of this graph." },

        { IDC_HORIZONTAL_ALIGNMENT, "Determines how the visualization gets horizontally aligned in the graph area." },

        { IDC_FLIP_HORIZONTALLY, "Renders the visualization from right to left." },
        { IDC_FLIP_VERTICALLY, "Renders the visualization upside down." },

        // X-axis
        { IDC_X_AXIS_MODE, "Determines the type of X-axis." },
        { IDC_X_AXIS_TOP, "Enables or disables an X-axis above the visualization." },
        { IDC_X_AXIS_BOTTOM, "Enables or disables an X-axis below the visualization." },

        // Y-axis
        { IDC_Y_AXIS_MODE, "Determines the type of Y-axis." },
        { IDC_Y_AXIS_LEFT, "Enables or disables an Y-axis left of the visualization." },
        { IDC_Y_AXIS_RIGHT, "Enables or disables an Y-axis right of the visualization." },

        { IDC_AMPLITUDE_LO, "Sets the lowest amplitude to display on the Y-axis." },
        { IDC_AMPLITUDE_HI, "Sets the highest amplitude to display on the Y-axis." },
        { IDC_AMPLITUDE_STEP, "Sets the amplitude increment." },

        { IDC_USE_ABSOLUTE, "Sets the min. amplitude to -∞ dB (0.0 on the linear scale) when enabled." },
        { IDC_GAMMA, "Sets index n of the n-th root calculation." },

        // Channels
        { IDC_CHANNELS, "Determines which channels are used by the visualization." },
        { IDC_ALL_CHANNELS, "Selects all channels." },
        { IDC_NO_CHANNELS, "Deselects all channels." },

        { IDC_CHANNEL_PAIRS, "Determines which combination of channels will be displayed." },
        { IDC_SWAP_CHANNELS, "Swaps the channels of a channel pair during visualisation e.g. the X and Y axis of an oscilloscope." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void graphs_page_t::InitializeControls() noexcept
{
    _SelectedGraph = 0;

    auto & gd = _State->_GraphDescriptions[_SelectedGraph];

    // Vertical Layout
    {
        SendDlgItemMessageW(IDC_VERTICAL_LAYOUT, BM_SETCHECK, _State->_VerticalLayout);
    }

    // Horizontal Alignment
    {
        auto w = (CComboBox) GetDlgItem(IDC_HORIZONTAL_ALIGNMENT);

        w.ResetContent();

        for (const auto & x : { L"Near", L"Center", L"Far", L"Fit" })
            w.AddString(x);

        w.SetCurSel((int) gd._HorizontalAlignment);
    }

    // X Axis
    {
        InitializeXAxisMode();
    }

    // Y Axis
    {
        InitializeYAxisMode();
    }

    {
        UDACCEL Accel[] =
        {
            { 1,     10 }, //  0.1
            { 2,    100 }, //  1.0
            { 3,    250 }, //  2.5
            { 4,    500 }, //  5.0
        };

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_LO)); _NumericEdits.push_back(ne);

            auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_LO_SPIN));

            w.SetAccel(_countof(Accel), Accel);
            w.SetRange32((int) (MinAmplitude * 10.), (int) (MaxAmplitude * 10.));

            SetDouble(IDC_AMPLITUDE_LO, gd._AmplitudeLo);
            w.SetPos32((int) (gd._AmplitudeLo * 10.));
        }

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_HI)); _NumericEdits.push_back(ne);

            auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_HI_SPIN));

            w.SetAccel(_countof(Accel), Accel);
            w.SetRange32((int) (MinAmplitude * 10), (int) (MaxAmplitude * 10.));

            SetDouble(IDC_AMPLITUDE_HI, gd._AmplitudeHi);
            w.SetPos32((int) (gd._AmplitudeHi * 10.));
        }

        {
            auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_STEP)); _NumericEdits.push_back(ne);

            auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_STEP_SPIN));

            w.SetAccel(_countof(Accel), Accel);
            w.SetRange32((int) (MinAmplitudeStep * 10), (int) (MaxAmplitudeStep * 10.));

            SetDouble(IDC_AMPLITUDE_STEP, gd._AmplitudeStep, 0, 1);
            w.SetPos32((int) (gd._AmplitudeStep * 10.));
        }
    }

    // Used by the Linear/n-th root y-axis mode.
    {
        SendDlgItemMessageW(IDC_USE_ABSOLUTE, BM_SETCHECK, gd._UseAbsolute);

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_GAMMA)); _NumericEdits.push_back(ne);
        SetDouble(IDC_GAMMA, gd._Gamma, 0, 1);
    }

    // Channels
    {
        assert(_countof(ChannelNames) == audio_chunk::defined_channel_count);
        assert(_countof(ChannelNames) == (size_t) Channels::Count);

        auto w = (CListBox) GetDlgItem(IDC_CHANNELS);

        w.ResetContent();

        for (const auto & x : ChannelNames)
            w.AddString(x);
    }

    // Channel pairs
    {
        auto w = (CComboBox) GetDlgItem(IDC_CHANNEL_PAIRS);

        w.ResetContent();

        for (const auto & x : { L"Front Left/Right", L"Back Left/Right", L"Front Center Left/Right", L"Side Left/Right", L"Top Front Left/Right", L"Top Back Left/Right" })
            w.AddString(x);

        w.SetCurSel((int) _State->_ChannelPair);
    }

    // Swap channels of a channel pair
    {
        SendDlgItemMessageW(IDC_SWAP_CHANNELS, BM_SETCHECK, gd._SwapChannels);
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void graphs_page_t::UpdateControls() noexcept
{
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);
    const bool IsBitMeter     = (_State->_VisualizationType == VisualizationType::BitMeter);
//  const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool IsOscilloscopeXY = IsOscilloscope && _State->_XYMode;

    /* Graph Descriptions */

    {
        auto w = (CListBox) GetDlgItem(IDC_GRAPH_SETTINGS);

        w.ResetContent();

        for (const auto & Iter : _State->_GraphDescriptions)
            w.AddString(Iter._Description.c_str());

        w.SetCurSel((int) _SelectedGraph);
    }

    if (_State->_VerticalLayout)
    {
        _State->_GridRowCount    = (size_t) _State->_GraphDescriptions.size();
        _State->_GridColumnCount = (size_t) 1;
    }
    else
    {
        _State->_GridRowCount    = (size_t) 1;
        _State->_GridColumnCount = (size_t) _State->_GraphDescriptions.size();
    }

    for (auto & gs : _State->_GraphDescriptions)
    {
        gs._HRatio = 1.f / (FLOAT) _State->_GridColumnCount;
        gs._VRatio = 1.f / (FLOAT) _State->_GridRowCount;
    }

    GetDlgItem(IDC_REMOVE_GRAPH).EnableWindow(_State->_GraphDescriptions.size() > 1);

    /* Vertical Layout **/

    const bool SupportsVerticalLayout = !(IsOscilloscope || IsBitMeter);

    GetDlgItem(IDC_VERTICAL_LAYOUT).EnableWindow(SupportsVerticalLayout);

    /* Graph settings */

    const auto & gd = _State->_GraphDescriptions[(size_t) _SelectedGraph];

    // Description
    SetDlgItemText(IDC_GRAPH_DESCRIPTION, gd._Description.c_str());

    // Layout
    {
        ((CComboBox) GetDlgItem(IDC_HORIZONTAL_ALIGNMENT)).SetCurSel((int) gd._HorizontalAlignment);

        CheckDlgButton(IDC_FLIP_HORIZONTALLY, gd._FlipHorizontally);
        CheckDlgButton(IDC_FLIP_VERTICALLY, gd._FlipVertically);

        // Enable / Disable the required controls.
        GetDlgItem(IDC_HORIZONTAL_ALIGNMENT).EnableWindow(!(IsLevelMeter || IsOscilloscope));

        const bool SupportsLayout = !(IsLevelMeter || IsOscilloscope || IsBitMeter);

        for (const auto ID : { IDC_FLIP_HORIZONTALLY, IDC_FLIP_VERTICALLY })
            GetDlgItem(ID).EnableWindow(SupportsLayout);
    }

    // X axis
    {
        ((CComboBox) GetDlgItem(IDC_X_AXIS_MODE)).SetCurSel((int) gd._XAxisMode);

        CheckDlgButton(IDC_X_AXIS_TOP,    gd._XAxisTop);
        CheckDlgButton(IDC_X_AXIS_BOTTOM, gd._XAxisBottom);

        // Enable / Disable the required controls.
        GetDlgItem(IDC_X_AXIS_MODE)  .EnableWindow(!(IsLevelMeter || IsBitMeter));

        GetDlgItem(IDC_X_AXIS_TOP)   .EnableWindow(gd.HasXAxis() && !(IsLevelMeter || IsOscilloscope || IsBitMeter));
        GetDlgItem(IDC_X_AXIS_BOTTOM).EnableWindow(gd.HasXAxis() && !(IsLevelMeter || IsOscilloscope));
    }

    // Y axis
    {
        ((CComboBox) GetDlgItem(IDC_Y_AXIS_MODE)).SetCurSel((int) gd._YAxisMode);

        CheckDlgButton(IDC_Y_AXIS_LEFT,  gd._YAxisLeft);
        CheckDlgButton(IDC_Y_AXIS_RIGHT, gd._YAxisRight);

        SetDouble(IDC_AMPLITUDE_LO, gd._AmplitudeLo, 0, 1);
        CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_LO_SPIN)).SetPos32((int) (gd._AmplitudeLo * 10.));

        SetDouble(IDC_AMPLITUDE_HI, gd._AmplitudeHi, 0, 1);
        CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_HI_SPIN)).SetPos32((int) (gd._AmplitudeHi * 10.));

        SetDouble(IDC_AMPLITUDE_STEP, gd._AmplitudeStep, 0, 1);
        CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_STEP_SPIN)).SetPos32((int) (gd._AmplitudeStep * 10.));

        SendDlgItemMessageW(IDC_USE_ABSOLUTE, BM_SETCHECK, gd._UseAbsolute);
        SetDouble(IDC_GAMMA, gd._Gamma, 0, 1);

        // Enable / Disable the required controls.
        GetDlgItem(IDC_Y_AXIS_MODE) .EnableWindow(!(IsLevelMeter || IsBitMeter));

        GetDlgItem(IDC_Y_AXIS_LEFT) .EnableWindow(gd.HasYAxis() && !(IsLevelMeter || IsOscilloscopeXY));
        GetDlgItem(IDC_Y_AXIS_RIGHT).EnableWindow(gd.HasYAxis() && !(IsLevelMeter || IsOscilloscopeXY || IsBitMeter));

        for (const auto & Iter : { IDC_AMPLITUDE_LO, IDC_AMPLITUDE_HI, IDC_AMPLITUDE_STEP })
            GetDlgItem(Iter).EnableWindow(gd.HasYAxis() && !(IsLevelMeter || IsOscilloscopeXY || IsBitMeter));

        const bool IsLinear = (gd._YAxisMode == YAxisMode::Linear);

        for (const auto & Iter : { IDC_USE_ABSOLUTE, IDC_GAMMA })
            GetDlgItem(Iter).EnableWindow(IsLinear && !(IsLevelMeter || IsOscilloscopeXY || IsBitMeter));
    }

    // Channels
    {
        auto w = (CListBox) GetDlgItem(IDC_CHANNELS);

        uint32_t Channels = gd._SelectedChannels;

        for (int i = 0; i < (int) _countof(ChannelNames); ++i)
        {
            w.SetSel(i, (Channels & 1) ? TRUE : FALSE);

            Channels >>= 1;
        }
    }

    // Channel Pairs
    GetDlgItem(IDC_CHANNEL_PAIRS).EnableWindow(IsOscilloscopeXY);

    SendDlgItemMessageW(IDC_SWAP_CHANNELS, BM_SETCHECK, gd._SwapChannels);
    GetDlgItem(IDC_SWAP_CHANNELS).EnableWindow(IsOscilloscopeXY);
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void graphs_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void graphs_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    auto cb = (CComboBox) w;

    const int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_GRAPH_SETTINGS:
        {
            _SelectedGraph = (size_t) ((CListBox) w).GetCurSel();

            UpdateControls();

            return;
        }

        case IDC_HORIZONTAL_ALIGNMENT:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            gs._HorizontalAlignment = (HorizontalAlignment) SelectedIndex;

            _IgnoreNotifications = true;

            UpdateControls();

            _IgnoreNotifications = false;

            ChangedSettings = ConfigurationChanges::Layout;
            break;
        }

        case IDC_X_AXIS_MODE:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            gs._XAxisMode = (XAxisMode) SelectedIndex;

            _IgnoreNotifications = true;

            UpdateControls();

            _IgnoreNotifications = false;
            break;
        }

        case IDC_Y_AXIS_MODE:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            gs._YAxisMode = (YAxisMode) SelectedIndex;

            _IgnoreNotifications = true;

            UpdateControls();

            _IgnoreNotifications = false;
            break;
        }

        case IDC_CHANNELS:
        {
            UpdateSelectedChannels();
            break;
        }

        #pragma endregion
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void graphs_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    auto ChangedSettings = ConfigurationChanges::All;
    auto & gd = _State->_GraphDescriptions[_SelectedGraph];

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        case IDC_GRAPH_DESCRIPTION:
        {
            gd._Description = Text;
            break;
        }

        // Y axis
        case IDC_AMPLITUDE_LO:
        {
            gd._AmplitudeLo = std::clamp(::_wtof(Text), MinAmplitude, gd._AmplitudeHi);
            break;
        }

        case IDC_AMPLITUDE_HI:
        {
            gd._AmplitudeHi = std::clamp(::_wtof(Text), gd._AmplitudeLo, MaxAmplitude);
            break;
        }

        case IDC_AMPLITUDE_STEP:
        {
            gd._AmplitudeStep = std::clamp(::_wtof(Text), MinAmplitudeStep, MaxAmplitudeStep);
            break;
        }

        case IDC_GAMMA:
        {
            gd._Gamma = std::clamp(::_wtof(Text), MinGamma, MaxGamma);
            break;
        }

        #pragma endregion
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void graphs_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        case IDC_AMPLITUDE_LO:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            SetDouble(id, gs._AmplitudeLo, 0, 1);
            break;
        }
        case IDC_AMPLITUDE_HI:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            SetDouble(id, gs._AmplitudeHi, 0, 1);
            break;
        }
        case IDC_AMPLITUDE_STEP:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            SetDouble(id, gs._AmplitudeStep, 0, 1);
            break;
        }
        case IDC_GAMMA:
        {
            auto & gs = _State->_GraphDescriptions[_SelectedGraph];

            SetDouble(id, gs._Gamma, 0, 1);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void graphs_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    auto & gd = _State->_GraphDescriptions[_SelectedGraph];

    switch (id)
    {
        default:
            return;

        case IDC_ADD_GRAPH:
        {
            graph_description_t NewGraphDescription = _State->_GraphDescriptions[_SelectedGraph];

            int Index = (int) _State->_GraphDescriptions.size();

            WCHAR Description[32]; ::StringCchPrintfW(Description, _countof(Description), L"Graph %d", Index + 1);

            NewGraphDescription._Description = Description;

            _State->_GraphDescriptions.insert(_State->_GraphDescriptions.begin() + (int) Index, NewGraphDescription);

            _IsInitializing = true;

            UpdateControls();

            _IsInitializing = false;

            ((CListBox) GetDlgItem(IDC_GRAPH_SETTINGS)).SetCurSel(Index);
            _SelectedGraph = (size_t) Index;
            break;
        }

        case IDC_REMOVE_GRAPH:
        {
            _State->_GraphDescriptions.erase(_State->_GraphDescriptions.begin() + (int) _SelectedGraph);

            _SelectedGraph = std::clamp(_SelectedGraph, (size_t) 0, _State->_GraphDescriptions.size() - 1);

            UpdateControls();
            break;
        }

        case IDC_VERTICAL_LAYOUT:
        {
            _State->_VerticalLayout = (bool) SendDlgItemMessageW(id, BM_GETCHECK);

            UpdateControls();
            break;
        }

        case IDC_FLIP_HORIZONTALLY:
        {
            gd._FlipHorizontally = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_FLIP_VERTICALLY:
        {
            gd._FlipVertically = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_X_AXIS_TOP:
        {
            gd._XAxisTop = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_X_AXIS_BOTTOM:
        {
            gd._XAxisBottom = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_Y_AXIS_LEFT:
        {
            gd._YAxisLeft = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_Y_AXIS_RIGHT:
        {
            gd._YAxisRight = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_USE_ABSOLUTE:
        {
            gd._UseAbsolute = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_ALL_CHANNELS:
        {
            auto lb = (CListBox) GetDlgItem(IDC_CHANNELS);

            lb.SelItemRange(TRUE, 0, 0xFFFF);

            UpdateSelectedChannels();
            break;
        }

        case IDC_NO_CHANNELS:
        {
            auto lb = (CListBox) GetDlgItem(IDC_CHANNELS);

            lb.SelItemRange(FALSE, 0, 0xFFFF);

            UpdateSelectedChannels();
            break;
        }

        case IDC_SWAP_CHANNELS:
        {
            gd._SwapChannels = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT graphs_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = ConfigurationChanges::All;
    auto & gd = _State->_GraphDescriptions[_SelectedGraph];

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_AMPLITUDE_LO_SPIN:
        {
            if (!SetProperty(gd._AmplitudeLo, ClampNewSpinPosition(nmud, MinAmplitude, gd._AmplitudeHi, 10.)))
                return -1;

            SetDouble(IDC_AMPLITUDE_LO, gd._AmplitudeLo, 0, 1);
            break;
        }

        case IDC_AMPLITUDE_HI_SPIN:
        {
            if (!SetProperty(gd._AmplitudeHi, ClampNewSpinPosition(nmud, gd._AmplitudeLo, MaxAmplitude, 10.)))
                return -1;

            SetDouble(IDC_AMPLITUDE_HI, gd._AmplitudeHi, 0, 1);
            break;
        }

        case IDC_AMPLITUDE_STEP_SPIN:
        {
            if (!SetProperty(gd._AmplitudeStep, ClampNewSpinPosition(nmud, MinAmplitudeStep, MaxAmplitudeStep, 10.)))
                return -1;

            SetDouble(IDC_AMPLITUDE_STEP, gd._AmplitudeStep, 0, 1);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}

/// <summary>
/// Fills the X-axis mode combobox with the modes relevant to the current visualization.
/// </summary>
void graphs_page_t::InitializeXAxisMode() noexcept
{
    auto w = (CComboBox) GetDlgItem(IDC_X_AXIS_MODE);

    w.ResetContent();

    if (_State->_VisualizationType != VisualizationType::Oscilloscope)
    {
        for (const auto & x : { L"None", L"Bands", L"Decades", L"Octaves", L"Notes" })
            w.AddString(x);
    }
    else
    {
        for (const auto & x : { L"Off", L"On" })
            w.AddString(x);

        _State->_GraphDescriptions[_SelectedGraph]._XAxisMode = (XAxisMode) std::clamp((int) _State->_GraphDescriptions[_SelectedGraph]._XAxisMode, 0, 1);
    }

    w.SetCurSel((int) _State->_GraphDescriptions[_SelectedGraph]._XAxisMode);
}

/// <summary>
/// Fills the y-axis mode combobox with the modes relevant to the current visualization.
/// </summary>
void graphs_page_t::InitializeYAxisMode() noexcept
{
    auto w = (CComboBox) GetDlgItem(IDC_Y_AXIS_MODE);

    w.ResetContent();

    if (!((_State->_VisualizationType == VisualizationType::Oscilloscope) && _State->_XYMode))
    {
        for (const auto & x : { L"None", L"Decibel", L"Linear/n-th root" })
            w.AddString(x);
    }
    else
    {
        for (const auto & x : { L"Off", L"On" })
            w.AddString(x);

        _State->_GraphDescriptions[_SelectedGraph]._YAxisMode = (YAxisMode) std::clamp((int) _State->_GraphDescriptions[_SelectedGraph]._YAxisMode, 0, 1);
    }

    w.SetCurSel((int) _State->_GraphDescriptions[_SelectedGraph]._YAxisMode);
}

/// <summary>
/// Updates the Selected Channels setting.
/// </summary>
void graphs_page_t::UpdateSelectedChannels() noexcept
{
    auto lb = (CListBox) GetDlgItem(IDC_CHANNELS);

    const int Count = lb.GetSelCount();

    std::vector<int> Items((size_t) Count);

    lb.GetSelItems(Count, Items.data());

    uint32_t Channels = 0;

    for (int Item : Items)
        Channels |= 1 << Item;

    _State->_GraphDescriptions[_SelectedGraph]._SelectedChannels = Channels;
}
