
/** $VER: ConfigurationDialog.cpp (2023.12.02) P. Stuer - Implements the configuration dialog. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "ConfigurationDialog.h"
#include "Math.h"
#include "Gradients.h"

/// <summary>
/// Initializes the controls of the dialog.
/// </summary>
void ConfigurationDialog::Initialize()
{
    Terminate();

    #pragma region Transform
    {
        auto w = (CComboBox) GetDlgItem(IDC_TRANSFORM);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Fast Fourier", L"Constant-Q" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration->_Transform);
    }
    #pragma endregion

    #pragma region Channels
    {
        _Channels.Initialize(GetDlgItem(IDC_CHANNELS));

        _Channels.SetExtendedMenuStyle(BMS_EX_CHECKMARKS);
        _Channels.SetMenu(IDM_CHANNELS);

        UpdateChannelsMenu();
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

        w.SetCurSel((int) _Configuration->_FFTSize);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_SUMMATION_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Minimum", L"Maximum", L"Sum", L"Residual Mean Square (RMS)", L"RMS Sum", L"Average", L"Median" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration->_SummationMethod);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_MAPPING_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Standard", L"Triangular Filter Bank" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration->_MappingMethod);
    }
    {
        SendDlgItemMessageW(IDC_SMOOTH_LOWER_FREQUENCIES, BM_SETCHECK, _Configuration->_SmoothLowerFrequencies);
        SendDlgItemMessageW(IDC_SMOOTH_GAIN_TRANSITION, BM_SETCHECK, _Configuration->_SmoothGainTransition);
    }
    {
        _KernelSize.Initialize(GetDlgItem(IDC_KERNEL_SIZE));

        SetDlgItemTextW(IDC_KERNEL_SIZE, pfc::wideFromUTF8(pfc::format_int(_Configuration->_KernelSize)));

        auto w = CUpDownCtrl(GetDlgItem(IDC_KERNEL_SIZE_SPIN));

        w.SetRange32(MinKernelSize, MaxKernelSize);
        w.SetPos32(_Configuration->_KernelSize);
    }
    #pragma endregion

    #pragma region Frequencies
    {
        {
            auto w = (CComboBox) GetDlgItem(IDC_DISTRIBUTION);

            w.ResetContent();

            const WCHAR * Labels[] = { L"Linear", L"Octaves", L"AveePlayer" };

            for (size_t i = 0; i < _countof(Labels); ++i)
                w.AddString(Labels[i]);

            w.SetCurSel((int) _Configuration->_FrequencyDistribution);
        }

        {
            UDACCEL Accel[] =
            {
                { 0,  1 },
                { 1, 10 },
                { 2, 50 },
            };

            _NumBands.Initialize(GetDlgItem(IDC_NUM_BANDS));

            SetDlgItemTextW(IDC_NUM_BANDS, pfc::wideFromUTF8(pfc::format_int((t_int64) _Configuration->_NumBands)));

            auto w = CUpDownCtrl(GetDlgItem(IDC_NUM_BANDS_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32(MinBands, MaxBands);
            w.SetPos32((int) _Configuration->_NumBands);
        }

        {
            UDACCEL Accel[] =
            {
                { 0,     100 }, //     1.0
                { 1,    1000 }, //    10.0
                { 2,    2500 }, //    25.0
                { 3,    5000 }, //    50.0
                { 4,   10000 }, //   100.0
                { 5,  100000 }, //  1000.0
                { 6, 1000000 }, // 10000.0
            };

            {
                _LoFrequency.Initialize(GetDlgItem(IDC_LO_FREQUENCY));

                SetFrequency(IDC_LO_FREQUENCY, _Configuration->_LoFrequency);

                auto w = CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
                w.SetPos32((int)(_Configuration->_LoFrequency * 100.));
            }

            {
                _HiFrequency.Initialize(GetDlgItem(IDC_HI_FREQUENCY));

                SetFrequency(IDC_HI_FREQUENCY, _Configuration->_HiFrequency);

                auto w = CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
                w.SetPos32((int)(_Configuration->_HiFrequency * 100.));
            }
        }

        {
            _MinNote.Initialize(GetDlgItem(IDC_MIN_NOTE));

            SetNote(IDC_MIN_NOTE, _Configuration->_MinNote);

            auto w = CUpDownCtrl(GetDlgItem(IDC_MIN_NOTE_SPIN));

            w.SetRange32(MinNote, MaxNote);
            w.SetPos32((int) _Configuration->_MinNote);
        }

        {
            _MaxNote.Initialize(GetDlgItem(IDC_MAX_NOTE));

            SetNote(IDC_MAX_NOTE, _Configuration->_MaxNote);

            auto w = CUpDownCtrl(GetDlgItem(IDC_MAX_NOTE_SPIN));

            w.SetRange32(MinNote, MaxNote);
            w.SetPos32((int) _Configuration->_MaxNote);
        }

        {
            _BandsPerOctave.Initialize(GetDlgItem(IDC_BANDS_PER_OCTAVE));

            SetDlgItemTextW(IDC_BANDS_PER_OCTAVE, pfc::wideFromUTF8(pfc::format_int(_Configuration->_BandsPerOctave)));

            auto w = CUpDownCtrl(GetDlgItem(IDC_BANDS_PER_OCTAVE_SPIN));

            w.SetRange32(MinBandsPerOctave, MaxBandsPerOctave);
            w.SetPos32((int) _Configuration->_BandsPerOctave);
        }

        {
            UDACCEL Accel[] =
            {
                { 0,    50 }, //   0.5
                { 1,   100 }, //   1.0
                { 2,  1000 }, //  10.0
                { 3,  2500 }, //  25.0
                { 4,  5000 }, //  50.0
                { 5, 10000 }, // 100.0
            };

            _Pitch.Initialize(GetDlgItem(IDC_PITCH));

            SetFrequency(IDC_PITCH, _Configuration->_Pitch);

            auto w = CUpDownCtrl(GetDlgItem(IDC_PITCH_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinPitch * 100.), (int) (MaxPitch * 100.));
            w.SetPos32((int) (_Configuration->_Pitch * 100.));
        }

        {
            _Transpose.Initialize(GetDlgItem(IDC_TRANSPOSE));

            SetDlgItemTextW(IDC_TRANSPOSE, pfc::wideFromUTF8(pfc::format_int(_Configuration->_Transpose)));

            auto w = CUpDownCtrl(GetDlgItem(IDC_TRANSPOSE_SPIN));

            w.SetRange32(MinTranspose, MaxTranspose);
            w.SetPos32(_Configuration->_Transpose);
        }

        {
            auto w = (CComboBox) GetDlgItem(IDC_SCALING_FUNCTION);

            w.ResetContent();

            const WCHAR * Labels[] = { L"Linear", L"Logarithmic", L"Shifted Logarithmic", L"Mel", L"Bark", L"Adjustable Bark", L"ERB", L"Cams", L"Hyperbolic Sine", L"Nth Root", L"Negative Exponential", L"Period" };

            for (size_t i = 0; i < _countof(Labels); ++i)
                w.AddString(Labels[i]);

            w.SetCurSel((int) _Configuration->_ScalingFunction);
        }

        {
            UDACCEL Accel[] =
            {
                { 0,    1 }, // 0.01
                { 1,    5 }, // 0.05
                { 2,   10 }, // 0.10
            };

            _SkewFactor.Initialize(GetDlgItem(IDC_SKEW_FACTOR));

            SetDlgItemTextW(IDC_SKEW_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration->_SkewFactor, 0, 2)));

            auto w = CUpDownCtrl(GetDlgItem(IDC_SKEW_FACTOR_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinSkewFactor * 100.), (int) (MaxSkewFactor * 100.));
            w.SetPos32((int) (_Configuration->_SkewFactor * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 0,   1 }, //  0.1
                { 1,   5 }, //  0.5
                { 2,  10 }, //  1.0
                { 3,  50 }, //  5.0
            };

            _Bandwidth.Initialize(GetDlgItem(IDC_BANDWIDTH));

            SetDlgItemTextW(IDC_BANDWIDTH, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Bandwidth, 0, 1)));

            auto w = CUpDownCtrl(GetDlgItem(IDC_BANDWIDTH_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinBandwidth * 10.), (int) (MaxBandwidth * 10.));
            w.SetPos32((int) (_Configuration->_Bandwidth * 10.));
        }
    }
    #pragma endregion

    #pragma region X Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_X_AXIS);

        w.ResetContent();

        const WCHAR * Labels[] = { L"None", L"Bands", L"Decades", L"Octaves", L"Notes" };

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _Configuration->_XAxisMode);
    }
    #pragma endregion

    #pragma region Y Axis
    {
        {
            auto w = (CComboBox) GetDlgItem(IDC_Y_AXIS);

            w.ResetContent();

            const WCHAR * Labels[] = { L"None", L"Decibel", L"Logarithmic" };

            for (size_t i = 0; i < _countof(Labels); ++i)
                w.AddString(Labels[i]);

            w.SetCurSel((int) _Configuration->_YAxisMode);
        }

        {
            UDACCEL Accel[] =
            {
                { 0,     10 }, //  0.1
                { 1,    100 }, //  1.0
                { 2,    250 }, //  2.5
                { 3,    500 }, //  5.0
            };

            {
                _AmplitudeLo.Initialize(GetDlgItem(IDC_AMPLITUDE_LO));

                SetDecibel(IDC_AMPLITUDE_LO, _Configuration->_AmplitudeLo);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_LO_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitude * 10.), (int) (MaxAmplitude * 10.));
                w.SetPos32((int) (_Configuration->_AmplitudeLo * 10.));
            }

            {
                _AmplitudeHi.Initialize(GetDlgItem(IDC_AMPLITUDE_HI));

                SetDecibel(IDC_AMPLITUDE_HI, _Configuration->_AmplitudeHi);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_HI_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitude * 10), (int) (MaxAmplitude * 10.));
                w.SetPos32((int) (_Configuration->_AmplitudeHi * 10.));
            }

            {
                _AmplitudeStep.Initialize(GetDlgItem(IDC_AMPLITUDE_STEP));

                SetDecibel(IDC_AMPLITUDE_STEP, _Configuration->_AmplitudeStep);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_STEP_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitudeStep * 10), (int) (MaxAmplitudeStep * 10.));
                w.SetPos32((int) (_Configuration->_AmplitudeStep * 10.));
            }
        }

        SendDlgItemMessageW(IDC_USE_ABSOLUTE, BM_SETCHECK, _Configuration->_UseAbsolute);

        _Gamma.Initialize(GetDlgItem(IDC_GAMMA));

        SetDlgItemTextW(IDC_GAMMA, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Gamma, 0, 1)));
    }
    #pragma endregion

    #pragma region Rendering
    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Solid", L"Custom", L"Prism 1", L"Prism 2", L"Prism 3", L"foobar2000", L"foobar2000 Dark Mode", L"Fire" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_ColorScheme);
    }
    {
        SendDlgItemMessageW(IDC_DRAW_BAND_BACKGROUND, BM_SETCHECK, _Configuration->_DrawBandBackground);
    }
    {
        SendDlgItemMessageW(IDC_SHOW_TOOLTIPS, BM_SETCHECK, _Configuration->_ShowToolTips);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_SMOOTHING_METHOD);

        w.ResetContent();

        const WCHAR * Labels[] = { L"Average", L"Peak" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }
        w.SetCurSel((int) _Configuration->_SmoothingMethod);

        SetDlgItemTextW(IDC_SMOOTHING_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration->_SmoothingFactor, 0, 1)));
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_PEAK_MODE);

        w.ResetContent();

        const WCHAR * Labels[] = { L"None", L"Classic", L"Gravity", L"AIMP", L"Fade Out" };

        for (size_t i = 0; i < _countof(Labels); ++i)
        {
            w.AddString(Labels[i]);
        }

        w.SetCurSel((int) _Configuration->_PeakMode);
    }
    {
        SetDlgItemTextW(IDC_HOLD_TIME, pfc::wideFromUTF8(pfc::format_float(_Configuration->_HoldTime, 0, 1)));
        SetDlgItemTextW(IDC_ACCELERATION, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Acceleration, 0, 1)));
    }
    #pragma endregion

    #pragma region Colors

    _Gradient.Initialize(GetDlgItem(IDC_GRADIENT));
    _Colors.Initialize(GetDlgItem(IDC_COLORS));

    _BackColor.Initialize(GetDlgItem(IDC_BACK_COLOR));
    _XTextColor.Initialize(GetDlgItem(IDC_X_TEXT_COLOR));
    _XLineColor.Initialize(GetDlgItem(IDC_X_LINE_COLOR));
    _YTextColor.Initialize(GetDlgItem(IDC_Y_TEXT_COLOR));
    _YLineColor.Initialize(GetDlgItem(IDC_Y_LINE_COLOR));
    _BandBackColor.Initialize(GetDlgItem(IDC_BAND_BACK_COLOR));

    UpdateColorControls();

    #pragma endregion

    UpdateControls();
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void ConfigurationDialog::Terminate()
{
    _Channels.Terminate();

    _KernelSize.Terminate();

    _NumBands.Terminate();
    _LoFrequency.Terminate();
    _HiFrequency.Terminate();
    _MinNote.Terminate();
    _MaxNote.Terminate();
    _BandsPerOctave.Terminate();
    _Pitch.Terminate();
    _Transpose.Terminate();
    _SkewFactor.Terminate();
    _Bandwidth.Terminate();

    _AmplitudeStep.Terminate();
    _AmplitudeHi.Terminate();
    _AmplitudeLo.Terminate();

    _Gamma.Terminate();

    _Colors.Terminate();
    _Gradient.Terminate();

    _BackColor.Terminate();

    _XTextColor.Terminate();
    _XLineColor.Terminate();

    _YTextColor.Terminate();
    _YLineColor.Terminate();

    _BandBackColor.Terminate();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void ConfigurationDialog::OnSelectionChanged(UINT, int id, CWindow w)
{
    if (_Configuration == nullptr)
        return;

    CComboBox cb = (CComboBox) w;

    int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
    #pragma region Transform
        case IDC_TRANSFORM:
        {
            _Configuration->_Transform = (Transform) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region FFT
        case IDC_FFT_SIZE:
        {
            _Configuration->_FFTSize = (FFTSize) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_MAPPING_METHOD:
        {
            _Configuration->_MappingMethod = (Mapping) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region Frequencies
        case IDC_DISTRIBUTION:
        {
            _Configuration->_FrequencyDistribution = (FrequencyDistribution) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_SCALING_FUNCTION:
        {
            _Configuration->_ScalingFunction = (ScalingFunction) SelectedIndex;
            break;
        }

        case IDC_SUMMATION_METHOD:
        {
            _Configuration->_SummationMethod = (SummationMethod) SelectedIndex;
            break;
        }

        case IDC_SMOOTHING_METHOD:
        {
            _Configuration->_SmoothingMethod = (SmoothingMethod) SelectedIndex;
            break;
        }
    #pragma endregion

    #pragma region X axis
        case IDC_X_AXIS:
        {
            _Configuration->_XAxisMode = (XAxisMode) SelectedIndex;
            break;
        }
    #pragma endregion

    #pragma region Y axis
        case IDC_Y_AXIS:
        {
            _Configuration->_YAxisMode = (YAxisMode) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion

    #pragma region Rendering
        case IDC_COLOR_SCHEME:
        {
            _Configuration->_ColorScheme = (ColorScheme) SelectedIndex;

            if (_Configuration->_ColorScheme != ColorScheme::Custom)
                _Configuration->_GradientStops = GetGradientStops(_Configuration->_ColorScheme);
            else
                _Configuration->_GradientStops = _Configuration->_CustomGradientStops;

            UpdateColorControls();
            break;
        }

        case IDC_COLORS:
        {
            // Remove and Reverse are only enabled when there is more than 1 color.
            bool HasMoreThanOneColor = (_Configuration->_GradientStops.size() > 1);

                GetDlgItem(IDC_REMOVE).EnableWindow(HasMoreThanOneColor);
                GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor);

            // Add and Remove are only enabled when a color is selected.
            auto lb = (CListBox) GetDlgItem(IDC_COLORS);

            bool HasSelection = (lb.GetCurSel() != LB_ERR);

                GetDlgItem(IDC_ADD).EnableWindow(HasSelection);
                GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor);

            return;
        }

        case IDC_PEAK_MODE:
        {
            _Configuration->_PeakMode = (PeakMode) SelectedIndex;

            UpdateControls();
            break;
        }
    #pragma endregion
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void ConfigurationDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((code != EN_CHANGE) || (_Configuration == nullptr))
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
    #pragma region FFT
        case IDC_FFT_SIZE_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_Configuration->_FFTSize)
            {
                default:
                    break;

                case FFTSize::FFTCustom:
                {
                    _Configuration->_FFTCustom = (size_t) Clamp(::_wtoi(Text), MinFFTSize, MaxFFTSize);
                    break;
                }

                case FFTSize::FFTDuration:
                {
                    _Configuration->_FFTDuration= Clamp(::_wtof(Text), MinFFTDuration, MaxFFTDuration);;
                    break;
                }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE:
        {
            _Configuration->_KernelSize = Clamp(::_wtoi(Text), MinKernelSize, MaxKernelSize);
            break;
        }
    #pragma endregion

    #pragma region Frequencies
        case IDC_NUM_BANDS:
        {
            _Configuration->_NumBands = (size_t) Clamp(::_wtoi(Text), MinBands, MaxBands);
            break;
        }

        case IDC_LO_FREQUENCY:
        {
            _Configuration->_LoFrequency = Min(Clamp(::_wtof(Text), MinFrequency, MaxFrequency), _Configuration->_HiFrequency);

            CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN)).SetPos32((int)(_Configuration->_LoFrequency * 100.));
            break;
        }

        case IDC_HI_FREQUENCY:
        {
            _Configuration->_HiFrequency = Max(Clamp(::_wtof(Text), MinFrequency, MaxFrequency), _Configuration->_LoFrequency);

            CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN)).SetPos32((int)(_Configuration->_HiFrequency * 100.));
            break;
        }

        case IDC_PITCH:
        {
            _Configuration->_Pitch = Clamp(::_wtof(Text), MinPitch, MaxPitch);
            break;
        }
    #pragma endregion

    #pragma region Y axis
        case IDC_AMPLITUDE_LO:
        {
            _Configuration->_AmplitudeLo = Min(Clamp(::_wtof(Text), MinAmplitude, MaxAmplitude), _Configuration->_AmplitudeHi);
            break;
        }

        case IDC_AMPLITUDE_HI:
        {
            _Configuration->_AmplitudeHi = Max(Clamp(::_wtof(Text), MinAmplitude, MaxAmplitude), _Configuration->_AmplitudeLo);
            break;
        }

        case IDC_AMPLITUDE_STEP:
        {
            _Configuration->_AmplitudeStep = Max(Clamp(::_wtof(Text), MinAmplitudeStep, MaxAmplitudeStep), _Configuration->_AmplitudeStep);
            break;
        }

        case IDC_GAMMA:
        {
            _Configuration->_Gamma = Clamp(::_wtof(Text), MinGamma, MaxGamma);
            break;
        }
    #pragma endregion

    #pragma region Peak indicator
        case IDC_SMOOTHING_FACTOR:
        {
            _Configuration->_SmoothingFactor = Clamp(::_wtof(Text), MinSmoothingFactor, MaxSmoothingFactor);
            break;
        }

        case IDC_HOLD_TIME:
        {
            _Configuration->_HoldTime = Clamp(::_wtof(Text), MinHoldTime, MaxHoldTime);
            break;
        }

        case IDC_ACCELERATION:
        {
            _Configuration->_Acceleration = Clamp(::_wtof(Text), MinAcceleration, MaxAcceleration);
            break;
        }
    #pragma endregion

        default:
            return;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void ConfigurationDialog::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    switch (id)
    {
    #pragma region FFT
        case IDC_KERNEL_SIZE:
        {
            SetDlgItemTextW(IDC_KERNEL_SIZE, pfc::wideFromUTF8(pfc::format_int(_Configuration->_KernelSize)));
            break;
        }
    #pragma endregion

    #pragma region Frequencies
        case IDC_NUM_BANDS:
        {
            SetDlgItemTextW(IDC_NUM_BANDS, pfc::wideFromUTF8(pfc::format_int((int) _Configuration->_NumBands)));
            break;
        }

        case IDC_LO_FREQUENCY:
        {
            SetFrequency(IDC_LO_FREQUENCY, _Configuration->_LoFrequency);
            break;
        }

        case IDC_HI_FREQUENCY:
        {
            SetFrequency(IDC_HI_FREQUENCY, _Configuration->_HiFrequency);
            break;
        }

        case IDC_PITCH:
        {
            SetFrequency(IDC_PITCH, _Configuration->_Pitch);
            break;
        }

        case IDC_SKEW_FACTOR:
        {
            SetDlgItemTextW(IDC_SKEW_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration->_SkewFactor, 0, 2)));
            break;
        }

        case IDC_BANDWIDTH:
        {
            SetDlgItemTextW(IDC_BANDWIDTH, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Bandwidth, 0, 1)));
            break;
        }
    #pragma endregion

    #pragma region Y axis
        case IDC_AMPLITUDE_LO:
        {
            SetDecibel(IDC_AMPLITUDE_LO, _Configuration->_AmplitudeLo);
            break;
        }

        case IDC_AMPLITUDE_HI:
        {
            SetDecibel(IDC_AMPLITUDE_HI, _Configuration->_AmplitudeHi);
            break;
        }

        case IDC_AMPLITUDE_STEP:
        {
            SetDecibel(IDC_AMPLITUDE_STEP, _Configuration->_AmplitudeStep);
            break;
        }

        case IDC_GAMMA:
        {
            SetDlgItemTextW(IDC_GAMMA, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Gamma, 0, 1)));
            break;
        }
    #pragma endregion

    #pragma region Peak indicator
        case IDC_SMOOTHING_FACTOR:
        {
            SetDlgItemTextW(IDC_SMOOTHING_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration->_SmoothingFactor, 0, 1)));
            break;
        }

        case IDC_HOLD_TIME:
        {
            SetDlgItemTextW(IDC_HOLD_TIME, pfc::wideFromUTF8(pfc::format_float(_Configuration->_HoldTime, 0, 1)));
            break;
        }

        case IDC_ACCELERATION:
        {
            SetDlgItemTextW(IDC_ACCELERATION, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Acceleration, 0, 1)));
            break;
        }
    #pragma endregion
    }

    return;
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void ConfigurationDialog::OnButtonClick(UINT, int id, CWindow)
{
    if (_Configuration == nullptr)
        return;

    switch (id)
    {
        case IDC_CHANNELS:
        {
            BOOL Handled = TRUE;

            _Channels.OnClicked(0, 0, 0, Handled);

            return;
        }

        case IDC_SMOOTH_LOWER_FREQUENCIES:
        {
            _Configuration->_SmoothLowerFrequencies = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SMOOTH_GAIN_TRANSITION:
        {
            _Configuration->_SmoothGainTransition = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_USE_ABSOLUTE:
        {
            _Configuration->_UseAbsolute = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_DRAW_BAND_BACKGROUND:
        {
            _Configuration->_DrawBandBackground = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SHOW_TOOLTIPS:
        {
            _Configuration->_ShowToolTips = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_ADD:
        {
            int Index = _Colors.GetCurSel();

            if ((Index == LB_ERR) || ((size_t) Index >= _Configuration->_GradientStops.size()))
                return;

            D2D1_COLOR_F Color = _Configuration->_GradientStops[(size_t) Index].color;

            if (!SelectColor(m_hWnd, Color))
                return;

            D2D1_GRADIENT_STOP gs = { 0.f, Color };

            _Configuration->_GradientStops.insert(_Configuration->_GradientStops.begin() + Index + 1, gs);

            _Configuration->UpdateGradient();

            UpdateColorControls();
            break;
        }

        case IDC_REMOVE:
        {
            // Don't remove the last color.
            if (_Colors.GetCount() == 1)
                return;

            int Index = _Colors.GetCurSel();

            if ((Index == LB_ERR) || ((size_t) Index >= _Configuration->_GradientStops.size()))
                return;

            _Configuration->_GradientStops.erase(_Configuration->_GradientStops.begin() + Index);

            _Configuration->UpdateGradient();

            UpdateColorControls();
            break;
        }

        case IDC_REVERSE:
        {
            std::reverse(_Configuration->_GradientStops.begin(), _Configuration->_GradientStops.end());

            _Configuration->UpdateGradient();

            UpdateColorControls();
            break;
        }

        case IDC_RESET:
        {
            _Configuration->Reset();

            Initialize();

            return ;
        }

        case IDOK:
        case IDCANCEL:
        {
            if (id == IDCANCEL)
                *_Configuration = _OldConfiguration;

            GetWindowRect(&_Configuration->_DialogBounds);

            Terminate();

            DestroyWindow();
            break;
        }

        default:
            return;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT ConfigurationDialog::OnDeltaPos(LPNMHDR nmhd)
{
    LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

    int NewPos = nmud->iPos + nmud->iDelta;

    switch (nmhd->idFrom)
    {
        case IDC_KERNEL_SIZE_SPIN:
        {
            _Configuration->_KernelSize = Clamp(NewPos, MinKernelSize, MaxKernelSize);
            break;
        }

        case IDC_LO_FREQUENCY_SPIN:
        {
            _Configuration->_LoFrequency = Min(Clamp(NewPos / 100., MinFrequency, MaxFrequency), _Configuration->_HiFrequency);
            SetFrequency(IDC_LO_FREQUENCY, _Configuration->_LoFrequency);

            CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN)).SetPos32((int)(_Configuration->_LoFrequency * 100.));
            break;
        }

        case IDC_HI_FREQUENCY_SPIN:
        {
            _Configuration->_HiFrequency = Max(Clamp(NewPos / 100., MinFrequency, MaxFrequency), _Configuration->_LoFrequency);
            SetFrequency(IDC_HI_FREQUENCY, _Configuration->_HiFrequency);

            CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN)).SetPos32((int)(_Configuration->_HiFrequency * 100.));
            break;
        }

        case IDC_NUM_BANDS_SPIN:
        {
            _Configuration->_NumBands = (size_t) Clamp(NewPos, MinBands, MaxBands);
            SetDlgItemTextW(IDC_NUM_BANDS, pfc::wideFromUTF8(pfc::format_int((int) _Configuration->_NumBands)));

            CUpDownCtrl(GetDlgItem(IDC_NUM_BANDS)).SetPos32((int)(_Configuration->_NumBands));
            break;
        }

        case IDC_MIN_NOTE_SPIN:
        {
            _Configuration->_MinNote = Min((uint32_t) Clamp(NewPos, MinNote, MaxNote), _Configuration->_MaxNote);
            SetNote(IDC_MIN_NOTE, _Configuration->_MinNote);

            CUpDownCtrl(GetDlgItem(IDC_MIN_NOTE_SPIN)).SetPos32((int)(_Configuration->_MinNote));
            break;
        }

        case IDC_MAX_NOTE_SPIN:
        {
            _Configuration->_MaxNote = Max((uint32_t) Clamp(NewPos, MinNote, MaxNote), _Configuration->_MinNote);
            SetNote(IDC_MAX_NOTE, _Configuration->_MaxNote);

            CUpDownCtrl(GetDlgItem(IDC_MAX_NOTE_SPIN)).SetPos32((int)(_Configuration->_MaxNote));
            break;
        }

        case IDC_BANDS_PER_OCTAVE_SPIN:
        {
            _Configuration->_BandsPerOctave = (uint32_t) Clamp(NewPos, MinBandsPerOctave, MaxBandsPerOctave);

            CUpDownCtrl(GetDlgItem(IDC_BANDS_PER_OCTAVE_SPIN)).SetPos32((int)(_Configuration->_BandsPerOctave));
            break;
        }

        case IDC_PITCH_SPIN:
        {
            _Configuration->_Pitch = Clamp((double) NewPos / 100., MinPitch, MaxPitch);
            SetFrequency(IDC_PITCH, _Configuration->_Pitch);

            CUpDownCtrl(GetDlgItem(IDC_PITCH_SPIN)).SetPos32((int)(_Configuration->_Pitch * 10.));
            break;
        }

        case IDC_TRANSPOSE_SPIN:
        {
            _Configuration->_Transpose = Clamp(NewPos, MinTranspose, MaxTranspose);

            CUpDownCtrl(GetDlgItem(IDC_TRANSPOSE_SPIN)).SetPos32((int)(_Configuration->_Transpose));
            break;
        }

        case IDC_AMPLITUDE_LO_SPIN:
        {
            _Configuration->_AmplitudeLo = Min(Clamp((double) NewPos / 10., MinAmplitude, MaxAmplitude), _Configuration->_AmplitudeHi);
            SetDecibel(IDC_AMPLITUDE_LO, _Configuration->_AmplitudeLo);

            CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_LO_SPIN)).SetPos32((int)(_Configuration->_AmplitudeLo * 10.));
            break;
        }

        case IDC_AMPLITUDE_HI_SPIN:
        {
            _Configuration->_AmplitudeHi = Max(Clamp((double) NewPos / 10., MinAmplitude, MaxAmplitude), _Configuration->_AmplitudeLo);
            SetDecibel(IDC_AMPLITUDE_HI, _Configuration->_AmplitudeHi);

            CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_HI_SPIN)).SetPos32((int)(_Configuration->_AmplitudeHi * 10.));
            break;
        }

        case IDC_AMPLITUDE_STEP_SPIN:
        {
            _Configuration->_AmplitudeStep = Clamp((double) NewPos / 10., MinAmplitudeStep, MaxAmplitudeStep);
            SetDecibel(IDC_AMPLITUDE_STEP, _Configuration->_AmplitudeStep);

            CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_STEP_SPIN)).SetPos32((int)(_Configuration->_AmplitudeStep * 10.));
            break;
        }

        case IDC_SKEW_FACTOR_SPIN:
        {
            _Configuration->_SkewFactor = Clamp((double) NewPos / 100., MinSkewFactor, MaxSkewFactor);
            SetDlgItemTextW(IDC_SKEW_FACTOR, pfc::wideFromUTF8(pfc::format_float(_Configuration->_SkewFactor, 0, 2)));
            break;
        }

        case IDC_BANDWIDTH_SPIN:
        {
            _Configuration->_Bandwidth = Clamp((double) NewPos / 10., MinBandwidth, MaxBandwidth);
            SetDlgItemTextW(IDC_BANDWIDTH, pfc::wideFromUTF8(pfc::format_float(_Configuration->_Bandwidth, 0, 1)));
            break;
        }

        default:
            return -1;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

    return 0;
}

/// <summary>
/// Handles a Change notification.
/// </summary>
LRESULT ConfigurationDialog::OnChanged(LPNMHDR nmhd)
{
    switch (nmhd->idFrom)
    {
        case IDC_COLORS:
        {
            std::vector<D2D1_COLOR_F> Colors;

            _Colors.GetColors(Colors);

            if (Colors.empty())
                return 0;

            {
                std::vector<D2D1_GRADIENT_STOP> & GradientStops = _Configuration->_GradientStops;

                GradientStops.clear();

                FLOAT Position = 0.f;

                for (const auto & Iter : Colors)
                {
                    D2D1_GRADIENT_STOP gs = { Position, Iter };

                    GradientStops.push_back(gs);

                    Position += 1.f / (FLOAT) Colors.size();
                }
            }

            _Configuration->_ColorScheme = ColorScheme::Custom;
            _Configuration->_CustomGradientStops = _Configuration->_GradientStops;

            {
                auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

                w.SetCurSel((int) _Configuration->_ColorScheme);
            }

            {
                _Gradient.SetGradientStops(_Configuration->_GradientStops);
            }
            break;
        }

        case IDC_BACK_COLOR:
        {
            _BackColor.GetColor(_Configuration->_BackColor);
            break;
        }

        case IDC_X_TEXT_COLOR:
        {
            _XTextColor.GetColor(_Configuration->_XTextColor);
            break;
        }

        case IDC_X_LINE_COLOR:
        {
            _XLineColor.GetColor(_Configuration->_XLineColor);
            break;
        }

        case IDC_Y_TEXT_COLOR:
        {
            _YTextColor.GetColor(_Configuration->_YTextColor);
            break;
        }

        case IDC_Y_LINE_COLOR:
        {
            _YLineColor.GetColor(_Configuration->_YLineColor);
            break;
        }

        case IDC_BAND_BACK_COLOR:
        {
            _BandBackColor.GetColor(_Configuration->_BandBackColor);
            break;
        }

        default:
            return -1;
    }

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

    return 0;
}

/// <summary>
/// Handles a selection from the Channels menu.
/// </summary>
void ConfigurationDialog::OnChannels(UINT, int id, HWND)
{
    CMenuHandle & Menu = _Channels.GetMenu();

    bool IsChecked = (Menu.GetMenuState((UINT) id, MF_BYCOMMAND) & MF_CHECKED);

    if (id == IDM_CHANNELS_LAST)
    {
        _Configuration->_SelectedChannels = IsChecked ? 0 : AllChannels;
    }
    else
    if (InRange(id, IDM_CHANNELS_FIRST, IDM_CHANNELS_LAST - 1))
    {
        uint32_t Mask = 1U << (id - IDM_CHANNELS_FIRST);

        if (IsChecked)
            _Configuration->_SelectedChannels &= ~Mask;
        else
            _Configuration->_SelectedChannels |=  Mask;
    }
    else
        return;

    UpdateChannelsMenu();

    ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
}

/// <summary>
/// Enables or disables controls based on the selection of the user.
/// </summary>
void ConfigurationDialog::UpdateControls()
{
    // Transform
    bool IsFFT = (_Configuration->_Transform == Transform::FFT);

        GetDlgItem(IDC_DISTRIBUTION).EnableWindow(IsFFT);

    // FFT
        GetDlgItem(IDC_FFT_SIZE).EnableWindow(IsFFT);

    bool State = (_Configuration->_FFTSize == FFTSize::FFTCustom) || (_Configuration->_FFTSize == FFTSize::FFTDuration);

        GetDlgItem(IDC_FFT_SIZE_PARAMETER).EnableWindow(IsFFT && State);

        GetDlgItem(IDC_SUMMATION_METHOD).EnableWindow(IsFFT);
        GetDlgItem(IDC_MAPPING_METHOD).EnableWindow(IsFFT);
        GetDlgItem(IDC_SMOOTH_LOWER_FREQUENCIES).EnableWindow(IsFFT);
        GetDlgItem(IDC_SMOOTH_GAIN_TRANSITION).EnableWindow(IsFFT);
        GetDlgItem(IDC_KERNEL_SIZE).EnableWindow(IsFFT);

        #pragma warning (disable: 4061)
        switch (_Configuration->_FFTSize)
        {
            default:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"");
                break;

            case FFTSize::FFTCustom:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER, pfc::wideFromUTF8(pfc::format_int((t_int64) _Configuration->_FFTCustom)));
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"samples");
                break;

            case FFTSize::FFTDuration:
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER, pfc::wideFromUTF8(pfc::format_float(_Configuration->_FFTDuration, 0, 1)));
                SetDlgItemTextW(IDC_FFT_SIZE_PARAMETER_UNIT, L"ms");
                break;
        }
        #pragma warning (default: 4061)

    // Frequencies
    State = (_Configuration->_FrequencyDistribution != FrequencyDistribution::Octaves);

        GetDlgItem(IDC_NUM_BANDS).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_LO_FREQUENCY).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_HI_FREQUENCY).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(IsFFT && State);
        GetDlgItem(IDC_SKEW_FACTOR).EnableWindow(State);

        GetDlgItem(IDC_MIN_NOTE).EnableWindow(!State);
        GetDlgItem(IDC_MAX_NOTE).EnableWindow(!State);
        GetDlgItem(IDC_BANDS_PER_OCTAVE).EnableWindow(!State);
        GetDlgItem(IDC_PITCH).EnableWindow(!State);
        GetDlgItem(IDC_TRANSPOSE).EnableWindow(!State);

    State = (_Configuration->_FrequencyDistribution == FrequencyDistribution::AveePlayer);

        GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(!State);

    // Y axis
    State = (_Configuration->_YAxisMode == YAxisMode::Logarithmic);

        GetDlgItem(IDC_USE_ABSOLUTE).EnableWindow(State);
        GetDlgItem(IDC_GAMMA).EnableWindow(State);

    // Peak indicators
    State = (_Configuration->_PeakMode == PeakMode::None);

        GetDlgItem(IDC_HOLD_TIME).EnableWindow(!State);
        GetDlgItem(IDC_ACCELERATION).EnableWindow(!State);
 
    UpdateColorControls();
}

/// <summary>
/// Updates the color controls with the current configuration.
/// </summary>
void ConfigurationDialog::UpdateColorControls()
{
    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.SetCurSel((int) _Configuration->_ColorScheme);
    }

    {
        _Gradient.SetGradientStops(_Configuration->_GradientStops);
    }

    {
        std::vector<D2D1_COLOR_F> Colors;

        for (const auto & Iter : _Configuration->_GradientStops)
            Colors.push_back(Iter.color);

        _Colors.SetColors(Colors);
    }

    {
        _BackColor.SetColor(_Configuration->_BackColor);
        _XTextColor.SetColor(_Configuration->_XTextColor);
        _XLineColor.SetColor(_Configuration->_XLineColor);
        _YTextColor.SetColor(_Configuration->_YTextColor);
        _YLineColor.SetColor(_Configuration->_YLineColor);
        _BandBackColor.SetColor(_Configuration->_BandBackColor);
    }

    // Remove and Reverse are only enabled when there is more than 1 color.
    bool HasMoreThanOneColor = (_Configuration->_GradientStops.size() > 1);

        GetDlgItem(IDC_REMOVE).EnableWindow(HasMoreThanOneColor);
        GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor);

    // Add and Remove are only enabled when a color is selected.
    auto lb = (CListBox) GetDlgItem(IDC_COLORS);

    bool HasSelection = (lb.GetCurSel() != LB_ERR);

        GetDlgItem(IDC_ADD).EnableWindow(HasSelection);
        GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor);
}

/// <summary>
/// Updates the Channes menu with the current configuration.
/// </summary>
void ConfigurationDialog::UpdateChannelsMenu()
{
    CMenuHandle & Menu = _Channels.GetMenu();

    uint32_t SelectedChannels = _Configuration->_SelectedChannels;

    for (UINT i = IDM_CHANNELS_FIRST; i < IDM_CHANNELS_LAST; ++i)
    {
        Menu.CheckMenuItem(i, (UINT) (MF_BYCOMMAND | ((SelectedChannels & 1) ? MF_CHECKED : 0)));

        SelectedChannels >>= 1;
    }

    Menu.CheckMenuItem(IDM_CHANNELS_LAST, (UINT) (MF_BYCOMMAND | ((_Configuration->_SelectedChannels == AllChannels) ? MF_CHECKED : 0)));
}
