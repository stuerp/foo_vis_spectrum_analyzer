
/** $VER: Page.h (2026.02.20) P. Stuer - Implements a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "DialogParameters.h"
#include "State.h"
#include "Resources.h"

#include "CNumericEdit.h"
#include "CColorButton.h"
#include "CColorListBox.h"

class page_t : public CDialogImpl<page_t>, public CDialogResize<page_t>
{
public:
    page_t(int id) : IDD(id), _IsInitializing(false), _IgnoreNotifications(false)
    {
    }

    virtual ~page_t() = default; // Virtual destructor for proper polymorphic deletion

    /// <summary>
    /// Initialize the dialog.
    /// </summary>
    virtual BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept
    {
        DlgResize_Init(false, true);

        auto dp = (const dialog_parameters_t *) lParam;

        _hParent = dp->_hWnd;
        _State = dp->_State;

        // Create the tooltip control.
        _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

        if (_ToolTipControl.IsWindow())
        {
            _ToolTipControl.SetMaxTipWidth(200);
            ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);
        }

        return TRUE;
    }

    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept
    {
    #ifdef _DEBUG
        return ::CreateSolidBrush(RGB(250, 250, 250));
    #else
        return FALSE;
    #endif
    }

    /// <summary>
    /// Handles the WM_DESTROY message.
    /// </summary>
    void OnDestroy() noexcept
    {
        Terminate();
    }

    virtual void OnSelectionChanged(UINT, int, CWindow) noexcept { };
    virtual void OnEditChange(UINT, int, CWindow) noexcept { };
    virtual void OnEditLostFocus(UINT code, int id, CWindow) noexcept { };
    virtual void OnButtonClick(UINT, int, CWindow) noexcept { };
    virtual void OnDoubleClick(UINT, int, CWindow) noexcept { };

    virtual LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept { return -1; }; // Prevent the change of the up-down control position.
    virtual LRESULT OnChanged(LPNMHDR nmhd) noexcept { return 0; };

    virtual LRESULT OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

protected:
    void ConfigurationChanged(Settings settings) const noexcept;

    virtual void UpdateControls() noexcept = 0;

    static int ClampNewSpinPosition(LPNMUPDOWN nmud, int minValue, int maxValue) noexcept;
    static double ClampNewSpinPosition(LPNMUPDOWN nmud, double minValue, double maxValue, double scale) noexcept;

    void SetInteger(int id, int64_t value) noexcept;
    void SetDouble(int id, double value, unsigned width = 0, unsigned precision = 2) noexcept;
    void SetNote(int id, uint32_t noteNumber) noexcept;

    /// <summary>
    /// Set the value of a property. Returns true if the property value actually changed.
    /// </summary>
    template <class T> bool SetProperty(T & propertyValue, T newValue)
    {
        if (propertyValue == newValue)
            return false;

        propertyValue = newValue;

        return true;
    }

private:
    void Terminate() noexcept;

    BEGIN_MSG_MAP(page_t)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
        MSG_WM_DESTROY(OnDestroy)

//      MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChanged)

        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnSelectionChanged) // This also handles LBN_SELCHANGE
        COMMAND_CODE_HANDLER_EX(LBN_DBLCLK, OnDoubleClick)
        COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
        COMMAND_CODE_HANDLER_EX(EN_KILLFOCUS, OnEditLostFocus)
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClick)

        NOTIFY_CODE_HANDLER_EX(UDN_DELTAPOS, OnDeltaPos)
        NOTIFY_CODE_HANDLER_EX(NM_CHANGED, OnChanged)

        CHAIN_MSG_MAP(CDialogResize<page_t>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(page_t)
    END_DLGRESIZE_MAP()

public:
    int IDD;

protected:
    HWND _hParent;
    state_t * _State;

    bool _IsInitializing;
    bool _IgnoreNotifications;

    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;

    CToolTipCtrl _ToolTipControl;

private:
    fb2k::CCoreDarkModeHooks _DarkMode;
};
