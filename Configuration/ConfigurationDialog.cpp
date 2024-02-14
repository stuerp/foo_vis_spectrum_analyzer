
/** $VER: ConfigurationDialog.cpp (2024.02.14) P. Stuer - Implements the configuration dialog. **/

#include "ConfigurationDialog.h"

#include "Gradients.h"
#include "Layout.h"

#include "CColorDialogEx.h"

#include "Direct2D.h"

#include "Theme.h"

#include "Log.h"

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL ConfigurationDialog::OnInitDialog(CWindow w, LPARAM lParam)
{
    _IsInitializing = true;

    _Theme.Initialize(_DarkMode);

    DlgResize_Init(true, true, WS_CLIPCHILDREN);

    DialogParameters * dp = (DialogParameters *) lParam;

    _hParent = dp->_hWnd;
    _State = dp->_State;

    if (IsRectEmpty(&_State->_DialogBounds))
    {
        _State->_DialogBounds.right  = W_A00;
        _State->_DialogBounds.bottom = H_A00;

        ::MapDialogRect(m_hWnd, &_State->_DialogBounds);
    }

    _OldConfiguration = *_State;

    Initialize();

    MoveWindow(&_State->_DialogBounds);

    _DarkMode.AddDialogWithControls(*this);

    // Create the tooltip control.
    {
        _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

        const std::map<int, LPCWSTR> Tips =
        {
            { IDC_METHOD, L"Method used to transform the samples" },

            { IDC_WINDOW_FUNCTION, L"Window function applied to the samples" },
            { IDC_WINDOW_PARAMETER, L"Parameter used by certain window functions like Gaussian and Kaiser windows" },
            { IDC_WINDOW_SKEW, L"Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples. Defaults to 0 (None)." },

            { IDC_REACTION_ALIGNMENT, L"Controls the delay between the actual playback and the visualization.\n"
                                       "< 0: All samples are ahead of the playback sample (with the first sample equal to the actual playback sample)\n"
                                       "= 0: The first half of samples are behind the current playback sample and the second half are ahead of it.\n"
                                       "> 0: All samples are behind the playback with the last sample equal to the current playback sample." },
            { IDC_CHANNELS, L"Determines which channels supply samples" },

            { IDC_NUM_BINS, L"Sets the number of bins used by the Fourier transforms" },
            { IDC_NUM_BINS_PARAMETER, L"Sets the parameter used to calculate the number of Fourier transform bins. Set the number of bins explicitly (Custom) or expressed as a number of ms taking the sample rate into account (Duration)" },

            { IDC_SUMMATION_METHOD, L"Method used to aggregate FFT coefficients" },
            { IDC_MAPPING_METHOD, L"Determines how the FFT coefficients are mapped to the frequency bins." },

            { IDC_SMOOTH_LOWER_FREQUENCIES, L"When enabled, the bandpower part only gets used when number of FFT bins to sum for each band is at least two or more." },
            { IDC_SMOOTH_GAIN_TRANSITION, L"Smooths the frequency slope of the aggregation modes" },

            { IDC_KERNEL_SIZE, L"Size of the Lanczos kernel used for interpolating the spectrum" },

            // Brown-Puckette CQT
            { IDC_BW_OFFSET, L"Offsets the bandwidth of the Brown-Puckette CQT" },
            { IDC_BW_CAP, L"Minimum Brown-Puckette CQT kernel size" },
            { IDC_BW_AMOUNT, L"Brown-Puckette CQT kernel size" },

            { IDC_GRANULAR_BW, L"Enable to don't constrain the bandwidth to powers of 2" },

            { IDC_KERNEL_SHAPE, L"Shape of the Brown-Puckette CQT kernel" },
            { IDC_KERNEL_SHAPE_PARAMETER, L"Parameter by certain window functions like Gaussian and Kaiser windows." },
            { IDC_KERNEL_ASYMMETRY, L"Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples." },

            // SWIFT
            { IDC_FBO, L"Determines the order of the filter bank" },
            { IDC_TR, L"Determines the maximum time resolution" },
            { IDC_BW_SWIFT, L"Determines the bandwidth used by the SWIFT transform" },

            // Frequencies
            { IDC_DISTRIBUTION, L"Determines how the frequencies are distributed" },
            { IDC_NUM_BANDS, L"Determines how many frequency bands are used" },

            { IDC_LO_FREQUENCY, L"Center frequency of the first band" },
            { IDC_HI_FREQUENCY, L"Center frequency of the last band" },

            { IDC_MIN_NOTE, L"Note that determines the center frequency of the first band" },
            { IDC_MAX_NOTE, L"Note that determines the center frequency of the last band" },

            { IDC_BANDS_PER_OCTAVE, L"Number of frequency bands per octave" },

            { IDC_PITCH, L"Tuning frequency" },
            { IDC_TRANSPOSE, L"Determines how many semitones the frequencies will be transposed" },

            { IDC_SCALING_FUNCTION, L"Function used to scale the frequencies" },
            { IDC_SKEW_FACTOR, L"Affects any adjustable frequency scaling functions like hyperbolic sine and nth root. Higher values means more linear spectrum" },
            { IDC_BANDWIDTH, L"Distance between the low and high frequency boundaries for each band" },

            { IDC_ACOUSTIC_FILTER, L"Weighting filter type" },

            { IDC_SLOPE_FN_OFFS, L"Slope function offset expressed in sample rate / FFT size in samples" },
            { IDC_SLOPE_OFFS, L"Frequency slope in dB per octave" },
            { IDC_SLOPE, L"Frequency slope offset" },

            { IDC_EQ_AMT, L"Equalization amount" },
            { IDC_EQ_DEPTH, L"Equalization offset" },
            { IDC_EQ_OFFS, L"Equalization depth" },

            { IDC_WT_AMT, L"Weighting amount" },

            { IDC_NUM_ARTWORK_COLORS, L"Max. number of colors to select from the artwork" },
            { IDC_LIGHTNESS_THRESHOLD, L"Determines when a color is considered light" },
            { IDC_COLOR_ORDER, L"Determines how to sort the colors selected from the artwork" },

            { IDC_SMOOTHING_METHOD, L"Determines how the spectrum coefficients are smoothed" },
            { IDC_SMOOTHING_FACTOR, L"Determines the strength of the smoothing" },

            { IDC_SHOW_TOOLTIPS, L"Display a tooltip with information about the frequency band" },
            { IDC_SUPPRESS_MIRROR_IMAGE, L"Prevents the mirror image of the spectrum (anything above the Nyquist frequency) from being rendered" },

            { IDC_BACKGROUND_MODE, L"Determines how to render the spectrum background" },

            { IDC_ARTWORK_OPACITY, L"Determines the opacity of the artwork, if displayed." },
            { IDC_FILE_PATH, L"foobar2000 script that returns the file path of an image to display instead of the artwork" },

            // X-axis
            { IDC_X_AXIS_MODE, L"Determines the type of X-axis" },
            { IDC_X_AXIS_TOP, L"Enables or disables an X-axis above the spectrum" },
            { IDC_X_AXIS_BOTTOM, L"Enables or disables an X-axis below the spectrum" },

            // Y-axis
            { IDC_Y_AXIS_MODE, L"Determines the type of Y-axis" },
            { IDC_Y_AXIS_LEFT, L"Enables or disables an Y-axis left of the spectrum" },
            { IDC_Y_AXIS_RIGHT, L"Enables or disables an Y-axis right of the spectrum" },

            { IDC_AMPLITUDE_LO, L"Sets the lowest amplitude to display on the Y-axis" },
            { IDC_AMPLITUDE_HI, L"Sets the highest amplitude to display on the Y-axis" },
            { IDC_AMPLITUDE_STEP, L"Sets the amplitude increment" },

            { IDC_USE_ABSOLUTE, L"Sets the min. amplitude to -∞ dB (0.0 on the linear scale) when enabled" },
            { IDC_GAMMA, L"Index n of the n-th root calculation" },

            // Visualization
            { IDC_VISUALIZATION, L"Selects the type of spectrum visualization" },

            { IDC_PEAK_MODE, L"Determines how to display the peak coefficients" },
            { IDC_HOLD_TIME, L"Determines how long the peak coefficients are held before they decay" },
            { IDC_ACCELERATION, L"Determines the accelaration of the peak coefficient decay" },

            { IDC_LED_MODE, L"Display the spectrum bars as LEDs" },

            // Styles
            { IDC_STYLES, L"Selects the visual element that will be styled" },

            { IDC_COLOR_SOURCE, L"Determines the source of the color that will be used to render the visual element" },
            { IDC_COLOR_INDEX, L"Selects the specific Windows, DUI or CUI color to use" },
            { IDC_COLOR_BUTTON, L"Shows the color that will be used to render the visual element. Click to modify it." },
            { IDC_COLOR_SCHEME, L"Selects the color scheme used to create a gradient with." },

            { IDC_GRADIENT, L"Shows the gradient created using the current color list" },
            { IDC_COLOR_LIST, L"Shows the colors in the current color scheme" },

            { IDC_ADD, L"Adds a color to the color list after the selected one. A built-in color scheme will automatically be converted and the Custom color scheme will be activated." },
            { IDC_REMOVE, L"Removes the selected color from the list" },
            { IDC_REVERSE, L"Reverses the list of colors" },

            { IDC_POSITION, L"Determines the position of the color in the gradient (in % of the total length of the gradient)" },
            { IDC_SPREAD, L"Evenly spreads the colors of the list in the gradient" },

            { IDC_HORIZONTAL_GRADIENT, L"Generates a horizontal instead of a vertical gradient" },

            { IDC_OPACITY, L"Determines the opacity of the resulting color brush" },
            { IDC_THICKNESS, L"Determines the thickness of the resulting color brush when applicable" },
        };

        for (const auto & Iter : Tips)
            _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(Iter.first).m_hWnd, nullptr, (LPWSTR) Iter.second));

        _ToolTipControl.SetMaxTipWidth(200);
    }

    _IsInitializing = false;

    return TRUE;
}

