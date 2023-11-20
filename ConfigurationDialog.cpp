
/** $VER: ConfigurationDialog.cpp (2023.11.20) P. Stuer - Implements the configuration dialog. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "ConfigurationDialog.h"
#include "Math.h"

/// <summary>
/// Initializes the controls of the dialog.
/// </summary>
void ConfigurationDialog::Initialize()
{
    #pragma region Transform
    {
        auto w = (CComboBox) GetDlgItem(IDC_TRANSFORM);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Fast Fourier", L"Constant-Q" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._Transform);
    }
    #pragma endregion

    #pragma region FFT
    {
        auto w = (CComboBox) GetDlgItem(IDC_FFT_SIZE);

        w.ResetContent();

        WCHAR Text[32] = { };

        for (int i = 64, j = 0; i <= 32768; i *= 2, ++j)
        {
            ::StringCchPrintfW(Text, _countof(Text), L"%i", i);

            w.AddString(Text);
        }

        w.AddString(L"Custom");
        w.AddString(L"Sample rate based");

        w.SetCurSel((int) _Configuration._FFTSize);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_SUMMATION_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Minimum", L"Maximum", L"Sum", L"Residual Mean Square (RMS)", L"RMS Sum", L"Average", L"Median" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._SummationMethod);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_MAPPING_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Standard", L"Mel" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._MappingMethod);
    }
    {
        SendDlgItemMessageW(IDC_SMOOTH_LOWER_FREQUENCIES, BM_SETCHECK, _Configuration._SmoothLowerFrequencies);
        SendDlgItemMessageW(IDC_SMOOTH_GAIN_TRANSITION, BM_SETCHECK, _Configuration._SmoothGainTransition);
    }
    {
        SetDlgItemTextW(IDC_KERNEL_SIZE, pfc::wideFromUTF8(pfc::format_int(_Configuration._KernelSize)));
    }
    #pragma endregion

    #pragma region Frequencies
    {
        auto w = (CComboBox) GetDlgItem(IDC_DISTRIBUTION);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Linear", L"Octaves", L"AveePlayer" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._FrequencyDistribution);

        SetDlgItemTextW(IDC_NUM_BANDS, pfc::wideFromUTF8(pfc::format_int((t_int64) _Configuration._NumBands)));

        SetDlgItemTextW(IDC_MIN_FREQUENCY, pfc::wideFromUTF8(pfc::format_int(_Configuration._MinFrequency)));
        SetDlgItemTextW(IDC_MAX_FREQUENCY, pfc::wideFromUTF8(pfc::format_int(_Configuration._MaxFrequency)));

        SetDlgItemTextW(IDC_MIN_NOTE, pfc::wideFromUTF8(pfc::format_int(_Configuration._MinNote)));
        SetDlgItemTextW(IDC_MAX_NOTE, pfc::wideFromUTF8(pfc::format_int(_Configuration._MaxNote)));
        SetDlgItemTextW(IDC_BANDS_PER_OCTAVE, pfc::wideFromUTF8(pfc::format_int(_Configuration._BandsPerOctave)));
        SetDlgItemTextW(IDC_PITCH, pfc::wideFromUTF8(pfc::format_float(_Configuration._Pitch, 0, 1)));
        SetDlgItemTextW(IDC_TRANSPOSE, pfc::wideFromUTF8(pfc::format_int(_Configuration._Transpose)));
        {
            auto w = (CComboBox) GetDlgItem(IDC_SCALING_FUNCTION);

            w.ResetContent();

            const WCHAR * Labels[] = { L"Linear", L"Logarithmic", L"Shifted Logarithmic", L"Mel", L"Bark", L"Adjustable Bark", L"ERB", L"Cams", L"Hyperbolic Sine", L"Nth Root", L"Negative Exponential", L"Period" };

            for (size_t i = 0; i < _countof(Labels); ++i)
                w.AddString(Labels[i]);

            w.SetCurSel((int) _Configuration._ScalingFunction);
        }
        SetDlgItemTextW(IDC_SKEW_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration._SkewFactor, 0, 1)));
        SetDlgItemTextW(IDC_BANDWIDTH, pfc::wideFromUTF8(pfc::format_float(_Configuration._Bandwidth, 0, 1)));
    }
    #pragma endregion

    #pragma region X Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_X_AXIS);

        w.ResetContent();

        const WCHAR * Labels[] = { L"None", L"Bands", L"Decades", L"Octaves", L"Notes" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._XAxisMode);
    }
    #pragma endregion

    #pragma region Y Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_Y_AXIS);

        w.ResetContent();

        const WCHAR * Labels[] = { L"None", L"Decibel", L"Logarithmic" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration._YAxisMode);

        SetDlgItemTextW(IDC_MIN_DECIBEL, pfc::wideFromUTF8(pfc::format_float(_Configuration._MinDecibel, 0, 1)));
        SetDlgItemTextW(IDC_MAX_DECIBEL, pfc::wideFromUTF8(pfc::format_float(_Configuration._MaxDecibel, 0, 1)));
        SendDlgItemMessageW(IDC_USE_ABSOLUTE, BM_SETCHECK, _Configuration._UseAbsolute);
        SetDlgItemTextW(IDC_GAMMA, pfc::wideFromUTF8(pfc::format_float(_Configuration._Gamma, 0, 1)));
    }
    #pragma endregion

    #pragma region Rendering
    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Solid", L"Custom", L"Prism 1", L"Prism 2", L"Prism 3", L"foobar2000", L"foobar2000 Dark Mode" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration._ColorScheme);
    }
    {
        SendDlgItemMessageW(IDC_DRAW_BAND_BACKGROUND, BM_SETCHECK, _Configuration._DrawBandBackground);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_SMOOTHING_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Average", L"Peak" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }
        w.SetCurSel((int) _Configuration._SmoothingMethod);

        SetDlgItemTextW(IDC_SMOOTHING_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration._SmoothingFactor, 0, 1)));
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_PEAK_MODE);

        w.ResetContent();

        const WCHAR * Labels[] = { L"None", L"Classic", L"Gravity", L"AIMP", L"Fade Out" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration._PeakMode);
    }
    {
        SetDlgItemTextW(IDC_HOLD_TIME, pfc::wideFromUTF8(pfc::format_float(_Configuration._HoldTime, 0, 1)));
        SetDlgItemTextW(IDC_ACCELERATION, pfc::wideFromUTF8(pfc::format_float(_Configuration._Acceleration, 0, 1)));
    }
    #pragma endregion

    UpdateControls();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void ConfigurationDialog::OnSelectionChanged(UINT, int id, CWindow w)
{
    CComboBox cb = (CComboBox) w;

    int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
    #pragma region Transform
        case IDC_TRANSFORM:
        {
            _Configuration._Transform = (Transform) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region FFT
        case IDC_FFT_SIZE:
        {
            _Configuration._FFTSize = (FFTSize) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_MAPPING_METHOD:
        {
            _Configuration._MappingMethod = (Mapping) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region Frequencies
        case IDC_DISTRIBUTION:
        {
            _Configuration._FrequencyDistribution = (FrequencyDistribution) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_SCALING_FUNCTION:
        {
            _Configuration._ScalingFunction = (ScalingFunction) SelectedIndex;
            break;
        }

        case IDC_SUMMATION_METHOD:
        {
            _Configuration._SummationMethod = (SummationMethod) SelectedIndex;
            break;
        }

        case IDC_SMOOTHING_METHOD:
        {
            _Configuration._SmoothingMethod = (SmoothingMethod) SelectedIndex;
            break;
        }
    #pragma endregion

    #pragma region X axis
        case IDC_X_AXIS:
        {
            _Configuration._XAxisMode = (XAxisMode) SelectedIndex;
            break;
        }

    #pragma region Y axis
        case IDC_Y_AXIS:
        {
            _Configuration._YAxisMode = (YAxisMode) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region Rendering
        case IDC_COLOR_SCHEME:
        {
            _Configuration._ColorScheme = (ColorScheme) SelectedIndex;
            break;
        }

        case IDC_PEAK_MODE:
        {
            _Configuration._PeakMode = (PeakMode) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void ConfigurationDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if (code != EN_CHANGE)
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
    #pragma region Transform
        case IDC_FFT_SIZE_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_Configuration._FFTSize)
            {
                default:
                    break;

                case FFTSize::FFTCustom:
                {
                    int Value = ::_wtoi(Text);

                    if (!InInterval(Value, 1, 32768))
                        return;

                    _Configuration._FFTCustom = (size_t) Value;
                    break;
                }

                case FFTSize::FFTDuration:
                {
                    double Value = ::_wtof(Text);

                    if (!InInterval(Value, 1., 100.))
                        return;

                    _Configuration._FFTDuration= Value;
                    break;
                }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE:
        {
            int Value = ::_wtoi(Text);

            if (!InInterval(Value, 1, 64))
                return;

            _Configuration._KernelSize = Value;
            break;
        }

        case IDC_GAMMA:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0.5, 10.0))
                return;

            _Configuration._Gamma = Value;
            break;
        }
    #pragma endregion

    #pragma region Frequencies
        case IDC_NUM_BANDS:
        {
            int Value = ::_wtoi(Text);

            if (!InInterval(Value, 2, 512))
                return;

            _Configuration._NumBands = (size_t) Value;
            break;
        }

        case IDC_MIN_FREQUENCY:
        {
            int Value = ::_wtoi(Text);

            if ((!InInterval(Value, 0, 96000)) || (Value >= (int) _Configuration._MaxFrequency))
                return;

            _Configuration._MinFrequency = (uint32_t) Value;
            break;
        }

        case IDC_MAX_FREQUENCY:
        {
            int Value = ::_wtoi(Text);

            if ((!InInterval(Value, 0, 96000)) || (Value <= (int) _Configuration._MinFrequency))
                return;

            _Configuration._MaxFrequency = (uint32_t) Value;
            break;
        }

        case IDC_MIN_NOTE:
        {
            int Value = ::_wtoi(Text);

            if ((!InInterval(Value, 0, 143)) || (Value >= (int) _Configuration._MaxNote))
                return;

            _Configuration._MinNote = (uint32_t) Value;
            break;
        }

        case IDC_MAX_NOTE:
        {
            int Value = ::_wtoi(Text);

            if ((!InInterval(Value, 0, 143)) || (Value <= (int) _Configuration._MinNote))
                return;

            _Configuration._MaxNote = (uint32_t) Value;
            break;
        }

        case IDC_BANDS_PER_OCTAVE:
        {
            int Value = ::_wtoi(Text);

            if (!InInterval(Value, 1, 48))
                return;

            _Configuration._BandsPerOctave = (uint32_t) Value;
            break;
        }

        case IDC_PITCH:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0., 96000.))
                return;

            _Configuration._Pitch = Value;
            break;
        }

        case IDC_SKEW_FACTOR:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0., 1.))
                return;

            _Configuration._SkewFactor = Value;
            break;
        }

        case IDC_BANDWIDTH:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0., 64.))
                return;

            _Configuration._Bandwidth = Value;
            break;
        }
    #pragma endregion

    #pragma region Y axis
        case IDC_MIN_DECIBEL:
        {
            double Value = ::_wtof(Text);

            if ((!InInterval(Value, -120., 0.)) || (Value >= _Configuration._MaxDecibel))
                return;

            _Configuration._MinDecibel = Value;
            break;
        }

        case IDC_MAX_DECIBEL:
        {
            double Value = ::_wtof(Text);

            if ((!InInterval(Value, -120., 0.)) || (Value <= _Configuration._MinDecibel))
                return;

            _Configuration._MaxDecibel = Value;
            break;
        }
    #pragma endregion

    #pragma region Peak indicator
        case IDC_HOLD_TIME:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0., 120.))
                return;

            _Configuration._HoldTime = Value;
            break;
        }

        case IDC_ACCELERATION:
        {
            double Value = ::_wtof(Text);

            if (!InInterval(Value, 0., 2.))
                return;

            _Configuration._Acceleration = Value;
            break;
        }
    #pragma endregion

        default:
            return;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void ConfigurationDialog::OnButtonClick(UINT, int id, CWindow)
{
    switch (id)
    {
        case IDC_SMOOTH_LOWER_FREQUENCIES:
        {
            _Configuration._SmoothLowerFrequencies = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SMOOTH_GAIN_TRANSITION:
        {
            _Configuration._SmoothGainTransition = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_USE_ABSOLUTE:
        {
            _Configuration._UseAbsolute = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_DRAW_BAND_BACKGROUND:
        {
            _Configuration._DrawBandBackground = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_RESET:
        {
            _Configuration.Reset();

            Initialize();
            break;
        }

        default:
            return;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Enables or disables controls based on the selection of the user.
/// </summary>
void ConfigurationDialog::UpdateControls()
{
    // Transform
    bool IsFFT = (_Configuration._Transform == Transform::FFT);

        GetDlgItem(IDC_DISTRIBUTION).EnableWindow(IsFFT);

    // FFT
        GetDlgItem(IDC_FFT_SIZE).EnableWindow(IsFFT);

    bool State = (_Configuration._FFTSize == FFTSize::FFTCustom) || (_Configuration._FFTSize == FFTSize::FFTDuration);

        GetDlgItem(IDC_FFT_SIZE_PARAMETER).EnableWindow(IsFFT && State);

        GetDlgItem(IDC_SUMMATION_METHOD).EnableWindow(IsFFT);
        GetDlgItem(IDC_MAPPING_METHOD).EnableWindow(IsFFT);
        GetDlgItem(IDC_SMOOTH_LOWER_FREQUENCIES).EnableWindow(IsFFT);
        GetDlgItem(IDC_SMOOTH_GAIN_TRANSITION).EnableWindow(IsFFT);
        GetDlgItem(IDC_KERNEL_SIZE).EnableWindow(IsFFT);

        #pragma warning (disable: 4061)
        switch (_Configuration._FFTSize)
        {
            default:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"");
                break;

            case FFTSize::FFTCustom:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER, pfc::wideFromUTF8(pfc::format_int((t_int64) _Configuration._FFTCustom)));
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"samples");
                break;

            case FFTSize::FFTDuration:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER, pfc::wideFromUTF8(pfc::format_float(_Configuration._FFTDuration, 0, 1)));
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"ms");
                break;
        }
        #pragma warning (default: 4061)

    // Frequencies
    State = (_Configuration._FrequencyDistribution != FrequencyDistribution::Octaves);

        GetDlgItem(IDC_NUM_BANDS).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_MIN_FREQUENCY).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_MAX_FREQUENCY).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_SKEW_FACTOR).EnableWindow(State);
        GetDlgItem(IDC_BANDWIDTH).EnableWindow(State);

        GetDlgItem(IDC_MIN_NOTE).EnableWindow(!State);
        GetDlgItem(IDC_MAX_NOTE).EnableWindow(!State);
        GetDlgItem(IDC_BANDS_PER_OCTAVE).EnableWindow(!State);
        GetDlgItem(IDC_PITCH).EnableWindow(!State);
        GetDlgItem(IDC_TRANSPOSE).EnableWindow(!State);

    State = (_Configuration._FrequencyDistribution == FrequencyDistribution::AveePlayer);

        GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(!State);

    // Y axis
    State = (_Configuration._YAxisMode == YAxisMode::Logarithmic);

        GetDlgItem(IDC_USE_ABSOLUTE).EnableWindow(State);
        GetDlgItem(IDC_GAMMA).EnableWindow(State);

    // Peak indicators
    State = (_Configuration._PeakMode == PeakMode::None);

        GetDlgItem(IDC_HOLD_TIME).EnableWindow(!State);
        GetDlgItem(IDC_ACCELERATION).EnableWindow(!State);
}
