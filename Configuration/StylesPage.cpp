
/** $VER: StylesPage.cpp (2026.02.22) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "StylesPage.h"
#include "Support.h"
#include "Log.h"

#include "Direct2D.h"
#include "CColorDialogEx.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL styles_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    DlgResize_Init(false, true);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_STYLES, "Selects the visual element that will be styled" },

        { IDC_COLOR_SOURCE, "Determines the source of the color that will be used to render the visual element. Select \"None\" to prevent rendering." },
        { IDC_COLOR_INDEX, "Selects the specific Windows, DUI or CUI color to use." },
        { IDC_COLOR_BUTTON, "Shows the color that will be used to render the visual element. Click to modify it." },
        { IDC_COLOR_SCHEME, "Selects the color scheme used to create a gradient with." },

        { IDC_GRADIENT, "Shows the gradient created using the current color list." },
        { IDC_COLOR_LIST, "Shows the colors in the current color scheme." },

        { IDC_ADD, "Adds a color to the color list after the selected one. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },
        { IDC_REMOVE, "Removes the selected color from the list. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },
        { IDC_REVERSE, "Reverses the list of colors. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },

        { IDC_POSITION, "Determines the position of the color in the gradient (in % of the total length of the gradient)" },
        { IDC_SPREAD, "Evenly spreads the colors of the list in the gradient" },

        { IDC_HORIZONTAL_GRADIENT, "Generates a horizontal instead of a vertical gradient." },
        { IDC_AMPLITUDE_BASED, "Determines the color of the bar based on the amplitude when using a horizontal gradient." },

        { IDC_OPACITY, "Determines the opacity of the resulting color brush." },
        { IDC_THICKNESS, "Determines the thickness of the resulting color brush when applicable." },

        { IDC_FONT_NAME, "Determines the opacity of the resulting color brush." },
        { IDC_FONT_NAME_SELECT, "Opens a dialog to select a font." },
        { IDC_FONT_SIZE, "Determines the size of the font in points." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Creates an initializes the controls of the page.
/// </summary>
void styles_page_t::InitializeControls() noexcept
{
    {
        InitializeStyles();
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SOURCE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Solid", L"Dominant Color", L"Gradient", L"Windows", L"User Interface" })
            w.AddString(x);
    }

    {
        _Color.Initialize(GetDlgItem(IDC_COLOR_BUTTON));
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.ResetContent();

        for (const auto & x : { L"Solid", L"Custom", L"Artwork", L"Prism 1", L"Prism 2", L"Prism 3", L"foobar2000", L"foobar2000 Dark Mode", L"Fire", L"Rainbow", L"SoX" })
            w.AddString(x);
    }

    {
        _Gradient.Initialize(GetDlgItem(IDC_GRADIENT));
        _Colors.Initialize(GetDlgItem(IDC_COLOR_LIST));

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_POSITION)); _NumericEdits.push_back(ne);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_OPACITY)); _NumericEdits.push_back(ne);

        auto w = CUpDownCtrl(GetDlgItem(IDC_OPACITY_SPIN));

        w.SetRange32((int) (MinOpacity * 100.f), (int) (MaxOpacity * 100.f));
        w.SetPos32(0);
        w.SetAccel(_countof(Accel), Accel);
    }

    {
        UDACCEL Accel[] =
        {
            { 1, 1 }, //  0.1
            { 2, 5 }, //  0.5
        };

        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_THICKNESS)); _NumericEdits.push_back(ne);

        auto w = CUpDownCtrl(GetDlgItem(IDC_THICKNESS_SPIN));

        w.SetRange32((int) (MinThickness * 10.), (int) (MaxThickness * 10.));
        w.SetAccel(_countof(Accel), Accel);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_FONT_SIZE)); _NumericEdits.push_back(ne);
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void styles_page_t::UpdateControls() noexcept
{
    style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

    // Update the controls based on the color source.
    switch (style->_ColorSource)
    {
        case ColorSource::None:
        case ColorSource::Solid:
        case ColorSource::DominantColor:
            break;

        case ColorSource::Gradient:
        {
            if (style->_ColorScheme == ColorScheme::Custom)
                style->_CurrentGradientStops = style->_CustomGradientStops;
            else
            if (style->_ColorScheme == ColorScheme::Artwork)
                style->_CurrentGradientStops = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : GetBuiltInGradientStops(ColorScheme::Artwork);
            else
                style->_CurrentGradientStops = GetBuiltInGradientStops(style->_ColorScheme);
            break;
        }

        case ColorSource::Windows:
        {
            auto w = (CComboBox) GetDlgItem(IDC_COLOR_INDEX);

            w.ResetContent();

            for (const auto & x : { L"Window Background", L"Window Text", L"Button Background", L"Button Text", L"Highlight Background", L"Highlight Text", L"Gray Text", L"Hot Light" })
                w.AddString(x);

            w.SetCurSel((int) std::clamp(style->_ColorIndex, 0U, (uint32_t) (w.GetCount() - 1)));
            break;
        }

        case ColorSource::UserInterface:
        {
            auto w = (CComboBox) GetDlgItem(IDC_COLOR_INDEX);

            w.ResetContent();

            if (_State->_IsDUI)
            {
                for (const auto & x : { L"Text", L"Background", L"Highlight", L"Selection", L"Dark mode" })
                    w.AddString(x);
            }
            else
            {
                for (const auto & x : { L"Text", L"Selected Text", L"Inactive Selected Text", L"Background", L"Selected Background", L"Inactive Selected Background", L"Active Item" })
                    w.AddString(x);
            }

            w.SetCurSel((int) std::clamp(style->_ColorIndex, 0U, (uint32_t) (w.GetCount() - 1)));
            break;
        }
    }

    UpdateCurrentColor(style);

    ((CComboBox) GetDlgItem(IDC_COLOR_SOURCE)).SetCurSel((int) style->_ColorSource);

    SendDlgItemMessageW(IDC_HORIZONTAL_GRADIENT, BM_SETCHECK, (WPARAM) style->Has(style_t::Features::HorizontalGradient));
    SendDlgItemMessageW(IDC_AMPLITUDE_BASED,     BM_SETCHECK, (WPARAM) style->Has(style_t::Features::AmplitudeBasedColor));

    SetInteger(IDC_OPACITY, (int64_t) (style->_Opacity * 100.f));
    ((CUpDownCtrl) GetDlgItem(IDC_OPACITY_SPIN)).SetPos32((int) (style->_Opacity * 100.f));

    SetDouble(IDC_THICKNESS, style->_Thickness, 0, 1);
    ((CUpDownCtrl) GetDlgItem(IDC_THICKNESS_SPIN)).SetPos32((int) (style->_Thickness * 10.f));

    SetDlgItemTextW(IDC_FONT_NAME, style->_FontName.c_str());
    SetDouble(IDC_FONT_SIZE, style->_FontSize, 0, 1);

    // Enable the contols as necessary.
    GetDlgItem(IDC_COLOR_INDEX).EnableWindow((style->_ColorSource == ColorSource::Windows) || (style->_ColorSource == ColorSource::UserInterface));
    GetDlgItem(IDC_COLOR_SCHEME).EnableWindow(style->_ColorSource == ColorSource::Gradient);

    GetDlgItem(IDC_HORIZONTAL_GRADIENT).EnableWindow(style->_ColorSource == ColorSource::Gradient);
    GetDlgItem(IDC_AMPLITUDE_BASED).EnableWindow((style->_ColorSource == ColorSource::Gradient) && style->Has(style_t::Features::AmplitudeAware | style_t::Features::HorizontalGradient));

    GetDlgItem(IDC_OPACITY)  .EnableWindow(style->IsEnabled() && style->Has(style_t::Features::SupportsOpacity));
    GetDlgItem(IDC_THICKNESS).EnableWindow(style->IsEnabled() && style->Has(style_t::Features::SupportsThickness));

    GetDlgItem(IDC_FONT_NAME).EnableWindow(style->IsEnabled() && style->Has(style_t::Features::SupportsFont));
    GetDlgItem(IDC_FONT_SIZE).EnableWindow(style->IsEnabled() && style->Has(style_t::Features::SupportsFont));

    UpdateColorControls();
}

/// <summary>
/// Deletes the controls of the page.
/// </summary>
void styles_page_t::TerminateControls() noexcept
{
    _Gradient.Terminate();
    _Colors.Terminate();

    _Color.Terminate();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void styles_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    auto cb = (CComboBox) w;

    int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_STYLES:
        {
            _SelectedStyle = (size_t) ((CListBox) w).GetCurSel();

            _IsInitializing = true;

            UpdateControls();

            _IsInitializing = false;

            return;
        }

        case IDC_COLOR_SOURCE:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_ColorSource = (ColorSource) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_INDEX:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_ColorIndex = (uint32_t) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_SCHEME:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_ColorScheme = (ColorScheme) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_LIST:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            // Show the position of the selected color of the gradient.
            const size_t Index = (size_t) _Colors.GetCurSel();

            if (!msc::InRange(Index, (size_t) 0, style->_CurrentGradientStops.size() - 1))
                return;

            const t_int64 Position = (t_int64) (style->_CurrentGradientStops[Index].position * 100.f);

            SetInteger(IDC_POSITION, Position);

            // Update the state of the buttons.
            const bool HasSelection = (Index != (size_t) LB_ERR);
            const bool HasMoreThanOneColor = (style->_CurrentGradientStops.size() > 1);
            const bool UseArtwork = (style->_ColorScheme == ColorScheme::Artwork);

            GetDlgItem(IDC_ADD).EnableWindow(HasSelection && !UseArtwork);
            GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

            GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor && !UseArtwork);

            GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);
            GetDlgItem(IDC_SPREAD).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

            return;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void styles_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
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

        // Artwork Colors
        case IDC_NUM_ARTWORK_COLORS:
        {
            _State->_NumArtworkColors = std::clamp((uint32_t) ::_wtoi(Text), MinArtworkColors, MaxArtworkColors);
            break;
        }

        case IDC_LIGHTNESS_THRESHOLD:
        {
            _State->_LightnessThreshold = (FLOAT) std::clamp(::_wtof(Text) / 100.f, MinLightnessThreshold, MaxLightnessThreshold);
            break;
        }

        // Color Scheme
        case IDC_POSITION:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            size_t SelectedIndex = (size_t) _Colors.GetCurSel();

            if (!msc::InRange(SelectedIndex, (size_t) 0, style->_CurrentGradientStops.size() - 1))
                return;

            int Position = std::clamp(::_wtoi(Text), 0, 100);

            if ((int) (style->_CurrentGradientStops[SelectedIndex].position * 100.f) == Position)
                return;

            style->_CurrentGradientStops[SelectedIndex].position = (FLOAT) Position / 100.f;

            style->_ColorScheme = ColorScheme::Custom;
            style->_CustomGradientStops = style->_CurrentGradientStops;

            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) style->_ColorScheme);
            _Gradient.SetGradientStops(style->_CurrentGradientStops);
            break;
        }

        case IDC_OPACITY:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_Opacity = (FLOAT) std::clamp(::_wtof(Text) / 100.f, MinOpacity, MaxOpacity);
            break;
        }

        case IDC_THICKNESS:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_Thickness = (FLOAT) std::clamp(::_wtof(Text), MinThickness, MaxThickness);
            break;
        }

        case IDC_FONT_NAME:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_FontName = Text;
            break;
        }

        case IDC_FONT_SIZE:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            style->_FontSize = (FLOAT) std::clamp(::_wtof(Text), MinFontSize, MaxFontSize);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void styles_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        case IDC_OPACITY:
        {
            SetInteger(id, (int64_t) (_State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle])->_Opacity * 100.f));
            break;
        }

        case IDC_THICKNESS:
        {
            SetDouble(id, _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle])->_Thickness, 0, 1);
            break;
        }

        case IDC_FONT_NAME:
        {
            break;
        }

        case IDC_FONT_SIZE:
        {
            SetDouble(id, _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle])->_FontSize, 0, 1);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void styles_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        case IDC_ADD:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            size_t SelectedIndex = (size_t) _Colors.GetCurSel();

            if (!msc::InRange(SelectedIndex, (size_t) 0, style->_CurrentGradientStops.size() - 1))
                return;

            D2D1_COLOR_F Color = style->_CurrentGradientStops[SelectedIndex].color;

            CColorDialogEx cd;

            if (!cd.SelectColor(m_hWnd, Color))
                return;

            style->_CurrentGradientStops.insert(style->_CurrentGradientStops.begin() + (int) SelectedIndex + 1, { 0.f, Color });
            
            UpdateGradientStopPositons(style, SelectedIndex + 1);

            UpdateColorControls();
            break;
        }

        case IDC_REMOVE:
        {
            // Don't remove the last color.
            if (_Colors.GetCount() == 1)
                return;

            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            size_t SelectedIndex = (size_t) _Colors.GetCurSel();

            if (!msc::InRange(SelectedIndex, (size_t) 0, style->_CurrentGradientStops.size() - 1))
                return;

            style->_CurrentGradientStops.erase(style->_CurrentGradientStops.begin() + (int) SelectedIndex);

            // Save the current result as custom gradient stops.
            style->_ColorScheme = ColorScheme::Custom;
            style->_CustomGradientStops = style->_CurrentGradientStops;

            UpdateColorControls();
            break;
        }

        case IDC_REVERSE:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            std::reverse(style->_CurrentGradientStops.begin(), style->_CurrentGradientStops.end());

            for (auto & gs : style->_CurrentGradientStops)
                gs.position = 1.f - gs.position;

            // Save the current result as custom gradient stops.
            style->_ColorScheme = ColorScheme::Custom;
            style->_CustomGradientStops = style->_CurrentGradientStops;

            UpdateColorControls();
            break;
        }

        case IDC_SPREAD:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            UpdateGradientStopPositons(style, ~0U);

            UpdateColorControls();
            break;
        }

        case IDC_HORIZONTAL_GRADIENT:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            if ((bool) SendDlgItemMessageW(id, BM_GETCHECK))
                Set(style->Flags, style_t::Features::HorizontalGradient);
            else
                UnSet(style->Flags, style_t::Features::HorizontalGradient);

            UpdateControls();
            break;
        }

        case IDC_AMPLITUDE_BASED:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            if ((bool) SendDlgItemMessageW(id, BM_GETCHECK))
                Set(style->Flags, style_t::Features::AmplitudeBasedColor);
            else
                UnSet(style->Flags, style_t::Features::AmplitudeBasedColor);
            break;
        }

        case IDC_FONT_NAME_SELECT:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            UINT DPI;

            GetDPI(m_hWnd, DPI);

            LOGFONTW lf =
            {
                .lfHeight         = -::MulDiv((int) style->_FontSize, (int) DPI, 72),
                .lfWeight         = FW_NORMAL,
                .lfCharSet        = DEFAULT_CHARSET,
                .lfOutPrecision   = OUT_DEFAULT_PRECIS,
                .lfClipPrecision  = CLIP_DEFAULT_PRECIS,
                .lfQuality        = CLEARTYPE_QUALITY,
                .lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE,
            };

            ::wcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), style->_FontName.c_str());

            CHOOSEFONTW cf =
            {
                .lStructSize = sizeof(cf),
                .hwndOwner   = m_hWnd,
                .lpLogFont   = &lf,
                .iPointSize  = (INT) (style->_FontSize * 10.f),
                .Flags       = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT,
             };

            if (!::ChooseFontW(&cf))
                return;

            style->_FontName = cf.lpLogFont->lfFaceName;
            style->_FontSize = (FLOAT) cf.iPointSize / 10.f;

            UpdateControls();
            break;
        }

    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a double click on a list box item.
/// </summary>
void styles_page_t::OnDoubleClick(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || (id != IDC_COLOR_LIST))
        return;

    SetMsgHandled(FALSE);
}

/// <summary>
/// Handles a Change notification from the custom controls.
/// </summary>
LRESULT styles_page_t::OnChanged(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = Settings::All;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_COLOR_LIST:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            std::vector<D2D1_COLOR_F> Colors;

            _Colors.GetColors(Colors);

            if (Colors.empty())
                return 0;

            style->_ColorScheme = ColorScheme::Custom;

            if (style->_ColorSource == ColorSource::Gradient)
            {
                for (size_t i = 0; i < Colors.size(); ++i)
                    style->_CustomGradientStops[i].color = Colors[i];

                style->_CurrentGradientStops = style->_CustomGradientStops;
            }
            else
                _Direct2D.CreateGradientStops(Colors, style->_CustomGradientStops);

            // Update the controls.
            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) style->_ColorScheme);
            _Gradient.SetGradientStops(style->_CustomGradientStops);
            break;
        }

        case IDC_COLOR_BUTTON:
        {
            style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

            _Color.GetColor(style->_CustomColor);

            style->_ColorSource = ColorSource::Solid; // Force the color source to Solid.
            style->_CurrentColor = style->_CustomColor;

            UpdateColorControls();
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}

/// <summary>
/// Fills the styles listbox with the styles used by the current visualization.
/// </summary>
void styles_page_t::InitializeStyles() noexcept
{
    auto w = (CListBox) GetDlgItem(IDC_STYLES);

    w.ResetContent();

    _ActiveStyles.clear();

    const auto User = (VisualizationTypes) ((uint64_t) 1 << (int) _State->_VisualizationType);

    for (const auto & ID : _State->_StyleManager.DisplayOrder)
    {
        const auto Style = _State->_StyleManager.GetStyle(ID);

        if ((uint64_t) Style->UsedBy & (uint64_t) User)
        {
            _ActiveStyles.push_back(ID);

            w.AddString(Style->Name.c_str());
        }
    }

    _SelectedStyle = 0;

    w.SetCurSel((int) _SelectedStyle);
}

/// <summary>
/// Updates the color controls with the current configuration.
/// </summary>
void styles_page_t::UpdateColorControls() noexcept
{
    style_t * style = _State->_StyleManager.GetStyle(_ActiveStyles[_SelectedStyle]);

    // Initialize the gradient stops.
    gradient_stops_t gs;

    if (style->_ColorSource == ColorSource::Gradient)
    {
        ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) style->_ColorScheme);

        if (style->_ColorScheme == ColorScheme::Custom)
            gs = style->_CustomGradientStops;
        else
        if (style->_ColorScheme == ColorScheme::Artwork)
            gs = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : GetBuiltInGradientStops(ColorScheme::Artwork);
        else
            gs = GetBuiltInGradientStops(style->_ColorScheme);
    }

    // Update the color button.
    _Color.SetColor(style->_CurrentColor);

    // Update the gradient control.
    _Gradient.SetGradientStops(gs);

    // Update the color list.
    std::vector<D2D1_COLOR_F> Colors;

    if (style->_ColorSource == ColorSource::Gradient)
    {
        int SelectedIndex = _Colors.GetCurSel();

        for (const auto & Iter : gs)
            Colors.push_back(Iter.color);

        _Colors.SetColors(Colors);

        if (SelectedIndex != LB_ERR)
        {
            SelectedIndex = std::clamp(SelectedIndex, 0, (int) gs.size() - 1);

            _Colors.SetCurSel(SelectedIndex);

            _IgnoreNotifications = true;

            t_int64 Position = (t_int64) (gs[(size_t) SelectedIndex].position * 100.f);
            SetInteger(IDC_POSITION, Position);

            _IgnoreNotifications = false;
        }
    }
    else
        _Colors.SetColors(Colors);

    // Update the state of the buttons.
    const bool HasSelection = (_Colors.GetCurSel() != LB_ERR);                // Add and Remove are only enabled when a color is selected.
    const bool HasMoreThanOneColor = (gs.size() > 1);                         // Remove and Reverse are only enabled when there is more than 1 color.
    const bool UseArtwork = (style->_ColorScheme == ColorScheme::Artwork);    // Gradient controls are disabled when the artwork provides the colors.

        GetDlgItem(IDC_ADD).EnableWindow(HasSelection && !UseArtwork);
        GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

        GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor && !UseArtwork);

        GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);
        GetDlgItem(IDC_SPREAD).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);
}

/// <summary>
/// Updates the current color based on the color source.
/// </summary>
void styles_page_t::UpdateCurrentColor(style_t * style) const noexcept
{
    style->UpdateCurrentColor(_State->_StyleManager.DominantColor, _State->_StyleManager.UserInterfaceColors);
}

/// <summary>
/// Updates the position of the current gradient colors.
/// </summary>
void styles_page_t::UpdateGradientStopPositons(style_t * style, size_t index) const noexcept
{
    auto & gs = style->_CurrentGradientStops;

    if (gs.empty())
        return;

    if (index == 0)
        gs[index].position = 0.f;
    else
    if (index == gs.size() - 1)
        gs[index].position = 1.f;
    else
    if (index != ~0U)
        gs[index].position = std::clamp(gs[index - 1].position + (gs[index + 1].position - gs[index - 1].position) / 2.f, 0.f, 1.f);
    else
    // Spread the positions of the stops between 0.f and 1.f.
    {
        FLOAT Position = 0.f;

        for (auto & Iter : gs)
        {
            Iter.position = Position / (FLOAT) (gs.size() - 1);
            Position++;
        }
    }

    // Save the current result as custom gradient stops.
    style->_ColorScheme = ColorScheme::Custom;
    style->_CustomGradientStops = gs;
}