/// <summary>
/// Initializes the controls of the dialog.
/// </summary>
void ConfigurationDialog::Initialize()
{
    Terminate();

    #pragma region Menu
    {
        _MenuList.Initialize(GetDlgItem(IDC_MENULIST));

        _MenuList.ResetContent();

        for (const auto & x : { L"Transform", L"Frequencies", L"Filters", L"Graph", L"Visualization", L"Styles" })
            _MenuList.AddString(x);

        _MenuList.SetCurSel((int) _State->_PageIndex);

        UpdatePages(_State->_PageIndex);
    }
    #pragma endregion

    #pragma region Transform
    {
        auto w = (CComboBox) GetDlgItem(IDC_METHOD);

        w.ResetContent();

        for (const auto & x : { L"FFT", L"CQT", L"SWIFT" })
            w.AddString(x);

        w.SetCurSel((int) _State->_Transform);
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
        auto w = (CComboBox) GetDlgItem(IDC_NUM_BINS);

        w.ResetContent();

        WCHAR Text[32] = { };

        for (int i = 64, j = 0; i <= 32768; i *= 2, ++j)
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
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_KERNEL_SIZE)); _NumericEdits.push_back(ne); SetInteger(IDC_KERNEL_SIZE, (int64_t) _State->_KernelSize);

        auto w = CUpDownCtrl(GetDlgItem(IDC_KERNEL_SIZE_SPIN));

        w.SetRange32(MinKernelSize, MaxKernelSize);
        w.SetPos32(_State->_KernelSize);
    }
    #pragma endregion

    #pragma region Window Function
    {
        auto w = (CComboBox) GetDlgItem(IDC_WINDOW_FUNCTION);

        w.ResetContent();

        const WCHAR * Labels[] =
        {
            L"Boxcar",
            L"Hann", L"Hamming", L"Blackman", L"Nuttall", L"Flat Top",
            L"Bartlett", L"Parzen",
            L"Welch", L"Power-of-sine", L"Power-of-circle",
            L"Gauss", L"Tukey", L"Kaiser", L"Poison",
            L"Hyperbolic secant", L"Quadratic spline", L"Ogg Vorbis", L"Cascaded sine", L"Galss"
        };

        assert(((size_t) WindowFunctions::Count == _countof(Labels)));

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _State->_WindowFunction);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_WINDOW_PARAMETER)); _NumericEdits.push_back(ne); SetDouble(IDC_WINDOW_PARAMETER, _State->_WindowParameter);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_WINDOW_SKEW)); _NumericEdits.push_back(ne); SetDouble(IDC_WINDOW_SKEW, _State->_WindowSkew);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_REACTION_ALIGNMENT)); _NumericEdits.push_back(ne); SetDouble(IDC_REACTION_ALIGNMENT, _State->_ReactionAlignment);
    }
    #pragma endregion

    #pragma region Brown-Puckette Kernel

    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BW_OFFSET)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_OFFSET, _State->_BandwidthOffset);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BW_CAP)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_CAP, _State->_BandwidthCap);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BW_AMOUNT)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_AMOUNT, _State->_BandwidthAmount);
    }

    SendDlgItemMessageW(IDC_GRANULAR_BW, BM_SETCHECK, _State->_GranularBW);

    {
        auto w = (CComboBox) GetDlgItem(IDC_KERNEL_SHAPE);

        w.ResetContent();

        const WCHAR * Labels[] =
        {
            L"Boxcar",
            L"Hann", L"Hamming", L"Blackman", L"Nuttall", L"Flat Top",
            L"Bartlett", L"Parzen",
            L"Welch", L"Power-of-sine", L"Power-of-circle",
            L"Gauss", L"Tukey", L"Kaiser", L"Poison",
            L"Hyperbolic secant", L"Quadratic spline", L"Ogg Vorbis", L"Cascaded sine", L"Galss"
        };

        assert(((size_t) WindowFunctions::Count == _countof(Labels)));

        for (size_t i = 0; i < _countof(Labels); ++i)
            w.AddString(Labels[i]);

        w.SetCurSel((int) _State->_KernelShape);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_KERNEL_SHAPE_PARAMETER)); _NumericEdits.push_back(ne); SetDouble(IDC_KERNEL_SHAPE_PARAMETER, _State->_KernelShapeParameter);
    }
    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_KERNEL_ASYMMETRY)); _NumericEdits.push_back(ne); SetDouble(IDC_KERNEL_ASYMMETRY, _State->_KernelAsymmetry);
    }

    #pragma endregion

    #pragma region SWIFT

    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_FBO)); _NumericEdits.push_back(ne); SetInteger(IDC_FBO, (int64_t) _State->_FilterBankOrder);
    }

    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_TR)); _NumericEdits.push_back(ne); SetDouble(IDC_TR, _State->_TimeResolution);
    }

    {
        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BW_SWIFT)); _NumericEdits.push_back(ne); SetDouble(IDC_BW_SWIFT, _State->_SWIFTBandwidth);
    }

    #pragma endregion

    #pragma region Frequencies
    {
        {
            auto w = (CComboBox) GetDlgItem(IDC_DISTRIBUTION);

            w.ResetContent();

            for (const auto & x : { L"Linear", L"Octaves", L"AveePlayer" })
                w.AddString(x);

            w.SetCurSel((int) _State->_FrequencyDistribution);
        }

        {
            UDACCEL Accel[] =
            {
                { 1,  5 },
                { 2, 10 },
                { 3, 50 },
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_NUM_BANDS)); _NumericEdits.push_back(ne); SetInteger(IDC_NUM_BANDS, (int64_t) _State->_NumBands);

            auto w = CUpDownCtrl(GetDlgItem(IDC_NUM_BANDS_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32(MinBands, MaxBands);
            w.SetPos32((int) _State->_NumBands);
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
                { 2,    1000 }, //    10.0
                { 3,    2500 }, //    25.0
                { 4,    5000 }, //    50.0
                { 5,   10000 }, //   100.0
                { 6,  100000 }, //  1000.0
                { 7, 1000000 }, // 10000.0
            };

            {
                CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_LO_FREQUENCY)); _NumericEdits.push_back(ne); SetDouble(IDC_LO_FREQUENCY, _State->_LoFrequency);

                auto w = CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
                w.SetPos32((int)(_State->_LoFrequency * 100.));
            }

            {
                CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_HI_FREQUENCY)); _NumericEdits.push_back(ne); SetDouble(IDC_HI_FREQUENCY, _State->_HiFrequency);

                auto w = CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinFrequency * 100.), (int) (MaxFrequency * 100.));
                w.SetPos32((int)(_State->_HiFrequency * 100.));
            }
        }

        {
            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_MIN_NOTE)); _NumericEdits.push_back(ne); SetNote(IDC_MIN_NOTE, _State->_MinNote);

            auto w = CUpDownCtrl(GetDlgItem(IDC_MIN_NOTE_SPIN));

            w.SetRange32(MinNote, MaxNote);
            w.SetPos32((int) _State->_MinNote);
        }

        {
            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_MAX_NOTE)); _NumericEdits.push_back(ne); SetNote(IDC_MAX_NOTE, _State->_MaxNote);

            auto w = CUpDownCtrl(GetDlgItem(IDC_MAX_NOTE_SPIN));

            w.SetRange32(MinNote, MaxNote);
            w.SetPos32((int) _State->_MaxNote);
        }

        {
            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BANDS_PER_OCTAVE)); _NumericEdits.push_back(ne); SetInteger(IDC_BANDS_PER_OCTAVE, (int64_t) _State->_BandsPerOctave);

            auto w = CUpDownCtrl(GetDlgItem(IDC_BANDS_PER_OCTAVE_SPIN));

            w.SetRange32(MinBandsPerOctave, MaxBandsPerOctave);
            w.SetPos32((int) _State->_BandsPerOctave);
        }

        {
            UDACCEL Accel[] =
            {
                { 1,    50 }, //   0.5
                { 2,   100 }, //   1.0
                { 3,  1000 }, //  10.0
                { 4,  2500 }, //  25.0
                { 5,  5000 }, //  50.0
                { 6, 10000 }, // 100.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_PITCH)); _NumericEdits.push_back(ne); SetDouble(IDC_PITCH, _State->_Pitch);

            auto w = CUpDownCtrl(GetDlgItem(IDC_PITCH_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinPitch * 100.), (int) (MaxPitch * 100.));
            w.SetPos32((int) (_State->_Pitch * 100.));
        }

        {
            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_TRANSPOSE)); _NumericEdits.push_back(ne); SetInteger(IDC_TRANSPOSE, _State->_Transpose);

            auto w = CUpDownCtrl(GetDlgItem(IDC_TRANSPOSE_SPIN));

            w.SetRange32(MinTranspose, MaxTranspose);
            w.SetPos32(_State->_Transpose);
        }

        {
            auto w = (CComboBox) GetDlgItem(IDC_SCALING_FUNCTION);

            w.ResetContent();

            for (const auto & x : { L"Linear", L"Logarithmic", L"Shifted Logarithmic", L"Mel", L"Bark", L"Adjustable Bark", L"ERB", L"Cams", L"Hyperbolic Sine", L"Nth Root", L"Negative Exponential", L"Period" })
                w.AddString(x);

            w.SetCurSel((int) _State->_ScalingFunction);
        }

        {
            UDACCEL Accel[] =
            {
                { 1,    1 }, // 0.01
                { 2,    5 }, // 0.05
                { 3,   10 }, // 0.10
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_SKEW_FACTOR)); _NumericEdits.push_back(ne); SetDouble(IDC_SKEW_FACTOR, _State->_SkewFactor);

            auto w = CUpDownCtrl(GetDlgItem(IDC_SKEW_FACTOR_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinSkewFactor * 100.), (int) (MaxSkewFactor * 100.));
            w.SetPos32((int) (_State->_SkewFactor * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,   1 }, //  0.1
                { 2,   5 }, //  0.5
                { 3,  10 }, //  1.0
                { 4,  50 }, //  5.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_BANDWIDTH)); _NumericEdits.push_back(ne); SetDouble(IDC_BANDWIDTH, _State->_Bandwidth, 0, 1);

            auto w = CUpDownCtrl(GetDlgItem(IDC_BANDWIDTH_SPIN));

            w.SetRange32((int) (MinBandwidth * 10.), (int) (MaxBandwidth * 10.));
            w.SetPos32((int) (_State->_Bandwidth * 10.));
            w.SetAccel(_countof(Accel), Accel);
        }
    }
    #pragma endregion

    #pragma region Acoustic Filter
    {
        {
            auto w = (CComboBox) GetDlgItem(IDC_ACOUSTIC_FILTER);

            w.ResetContent();

            for (const auto & x : { L"None", L"A-weighting", L"B-weighting", L"C-weighting", L"D-weighting", L"M-weighting (ITU-R 468)" })
                w.AddString(x);

            w.SetCurSel((int) _State->_WeightingType);
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_SLOPE_FN_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE_FN_OFFS, _State->_SlopeFunctionOffset);

            auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_FN_OFFS_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinSlopeFunctionOffset * 100.), (int) (MaxSlopeFunctionOffset * 100.));
            w.SetPos32((int)(_State->_SlopeFunctionOffset * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_SLOPE)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE, _State->_Slope);

            auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinSlope * 100.), (int) (MaxSlope * 100.));
            w.SetPos32((int)(_State->_Slope* 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
                { 2,    1000 }, //    10.0
                { 3,    2500 }, //    25.0
                { 4,    5000 }, //    50.0
                { 5,   10000 }, //   100.0
                { 6,  100000 }, //  1000.0
                { 7, 1000000 }, // 10000.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_SLOPE_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_SLOPE_OFFS, _State->_SlopeOffset);

            auto w = CUpDownCtrl(GetDlgItem(IDC_SLOPE_OFFS_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinSlopeOffset * 100.), (int) (MaxSlopeOffset * 100.));
            w.SetPos32((int)(_State->_SlopeOffset * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_EQ_AMT)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_AMT, _State->_EqualizeAmount);

            auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_AMT_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinEqualizeAmount * 100.), (int) (MaxEqualizeAmount * 100.));
            w.SetPos32((int)(_State->_EqualizeAmount * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
                { 2,    1000 }, //    10.0
                { 3,    2500 }, //    25.0
                { 4,    5000 }, //    50.0
                { 5,   10000 }, //   100.0
                { 6,  100000 }, //  1000.0
                { 7, 1000000 }, // 10000.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_EQ_OFFS)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_OFFS, _State->_EqualizeOffset);

            auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_OFFS_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinEqualizeOffset * 100.), (int) (MaxEqualizeOffset * 100.));
            w.SetPos32((int)(_State->_EqualizeOffset * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,     100 }, //     1.0
                { 2,    1000 }, //    10.0
                { 3,    2500 }, //    25.0
                { 4,    5000 }, //    50.0
                { 5,   10000 }, //   100.0
                { 6,  100000 }, //  1000.0
                { 7, 1000000 }, // 10000.0
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_EQ_DEPTH)); _NumericEdits.push_back(ne); SetDouble(IDC_EQ_DEPTH, _State->_EqualizeDepth);

            auto w = CUpDownCtrl(GetDlgItem(IDC_EQ_DEPTH_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinEqualizeDepth * 100.), (int) (MaxEqualizeDepth * 100.));
            w.SetPos32((int)(_State->_EqualizeDepth * 100.));
        }

        {
            UDACCEL Accel[] =
            {
                { 1,  1 }, // 0.01
                { 2,  5 }, // 0.05
                { 3, 10 }, // 0.10
            };

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_WT_AMT)); _NumericEdits.push_back(ne); SetDouble(IDC_WT_AMT, _State->_WeightingAmount);

            auto w = CUpDownCtrl(GetDlgItem(IDC_WT_AMT_SPIN));

            w.SetAccel(_countof(Accel), Accel);

            w.SetRange32((int) (MinWeightingAmount * 100.), (int) (MaxWeightingAmount * 100.));
            w.SetPos32((int)(_State->_WeightingAmount * 100.));
        }
    }
    #pragma endregion

    #pragma region X Axis
    {
        auto w = (CComboBox) GetDlgItem(IDC_X_AXIS_MODE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Bands", L"Decades", L"Octaves", L"Notes" })
            w.AddString(x);

        w.SetCurSel((int) _State->_XAxisMode);
    }

    {
        SendDlgItemMessageW(IDC_X_AXIS_TOP, BM_SETCHECK, _State->_XAxisTop);
        SendDlgItemMessageW(IDC_X_AXIS_BOTTOM, BM_SETCHECK, _State->_XAxisBottom);
    }
    #pragma endregion

    #pragma region Y Axis
    {
        {
            auto w = (CComboBox) GetDlgItem(IDC_Y_AXIS_MODE);

            w.ResetContent();

            for (const auto & x : { L"None", L"Decibel", L"Linear/n-th root" })
                w.AddString(x);

            w.SetCurSel((int) _State->_YAxisMode);
        }

        {
            SendDlgItemMessageW(IDC_Y_AXIS_LEFT, BM_SETCHECK, _State->_YAxisLeft);
            SendDlgItemMessageW(IDC_Y_AXIS_RIGHT, BM_SETCHECK, _State->_YAxisRight);
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
                CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_LO)); _NumericEdits.push_back(ne); SetDouble(IDC_AMPLITUDE_LO, _State->_AmplitudeLo);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_LO_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitude * 10.), (int) (MaxAmplitude * 10.));
                w.SetPos32((int) (_State->_AmplitudeLo * 10.));
            }

            {
                CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_HI)); _NumericEdits.push_back(ne); SetDouble(IDC_AMPLITUDE_HI, _State->_AmplitudeHi);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_HI_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitude * 10), (int) (MaxAmplitude * 10.));
                w.SetPos32((int) (_State->_AmplitudeHi * 10.));
            }

            {
                CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_AMPLITUDE_STEP)); _NumericEdits.push_back(ne); SetDouble(IDC_AMPLITUDE_STEP, _State->_AmplitudeStep, 0, 1);

                auto w = CUpDownCtrl(GetDlgItem(IDC_AMPLITUDE_STEP_SPIN));

                w.SetAccel(_countof(Accel), Accel);

                w.SetRange32((int) (MinAmplitudeStep * 10), (int) (MaxAmplitudeStep * 10.));
                w.SetPos32((int) (_State->_AmplitudeStep * 10.));
            }
        }

        // Used by the Linear/n-th root y-axis mode.
        {
            SendDlgItemMessageW(IDC_USE_ABSOLUTE, BM_SETCHECK, _State->_UseAbsolute);

            CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_GAMMA)); _NumericEdits.push_back(ne); SetDouble(IDC_GAMMA, _State->_Gamma, 0, 1);
        }
    }
    #pragma endregion

    #pragma region Common
    {
        auto w = (CComboBox) GetDlgItem(IDC_SMOOTHING_METHOD);

        w.ResetContent();

        for (const auto & x : { L"None", L"Average", L"Peak" })
            w.AddString(x);

        w.SetCurSel((int) _State->_SmoothingMethod);

        SetDouble(IDC_SMOOTHING_FACTOR, _State->_SmoothingFactor, 0, 1);
    }
    {
        SendDlgItemMessageW(IDC_SHOW_TOOLTIPS, BM_SETCHECK, _State->_ShowToolTips);
        SendDlgItemMessageW(IDC_SUPPRESS_MIRROR_IMAGE, BM_SETCHECK, _State->_SuppressMirrorImage);
    }
    {
        auto w = (CComboBox) GetDlgItem(IDC_BACKGROUND_MODE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Solid", L"Artwork" })
            w.AddString(x);

        w.SetCurSel((int) _State->_BackgroundMode);
    }
    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_ARTWORK_OPACITY)); _NumericEdits.push_back(ne); SetInteger(IDC_ARTWORK_OPACITY, (int64_t) (_State->_ArtworkOpacity * 100.f));

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

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_NUM_ARTWORK_COLORS)); _NumericEdits.push_back(ne); SetInteger(IDC_NUM_ARTWORK_COLORS, _State->_NumArtworkColors);

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

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_LIGHTNESS_THRESHOLD)); _NumericEdits.push_back(ne); SetInteger(IDC_LIGHTNESS_THRESHOLD, (int64_t) (_State->_LightnessThreshold * 100.f));

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
        GetDlgItem(IDC_FILE_PATH).SetWindowTextW(pfc::wideFromUTF8(_State->_ArtworkFilePath));
    }
    #pragma endregion

    #pragma region Visualization
    {
        auto w = (CComboBox) GetDlgItem(IDC_VISUALIZATION);

        w.ResetContent();

        for (const auto & x : { L"Bars", L"Curve" })
            w.AddString(x);

        w.SetCurSel((int) _State->_VisualizationType);
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_PEAK_MODE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Classic", L"Gravity", L"AIMP", L"Fade Out", L"Fading AIMP" })
            w.AddString(x);

        w.SetCurSel((int) _State->_PeakMode);
    }

    {
        SetDouble(IDC_HOLD_TIME, _State->_HoldTime, 0, 1);
        SetDouble(IDC_ACCELERATION, _State->_Acceleration, 0, 1);
    }

    #pragma region Bars

    {
        SendDlgItemMessageW(IDC_LED_MODE, BM_SETCHECK, _State->_LEDMode);
    }

    #pragma endregion

    #pragma endregion

    #pragma region Styles
    {
        _State->_CurrentStyle = (int) VisualElement::Background;

        auto w = (CListBox) GetDlgItem(IDC_STYLES);

        std::vector<Style> Styles;

        _State->_StyleManager.GetStyles(Styles);

        for (const auto & x : Styles)
            w.AddString(pfc::wideFromUTF8(x._Name));

        w.SetCurSel(_State->_CurrentStyle);
    }

    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SOURCE);

        w.ResetContent();

        for (const auto & x : { L"None", L"Solid", L"Dominant Color", L"Gradient", L"Windows", L"User Interface" })
            w.AddString(x);
    }

    _Color.Initialize(GetDlgItem(IDC_COLOR_BUTTON));

    {
        auto w = (CComboBox) GetDlgItem(IDC_COLOR_SCHEME);

        w.ResetContent();

        for (const auto & x : { L"Solid", L"Custom", L"Artwork", L"Prism 1", L"Prism 2", L"Prism 3", L"foobar2000", L"foobar2000 Dark Mode", L"Fire", L"Rainbow" })
            w.AddString(x);
    }

    {
        _Gradient.Initialize(GetDlgItem(IDC_GRADIENT));
        _Colors.Initialize(GetDlgItem(IDC_COLOR_LIST));

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_POSITION)); _NumericEdits.push_back(ne);
    }

    {
        UDACCEL Accel[] =
        {
            { 1,  1 },
            { 2,  5 },
            { 3, 10 },
        };

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_OPACITY)); _NumericEdits.push_back(ne);

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

        CNumericEdit * ne = new CNumericEdit(); ne->Initialize(GetDlgItem(IDC_THICKNESS)); _NumericEdits.push_back(ne);

        auto w = CUpDownCtrl(GetDlgItem(IDC_THICKNESS_SPIN));

        w.SetRange32((int) (MinThickness * 10.), (int) (MaxThickness * 10.));
        w.SetAccel(_countof(Accel), Accel);
    }

    UpdateStyleControls();
    #pragma endregion

    UpdateControls();
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void ConfigurationDialog::Terminate()
{
    if (_ToolTipControl.IsWindow())
        _ToolTipControl.DestroyWindow();

    _MenuList.Terminate();

    _Channels.Terminate();

    _Color.Terminate();

    _Colors.Terminate();
    _Gradient.Terminate();

    for (auto & Iter : _NumericEdits)
    {
        Iter->Terminate();
        delete Iter;
    }

    _NumericEdits.clear();
}

