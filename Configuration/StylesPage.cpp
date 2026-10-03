
/** $VER: StylesPage.cpp (2026.10.02) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "StylesPage.h"
#include "Support.h"

#include "ColorDialog.h"
#include "ColorListBox.h"
#include "Gradient.h"

#include <Toggle.h>

namespace
{
    static bool IsValidColorIndex(int colorIndex, size_t colorCount) noexcept
    {
        return (colorIndex >= 0) && ((size_t) colorIndex < colorCount);
    }
}

/// <summary>
/// Initializes the page.
/// </summary>
BOOL styles_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    DlgResize_Init(false, true); // This page has resizable controls.

    static const std::unordered_map<int, const char *> Tips =
    {
        { IDC_STYLES, "Selects the visual element that will be styled" },

        { IDC_SCOPE, "Determines the scope of the styles being edited." },

        { IDC_COLOR_SOURCE, "Determines the source of the color that will be used to render the visual element. Select \"None\" to prevent rendering." },
        { IDC_COLOR_INDEX, "Selects the specific Windows, DUI or CUI color to use." },
        { IDC_COLOR_BUTTON, "Shows the color that will be used to render the visual element. Click to modify it." },
        { IDC_COLOR_SCHEME, "Selects the color scheme used to create a gradient with." },

        { IDC_GRADIENT, "Shows the gradient created using the current color list." },
        { IDC_GRADIENT_COLORS, "Shows the colors in the current color scheme." },

        { IDC_ADD, "Adds a color to the color list after the selected one. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },
        { IDC_REMOVE, "Removes the selected color from the list. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },
        { IDC_REVERSE, "Reverses the list of colors. A built-in color scheme will automatically be converted to a custom color scheme and that scheme will be activated." },

        { IDC_POSITION, "Determines the position of the color in the gradient (in % of the total length of the gradient)" },
        { IDC_SPREAD, "Evenly spreads the colors of the list in the gradient" },

        { IDC_HORIZONTAL_GRADIENT, "Generates a horizontal instead of a vertical gradient." },
        { IDC_AMPLITUDE_BASED, "Determines the color of the bar based on the amplitude when using a horizontal gradient." },

        { IDC_GRADIENT_COLOR_SOURCE, "Determines the source of the selected gradient stop color." },
        { IDC_GRADIENT_COLOR_INDEX, "Selects the specific Windows, DUI or CUI color to use for the selected gradient stop color." },

        { IDC_OPACITY, "Determines the opacity of the resulting color brush." },
        { IDC_THICKNESS, "Determines the thickness of the resulting color brush when applicable." },

        { IDC_FONT_NAME, "Specifies the name of the font." },
        { IDC_FONT_NAME_SELECT, "Opens a dialog to select a font." },
        { IDC_FONT_SIZE, "Determines the size of the font in points." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    _StyleManager = &_State->_StyleManager;

    return TRUE;
}

/// <summary>
/// Creates and initializes the controls of the page.
/// </summary>
void styles_page_t::InitializeControls() noexcept
{
    {
        InitializeStyles();
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_SCOPE);

        w.ResetContent();

        w.AddString(L"Global");

        if (_State->_GraphOptions.size() > 1)
        {
            uint32_t i = 1;

            for (const auto & gd : _State->_GraphOptions)
            {
                w.AddString(!gd._Description.empty() ? gd._Description.c_str() : msc::FormatText(L"Graph %u", i).c_str());
                ++i;
            }
        }

        w.SetCurSel(0);
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SOURCE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Solid", L"Dominant Color", L"Gradient", L"Windows", L"User Interface" })
            w.AddString(x);
    }

    {
        _ColorButton.Initialize(GetDlgItem(IDC_COLOR_BUTTON));
    }

    {
        static const WCHAR * ColorMapNames[] =
        {
            L"Solid", L"Custom", L"Artwork",
            L"Prism 1", L"Prism 2", L"Prism 3",
            L"foobar2000", L"foobar2000 Dark Mode",
            L"Fire", L"Rainbow",
            L"SoX", 
            L"Turbo",
            L"Viridis", L"Plasma", L"Inferno", L"Magma", L"Cividis",
            L"Gold", L"Triband",
        };

        static_assert((_countof(ColorMapNames) - 1) == (size_t) ColorScheme::Max, "");

        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.ResetContent();

        for (const auto & x : ColorMapNames)
            w.AddString(x);
    }

    {
        _GradientButton.Initialize(GetDlgItem(IDC_GRADIENT));
        _ColorListBox  .Initialize(GetDlgItem(IDC_GRADIENT_COLORS));

        auto ne = std::make_shared<numeric_edit_t>(); ne->Initialize(GetDlgItem(IDC_POSITION)); _NumericEdits.push_back(ne);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        auto ne = std::make_shared<numeric_edit_t>(); ne->Initialize(GetDlgItem(IDC_OPACITY)); _NumericEdits.push_back(ne);

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

        auto ne = std::make_shared<numeric_edit_t>(); ne->Initialize(GetDlgItem(IDC_THICKNESS)); _NumericEdits.push_back(ne);

        auto w = CUpDownCtrl(GetDlgItem(IDC_THICKNESS_SPIN));

        w.SetRange32((int) (MinThickness * 10.), (int) (MaxThickness * 10.));
        w.SetAccel(_countof(Accel), Accel);
    }

    {
        auto ne = std::make_shared<numeric_edit_t>(); ne->Initialize(GetDlgItem(IDC_FONT_SIZE)); _NumericEdits.push_back(ne);
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE);

        w.ResetContent();

        for (const auto & x : { L"Solid", L"Dominant Color", L"Windows", L"User Interface" })
            w.AddString(x);
    }

    UpdateControls();
}

/// <summary>
/// Deletes the controls of the page.
/// </summary>
void styles_page_t::TerminateControls() noexcept
{
    _GradientButton.Terminate();
    _ColorListBox.Terminate();

    _ColorButton.Terminate();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void styles_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow window) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = ConfigurationChanges::All;

    auto cb = (CComboBox) window;

    const int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        default:
            return;

        case IDC_STYLES:
        {
            auto Scope = msc::toggle_t(_IsInitializing, true);

            _SelectedStyle = (size_t) ((CListBox) window).GetCurSel();

            UpdateControls();

            return;
        }

        case IDC_SCOPE:
        {
            _StyleManager = (SelectedIndex == 0) ? &_State->_StyleManager : &_State->_GraphOptions[(size_t) SelectedIndex - 1]._StyleManager;

            UpdateControls();
            return;
        }

        case IDC_COLOR_SOURCE:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            Style->_ColorSource = (ColorSource) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_INDEX:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            Style->_ColorIndex = (uint32_t) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_COLOR_SCHEME:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            Style->_ColorScheme = (ColorScheme) SelectedIndex;

            // Clear the selected gradient color.
            ((CListBox) GetDlgItem(IDC_GRADIENT_COLORS)).SetCurSel(-1);

            UpdateControls();
            break;
        }

        case IDC_GRADIENT_COLORS:
        {
            const style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            const auto SelectedColor = _ColorListBox.GetCurSel();

            if (!IsValidColorIndex(SelectedColor, Style->_CurrentGradientStops.size()))
                return;

            {
                const auto & cgs = Style->_CurrentGradientStops[(size_t) SelectedColor];

                const auto Position = (int64_t) (cgs.position * 100.f);

                SetInteger(IDC_POSITION, Position);
            }

            // Enable the gradient controls as necessary.
            {
                const bool HasSelection        = (SelectedColor != LB_ERR);                     // Add and Remove are only enabled when a color is selected.
                const bool HasMoreThanOneColor = (Style->_CustomGradient.size() > 1);           // Remove and Reverse are only enabled when there is more than 1 color.
                const bool IsArtworkScheme     = (Style->_ColorScheme == ColorScheme::Artwork);

                GetDlgItem(IDC_ADD)     .EnableWindow(HasSelection &&                        !IsArtworkScheme);
                GetDlgItem(IDC_REMOVE)  .EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);

                GetDlgItem(IDC_REVERSE) .EnableWindow(                HasMoreThanOneColor && !IsArtworkScheme);

                GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);
                GetDlgItem(IDC_SPREAD)  .EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);
            }

            // The Custom scheme has additional options.
            {
                const bool IsCustomScheme = (Style->_ColorScheme == ColorScheme::Custom);

                GetDlgItem(IDC_GRADIENT_COLOR_SOURCE).EnableWindow(IsCustomScheme);

                if (IsCustomScheme)
                {
                    const auto & cgs = Style->_CustomGradient[(size_t) SelectedColor];

                    ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE)).SetCurSel((int) cgs.ColorSource);

                    UpdateColorIndexControl(Style, SelectedColor);

                    GetDlgItem(IDC_GRADIENT_COLOR_INDEX).EnableWindow((cgs.ColorSource == GradientStopSource::Windows) || (cgs.ColorSource == GradientStopSource::UserInterface));
                }
            }

            return;
        }

        case IDC_GRADIENT_COLOR_SOURCE:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            const auto SelectedColor = _ColorListBox.GetCurSel();

            if (!IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
                return;

            auto & gs = Style->_CustomGradient[(size_t) SelectedColor];

            gs.ColorSource = (GradientStopSource) SelectedIndex;
            gs.SetColor(_State);

            UpdateControls();
            break;
        }

        case IDC_GRADIENT_COLOR_INDEX:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            const auto SelectedColor = _ColorListBox.GetCurSel();

            if (!IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
                return;

            auto & gs = Style->_CustomGradient[(size_t) SelectedColor];

            gs.ColorIndex = (uint32_t) SelectedIndex;
            gs.SetColor(_State);

            UpdateControls();
            break;
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

    auto ChangedSettings = ConfigurationChanges::All;

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

    switch (id)
    {
        default:
            return;

        // Color Scheme
        case IDC_POSITION:
        {
            const auto SelectedColor = _ColorListBox.GetCurSel();

            if (!IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
                return;

            auto & cgs = Style->_CustomGradient[(size_t) SelectedColor];

            // Update the position of the selected gradient stop.
            {
                int Position = std::clamp(::_wtoi(Text), 0, 100);

                if ((int) (cgs.position * 100.f) == Position)
                    return;

                cgs.position = (FLOAT) Position / 100.f;
            }

            Style->_ColorScheme = ColorScheme::Custom;

            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) Style->_ColorScheme);
            break;
        }

        case IDC_OPACITY:
        {
            if (!SetProperty(Style->_Opacity, (FLOAT) std::clamp(::_wtof(Text) / 100.f, MinOpacity, MaxOpacity)))
                return;

            break;
        }

        case IDC_THICKNESS:
        {
            if (!SetProperty(Style->_Thickness, (FLOAT) std::clamp(::_wtof(Text), MinThickness, MaxThickness)))
                return;

            break;
        }

        case IDC_FONT_NAME:
        {
            if (!SetProperty(Style->_FontName, Text))
                return;

            break;
        }

        case IDC_FONT_SIZE:
        {
            if (!SetProperty(Style->_FontSize, (FLOAT) std::clamp(::_wtof(Text), MinFontSize, MaxFontSize)))
                return;

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

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        case IDC_OPACITY:
        {
            SetInteger(id, (int64_t) (_StyleManager->GetStyle(_ActiveStyles[_SelectedStyle])->_Opacity * 100.f));
            break;
        }

        case IDC_THICKNESS:
        {
            SetDouble(id, _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle])->_Thickness, 0, 1);
            break;
        }

        case IDC_FONT_NAME:
        {
            break;
        }

        case IDC_FONT_SIZE:
        {
            SetDouble(id, _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle])->_FontSize, 0, 1);
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

    auto ChangedSettings = ConfigurationChanges::All;

    switch (id)
    {
        default:
            return;

        case IDC_ADD:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (Style->_ColorScheme != ColorScheme::Custom)
            {
                Style->_ColorScheme    = ColorScheme::Custom;
                Style->_CustomGradient = gradient_t::ConvertFormat(Style->_CurrentGradientStops);
            }

            {
                const auto SelectedColor = _ColorListBox.GetCurSel();

                if (!IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
                    return;

                auto Color = Style->_CustomGradient[(size_t) SelectedColor].color;

                {
                    color_dialog_t cd;

                    if (!cd.SelectColor(m_hWnd, Color))
                        return;
                }

                Style->_CustomGradient.insert(Style->_CustomGradient.begin() + SelectedColor + 1, { { 0.f, Color }, GradientStopSource::Solid, 0 });
            
                UpdateGradientStopPositons(Style->_CustomGradient, (size_t) (SelectedColor + 1));
            }

            UpdateColorControls();
            break;
        }

        case IDC_REMOVE:
        {
            // Don't remove the last color.
            if (_ColorListBox.GetCount() == 1)
                return;

            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (Style->_ColorScheme != ColorScheme::Custom)
            {
                Style->_ColorScheme    = ColorScheme::Custom;
                Style->_CustomGradient = gradient_t::ConvertFormat(Style->_CurrentGradientStops);
            }

            {
                const auto SelectedColor = _ColorListBox.GetCurSel();

                if (!IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
                    return;

                Style->_CustomGradient.erase(Style->_CustomGradient.begin() + SelectedColor);

                UpdateGradientStopPositons(Style->_CustomGradient, (size_t) (SelectedColor + 1));
            }

            UpdateColorControls();
            break;
        }

        case IDC_REVERSE:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (Style->_ColorScheme != ColorScheme::Custom)
            {
                Style->_ColorScheme    = ColorScheme::Custom;
                Style->_CustomGradient = gradient_t::ConvertFormat(Style->_CurrentGradientStops);
            }

            {
                std::reverse(Style->_CustomGradient.begin(), Style->_CustomGradient.end());

                for (auto & gs : Style->_CustomGradient)
                    gs.position = 1.f - gs.position;
            }

            UpdateColorControls();
            break;
        }

        case IDC_SPREAD:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (Style->_ColorScheme != ColorScheme::Custom)
            {
                Style->_ColorScheme    = ColorScheme::Custom;
                Style->_CustomGradient = gradient_t::ConvertFormat(Style->_CurrentGradientStops);
            }

            {
                UpdateGradientStopPositons(Style->_CustomGradient, ~(size_t) 0);
            }

            UpdateColorControls();
            break;
        }

        case IDC_HORIZONTAL_GRADIENT:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if ((bool) SendDlgItemMessageW(id, BM_GETCHECK))
                Set(Style->_Flags, style_t::Features::HorizontalGradient);
            else
                UnSet(Style->_Flags, style_t::Features::HorizontalGradient);

            UpdateControls();
            break;
        }

        case IDC_AMPLITUDE_BASED:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if ((bool) SendDlgItemMessageW(id, BM_GETCHECK))
                Set(Style->_Flags, style_t::Features::AmplitudeBasedColor);
            else
                UnSet(Style->_Flags, style_t::Features::AmplitudeBasedColor);
            break;
        }

        case IDC_FONT_NAME_SELECT:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            UINT DPI;

            GetDPI(m_hWnd, DPI);

            LOGFONTW lf =
            {
                .lfHeight         = -::MulDiv((int) Style->_FontSize, (int) DPI, 72),
                .lfWeight         = FW_NORMAL,
                .lfCharSet        = DEFAULT_CHARSET,
                .lfOutPrecision   = OUT_DEFAULT_PRECIS,
                .lfClipPrecision  = CLIP_DEFAULT_PRECIS,
                .lfQuality        = CLEARTYPE_QUALITY,
                .lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE,
            };

            ::wcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), Style->_FontName.c_str());

            CHOOSEFONTW cf =
            {
                .lStructSize = sizeof(cf),
                .hwndOwner   = m_hWnd,
                .lpLogFont   = &lf,
                .iPointSize  = (INT) (Style->_FontSize * 10.f),
                .Flags       = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT,
             };

            if (!::ChooseFontW(&cf))
                return;

            Style->_FontName = cf.lpLogFont->lfFaceName;
            Style->_FontSize = (FLOAT) cf.iPointSize / 10.f;

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
    if ((_State == nullptr) || (id != IDC_GRADIENT_COLORS))
        return;

    SetMsgHandled(FALSE);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT styles_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = ConfigurationChanges::All;

    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_OPACITY_SPIN:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (!SetProperty(Style->_Opacity, (FLOAT) ClampNewSpinPosition(nmud, MinOpacity, MaxOpacity, 100.)))
                return -1;

            SetInteger(IDC_OPACITY, (int64_t) (Style->_Opacity * 100.f));
            break;
        }

        case IDC_THICKNESS_SPIN:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            if (!SetProperty(Style->_Thickness, (FLOAT) ClampNewSpinPosition(nmud, MinThickness, MaxThickness, 10.)))
                return -1;

            SetDouble(IDC_THICKNESS, Style->_Thickness, 0, 1);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}

/// <summary>
/// Handles a Change notification from the custom controls.
/// </summary>
LRESULT styles_page_t::OnChanged(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = ConfigurationChanges::All;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_GRADIENT_COLORS:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            std::vector<D2D1_COLOR_F> Colors;

            {
                _ColorListBox.GetColors(Colors);

                if (Colors.empty())
                    return 0;
            }

            Style->_ColorScheme = ColorScheme::Custom;

            if (Style->_ColorSource != ColorSource::Gradient)
            {
                // Initialize the custom gradient with the current colors.
                Style->_CurrentGradientStops = gradient_t::CreateGradientStops(Colors);
                Style->_CustomGradient       = gradient_t::ConvertFormat(Style->_CurrentGradientStops);
            }
            else
            {
                assert(Colors.size() == Style->_CustomGradient.size());

                // Update the custom gradient with the current colors.
                for (size_t i = 0; i < Colors.size(); ++i)
                    Style->_CustomGradient[i].color = Colors[i];

                Style->_CurrentGradientStops = gradient_t::ConvertFormat(Style->_CustomGradient);
            }

            // Update the controls.
            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) Style->_ColorScheme);
            _GradientButton.SetGradientStops(Style->_CurrentGradientStops);
            break;
        }

        case IDC_COLOR_BUTTON:
        {
            style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

            _ColorButton.GetColor(Style->_CustomColor);

            Style->_ColorSource  = ColorSource::Solid; // Force the color source to Solid.
            Style->_CurrentColor = Style->_CustomColor;

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

    static_assert(_countof(_StyleDisplayOrder) == (size_t) VisualElement::Count, "");

    for (const auto & Id : _StyleDisplayOrder)
    {
        const style_t * const Style = _StyleManager->GetStyle(Id);

        if ((uint64_t) Style->_UsedBy & (uint64_t) User)
        {
            _ActiveStyles.push_back(Id);

            w.AddString(Style->_Name.c_str());
        }
    }

    _SelectedStyle = 0;

    w.SetCurSel((int) _SelectedStyle);
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void styles_page_t::UpdateControls() noexcept
{
    if (_ActiveStyles.empty())
        return;

    auto Scope = msc::toggle_t(_IgnoreNotifications, true);

    // Update the Scope combobox.
    {
        GetDlgItem(IDC_SCOPE).EnableWindow(_State->_GraphOptions.size() > 1);
    }

    style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

    switch (Style->_ColorSource)
    {
        case ColorSource::None:
        case ColorSource::Solid:
        case ColorSource::DominantColor:
            break;

        case ColorSource::Gradient:
        {
            if (Style->_ColorScheme == ColorScheme::Custom)
            {
                Style->_CurrentGradientStops = gradient_t::ConvertFormat(Style->_CustomGradient);
            }
            else
            if (Style->_ColorScheme == ColorScheme::Artwork)
            {
                Style->_CurrentGradientStops = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : gradient_t::GetBuiltIn(ColorScheme::Artwork);
            }
            else
            {
                Style->_CurrentGradientStops = gradient_t::GetBuiltIn(Style->_ColorScheme);
            }
            break;
        }

        case ColorSource::Windows:
        {
            auto w = (CComboBox) GetDlgItem(IDC_COLOR_INDEX);

            w.ResetContent();

            for (const auto & x : { L"Window Background", L"Window Text", L"Button Background", L"Button Text", L"Highlight Background", L"Highlight Text", L"Gray Text", L"Hot Light" })
                w.AddString(x);

            w.SetCurSel((int) std::clamp(Style->_ColorIndex, 0u, (uint32_t) (w.GetCount() - 1)));
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

            w.SetCurSel((int) std::clamp(Style->_ColorIndex, 0u, (uint32_t) (w.GetCount() - 1)));
            break;
        }
    }

    // Updates the current color based on the color source.
    Style->SetColor(_State);

    ((CComboBox) GetDlgItem(IDC_COLOR_SOURCE)).SetCurSel((int) Style->_ColorSource);

    SendDlgItemMessageW(IDC_HORIZONTAL_GRADIENT, BM_SETCHECK, (WPARAM) Style->Has(style_t::Features::HorizontalGradient));
    SendDlgItemMessageW(IDC_AMPLITUDE_BASED,     BM_SETCHECK, (WPARAM) Style->Has(style_t::Features::AmplitudeBasedColor));

    SetInteger(IDC_OPACITY, (int64_t) (Style->_Opacity * 100.f));
    ((CUpDownCtrl) GetDlgItem(IDC_OPACITY_SPIN)).SetPos32((int) (Style->_Opacity * 100.f));

    SetDouble(IDC_THICKNESS, Style->_Thickness, 0, 1);
    ((CUpDownCtrl) GetDlgItem(IDC_THICKNESS_SPIN)).SetPos32((int) (Style->_Thickness * 10.f));

    SetDlgItemTextW(IDC_FONT_NAME, Style->_FontName.c_str());
    SetDouble(IDC_FONT_SIZE, Style->_FontSize, 0, 1);

    // Enable the controls as necessary.
    {
        const bool IsGradient = Style->_ColorSource == ColorSource::Gradient;

        GetDlgItem(IDC_COLOR_INDEX)        .EnableWindow((Style->_ColorSource == ColorSource::Windows) || (Style->_ColorSource == ColorSource::UserInterface));
        GetDlgItem(IDC_COLOR_BUTTON)       .EnableWindow(!IsGradient);
        GetDlgItem(IDC_COLOR_SCHEME)       .EnableWindow( IsGradient);
        GetDlgItem(IDC_GRADIENT)           .EnableWindow( IsGradient);

        GetDlgItem(IDC_HORIZONTAL_GRADIENT).EnableWindow(IsGradient);
        GetDlgItem(IDC_AMPLITUDE_BASED)    .EnableWindow(IsGradient && Style->Has(style_t::Features::AmplitudeAware | style_t::Features::HorizontalGradient));

        GetDlgItem(IDC_OPACITY)            .EnableWindow(Style->IsEnabled() && Style->Has(style_t::Features::SupportsOpacity));
        GetDlgItem(IDC_THICKNESS)          .EnableWindow(Style->IsEnabled() && Style->Has(style_t::Features::SupportsThickness));

        GetDlgItem(IDC_FONT_NAME)          .EnableWindow(Style->IsEnabled() && Style->Has(style_t::Features::SupportsFont));
        GetDlgItem(IDC_FONT_SIZE)          .EnableWindow(Style->IsEnabled() && Style->Has(style_t::Features::SupportsFont));
    }

    UpdateColorControls();
}

/// <summary>
/// Updates the color controls with the current configuration.
/// </summary>
void styles_page_t::UpdateColorControls() noexcept
{
    const style_t * const Style = _StyleManager->GetStyle(_ActiveStyles[_SelectedStyle]);

    const bool IsArtworkScheme = (Style->_ColorScheme == ColorScheme::Artwork); // Gradient controls are disabled when the artwork provides the colors.

    // Initializes a gradient stops vector.
    std::vector<D2D1_GRADIENT_STOP> gs;

    if (Style->_ColorSource == ColorSource::Gradient)
    {
        ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) Style->_ColorScheme);

        if (Style->_ColorScheme == ColorScheme::Custom)
        {
            gs = gradient_t::ConvertFormat(Style->_CustomGradient);
        }
        else
        if (Style->_ColorScheme == ColorScheme::Artwork)
        {
            gs = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : gradient_t::GetBuiltIn(ColorScheme::Artwork);
        }
        else
        {
            gs = gradient_t::GetBuiltIn(Style->_ColorScheme);
        }
    }

    // Update the color button.
    _ColorButton.SetColor(Style->_CurrentColor);

    // Update the gradient control.
    _GradientButton.SetGradientStops(gs);
;
    // Update the color list.
    std::vector<D2D1_COLOR_F> Colors;

    bool IsIndexedGradientColor = false;

    if (Style->_ColorSource == ColorSource::Gradient)
    {
        const bool IsCustomScheme = (Style->_ColorScheme == ColorScheme::Custom);

        // Save the selected color.
        int SelectedColor = _ColorListBox.GetCurSel();

        // Update the list box with the colors of the custom gradient.
        {
            for (const auto & Iter : gs)
                Colors.push_back(Iter.color);

            _ColorListBox.SetColors(Colors);
        }

        // Update the gradient controls.
        if (IsValidColorIndex(SelectedColor, Style->_CustomGradient.size()))
        {
            SelectedColor = std::clamp(SelectedColor, 0, (int) gs.size() - 1);

            // Restore the selected color.
            _ColorListBox.SetCurSel(SelectedColor);

            // Update the position control.
            {
                auto Scope = msc::toggle_t(_IgnoreNotifications, true);

                const int64_t Position = (int64_t) (gs[(size_t) SelectedColor].position * 100.f);

                SetInteger(IDC_POSITION, Position);
            }

            // Update the gradient stop controls.
            if (IsCustomScheme)
            {
                const auto & cgs = Style->_CustomGradient[(size_t) SelectedColor];

                IsIndexedGradientColor = ((cgs.ColorSource == GradientStopSource::Windows) || (cgs.ColorSource == GradientStopSource::UserInterface));

                ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE)).SetCurSel((int) cgs.ColorSource);
                ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_INDEX)) .SetCurSel(IsIndexedGradientColor ? (int) cgs.ColorIndex : -1);

            }
            else
            {
                ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE)).SetCurSel(-1);
                ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_INDEX)) .SetCurSel(-1);
            }
        }
        else
        {
            ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE)).SetCurSel(-1);
        }

        UpdateColorIndexControl(Style, SelectedColor);
    }
    else
    {
        // Initializes the color list box.
        _ColorListBox.SetColors(Colors);

        ((CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_SOURCE)).SetCurSel(-1);
    }

    // Enable the gradient controls as necessary.
    {
        const bool HasSelection        = (_ColorListBox.GetCurSel() != LB_ERR);         // Add and Remove are only enabled when a color is selected.
        const bool HasMoreThanOneColor = (gs.size() > 1);                               // Remove and Reverse are only enabled when there is more than 1 color.

        GetDlgItem(IDC_ADD)     .EnableWindow(HasSelection &&                        !IsArtworkScheme);
        GetDlgItem(IDC_REMOVE)  .EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);

        GetDlgItem(IDC_REVERSE) .EnableWindow(                HasMoreThanOneColor && !IsArtworkScheme);

        GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);
        GetDlgItem(IDC_SPREAD)  .EnableWindow(HasSelection && HasMoreThanOneColor && !IsArtworkScheme);

        GetDlgItem(IDC_GRADIENT_COLOR_SOURCE).EnableWindow(HasSelection);
        GetDlgItem(IDC_GRADIENT_COLOR_INDEX) .EnableWindow(IsIndexedGradientColor);
    }
}

/// <summary>
/// Updates the positions of the current gradient colors starting at the specified index.
/// </summary>
void styles_page_t::UpdateGradientStopPositons(std::vector<gradient_stop_t> & gs, size_t index) const noexcept
{
    if (gs.empty())
        return;

    if (gs.size() == 1)
    {
        gs[0].position = 0.f;

        return;
    }

    if (index == ~(size_t) 0)
    {
        // Spread the positions of the stops between 0 and 1.
        const FLOAT Denominator = (FLOAT) (gs.size() - 1);

        for (size_t i = 0; i < gs.size(); ++i)
            gs[i].position = (FLOAT) i / Denominator;

        return;
    }

    if (index >= gs.size())
        index = gs.size() - 1;

    if (index == 0)
    {
        gs[index].position = 0.f;

        return;
    }

    if (index == gs.size() - 1)
    {
        gs[index].position = 1.f;

        return;
    }

    gs[index].position = std::clamp(gs[index - 1].position + (gs[index + 1].position - gs[index - 1].position) / 2.f, 0.f, 1.f);
}

/// <summary>
/// Configures the gradient stop source and gradent stop index controls based on the selected style and color index.
/// </summary>
void styles_page_t::UpdateColorIndexControl(const style_t * style, int colorIndex) noexcept
{
    auto w = (CComboBox) GetDlgItem(IDC_GRADIENT_COLOR_INDEX);

    w.ResetContent();

    if (!IsValidColorIndex(colorIndex, style->_CustomGradient.size()))
        return;

    const auto & gss = style->_CustomGradient[(size_t) colorIndex];

    if (gss.ColorSource == GradientStopSource::Windows)
    {
        for (const auto & x : { L"Window Background", L"Window Text", L"Button Background", L"Button Text", L"Highlight Background", L"Highlight Text", L"Gray Text", L"Hot Light" })
            w.AddString(x);

        w.SetCurSel((int) std::clamp(gss.ColorIndex, 0u, (uint32_t) (w.GetCount() - 1)));

        return;
    }

    if (gss.ColorSource == GradientStopSource::UserInterface)
    {
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

        w.SetCurSel((int) std::clamp(gss.ColorIndex, 0u, (uint32_t) (w.GetCount() - 1)));

        return;
    }
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT styles_page_t::OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    __super::OnConfigurationChanged(msg, wParam, lParam);

    switch (wParam)
    {
        case CC_COLORS:
        {
            auto Scope = msc::toggle_t(_IsInitializing, true);

            UpdateControls();
            break;
        }
    }

    return 0;
}
