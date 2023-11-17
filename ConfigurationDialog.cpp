
/** $VER: ConfigurationDialog.cpp (2023.11.16) P. Stuer - Implements the configuration dialog. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"

#include "ConfigurationDialog.h"

void ConfigurationDialog::Initialize()
{
    #pragma region Frequencies
    {
        auto w = (CComboBox) GetDlgItem(IDC_FREQUENCIES);

        const WCHAR * Labels[] = { L"Linear", L"Octaves", L"AveePlayer" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_FrequencyDistribution);
    }
    #pragma endregion

    #pragma region FFT Size
    {
        auto w = (CComboBox) GetDlgItem(IDC_FFT_SIZE);

        int SelectedIndex = -1;

        for (int i = 64, j = 0; i <= 32768; i *= 2, ++j)
        {
            w.AddString(pfc::wideFromUTF8(pfc::format_int(i)));

            if (i == (int) _Configuration->_FFTSize)
                SelectedIndex = j;
        }

        w.SetCurSel(SelectedIndex);
    }
    #pragma endregion

    #pragma region X Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_X_AXIS);

        const WCHAR * Labels[] = { L"Bands", L"Decades", L"Octaves", L"Notes" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_XAxisMode);
    }
    #pragma endregion

    #pragma region Y Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_Y_AXIS);

        const WCHAR * Labels[] = { L"Decibel", L"Logarithmic" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_YAxisMode);
    }
    #pragma endregion

    #pragma region Color Scheme
    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        const WCHAR * Labels[] = { L"Solid", L"Custom", L"Prism 1", L"Prism 2", L"Prism 3", L"foobar2000", L"foobar2000 Dark Mode" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_ColorScheme);
    }

    #pragma region Peak Mode
    {
        auto w = (CComboBox) GetDlgItem(IDC_PEAK_MODE);

        const WCHAR * Labels[] = { L"None", L"Classic", L"Gravity", L"AIMP", L"Fade Out" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_ColorScheme);
    }
    #pragma endregion
}