/// <summary>
/// Handles the WM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT ConfigurationDialog::OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case CC_GRADIENT_STOPS:
        {
            UpdateColorSchemeControls();
            break;
        }
    }

    SetMsgHandled(TRUE);

    return 0;
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void ConfigurationDialog::OnSelectionChanged(UINT, int id, CWindow w)
{
    if (_State == nullptr)
        return;

    CComboBox cb = (CComboBox) w;

    int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        #pragma region Menu
        case IDC_MENULIST:
        {
            size_t Selection = (size_t) _MenuList.GetCurSel();

            UpdatePages(Selection);

            _State->_PageIndex = Selection;
            return;
        }
        #pragma endregion

        #pragma region Transform
        case IDC_METHOD:
        {
            _State->_Transform = (Transform) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_WINDOW_FUNCTION:
        {
            _State->_WindowFunction = (WindowFunctions) SelectedIndex;

            UpdateControls();
            break;
        }
        #pragma endregion

        #pragma region FFT
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
        #pragma endregion

        #pragma region Frequencies
        case IDC_DISTRIBUTION:
        {
            _State->_FrequencyDistribution = (FrequencyDistribution) SelectedIndex;

            UpdateControls();
            break;
        }

        case IDC_SCALING_FUNCTION:
        {
            _State->_ScalingFunction = (ScalingFunction) SelectedIndex;
            break;
        }

        case IDC_SUMMATION_METHOD:
        {
            _State->_SummationMethod = (SummationMethod) SelectedIndex;
            break;
        }

        case IDC_SMOOTHING_METHOD:
        {
            _State->_SmoothingMethod = (SmoothingMethod) SelectedIndex;
            break;
        }
        #pragma endregion

        #pragma region Filters
        case IDC_ACOUSTIC_FILTER:
        {
            _State->_WeightingType = (WeightingType) SelectedIndex;
            UpdateControls();
            break;
        }
        #pragma endregion

        #pragma region Artwork Colors

        case IDC_COLOR_ORDER:
        {
            _State->_ColorOrder = (ColorOrder) SelectedIndex;
            _State->_NewArtworkParameters = true;
            break;
        }

        #pragma endregion

        #pragma region Background Mode

        case IDC_BACKGROUND_MODE:
        {
            _State->_BackgroundMode = (BackgroundMode) SelectedIndex;

            UpdateControls();
            break;
        }

        #pragma endregion

        #pragma region X axis

        case IDC_X_AXIS_MODE:
        {
            _State->_XAxisMode = (XAxisMode) SelectedIndex;
            break;
        }

        #pragma endregion

        #pragma region Y axis
        case IDC_Y_AXIS_MODE:
        {
            _State->_YAxisMode = (YAxisMode) SelectedIndex;

            UpdateControls();
            break;
        }
        #pragma endregion

        #pragma region Visualization

        case IDC_VISUALIZATION:
        {
            _State->_VisualizationType = (VisualizationType) SelectedIndex;

            UpdateControls();
            break;
        }

        #pragma region Bars

        case IDC_PEAK_MODE:
        {
            _State->_PeakMode = (PeakMode) SelectedIndex;

            UpdateControls();
            break;
        }

        #pragma endregion

        #pragma endregion

        #pragma region Styles

        case IDC_STYLES:
        {
            _State->_CurrentStyle = ((CListBox) w).GetCurSel();

            UpdateStyleControls();

            return;
        }

        case IDC_COLOR_SOURCE:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_ColorSource = (ColorSource) SelectedIndex;

            UpdateStyleControls();

            break;
        }

        case IDC_COLOR_INDEX:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_ColorIndex = (uint32_t) SelectedIndex;

            UpdateStyleControls();

            break;
        }

        case IDC_COLOR_SCHEME:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_ColorScheme = (ColorScheme) SelectedIndex;

            UpdateColorSchemeControls();
            UpdateStyleControls();

            break;
        }

        case IDC_COLOR_LIST:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            // Show the position of the selected color of the gradient.
            size_t Index = (size_t) _Colors.GetCurSel();

            if (!InRange(Index, (size_t) 0, style->_GradientStops.size() - 1))
                return;

                t_int64 Position = (t_int64) (style->_GradientStops[Index].position * 100.f);
                SetInteger(IDC_POSITION, Position);

            // Update the state of the buttons.
            bool HasSelection = (Index != LB_ERR);
            bool UseArtwork = (style->_ColorScheme == ColorScheme::Artwork);
            bool HasMoreThanOneColor = (style->_GradientStops.size() > 1);

                GetDlgItem(IDC_ADD).EnableWindow(HasSelection && !UseArtwork);
                GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor && !UseArtwork);
                GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

                GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);
                GetDlgItem(IDC_SPREAD).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

            return;
        }

        #pragma endregion
    }

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void ConfigurationDialog::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || (code != EN_CHANGE))
        return;

    WCHAR Text[MAX_PATH];

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        #pragma region FFT

        case IDC_NUM_BINS_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_State->_FFTMode)
            {
                default:
                    break;

                case FFTMode::FFTCustom: { _State->_FFTCustom = (size_t) Clamp(::_wtoi(Text), MinFFTSize, MaxFFTSize); break; }
                case FFTMode::FFTDuration: { _State->_FFTDuration= Clamp(::_wtof(Text), MinFFTDuration, MaxFFTDuration); break; }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE: { _State->_KernelSize = Clamp(::_wtoi(Text), MinKernelSize, MaxKernelSize); break; }
        case IDC_WINDOW_PARAMETER: { _State->_WindowParameter = Clamp(::_wtof(Text), MinWindowParameter, MaxWindowParameter); break; }
        case IDC_WINDOW_SKEW: { _State->_WindowSkew = Clamp(::_wtof(Text), MinWindowSkew, MaxWindowSkew); break; }

        case IDC_REACTION_ALIGNMENT: { _State->_ReactionAlignment = Clamp(::_wtof(Text), MinReactionAlignment, MaxReactionAlignment); break; }

        #pragma endregion

        #pragma region Brown-Puckette CQT

        case IDC_BW_OFFSET: { _State->_BandwidthOffset = Clamp(::_wtof(Text), MinBandwidthOffset, MaxBandwidthOffset); break; }
        case IDC_BW_CAP: { _State->_BandwidthCap = Clamp(::_wtof(Text), MinBandwidthCap, MaxBandwidthCap); break; }
        case IDC_BW_AMOUNT: { _State->_BandwidthAmount = Clamp(::_wtof(Text), MinBandwidthAmount, MaxBandwidthAmount); break; }
        case IDC_KERNEL_SHAPE_PARAMETER: { _State->_KernelShapeParameter = Clamp(::_wtof(Text), MinWindowParameter, MaxWindowParameter); break; }
        case IDC_KERNEL_ASYMMETRY: { _State->_KernelAsymmetry = Clamp(::_wtof(Text), MinWindowSkew, MaxWindowSkew); break; }

        #pragma endregion

        #pragma region SWIFT

        case IDC_FBO: { _State->_FilterBankOrder = Clamp((size_t) ::_wtoi(Text), MinFilterBankOrder, MaxFilterBankOrder); break; }
        case IDC_TR: { _State->_TimeResolution = Clamp(::_wtof(Text), MinTimeResolution, MaxTimeResolution); break; }
        case IDC_BW_SWIFT: { _State->_SWIFTBandwidth = Clamp(::_wtof(Text), MinSWIFTBandwidth, MaxSWIFTBandwidth); break; }

        #pragma endregion

        #pragma region Frequencies

        case IDC_NUM_BANDS: { _State->_NumBands = (size_t) Clamp(::_wtoi(Text), MinBands, MaxBands); break; }

        case IDC_LO_FREQUENCY:
        {
            _State->_LoFrequency = Min(Clamp(::_wtof(Text), MinFrequency, MaxFrequency), _State->_HiFrequency);

            CUpDownCtrl(GetDlgItem(IDC_LO_FREQUENCY_SPIN)).SetPos32((int)(_State->_LoFrequency * 100.));
            break;
        }

        case IDC_HI_FREQUENCY:
        {
            _State->_HiFrequency = Max(Clamp(::_wtof(Text), MinFrequency, MaxFrequency), _State->_LoFrequency);

            CUpDownCtrl(GetDlgItem(IDC_HI_FREQUENCY_SPIN)).SetPos32((int)(_State->_HiFrequency * 100.));
            break;
        }

        case IDC_PITCH: { _State->_Pitch = Clamp(::_wtof(Text), MinPitch, MaxPitch); break; }

        #pragma endregion

        #pragma region Filters

        #define ON_EDIT_CHANGE_DOUBLE(x,y) _State->_##x = Clamp(::_wtof(Text), Min##x, Max##x); CUpDownCtrl(GetDlgItem(y)).SetPos32((int)(_State->_##x * 100.));

        case IDC_SLOPE_FN_OFFS: { ON_EDIT_CHANGE_DOUBLE(SlopeFunctionOffset, IDC_SLOPE_FN_OFFS); break; }
        case IDC_SLOPE:         { ON_EDIT_CHANGE_DOUBLE(Slope, IDC_SLOPE); break; }
        case IDC_SLOPE_OFFS:    { ON_EDIT_CHANGE_DOUBLE(SlopeOffset, IDC_SLOPE_OFFS); break; }
        case IDC_EQ_AMT:        { ON_EDIT_CHANGE_DOUBLE(EqualizeAmount, IDC_EQ_AMT); break; }
        case IDC_EQ_OFFS:       { ON_EDIT_CHANGE_DOUBLE(EqualizeOffset, IDC_EQ_OFFS); break; }
        case IDC_EQ_DEPTH:      { ON_EDIT_CHANGE_DOUBLE(EqualizeDepth, IDC_EQ_DEPTH); break; }
        case IDC_WT_AMT:        { ON_EDIT_CHANGE_DOUBLE(WeightingAmount, IDC_WT_AMT); break; }

        #pragma endregion

        #pragma region Artwork Image

        case IDC_ARTWORK_OPACITY: { _State->_ArtworkOpacity = (FLOAT) Clamp(::_wtof(Text) / 100.f, MinArtworkOpacity, MaxArtworkOpacity); break; }

        #pragma endregion

        #pragma region Script

        case IDC_FILE_PATH:
        {
            _State->_ArtworkFilePath = pfc::utf8FromWide(Text);
            break;
        }

        #pragma endregion

        #pragma region Y axis

        case IDC_AMPLITUDE_LO: { _State->_AmplitudeLo = Min(Clamp(::_wtof(Text), MinAmplitude, MaxAmplitude), _State->_AmplitudeHi); break; }
        case IDC_AMPLITUDE_HI: { _State->_AmplitudeHi = Max(Clamp(::_wtof(Text), MinAmplitude, MaxAmplitude), _State->_AmplitudeLo); break; }
        case IDC_AMPLITUDE_STEP: { _State->_AmplitudeStep = Max(Clamp(::_wtof(Text), MinAmplitudeStep, MaxAmplitudeStep), _State->_AmplitudeStep); break; }
        case IDC_GAMMA: { _State->_Gamma = Clamp(::_wtof(Text), MinGamma, MaxGamma); break; }

        #pragma endregion

        #pragma region Visualization

        #pragma region Peak indicator

        case IDC_SMOOTHING_FACTOR: { _State->_SmoothingFactor = Clamp(::_wtof(Text), MinSmoothingFactor, MaxSmoothingFactor); break; }
        case IDC_HOLD_TIME: { _State->_HoldTime = Clamp(::_wtof(Text), MinHoldTime, MaxHoldTime); break; }
        case IDC_ACCELERATION: { _State->_Acceleration = Clamp(::_wtof(Text), MinAcceleration, MaxAcceleration); break; }

        #pragma endregion

        #pragma endregion

        #pragma region Styles

        #pragma region Artwork Colors

        case IDC_NUM_ARTWORK_COLORS:
        {
            _State->_NumArtworkColors = Clamp((uint32_t) ::_wtoi(Text), MinArtworkColors, MaxArtworkColors);
            _State->_NewArtworkParameters = true;
            break;
        }

        case IDC_LIGHTNESS_THRESHOLD:
        {
            _State->_LightnessThreshold = (FLOAT) Clamp(::_wtof(Text) / 100.f, MinLightnessThreshold, MaxLightnessThreshold);
            _State->_NewArtworkParameters = true;
            break;
        }

        #pragma endregion

        #pragma region Color Scheme

        case IDC_POSITION:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            size_t Index = (size_t) _Colors.GetCurSel();

            if (!InRange(Index, (size_t) 0, style->_GradientStops.size() - 1))
                return;

            int Position = Clamp(::_wtoi(Text), 0, 100);

            if ((int) (style->_GradientStops[Index].position * 100.f) == Position)
                break;

            style->_GradientStops[Index].position = (FLOAT) Position / 100.f;

            style->_ColorScheme = ColorScheme::Custom;
            style->_CustomGradientStops = style->_GradientStops;

            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)). SetCurSel((int) style->_ColorScheme);
            _Gradient.SetGradientStops(style->_GradientStops);
            break;
        }

        #pragma endregion

        case IDC_OPACITY:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_Opacity = (FLOAT) Clamp(::_wtof(Text) / 100.f, MinOpacity, MaxOpacity);
            break;
        }

        case IDC_THICKNESS:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_Thickness = (FLOAT) Clamp(::_wtof(Text), MinThickness, MaxThickness);
            break;
        }

        #pragma endregion

        default:
            return;
    }

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void ConfigurationDialog::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    switch (id)
    {
        // FFT
        case IDC_NUM_BINS_PARAMETER:
        {
            #pragma warning (disable: 4061)
            switch (_State->_FFTMode)
            {
                default:
                    break;

                case FFTMode::FFTCustom: { SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTCustom); break; }
                case FFTMode::FFTDuration: { SetInteger(IDC_NUM_BINS_PARAMETER, (int64_t) _State->_FFTDuration); break; }
            }
            #pragma warning (default: 4061)
            break;
        }

        case IDC_KERNEL_SIZE:           { SetInteger(id, _State->_KernelSize); break; }
        case IDC_WINDOW_PARAMETER:      { SetDouble(id, _State->_WindowParameter); break; }
        case IDC_WINDOW_SKEW:           { SetDouble(id, _State->_WindowSkew); break; }
        case IDC_REACTION_ALIGNMENT:    { SetDouble(id, _State->_ReactionAlignment); break; }

        // Brown-Puckette CQT
        case IDC_BW_OFFSET:             { SetDouble(id, _State->_BandwidthOffset); break; }
        case IDC_BW_CAP:                { SetDouble(id, _State->_BandwidthCap); break; }
        case IDC_BW_AMOUNT:             { SetDouble(id, _State->_BandwidthAmount); break; }
        case IDC_KERNEL_SHAPE_PARAMETER:{ SetDouble(id, _State->_KernelShapeParameter); break; }
        case IDC_KERNEL_ASYMMETRY:      { SetDouble(id, _State->_KernelAsymmetry); break; }

        // SWIFT
        case IDC_FBO:                   { SetInteger(id, (int64_t) _State->_FilterBankOrder); break; }
        case IDC_TR:                    { SetDouble(id, _State->_TimeResolution); break; }
        case IDC_BW_SWIFT:              { SetDouble(id, _State->_SWIFTBandwidth); break; }

        // Frequencies
        case IDC_NUM_BANDS:             { SetInteger(id, (int64_t) _State->_NumBands); break; }
        case IDC_LO_FREQUENCY:          { SetDouble(id, _State->_LoFrequency); break; }
        case IDC_HI_FREQUENCY:          { SetDouble(id, _State->_HiFrequency); break; }
        case IDC_PITCH:                 { SetDouble(id, _State->_Pitch); break; }
        case IDC_SKEW_FACTOR:           { SetDouble(id, _State->_SkewFactor); break; }
        case IDC_BANDWIDTH:             { SetDouble(id, _State->_Bandwidth, 0, 1); break; }

        // Filters
        case IDC_SLOPE_FN_OFFS:         { SetDouble(id, _State->_SlopeFunctionOffset); break; }
        case IDC_SLOPE:                 { SetDouble(id, _State->_Slope); break; }
        case IDC_SLOPE_OFFS:            { SetDouble(id, _State->_SlopeOffset); break; }
        case IDC_EQ_AMT:                { SetDouble(id, _State->_EqualizeAmount); break; }
        case IDC_EQ_OFFS:               { SetDouble(id, _State->_EqualizeOffset); break; }
        case IDC_EQ_DEPTH:              { SetDouble(id, _State->_EqualizeDepth); break; }
        case IDC_WT_AMT:                { SetDouble(id, _State->_WeightingAmount); break; }

        // Artwork Colors
        case IDC_NUM_ARTWORK_COLORS:    { SetInteger(id, _State->_NumArtworkColors); break; }
        case IDC_LIGHTNESS_THRESHOLD:   { SetInteger(id, (int64_t) (_State->_LightnessThreshold * 100.f)); break; }

        // Artwork Image
        case IDC_ARTWORK_OPACITY:       { SetInteger(id, (int64_t) (_State->_ArtworkOpacity * 100.f)); break; }

        // Y axis
        case IDC_AMPLITUDE_LO:          { SetDouble(id, _State->_AmplitudeLo, 0, 1); break; }
        case IDC_AMPLITUDE_HI:          { SetDouble(id, _State->_AmplitudeHi, 0, 1); break; }
        case IDC_AMPLITUDE_STEP:        { SetDouble(id, _State->_AmplitudeStep, 0, 1); break; }
        case IDC_GAMMA:                 { SetDouble(id, _State->_Gamma, 0, 1); break; }

        // Spectrum smoothing
        case IDC_SMOOTHING_FACTOR:      { SetDouble(id, _State->_SmoothingFactor, 0, 1); break; }

        // Peak indicator
        case IDC_HOLD_TIME:             { SetDouble(id, _State->_HoldTime, 0, 1); break; }
        case IDC_ACCELERATION:          { SetDouble(id, _State->_Acceleration, 0, 1); break; }

        // Styles
        case IDC_OPACITY:               { SetInteger(id, (int64_t) (_State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle)->_Opacity * 100.f)); break; }
        case IDC_THICKNESS:             { SetDouble(id, _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle)->_Thickness, 0, 1); break; }
    }

    return;
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void ConfigurationDialog::OnButtonClick(UINT, int id, CWindow)
{
    if (_State == nullptr)
        return;

    switch (id)
    {
        case IDC_CHANNELS:
        {
            BOOL Handled = TRUE;

            _Channels.OnClicked(0, 0, 0, Handled);

            return;
        }

        case IDC_GRANULAR_BW: { _State->_GranularBW = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_SMOOTH_LOWER_FREQUENCIES: { _State->_SmoothLowerFrequencies = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_SMOOTH_GAIN_TRANSITION: { _State->_SmoothGainTransition = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }

        case IDC_X_AXIS_TOP: { _State->_XAxisTop = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_X_AXIS_BOTTOM: { _State->_XAxisBottom = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_Y_AXIS_LEFT: { _State->_YAxisLeft = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_Y_AXIS_RIGHT: { _State->_YAxisRight = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }

        case IDC_USE_ABSOLUTE: { _State->_UseAbsolute = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_SHOW_TOOLTIPS: { _State->_ShowToolTips = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }
        case IDC_SUPPRESS_MIRROR_IMAGE: { _State->_SuppressMirrorImage = (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }

        case IDC_LED_MODE: { _State->_LEDMode= (bool) SendDlgItemMessageW(id, BM_GETCHECK); break; }

        case IDC_ADD:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            size_t Index = (size_t) _Colors.GetCurSel();

            if (!InRange(Index, (size_t) 0, style->_GradientStops.size() - 1))
                return;

            D2D1_COLOR_F Color = style->_GradientStops[Index].color;

            CColorDialogEx cd;

            if (!cd.SelectColor(m_hWnd, Color))
                return;

            style->_GradientStops.insert(style->_GradientStops.begin() + (int) Index + 1, { 0.f, Color });
            
            UpdateGradientStopPositons(style);
            UpdateColorSchemeControls();
            break;
        }

        case IDC_REMOVE:
        {
            // Don't remove the last color.
            if (_Colors.GetCount() == 1)
                return;

            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            size_t Index = (size_t) _Colors.GetCurSel();

            if (!InRange(Index, (size_t) 0, style->_GradientStops.size() - 1))
                return;

            style->_GradientStops.erase(style->_GradientStops.begin() + (int) Index);

            UpdateGradientStopPositons(style);
            UpdateColorSchemeControls();
            break;
        }

        case IDC_REVERSE:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            std::reverse(style->_GradientStops.begin(), style->_GradientStops.end());

            UpdateGradientStopPositons(style);
            UpdateColorSchemeControls();
            break;
        }

        case IDC_SPREAD:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            UpdateGradientStopPositons(style);
            UpdateColorSchemeControls();
            break;
        }

        case IDC_HORIZONTAL_GRADIENT:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            if ((bool) SendDlgItemMessageW(id, BM_GETCHECK))
                style->_Flags |= Style::HorizontalGradient;
            else
                style->_Flags &= ~Style::HorizontalGradient;
            break;
        }

        case IDC_RESET:
        {
            _State->Reset();

            Initialize();

            return ;
        }

        case IDOK:
        case IDCANCEL:
        {
            if (id == IDCANCEL)
                *_State = _OldConfiguration;

            GetWindowRect(&_State->_DialogBounds);

            Terminate();

            DestroyWindow();

            _State = nullptr;
            break;
        }

        default:
            return;
    }

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);
}

/// <summary>
/// Handles a notification from an UpDown control.
/// </summary>
LRESULT ConfigurationDialog::OnDeltaPos(LPNMHDR nmhd)
{
    if (_State == nullptr)
        return -1;

    LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

    switch (nmhd->idFrom)
    {
        case IDC_KERNEL_SIZE_SPIN:
        {
            _State->_KernelSize = ClampNewSpinPosition(nmud, MinKernelSize, MaxKernelSize);
            break;
        }

        case IDC_LO_FREQUENCY_SPIN:
        {
            _State->_LoFrequency = Min(ClampNewSpinPosition(nmud, MinFrequency, MaxFrequency, 100.), _State->_HiFrequency);
            SetDouble(IDC_LO_FREQUENCY, _State->_LoFrequency);
            break;
        }

        case IDC_HI_FREQUENCY_SPIN:
        {
            _State->_HiFrequency = Max(ClampNewSpinPosition(nmud, MinFrequency, MaxFrequency, 100.), _State->_LoFrequency);
            SetDouble(IDC_HI_FREQUENCY, _State->_HiFrequency);
            break;
        }

        case IDC_NUM_BANDS_SPIN:
        {
            _State->_NumBands = (size_t) ClampNewSpinPosition(nmud, MinBands, MaxBands);
            SetInteger(IDC_NUM_BANDS, (int64_t) _State->_NumBands);
            break;
        }

        case IDC_MIN_NOTE_SPIN:
        {
            _State->_MinNote = Min((uint32_t) ClampNewSpinPosition(nmud, MinNote, MaxNote), _State->_MaxNote);
            SetNote(IDC_MIN_NOTE, _State->_MinNote);
            break;
        }

        case IDC_MAX_NOTE_SPIN:
        {
            _State->_MaxNote = Max((uint32_t) ClampNewSpinPosition(nmud, MinNote, MaxNote), _State->_MinNote);
            SetNote(IDC_MAX_NOTE, _State->_MaxNote);
            break;
        }

        case IDC_BANDS_PER_OCTAVE_SPIN:
        {
            _State->_BandsPerOctave = (uint32_t) ClampNewSpinPosition(nmud, MinBandsPerOctave, MaxBandsPerOctave);
            break;
        }

        case IDC_PITCH_SPIN:
        {
            _State->_Pitch = ClampNewSpinPosition(nmud, MinPitch, MaxPitch, 100.);
            SetDouble(IDC_PITCH, _State->_Pitch);
            break;
        }

        case IDC_TRANSPOSE_SPIN:
        {
            _State->_Transpose = ClampNewSpinPosition(nmud, MinTranspose, MaxTranspose);
            break;
        }

        // Filters
        case IDC_SLOPE_FN_OFFS_SPIN:
        {
            _State->_SlopeFunctionOffset = ClampNewSpinPosition(nmud, MinSlopeFunctionOffset, MaxSlopeFunctionOffset, 100.);
            SetDouble(IDC_SLOPE_FN_OFFS, _State->_SlopeFunctionOffset);
            break;
        }

        case IDC_SLOPE_SPIN:
        {
            _State->_Slope = ClampNewSpinPosition(nmud, MinSlope, MaxSlope, 100.);
            SetDouble(IDC_SLOPE, _State->_Slope);
            break;
        }

        case IDC_SLOPE_OFFS_SPIN:
        {
            _State->_SlopeOffset = ClampNewSpinPosition(nmud, MinSlopeOffset, MaxSlopeOffset, 100.);
            SetDouble(IDC_SLOPE_OFFS, _State->_SlopeOffset);
            break;
        }

        case IDC_EQ_AMT_SPIN:
        {
            _State->_EqualizeAmount = ClampNewSpinPosition(nmud, MinEqualizeAmount, MaxEqualizeAmount, 100.);
            SetDouble(IDC_EQ_AMT, _State->_EqualizeAmount);
            break;
        }

        case IDC_EQ_OFFS_SPIN:
        {
            _State->_EqualizeOffset = ClampNewSpinPosition(nmud, MinEqualizeOffset, MaxEqualizeOffset, 100.);
            SetDouble(IDC_EQ_OFFS, _State->_EqualizeOffset);
            break;
        }

        case IDC_EQ_DEPTH_SPIN:
        {
            _State->_EqualizeDepth = ClampNewSpinPosition(nmud, MinEqualizeDepth, MaxEqualizeDepth, 100.); 
            SetDouble(IDC_EQ_DEPTH, _State->_EqualizeDepth);
            break;
        }

        case IDC_WT_AMT_SPIN:
        {
            _State->_WeightingAmount = ClampNewSpinPosition(nmud, MinWeightingAmount, MaxWeightingAmount, 100.);
            SetDouble(IDC_WT_AMT, _State->_WeightingAmount);
            break;
        }

        case IDC_AMPLITUDE_LO_SPIN:
        {
            _State->_AmplitudeLo = Min(ClampNewSpinPosition(nmud, MinAmplitude, MaxAmplitude, 10.), _State->_AmplitudeHi);
            SetDouble(IDC_AMPLITUDE_LO, _State->_AmplitudeLo, 0, 1);
            break;
        }

        case IDC_AMPLITUDE_HI_SPIN:
        {
            _State->_AmplitudeHi = Max(ClampNewSpinPosition(nmud, MinAmplitude, MaxAmplitude, 10.), _State->_AmplitudeLo);
            SetDouble(IDC_AMPLITUDE_HI, _State->_AmplitudeHi, 0, 1);
            break;
        }

        case IDC_AMPLITUDE_STEP_SPIN:
        {
            _State->_AmplitudeStep = ClampNewSpinPosition(nmud, MinAmplitudeStep, MaxAmplitudeStep, 10.);
            SetDouble(IDC_AMPLITUDE_STEP, _State->_AmplitudeStep, 0, 1);
            break;
        }

        case IDC_SKEW_FACTOR_SPIN:
        {
            _State->_SkewFactor = ClampNewSpinPosition(nmud, MinSkewFactor, MaxSkewFactor, 100.);
            SetDouble(IDC_SKEW_FACTOR, _State->_SkewFactor);
            break;
        }

        case IDC_BANDWIDTH_SPIN:
        {
            _State->_Bandwidth = ClampNewSpinPosition(nmud, MinBandwidth, MaxBandwidth, 10.);
            SetDouble(IDC_BANDWIDTH, _State->_Bandwidth, 0, 1);
            break;
        }

        case IDC_NUM_ARTWORK_COLORS_SPIN:
        {
            _State->_NumArtworkColors = (size_t) ClampNewSpinPosition(nmud, MinArtworkColors, MaxArtworkColors);
            SetInteger(IDC_NUM_ARTWORK_COLORS, _State->_NumArtworkColors);
            break;
        }

        case IDC_LIGHTNESS_THRESHOLD_SPIN:
        {
            _State->_LightnessThreshold = (FLOAT) ClampNewSpinPosition(nmud, MinLightnessThreshold, MaxLightnessThreshold, 100.);
            SetInteger(IDC_LIGHTNESS_THRESHOLD, (int64_t) (_State->_LightnessThreshold * 100.f));
            break;
        }

        case IDC_ARTWORK_OPACITY_SPIN:
        {
            _State->_ArtworkOpacity = (FLOAT) ClampNewSpinPosition(nmud, MinArtworkOpacity, MaxArtworkOpacity, 100.);
            SetInteger(IDC_ARTWORK_OPACITY, (int64_t) (_State->_ArtworkOpacity * 100.f));
            break;
        }

        case IDC_OPACITY_SPIN:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_Opacity = (FLOAT) ClampNewSpinPosition(nmud, MinOpacity, MaxOpacity, 100.);
            SetInteger(IDC_OPACITY, (int64_t) (style->_Opacity * 100.f));
            break;
        }

        case IDC_THICKNESS_SPIN:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            style->_Thickness = (FLOAT) ClampNewSpinPosition(nmud, MinThickness, MaxThickness, 10.);
            SetDouble(IDC_THICKNESS, style->_Thickness, 0, 1);
            break;
        }

        default:
            return -1;
    }

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);

    return 0;
}

