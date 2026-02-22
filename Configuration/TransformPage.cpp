
/** $VER: TransformPage.cpp (2026.02.20) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "TransformPage.h"
#include "Support.h"
#include "Log.h"

// Display names for the window function / kernel drop list.
static const WCHAR * WindowFunctionNames[] =
{
    L"Box Car (Rectangular)",
    L"Hann", L"Hamming", L"Blackman", L"Nuttall", L"Flat Top",
    L"Bartlett (Triangular)", L"Parzen",
    L"Welch", L"Power-of-sine", L"Power-of-circle",
    L"Gaussian", L"Tukey", L"Kaiser", L"Poison",
    L"Hyperbolic secant", L"Quadratic spline", L"Ogg Vorbis", L"Cascaded sine", L"Galss"
};

/// <summary>
/// Initializes the page.
/// </summary>
BOOL transform_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_METHOD, "Method used to transform the samples" },

        { IDC_WINDOW_FUNCTION, "Window function applied to the samples" },
        { IDC_WINDOW_PARAMETER, "Parameter used by certain window functions like Gaussian and Kaiser windows" },
        { IDC_WINDOW_SKEW, "Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples. Defaults to 0 (None)." },

        { IDC_REACTION_ALIGNMENT, "Controls the delay between the actual playback and the visualization.\n"
                                    "< 0: All samples are ahead of the playback sample (with the first sample equal to the actual playback sample).\n"
                                    "= 0: The first half of samples are behind the current playback sample and the second half are ahead of it.\n"
                                    "> 0: All samples are behind the playback with the last sample equal to the current playback sample." },

        { IDC_NUM_BINS, "Sets the number of bins used by the Fourier transforms" },
        { IDC_NUM_BINS_PARAMETER, "Sets the parameter used to calculate the number of Fourier transform bins. Set the number of bins explicitly (Custom) or expressed as a number of ms taking the sample rate into account (Duration)" },

        { IDC_SUMMATION_METHOD, "Method used to aggregate FFT coefficients" },
        { IDC_MAPPING_METHOD, "Determines how the FFT coefficients are mapped to the frequency bins." },

        { IDC_SMOOTH_LOWER_FREQUENCIES, "When enabled, the bandpower part only gets used when number of FFT bins to sum for each band is at least two or more." },
        { IDC_SMOOTH_GAIN_TRANSITION, "Smooths the frequency slope of the aggregation modes." },

        { IDC_KERNEL_SIZE, "Determines the size of the Lanczos kernel. The kernel is used to create a smooth transition between the FFT coefficients resulting in a visual pleasing result." },

        // Brown-Puckette CQT
        { IDC_BW_OFFSET, "Offsets the bandwidth of the Brown-Puckette CQT" },
        { IDC_BW_CAP, "Minimum Brown-Puckette CQT kernel size" },
        { IDC_BW_AMOUNT, "Brown-Puckette CQT kernel size" },

        { IDC_GRANULAR_BW, "When disabled constrains the bandwidth to powers of 2." },

        { IDC_KERNEL_SHAPE, "Determines the shape of the Brown-Puckette CQT kernel." },
        { IDC_KERNEL_SHAPE_PARAMETER, "Parameter used by certain window functions like Gaussian and Kaiser windows." },
        { IDC_KERNEL_ASYMMETRY, "Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples." },

        // IIR (SWIFT / Analog-style)
        { IDC_FBO, "Determines the order of the filter bank used to calculate the SWIFT and Analog-style transforms." },
        { IDC_TR, "Determines the maximum time resolution used by the SWIFT and Analog-style transforms." },
        { IDC_IIR_BW, "Determines the bandwidth used by the SWIFT and Analog-style transforms." },
        { IDC_CONSTANT_Q, "Uses constant-Q instead of variable-Q in the IIR transforms." },
        { IDC_COMPENSATE_BW, "Compensate bandwidth for narrowing on higher order IIR filters banks." },
        { IDC_PREWARPED_Q, "Prewarps Q to ensure the actual bandwidth is truly logarithmic at anything closer to the Nyquist frequency." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void transform_page_t::InitializeControls() noexcept
{
    #pragma region Transform
    {
        auto w = (CComboBox) GetDlgItem(IDC_METHOD);

        w.ResetContent();

        for (const auto & x : { L"FFT", L"CQT", L"SWIFT", L"Analog-style" })
            w.AddString(x);

        w.SetCurSel((int) _State->_Transform);
    }
    #pragma endregion

    #pragma region FFT
    {
        auto w = (CComboBox) GetDlgItem(IDC_NUM_BINS);

        w.ResetContent();

        WCHAR Text[32] = { };

        for (int i = 64, j = 0; i <= 65536; i *= 2, ++j)
        {
            ::StringCchPrintfW(Text, _countof(Text), L"%i", i);

            w.AddString(Text);
        }

        w.AddString(L"Custom");
        w.AddString(L"Sample rate based");

        w.SetCurSel((int) _State->_FFTMode);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_SUMMATION_METHOD);

        w.ResetContent();

        for (const auto & x : { L"Minimum", L"Maximum", L"Sum", L"RMS (Residual Mean Square)", L"RMS Sum", L"Average", L"Median" })
            w.AddString(x);

        w.SetCurSel((int) _State->_SummationMethod);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_MAPPING_METHOD);

        w.ResetContent();

        for (const auto & x : { L"Standard", L"Triangular Filter Bank", L"Brown-Puckette CQT" })
            w.AddString(x);

        w.SetCurSel((int) _State->_MappingMethod);
    }
    {
        SendDlgItemMessageW(IDC_SMOOTH_LOWER_FREQUENCIES, BM_SETCHECK, _State->_SmoothLowerFrequencies);
        SendDlgItemMessageW(IDC_SMOOTH_GAIN_TRANSITION, BM_SETCHECK, _State->_SmoothGainTransition);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_KERNEL_SIZE)); _NumericEdits.push_back(ne); SetInteger(IDC_KERNEL_SIZE, (int64_t) _State->_KernelSize);

        auto w = CUpDownCtrl(GetDlgItem(IDC_KERNEL_SIZE_SPIN));

        w.SetRange32(MinKernelSize, MaxKernelSize);
        w.SetPos32(_State->_KernelSize);
    }
    #pragma endregion

    #pragma region Window Function
    {
        auto w = (CComboBox) GetDlgItem(IDC_WINDOW_FUNCTION);

        w.ResetContent();

        assert(((size_t) WindowFunction::Count == _countof(WindowFunctionNames)));

        for (size_t i = 0; i < _countof(WindowFunctionNames); ++i)
            w.AddString(WindowFunctionNames[i]);

        w.SetCurSel((int) _State->_WindowFunction);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_WINDOW_PARAMETER)); _NumericEdits.push_back(ne); SetDouble(IDC_WINDOW_PARAMETER, _State->_WindowParameter);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_WINDOW_SKEW)); _NumericEdits.push_back(ne); SetDouble(IDC_WINDOW_SKEW, _State->_WindowSkew);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_REACTION_ALIGNMENT)); _NumericEdits.push_back(ne); SetDouble(IDC_REACTION_ALIGNMENT, _State->_ReactionAlignment);
    }
    #pragma endregion

    #pragma region Brown-Puckette Kernel

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BW_OFFSET)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_OFFSET, _State->_BandwidthOffset);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BW_CAP)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_CAP, _State->_BandwidthCap);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_BW_AMOUNT)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_AMOUNT, _State->_BandwidthAmount);
    }

    SendDlgItemMessageW(IDC_GRANULAR_BW, BM_SETCHECK, _State->_UseGranularBandwidth);

    {
        auto w = (CComboBox) GetDlgItem(IDC_KERNEL_SHAPE);

        w.ResetContent();

        assert(((size_t) WindowFunction::Count == _countof(WindowFunctionNames)));

        for (size_t i = 0; i < _countof(WindowFunctionNames); ++i)
            w.AddString(WindowFunctionNames[i]);

        w.SetCurSel((int) _State->_KernelShape);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_KERNEL_SHAPE_PARAMETER)); _NumericEdits.push_back(ne); SetDouble(IDC_KERNEL_SHAPE_PARAMETER, _State->_KernelShapeParameter);
    }
    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_KERNEL_ASYMMETRY)); _NumericEdits.push_back(ne); SetDouble(IDC_KERNEL_ASYMMETRY, _State->_KernelAsymmetry);
    }

    #pragma endregion

    #pragma region IIR (SWIFT / Analog-style)

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_FBO)); _NumericEdits.push_back(ne); SetInteger(IDC_FBO, (int64_t) _State->_FilterBankOrder);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_TR)); _NumericEdits.push_back(ne); SetDouble(IDC_TR, _State->_TimeResolution);
    }

    {
        auto ne = std::make_shared<CNumericEdit>(); ne->Initialize(GetDlgItem(IDC_IIR_BW)); _NumericEdits.push_back(ne); SetDouble(IDC_IIR_BW, _State->_IIRBandwidth);
    }

    SendDlgItemMessageW(IDC_CONSTANT_Q, BM_SETCHECK, _State->_ConstantQ);
    SendDlgItemMessageW(IDC_COMPENSATE_BW, BM_SETCHECK, _State->_CompensateBW);
    SendDlgItemMessageW(IDC_PREWARPED_Q, BM_SETCHECK, _State->_PreWarpQ);

    #pragma endregion

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void transform_page_t::UpdateControls() noexcept
{
    const bool IsPeakMeter    = (_State->_VisualizationType == VisualizationType::PeakMeter);
    const bool IsLevelMeter   = (_State->_VisualizationType == VisualizationType::LevelMeter);
    const bool IsOscilloscope = (_State->_VisualizationType == VisualizationType::Oscilloscope);
    const bool IsTester       = (_State->_VisualizationType == VisualizationType::Tester);

    const bool SupportsTransform = !(IsPeakMeter && IsLevelMeter && IsOscilloscope && IsTester);

    if (SupportsTransform)
    {
        const bool IsFFT = (_State->_Transform == Transform::FFT);
        const bool IsIIR = (_State->_Transform == Transform::SWIFT) || (_State->_Transform == Transform::AnalogStyle);

        // Transform
        bool HasParameter = (_State->_WindowFunction == WindowFunction::PowerOfSine)
                         || (_State->_WindowFunction == WindowFunction::PowerOfCircle)
                         || (_State->_WindowFunction == WindowFunction::Gaussian)
                         || (_State->_WindowFunction == WindowFunction::Tukey)
                         || (_State->_WindowFunction == WindowFunction::Kaiser)
                         || (_State->_WindowFunction == WindowFunction::Poison)
                         || (_State->_WindowFunction == WindowFunction::HyperbolicSecant);

        GetDlgItem(IDC_WINDOW_FUNCTION).EnableWindow(!IsIIR);
        GetDlgItem(IDC_WINDOW_PARAMETER).EnableWindow(HasParameter && !IsIIR);
        GetDlgItem(IDC_WINDOW_SKEW).EnableWindow(!IsIIR);

        // FFT
        {
            for (const auto & Iter : { IDC_SUMMATION_METHOD, IDC_MAPPING_METHOD, IDC_SMOOTH_LOWER_FREQUENCIES, IDC_SMOOTH_GAIN_TRANSITION, IDC_KERNEL_SIZE })
                GetDlgItem(Iter).EnableWindow(IsFFT);

            for (const auto & Iter : { IDC_NUM_BINS,  })
                GetDlgItem(Iter).EnableWindow(IsFFT);

            const bool NotFixed = (_State->_FFTMode == FFTMode::FFTCustom) || (_State->_FFTMode == FFTMode::FFTDuration);

            GetDlgItem(IDC_NUM_BINS_PARAMETER).EnableWindow((IsFFT || IsIIR) && NotFixed);

            #pragma warning (disable: 4061)
            switch (_State->_FFTMode)
            {
                default:
                    SetDlgItemTextW(IDC_NUM_BINS_PARAMETER_UNIT, L"");
                    break;

                case FFTMode::FFTCustom:
                    SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTCustom);
                    SetDlgItemTextW(IDC_NUM_BINS_PARAMETER_UNIT, L"samples");
                    break;

                case FFTMode::FFTDuration:
                    SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTDuration);
                    SetDlgItemTextW(IDC_NUM_BINS_PARAMETER_UNIT, L"ms");
                    break;
            }
            #pragma warning (default: 4061)
        }

        // Brown-Puckette CQT
        {
            const bool IsBrownPuckette = IsFFT && (_State->_MappingMethod == Mapping::BrownPuckette);

            for (const auto & Iter : { IDC_BW_OFFSET, IDC_BW_CAP, IDC_BW_AMOUNT, IDC_GRANULAR_BW, IDC_KERNEL_SHAPE, IDC_KERNEL_ASYMMETRY, })
                GetDlgItem(Iter).EnableWindow(IsBrownPuckette);

            GetDlgItem(IDC_KERNEL_SHAPE_PARAMETER).EnableWindow(IsBrownPuckette && HasParameter);
        }

        // IIR (SWIFT / Analog-style)
        {
            for (const auto & Iter : { IDC_FBO, IDC_TR, IDC_IIR_BW, IDC_CONSTANT_Q,IDC_COMPENSATE_BW, })
                GetDlgItem(Iter).EnableWindow(IsIIR);

            GetDlgItem(IDC_PREWARPED_Q).EnableWindow(_State->_Transform == Transform::AnalogStyle);
        }
    }
    else
    {
        for (const auto & Iter :
        {
            IDC_METHOD,
            IDC_WINDOW_FUNCTION, IDC_WINDOW_PARAMETER, IDC_WINDOW_SKEW, IDC_REACTION_ALIGNMENT,
            IDC_NUM_BINS, IDC_NUM_BINS_PARAMETER,
            IDC_SUMMATION_METHOD, IDC_MAPPING_METHOD,
            IDC_SMOOTH_LOWER_FREQUENCIES, IDC_SMOOTH_GAIN_TRANSITION,
            IDC_KERNEL_SIZE, IDC_KERNEL_SIZE_SPIN,
            IDC_BW_OFFSET, IDC_BW_CAP, IDC_BW_AMOUNT, IDC_GRANULAR_BW,
            IDC_KERNEL_SHAPE, IDC_KERNEL_SHAPE_PARAMETER, IDC_KERNEL_ASYMMETRY,
            IDC_FBO, IDC_TR, IDC_IIR_BW, IDC_CONSTANT_Q, IDC_COMPENSATE_BW, IDC_PREWARPED_Q,
        })
            GetDlgItem(Iter).EnableWindow(SupportsTransform);
    }
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void transform_page_t::TerminateControls() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void transform_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
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

        case IDC_METHOD:
        {
            _State->_Transform = (Transform) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_WINDOW_FUNCTION:
        {
            _State->_WindowFunction = (WindowFunction) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_NUM_BINS:
        {
            _State->_FFTMode = (FFTMode) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_MAPPING_METHOD:
        {
            _State->_MappingMethod = (Mapping) SelectedIndex;

            UpdateControls();
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void transform_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
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

        #pragma region FFT

        case IDC_NUM_BINS_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_State->_FFTMode)
            {
                default:
                    break;

                case FFTMode::FFTCustom:
                {
                    _State->_FFTCustom = (size_t) std::clamp(::_wtoi(Text), MinFFTSize, MaxFFTSize);
                    break;
                }

                case FFTMode::FFTDuration:
                {
                    _State->_FFTDuration= std::clamp(::_wtof(Text), MinFFTDuration, MaxFFTDuration);
                    break;
                }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE:
        {
            _State->_KernelSize = std::clamp(::_wtoi(Text), MinKernelSize, MaxKernelSize);
            break;
        }

        case IDC_WINDOW_PARAMETER:
        {
            _State->_WindowParameter = std::clamp(::_wtof(Text), MinWindowParameter, MaxWindowParameter);
            break;
        }

        case IDC_WINDOW_SKEW:
        {
            _State->_WindowSkew = std::clamp(::_wtof(Text), MinWindowSkew, MaxWindowSkew);
            break;
        }

        case IDC_REACTION_ALIGNMENT:
        {
            _State->_ReactionAlignment = std::clamp(::_wtof(Text), MinReactionAlignment, MaxReactionAlignment);
            break;
        }

        #pragma endregion

        #pragma region Brown-Puckette CQT

        case IDC_BW_OFFSET:
        {
            _State->_BandwidthOffset = std::clamp(::_wtof(Text), MinBandwidthOffset, MaxBandwidthOffset);
            break;
        }

        case IDC_BW_CAP:
        {
            _State->_BandwidthCap = std::clamp(::_wtof(Text), MinBandwidthCap, MaxBandwidthCap);
            break;
        }

        case IDC_BW_AMOUNT:
        {
            _State->_BandwidthAmount = std::clamp(::_wtof(Text), MinBandwidthAmount, MaxBandwidthAmount);
            break;
        }

        case IDC_KERNEL_SHAPE_PARAMETER:
        {
            _State->_KernelShapeParameter = std::clamp(::_wtof(Text), MinWindowParameter, MaxWindowParameter);
            break;
        }

        case IDC_KERNEL_ASYMMETRY:
        {
            _State->_KernelAsymmetry = std::clamp(::_wtof(Text), MinWindowSkew, MaxWindowSkew);
            break;
        }

        #pragma endregion

        #pragma region SWIFT / Analog-style

        case IDC_FBO:
        {
            _State->_FilterBankOrder = std::clamp((size_t) ::_wtoi(Text), MinFilterBankOrder, MaxFilterBankOrder);
            break;
        }

        case IDC_TR:
        {
            _State->_TimeResolution = std::clamp(::_wtof(Text), MinTimeResolution, MaxTimeResolution);
            break;
        }

        case IDC_IIR_BW:
        {
            _State->_IIRBandwidth = std::clamp(::_wtof(Text), MinIIRBandwidth, MaxIIRBandwidth);
            break;
        }

        #pragma endregion
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void transform_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        // FFT
        case IDC_NUM_BINS_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_State->_FFTMode)
            {
                default:
                    break;

                case FFTMode::FFTCustom:
                {
                    SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTCustom); break;
                }

                case FFTMode::FFTDuration:
                {
                    SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTDuration); break;
                }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE:
        {
            SetInteger(id, _State->_KernelSize); break;
        }

        case IDC_WINDOW_PARAMETER:
        {
            SetDouble(id, _State->_WindowParameter); break;
        }

        case IDC_WINDOW_SKEW:
        {
            SetDouble(id, _State->_WindowSkew); break;
        }

        case IDC_REACTION_ALIGNMENT:
        {
            SetDouble(id, _State->_ReactionAlignment); break;
        }

        // Brown-Puckette CQT
        case IDC_BW_OFFSET:
        {
            SetDouble(id, _State->_BandwidthOffset); break;
        }

        case IDC_BW_CAP:
        {
            SetDouble(id, _State->_BandwidthCap); break;
        }

        case IDC_BW_AMOUNT:
        {
            SetDouble(id, _State->_BandwidthAmount); break;
        }

        case IDC_KERNEL_SHAPE_PARAMETER:
        {
            SetDouble(id, _State->_KernelShapeParameter); break;
        }

        case IDC_KERNEL_ASYMMETRY:
        {
            SetDouble(id, _State->_KernelAsymmetry); break;
        }

        // SWIFT / Analog-style
        case IDC_FBO:
        {
            SetInteger(id, (int64_t) _State->_FilterBankOrder); break;
        }

        case IDC_TR:
        {
            SetDouble(id, _State->_TimeResolution); break;
        }

        case IDC_IIR_BW:
        {
            SetDouble(id, _State->_IIRBandwidth); break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void transform_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    auto ChangedSettings = Settings::All;

    switch (id)
    {
        default:
            return;

        case IDC_SMOOTH_LOWER_FREQUENCIES:
        {
            _State->_SmoothLowerFrequencies = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_SMOOTH_GAIN_TRANSITION:
        {
            _State->_SmoothGainTransition = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_GRANULAR_BW:
        {
            _State->_UseGranularBandwidth = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_CONSTANT_Q:
        {
            _State->_ConstantQ = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_COMPENSATE_BW:
        {
            _State->_CompensateBW = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }

        case IDC_PREWARPED_Q:
        {
            _State->_PreWarpQ = (bool) SendDlgItemMessageW(id, BM_GETCHECK);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT transform_page_t::OnDeltaPos(LPNMHDR nmhd) noexcept
{
    if (_State == nullptr)
        return -1;

    auto ChangedSettings = Settings::All;
    auto nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        default:
            return -1;

        case IDC_KERNEL_SIZE_SPIN:
        {
            _State->_KernelSize = ClampNewSpinPosition(nmud, MinKernelSize, MaxKernelSize);
            break;
        }
    }

    ConfigurationChanged(ChangedSettings);

    return 0;
}
