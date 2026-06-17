
/** $VER: StylesPage.h (2026.06.15) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include "Page.h"
#include "Resources.h"
#include "Constants.h"

class styles_page_t : public CDialogResize<styles_page_t>, public page_t
{
public:
    styles_page_t(int id) : page_t(id), _SelectedStyle() { }

    styles_page_t(const styles_page_t &) = delete;
    styles_page_t & operator=(const styles_page_t &) = delete;
    styles_page_t(styles_page_t &&) = delete;
    styles_page_t & operator=(styles_page_t &&) = delete;

    virtual ~styles_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;
    void OnDoubleClick(UINT code, int id, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept final;
    LRESULT OnChanged(LPNMHDR nmhd) noexcept override final;

    LRESULT OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept override final;

    BEGIN_MSG_MAP(styles_page_t)
        CHAIN_MSG_MAP(page_t)
        CHAIN_MSG_MAP(CDialogResize<styles_page_t>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(styles_page_t)
        DLGRESIZE_CONTROL(IDC_STYLES, DLSZ_SIZE_Y)

        DLGRESIZE_CONTROL(IDC_GRADIENT, DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_COLOR_LIST, DLSZ_SIZE_Y | DLSZ_REPAINT)

        DLGRESIZE_CONTROL(IDC_OPACITY_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_OPACITY, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_OPACITY_SPIN, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_OPACITY_UNIT, DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDC_THICKNESS_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_THICKNESS, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_THICKNESS_SPIN, DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDC_FONT_NAME_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FONT_NAME, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FONT_NAME_SELECT, DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDC_FONT_SIZE_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FONT_SIZE, DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

private:
    void InitializeControls() noexcept;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept;

    void InitializeStyles() noexcept;
    void UpdateColorControls() noexcept;
    void UpdateGradientStopPositons(style_t * style, size_t index) const noexcept;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;
    std::vector<VisualElement> _ActiveStyles;   // The styles that are relevant for the current visualization.
    size_t _SelectedStyle;                      // Index of the selected style in the listbox.

    CColorButton _Color;
    CColorButton _Gradient;
    CColorListBox _Colors;

    style_manager_t * _StyleManager;

    // An index of the styles to determine the display order in the configuration dialog.
    const VisualElement _StyleDisplayOrder[(size_t) VisualElement::Count] =
    {
        VisualElement::GraphBackground,
        VisualElement::GraphDescriptionText,
        VisualElement::GraphDescriptionBackground,

        VisualElement::XAxisText,
        VisualElement::XAxisLine,

        VisualElement::YAxisText,
        VisualElement::YAxisLine,

        VisualElement::HorizontalGridLine,
        VisualElement::VerticalGridLine,

        VisualElement::NyquistMarker,

        // Spectrum Analyzer
        VisualElement::BarArea,
        VisualElement::BarTop,

        VisualElement::BarPeakArea,
        VisualElement::BarPeakTop,

        VisualElement::BarDarkBackground,
        VisualElement::BarLightBackground,

        VisualElement::CurveLine,
        VisualElement::CurveArea,
        VisualElement::CurvePeakLine,
        VisualElement::CurvePeakArea,

        // Spectrogram
        VisualElement::Spectrogram,

        // Peak Meter
        VisualElement::BarBackground,
        VisualElement::BarPeakLevel,
        VisualElement::Bar0dBPeakLevel,
        VisualElement::BarMaxPeakLevel,
        VisualElement::BarPeakLevelText,
        VisualElement::BarRMSLevel,
        VisualElement::Bar0dBRMSLevel,
        VisualElement::BarRMSLevelText,

        // Level Meter
        VisualElement::BarLeftRight,
        VisualElement::BarLeftRightIndicator,
        VisualElement::BarMidSide,
        VisualElement::BarMidSideIndicator,
        VisualElement::LevelMeterAxis,

        // Oscilloscope
        VisualElement::SignalLine,

        // Bit Meter
        VisualElement::BarSign,
        VisualElement::BarExponent,
        VisualElement::BarMantissa,
    };
};