/// <summary>
/// Handles a Change notification from the custom controls.
/// </summary>
LRESULT ConfigurationDialog::OnChanged(LPNMHDR nmhd)
{
    if (_State == nullptr)
        return -1;

    switch (nmhd->idFrom)
    {
        case IDC_COLOR_LIST:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            std::vector<D2D1_COLOR_F> Colors;

            _Colors.GetColors(Colors);

            if (Colors.empty())
                return 0;

            style->_ColorScheme = ColorScheme::Custom;
            _Direct2D.CreateGradientStops(Colors, style->_CustomGradientStops);

            if (style->_ColorSource == ColorSource::Gradient)
                style->_GradientStops = style->_CustomGradientStops;

            // Update the controls.
            ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) style->_ColorScheme);
            _Gradient.SetGradientStops(style->_CustomGradientStops);
            break;
        }

        case IDC_COLOR_BUTTON:
        {
            Style * style = _State->_StyleManager.GetStyle((VisualElement) _State->_CurrentStyle);

            _Color.GetColor(style->_CustomColor);

            style->_ColorSource = ColorSource::Solid; // Force the color source to Solid.
            style->_Color = style->_CustomColor;

            UpdateStyleControls();
            break;
        }

        default:
            return -1;
    }

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);

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
        _State->_SelectedChannels = IsChecked ? 0 : AllChannels;
    }
    else
    if (InRange(id, IDM_CHANNELS_FIRST, IDM_CHANNELS_LAST - 1))
    {
        uint32_t Mask = 1U << (id - IDM_CHANNELS_FIRST);

        if (IsChecked)
            _State->_SelectedChannels &= ~Mask;
        else
            _State->_SelectedChannels |=  Mask;
    }
    else
        return;

    UpdateChannelsMenu();

    if (!_IsInitializing)
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);
}

/// <summary>
/// Updates the visibility of the controls.
/// </summary>
void ConfigurationDialog::UpdatePages(size_t index) const noexcept
{
    static const int Page1[] =
    {
        // Transform
        IDC_TRANSFORM_GROUP,
            IDC_METHOD_LBL, IDC_METHOD,
            IDC_WINDOW_FUNCTION_LBL, IDC_WINDOW_FUNCTION,
            IDC_WINDOW_PARAMETER_LBL, IDC_WINDOW_PARAMETER,
            IDC_WINDOW_SKEW_LBL, IDC_WINDOW_SKEW,
            IDC_REACTION_ALIGNMENT_LBL, IDC_REACTION_ALIGNMENT,
            IDC_CHANNELS,

        // FFT
        IDC_FFT_GROUP,
            IDC_NUM_BINS_LBL, IDC_NUM_BINS, IDC_NUM_BINS_PARAMETER_NAME, IDC_NUM_BINS_PARAMETER, IDC_NUM_BINS_PARAMETER_UNIT,
            IDC_SUMMATION_METHOD_LBL, IDC_SUMMATION_METHOD,
            IDC_MAPPING_METHOD_LBL, IDC_MAPPING_METHOD,
            IDC_SMOOTH_LOWER_FREQUENCIES,
            IDC_SMOOTH_GAIN_TRANSITION,
            IDC_KERNEL_SIZE_LBL, IDC_KERNEL_SIZE, IDC_KERNEL_SIZE_SPIN,

        // Brown-Puckette CQT
        IDC_BP_GROUP,
            IDC_BW_OFFSET_LBL, IDC_BW_OFFSET, IDC_BW_CAP_LBL, IDC_BW_CAP, IDC_BW_AMOUNT_LBL, IDC_BW_AMOUNT, IDC_GRANULAR_BW,
            IDC_KERNEL_SHAPE_LBL, IDC_KERNEL_SHAPE, IDC_KERNEL_SHAPE_PARAMETER_LBL, IDC_KERNEL_SHAPE_PARAMETER, IDC_KERNEL_ASYMMETRY_LBL, IDC_KERNEL_ASYMMETRY,

        // SWIFT
        IDC_SWIFT_GROUP,
            IDC_FBO_LBL, IDC_FBO, IDC_TR_LBL, IDC_TR, IDC_BW_SWIFT_LBL, IDC_BW_SWIFT
    };

    static const int Page2[] =
    {
        // Frequencies
        IDC_FREQUENCIES_GROUP,
            IDC_DISTRIBUTION_LBL, IDC_DISTRIBUTION,
            IDC_NUM_BANDS_LBL, IDC_NUM_BANDS, IDC_NUM_BANDS_SPIN,
            IDC_RANGE_LBL_1, IDC_LO_FREQUENCY, IDC_LO_FREQUENCY_SPIN, IDC_RANGE_LBL_2, IDC_HI_FREQUENCY, IDC_HI_FREQUENCY_SPIN, IDC_RANGE_LBL_3,
            IDC_MIN_NOTE_LBL, IDC_MIN_NOTE, IDC_MIN_NOTE_SPIN, IDC_MAX_NOTE_LBL, IDC_MAX_NOTE, IDC_MAX_NOTE_SPIN,
            IDC_BANDS_PER_OCTAVE_LBL, IDC_BANDS_PER_OCTAVE, IDC_BANDS_PER_OCTAVE_SPIN,
            IDC_PITCH_LBL_1, IDC_PITCH, IDC_PITCH_SPIN, IDC_PITCH_LBL_2,
            IDC_TRANSPOSE_LBL, IDC_TRANSPOSE, IDC_TRANSPOSE_SPIN,
            IDC_SCALING_FUNCTION_LBL, IDC_SCALING_FUNCTION,
            IDC_SKEW_FACTOR_LBL, IDC_SKEW_FACTOR, IDC_SKEW_FACTOR_SPIN,
            IDC_BANDWIDTH_LBL, IDC_BANDWIDTH, IDC_BANDWIDTH_SPIN,
    };

    static const int Page3[] =
    {
        // Filters
        IDC_FILTERS_GROUP,
            IDC_ACOUSTIC_FILTER_LBL, IDC_ACOUSTIC_FILTER,

            IDC_SLOPE_FN_OFFS_LBL, IDC_SLOPE_FN_OFFS, IDC_SLOPE_FN_OFFS_SPIN,
            IDC_SLOPE_LBL, IDC_SLOPE, IDC_SLOPE_SPIN, IDC_SLOPE_UNIT,
            IDC_SLOPE_OFFS_LBL, IDC_SLOPE_OFFS, IDC_SLOPE_OFFS_SPIN, IDC_SLOPE_OFFS_UNIT,

            IDC_EQ_AMT_LBL, IDC_EQ_AMT, IDC_EQ_AMT_SPIN,
            IDC_EQ_OFFS_LBL, IDC_EQ_OFFS, IDC_EQ_OFFS_SPIN, IDC_EQ_OFFS_UNIT,
            IDC_EQ_DEPTH_LBL, IDC_EQ_DEPTH, IDC_EQ_DEPTH_SPIN, IDC_EQ_DEPTH_UNIT,

            IDC_WT_AMT_LBL, IDC_WT_AMT, IDC_WT_AMT_SPIN,
    };

    static const int Page4[] =
    {
        // Common
        IDC_COMMON,
            IDC_SMOOTHING_METHOD, IDC_SMOOTHING_METHOD_LBL, IDC_SMOOTHING_FACTOR, IDC_SMOOTHING_FACTOR_LBL,
            IDC_SHOW_TOOLTIPS, IDC_SUPPRESS_MIRROR_IMAGE,

            IDC_BACKGROUND_MODE_LBL, IDC_BACKGROUND_MODE,
            IDC_ARTWORK_OPACITY_LBL, IDC_ARTWORK_OPACITY, IDC_ARTWORK_OPACITY_SPIN, IDC_ARTWORK_OPACITY_LBL_2,
            IDC_FILE_PATH_LBL, IDC_FILE_PATH,
            IDC_NUM_ARTWORK_COLORS_LBL, IDC_NUM_ARTWORK_COLORS, IDC_NUM_ARTWORK_COLORS_SPIN,
            IDC_LIGHTNESS_THRESHOLD_LBL, IDC_LIGHTNESS_THRESHOLD, IDC_LIGHTNESS_THRESHOLD_SPIN, IDC_LIGHTNESS_THRESHOLD_LBL_2,
            IDC_COLOR_ORDER_LBL, IDC_COLOR_ORDER,

        // X axis
        IDC_X_AXIS,
            IDC_X_AXIS_MODE_LBL, IDC_X_AXIS_MODE,
            IDC_X_AXIS_TOP, IDC_X_AXIS_BOTTOM,

        // Y axis
        IDC_Y_AXIS,
            IDC_Y_AXIS_MODE_LBL, IDC_Y_AXIS_MODE,
            IDC_Y_AXIS_LEFT, IDC_Y_AXIS_RIGHT,
            IDC_AMPLITUDE_LBL_1, IDC_AMPLITUDE_LO, IDC_AMPLITUDE_LO_SPIN, IDC_AMPLITUDE_LBL_2, IDC_AMPLITUDE_HI, IDC_AMPLITUDE_HI_SPIN, IDC_AMPLITUDE_LBL_3,
            IDC_AMPLITUDE_STEP_LBL_1,IDC_AMPLITUDE_STEP, IDC_AMPLITUDE_STEP_SPIN, IDC_AMPLITUDE_STEP_LBL_2,
            IDC_USE_ABSOLUTE,
            IDC_GAMMA_LBL, IDC_GAMMA,
    };

    static const int Page5[] =
    {
        IDC_VISUALIZATION_LBL, IDC_VISUALIZATION,

        IDC_PEAK_MODE, IDC_PEAK_MODE_LBL,
        IDC_HOLD_TIME, IDC_HOLD_TIME_LBL, IDC_ACCELERATION, IDC_ACCELERATION_LBL,

        IDC_BARS,
            IDC_LED_MODE,
    };

    static const int Page6[] =
    {
        IDC_STYLES,

        IDC_COLOR_SOURCE_LBL, IDC_COLOR_SOURCE,
        IDC_COLOR_INDEX_LBL, IDC_COLOR_INDEX,
        IDC_COLOR_BUTTON_LBL, IDC_COLOR_BUTTON,

        IDC_OPACITY_LBL, IDC_OPACITY, IDC_OPACITY_SPIN, IDC_OPACITY_UNIT,
        IDC_THICKNESS_LBL, IDC_THICKNESS, IDC_THICKNESS_SPIN,

        IDC_COLOR_SCHEME_LBL, IDC_COLOR_SCHEME,
        IDC_GRADIENT, IDC_COLOR_LIST, IDC_ADD, IDC_REMOVE, IDC_REVERSE,
        IDC_POSITION, IDC_POSITION_LBL, IDC_SPREAD,
        IDC_HORIZONTAL_GRADIENT,
    };

    int Mode = (index == 0) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page1); ++i)
    {
        auto w = GetDlgItem(Page1[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }

    Mode = (index == 1) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page2); ++i)
    {
        auto w = GetDlgItem(Page2[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }

    Mode = (index == 2) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page3); ++i)
    {
        auto w = GetDlgItem(Page3[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }

    Mode = (index == 3) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page4); ++i)
    {
        auto w = GetDlgItem(Page4[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }

    Mode = (index == 4) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page5); ++i)
    {
        auto w = GetDlgItem(Page5[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }

    Mode = (index == 5) ? SW_SHOW : SW_HIDE;

    for (size_t i = 0; i < _countof(Page6); ++i)
    {
        auto w = GetDlgItem(Page6[i]);

        if (w.IsWindow())
            w.ShowWindow(Mode);
    }
}

/// <summary>
/// Enables or disables controls based on the selection of the user.
/// </summary>
void ConfigurationDialog::UpdateControls()
{
    const bool IsFFT = (_State->_Transform == Transform::FFT);
    const bool IsSWIFT = (_State->_Transform == Transform::SWIFT);

    // Transform
    bool HasParameter = (_State->_WindowFunction == WindowFunctions::PowerOfSine)
                     || (_State->_WindowFunction == WindowFunctions::PowerOfCircle)
                     || (_State->_WindowFunction == WindowFunctions::Gauss)
                     || (_State->_WindowFunction == WindowFunctions::Tukey)
                     || (_State->_WindowFunction == WindowFunctions::Kaiser)
                     || (_State->_WindowFunction == WindowFunctions::Poison)
                     || (_State->_WindowFunction == WindowFunctions::HyperbolicSecant);

    GetDlgItem(IDC_WINDOW_FUNCTION).EnableWindow(!IsSWIFT);
    GetDlgItem(IDC_WINDOW_PARAMETER).EnableWindow(HasParameter && !IsSWIFT);
    GetDlgItem(IDC_WINDOW_SKEW).EnableWindow(!IsSWIFT);

    // FFT
    for (const auto & Iter : { IDC_SUMMATION_METHOD, IDC_MAPPING_METHOD, IDC_SMOOTH_LOWER_FREQUENCIES, IDC_SMOOTH_GAIN_TRANSITION, IDC_KERNEL_SIZE })
        GetDlgItem(Iter).EnableWindow(IsFFT);

    for (const auto & Iter : { IDC_NUM_BINS, IDC_DISTRIBUTION })
        GetDlgItem(Iter).EnableWindow(IsFFT || IsSWIFT);

    const bool NotFixed = (_State->_FFTMode == FFTMode::FFTCustom) || (_State->_FFTMode == FFTMode::FFTDuration);

        GetDlgItem(IDC_NUM_BINS_PARAMETER).EnableWindow((IsFFT || IsSWIFT) && NotFixed);

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

    // Brown-Puckette CQT
    const bool IsBrownPuckette = (_State->_MappingMethod == Mapping::BrownPuckette) && IsFFT;

        for (const auto & Iter : { IDC_BW_OFFSET, IDC_BW_CAP, IDC_BW_AMOUNT, IDC_GRANULAR_BW, IDC_KERNEL_SHAPE, IDC_KERNEL_ASYMMETRY, })
            GetDlgItem(Iter).EnableWindow(IsBrownPuckette);

    GetDlgItem(IDC_KERNEL_SHAPE_PARAMETER).EnableWindow(IsBrownPuckette && HasParameter);

    // SWIFT
    for (const auto & Iter : { IDC_FBO, IDC_TR, IDC_BW_SWIFT, })
        GetDlgItem(Iter).EnableWindow(IsSWIFT);

    // Frequencies
    const bool IsOctaves = (_State->_FrequencyDistribution == FrequencyDistribution::Octaves);
    const bool IsAveePlayer = (_State->_FrequencyDistribution == FrequencyDistribution::AveePlayer);

        GetDlgItem(IDC_NUM_BANDS).EnableWindow(IsFFT && !IsOctaves);
        GetDlgItem(IDC_LO_FREQUENCY).EnableWindow(IsFFT && !IsOctaves);
        GetDlgItem(IDC_HI_FREQUENCY).EnableWindow(IsFFT && !IsOctaves);
        GetDlgItem(IDC_SCALING_FUNCTION).EnableWindow(IsFFT && !IsOctaves && !IsAveePlayer);

        GetDlgItem(IDC_SKEW_FACTOR).EnableWindow(!IsOctaves);

        for (const auto & Iter : { IDC_MIN_NOTE, IDC_MAX_NOTE, IDC_BANDS_PER_OCTAVE, IDC_PITCH, IDC_TRANSPOSE, })
            GetDlgItem(Iter).EnableWindow(IsOctaves);

    // Filters
    const bool HasFilter = (_State->_WeightingType != WeightingType::None);

        for (const auto & Iter : { IDC_SLOPE_FN_OFFS, IDC_SLOPE_FN_OFFS, IDC_SLOPE, IDC_SLOPE_OFFS, IDC_EQ_AMT, IDC_EQ_OFFS, IDC_EQ_DEPTH, IDC_WT_AMT })
            GetDlgItem(Iter).EnableWindow(HasFilter);

    // Background Mode
    const bool UseArtworkForBackground = (_State->_BackgroundMode == BackgroundMode::Artwork);

        GetDlgItem(IDC_ARTWORK_OPACITY).EnableWindow(UseArtworkForBackground);
        GetDlgItem(IDC_FILE_PATH).EnableWindow(UseArtworkForBackground);

    // X axis
    GetDlgItem(IDC_X_AXIS_TOP).EnableWindow(_State->_XAxisMode != XAxisMode::None);
    GetDlgItem(IDC_X_AXIS_BOTTOM).EnableWindow(_State->_XAxisMode != XAxisMode::None);

    // Y axis
    GetDlgItem(IDC_Y_AXIS_LEFT).EnableWindow(_State->_YAxisMode != YAxisMode::None);
    GetDlgItem(IDC_Y_AXIS_RIGHT).EnableWindow(_State->_YAxisMode != YAxisMode::None);

    const bool IsLinear = (_State->_YAxisMode == YAxisMode::Linear);

        for (const auto & Iter : { IDC_USE_ABSOLUTE, IDC_GAMMA })
            GetDlgItem(Iter).EnableWindow(IsLinear);

    // Visualization
    const bool ShowPeaks = (_State->_PeakMode != PeakMode::None);

        for (const auto & Iter : { IDC_HOLD_TIME, IDC_ACCELERATION })
            GetDlgItem(Iter).EnableWindow(ShowPeaks);
 
    // Bars
    const bool IsBars = _State->_VisualizationType == VisualizationType::Bars;

        for (const auto & Iter : { IDC_LED_MODE })
            GetDlgItem(Iter).EnableWindow(IsBars);
}

/// <summary>
/// Gets the selected Windows color.
/// </summary>
static D2D1_COLOR_F GetWindowsColor(uint32_t index) noexcept
{
    static const int ColorIndex[] =
    {
        COLOR_WINDOW,           // Window Background
        COLOR_WINDOWTEXT,       // Window Text
        COLOR_BTNFACE,          // Button Background
        COLOR_BTNTEXT,          // Button Text
        COLOR_HIGHLIGHT,        // Highlight Background
        COLOR_HIGHLIGHTTEXT,    // Highlight Text
        COLOR_GRAYTEXT,         // Gray Text
        COLOR_HOTLIGHT,         // Hot Light
    };

    return D2D1::ColorF(::GetSysColor(ColorIndex[Clamp(index, 0U, (uint32_t) _countof(ColorIndex) - 1)]));
}

/// <summary>
/// Gets the selected DUI color.
/// </summary>
static D2D1_COLOR_F GetDUIColor(uint32_t index) noexcept
{
    static const int ColorIndex[] =
    {
        COLOR_WINDOW,           // Window Background
        COLOR_WINDOWTEXT,       // Window Text
        COLOR_BTNFACE,          // Button Background
        COLOR_BTNTEXT,          // Button Text
        COLOR_HIGHLIGHT,        // Highlight Background
        COLOR_HIGHLIGHTTEXT,    // Highlight Text
        COLOR_GRAYTEXT,         // Gray Text
        COLOR_HOTLIGHT,         // Hot Light
    };

    return D2D1::ColorF(::GetSysColor(ColorIndex[Clamp(index, 0U, (uint32_t) _countof(ColorIndex) - 1)]));
}

/// <summary>
/// Updates the style controls with the current configuration.
/// </summary>
void ConfigurationDialog::UpdateStyleControls()
{
    int StyleIndex = ((CListBox) GetDlgItem(IDC_STYLES)).GetCurSel();

    Style * style = _State->_StyleManager.GetStyle((VisualElement) StyleIndex);

    // Update the controls based on the color source.
    switch (style->_ColorSource)
    {
        case ColorSource::None:

        case ColorSource::Solid:
        {
            style->_Color = style->_CustomColor;
            break;
        }

        case ColorSource::DominantColor:
        {
            style->_Color = _State->_DominantColor;
            break;
        }

        case ColorSource::Gradient:
        {
            if (style->_ColorScheme == ColorScheme::Custom)
                style->_GradientStops = style->_CustomGradientStops;
            else
            if (style->_ColorScheme == ColorScheme::Artwork)
                style->_GradientStops = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : GetGradientStops(ColorScheme::Artwork);
            else
                style->_GradientStops = GetGradientStops(style->_ColorScheme);
            break;
        }

        case ColorSource::Windows:
        {
            auto w = (CComboBox) GetDlgItem(IDC_COLOR_INDEX);

            w.ResetContent();

            for (const auto & x : { L"Window Background", L"Window Text", L"Button Background", L"Button Text", L"Highlight Background", L"Highlight Text", L"Gray Text", L"Hot Light" })
                w.AddString(x);

            w.SetCurSel((int) Clamp(style->_ColorIndex, 0U, (uint32_t) (w.GetCount() - 1)));

            style->_Color = GetWindowsColor(style->_ColorIndex);
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

                style->_Color = GetDUIColor(style->_ColorIndex);
            }
            else
            {
                for (const auto & x : { L"Text", L"Selected Text", L"Inactive Selected Text", L"Background", L"Selected Background", L"Inactive Selected Background", L"Active Item" })
                    w.AddString(x);
            }

            w.SetCurSel((int) Clamp(style->_ColorIndex, 0U, (uint32_t) (w.GetCount() - 1)));

            style->_Color = _State->_UserInterfaceColors[Clamp((size_t) style->_ColorIndex, (size_t) 0, _State->_UserInterfaceColors.size())];
            break;
        }
    }

    ((CComboBox) GetDlgItem(IDC_COLOR_SOURCE)).SetCurSel((int) style->_ColorSource);

    SendDlgItemMessageW(IDC_HORIZONTAL_GRADIENT, BM_SETCHECK, style->_Flags & Style::HorizontalGradient);

    SetInteger(IDC_OPACITY, (int64_t) (style->_Opacity * 100.f));
    ((CUpDownCtrl) GetDlgItem(IDC_OPACITY_SPIN)).SetPos32((int) (style->_Opacity * 100.f));

    SetDouble(IDC_THICKNESS, style->_Thickness, 0, 1);
    ((CUpDownCtrl) GetDlgItem(IDC_THICKNESS_SPIN)).SetPos32((int) (style->_Thickness * 10.f));

    GetDlgItem(IDC_COLOR_INDEX).EnableWindow((style->_ColorSource == ColorSource::Windows) || (style->_ColorSource == ColorSource::UserInterface));
    GetDlgItem(IDC_COLOR_BUTTON).EnableWindow((style->_ColorSource == ColorSource::Solid) || (style->_ColorSource == ColorSource::DominantColor) || (style->_ColorSource == ColorSource::Windows) || (style->_ColorSource == ColorSource::UserInterface));
    GetDlgItem(IDC_COLOR_SCHEME).EnableWindow(style->_ColorSource == ColorSource::Gradient);
    GetDlgItem(IDC_HORIZONTAL_GRADIENT).EnableWindow(style->_ColorSource == ColorSource::Gradient);
    GetDlgItem(IDC_OPACITY).EnableWindow((style->_ColorSource != ColorSource::None) && (style->_Flags & Style::SupportsOpacity));
    GetDlgItem(IDC_THICKNESS).EnableWindow((style->_ColorSource != ColorSource::None) && (style->_Flags & Style::SupportsThickness));

    UpdateColorSchemeControls();
}

/// <summary>
/// Updates the color controls with the current configuration.
/// </summary>
void ConfigurationDialog::UpdateColorSchemeControls()
{
    int StyleIndex = ((CListBox) GetDlgItem(IDC_STYLES)).GetCurSel();

    Style * style = _State->_StyleManager.GetStyle((VisualElement) StyleIndex);

    // Update the color button.
    _Color.SetColor(style->_Color);

    ((CComboBox) GetDlgItem(IDC_COLOR_SCHEME)).SetCurSel((int) style->_ColorScheme);

    // Get the gradient stops.
    GradientStops gs;

    if (style->_ColorScheme == ColorScheme::Custom)
        gs = style->_CustomGradientStops;
    else
    if (style->_ColorScheme == ColorScheme::Artwork)
        gs = !_State->_ArtworkGradientStops.empty() ? _State->_ArtworkGradientStops : GetGradientStops(ColorScheme::Artwork);
    else
        gs = GetGradientStops(style->_ColorScheme);

    // Update the gradient control.
    _Gradient.SetGradientStops(gs);

    // Update the list of colors.
    {
        int Index = _Colors.GetCurSel();

        std::vector<D2D1_COLOR_F> Colors;

        for (const auto & Iter : gs)
            Colors.push_back(Iter.color);

        _Colors.SetColors(Colors);

        _Colors.SetCurSel(Index);
    }

    auto lb = (CListBox) GetDlgItem(IDC_COLOR_LIST);

    bool HasSelection = (lb.GetCurSel() != LB_ERR);                     // Add and Remove are only enabled when a color is selected.
    bool HasMoreThanOneColor = (gs.size() > 1);                         // Remove and Reverse are only enabled when there is more than 1 color.
    bool UseArtwork = (style->_ColorScheme == ColorScheme::Artwork);    // Gradient controls are disabled when the artwork provides the colors.

    GetDlgItem(IDC_ADD).EnableWindow(HasSelection && !UseArtwork);
    GetDlgItem(IDC_REMOVE).EnableWindow(HasSelection && HasMoreThanOneColor && !UseArtwork);

    GetDlgItem(IDC_REVERSE).EnableWindow(HasMoreThanOneColor && !UseArtwork);

    GetDlgItem(IDC_POSITION).EnableWindow(HasSelection && !UseArtwork);
    GetDlgItem(IDC_SPREAD).EnableWindow(HasSelection && !UseArtwork);
}

/// <summary>
/// Updates the position of the current gradient colors.
/// </summary>
void ConfigurationDialog::UpdateGradientStopPositons(Style * style)
{
    if (style->_GradientStops.size() == 0)
        return;

    if (style->_GradientStops.size() == 1)
        style->_GradientStops[0].position = 1.f;
    else
    {
        FLOAT Position = 0.f;

        for (auto & Iter : style->_GradientStops)
        {
            Iter.position = Position / (FLOAT) (style->_GradientStops.size() - 1);
            Position++;
        }
    }

    // Save the current result as custom gradient stops.
    style->_ColorScheme = ColorScheme::Custom;
    style->_CustomGradientStops = style->_GradientStops;
}

/// <summary>
/// Updates the Channes menu with the current configuration.
/// </summary>
void ConfigurationDialog::UpdateChannelsMenu()
{
    CMenuHandle & Menu = _Channels.GetMenu();

    uint32_t SelectedChannels = _State->_SelectedChannels;

    for (UINT i = IDM_CHANNELS_FIRST; i < IDM_CHANNELS_LAST; ++i)
    {
        Menu.CheckMenuItem(i, (UINT) (MF_BYCOMMAND | ((SelectedChannels & 1) ? MF_CHECKED : 0)));

        SelectedChannels >>= 1;
    }

    Menu.CheckMenuItem(IDM_CHANNELS_LAST, (UINT) (MF_BYCOMMAND | ((_State->_SelectedChannels == AllChannels) ? MF_CHECKED : 0)));
}

/// <summary>
/// Validates and updates the notification from the up-down control.
/// </summary>
int ConfigurationDialog::ClampNewSpinPosition(LPNMUPDOWN nmud, int minValue, int maxValue) noexcept
{
    if ((nmud->iPos + nmud->iDelta) < minValue)
    {
        nmud->iPos   = minValue;
        nmud->iDelta = 0;
    }
    else
    if ((nmud->iPos + nmud->iDelta) > maxValue)
    {
        nmud->iPos   = maxValue;
        nmud->iDelta = 0;
    }

    return nmud->iPos + nmud->iDelta;
}

/// <summary>
/// Validates and updates the notification from the up-down control.
/// </summary>
double ConfigurationDialog::ClampNewSpinPosition(LPNMUPDOWN nmud, double minValue, double maxValue, double scaleFactor) noexcept
{
    if ((nmud->iPos + nmud->iDelta) / scaleFactor < minValue)
    {
        nmud->iPos   = (int) (minValue * scaleFactor);
        nmud->iDelta = 0;
    }
    else
    if ((nmud->iPos + nmud->iDelta) / scaleFactor > maxValue)
    {
        nmud->iPos   = (int) (maxValue * scaleFactor);
        nmud->iDelta = 0;
    }

    return (double) (nmud->iPos + nmud->iDelta) / scaleFactor;
}

/// <summary>
/// Sets the display version of an integer number.
/// </summary>
void ConfigurationDialog::SetInteger(int id, int64_t value) noexcept
{
    SetDlgItemTextW(id, pfc::wideFromUTF8(pfc::format_int(value)));
}

/// <summary>
/// Sets the display version of a real number.
/// </summary>
void ConfigurationDialog::SetDouble(int id, double value, unsigned width, unsigned precision) noexcept
{
    SetDlgItemTextW(id, pfc::wideFromUTF8(pfc::format_float(value, width, precision)));
}

/// <summary>
/// Sets the display version of the note number.
/// </summary>
void ConfigurationDialog::SetNote(int id, uint32_t noteNumber) noexcept
{
    static const WCHAR * Notes[] = { L"C%d", L"C#%d", L"D%d", L"D#%d", L"E%d", L"F%d", L"F#%d", L"G%d", L"G#%d", L"A%d", L"A#%d", L"B%d" };

    WCHAR Text[16] = { };

    size_t NoteIndex = (size_t) (noteNumber % 12);
    int Octave = (int) (noteNumber / 12);

    ::StringCchPrintfW(Text, _countof(Text), Notes[NoteIndex], Octave);

    SetDlgItemTextW(id, Text);
}
