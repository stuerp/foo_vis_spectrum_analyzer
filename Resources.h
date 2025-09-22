
/** $VER: Resources.h (2025.09.22) P. Stuer **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#define NUM_FILE_MAJOR          0
#define NUM_FILE_MINOR          8
#define NUM_FILE_PATCH          0
#define NUM_FILE_PRERELEASE     0

#define NUM_PRODUCT_MAJOR       0
#define NUM_PRODUCT_MINOR       8
#define NUM_PRODUCT_PATCH       0
#define NUM_PRODUCT_PRERELEASE  0

#define STR_RELEASE_TAG         ""

/** Component specific **/

#define STR_COMPONENT_NAME          "Spectrum Analyzer"
#define STR_COMPONENT_VERSION       TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE) STR_RELEASE_TAG
#define STR_COMPONENT_BASENAME      "foo_vis_spectrum_analyzer"
#define STR_COMPONENT_FILENAME      STR_COMPONENT_BASENAME ".dll"
#define STR_COMPONENT_COMPANY_NAME  ""
#define STR_COMPONENT_COPYRIGHT     "Copyright (c) 2023-2025 P. Stuer. All rights reserved."
#define STR_COMPONENT_COMMENTS      ""
#define STR_COMPONENT_DESCRIPTION   "A spectrum analyzer for foobar2000"
#define STR_COMPONENT_COMMENT       "\n" \
                                    "Based on:\n" \
                                    "- The Audio Spectrum project (https://codepen.io/TF3RDL/pen/poQJwRW)\n" \
                                    "- SWIFT, Sliding Windowed Infinite Fourier Transform (https://codepen.io/TF3RDL/pen/JjBzjeY)\n" \
                                    "- Analog-style spectrum analyzer and sliding DFT visualization using AudioWorklet (https://codepen.io/TF3RDL/pen/MWLzPoO)"
#define STR_COMPONENT_URL           "https://github.com/stuerp/" STR_COMPONENT_BASENAME


/** Generic **/

#define STR_COMPANY_NAME        TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT           TEXT(STR_COMPONENT_COPYRIGHT)

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TEXT(STR_COMPONENT_VERSION)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define STR_PRODUCT_NAME        STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TEXT(TOSTRING(NUM_PRODUCT_MAJOR)) TEXT(".") TEXT(TOSTRING(NUM_PRODUCT_MINOR)) TEXT(".") TEXT(TOSTRING(NUM_PRODUCT_PATCH)) TEXT(".") TEXT(TOSTRING(NUM_PRODUCT_PRERELEASE)) TEXT(STR_RELEASE_TAG)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT(STR_COMPONENT_URL)
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Window **/

#define GUID_UI_ELEMENT         {0x3247c894,0xe585,0x4025,{0xa8,0x66,0xc7,0xd4,0x93,0x3f,0xb2,0xe3}} // {3247c894-e585-4025-a866-c7d4933fb2e3}
#define STR_WINDOW_CLASS_NAME   "{08e851a2-ec49-467e-a336-775d79ee26de}"

#define UM_CONFIGURATION_CHANGED        WM_USER + 1

#define CC_PRESET_LOADED                1       // The user has loaded a preset from the context menu.
#define CC_COLORS                       2       // The colors have changed, either by the render thread or by the user in the main foobar2000 preference dialog.

/** State **/

#define IDD_CONFIGURATION               1000

// Menu List

#define IDC_MENULIST                    1500

#pragma region Transform

#define IDC_TRANSFORM_GROUP             2000

#define IDC_METHOD_LBL                  2010
#define IDC_METHOD                      2012

#define IDC_WINDOW_FUNCTION_LBL         2020
#define IDC_WINDOW_FUNCTION             2022
#define IDC_WINDOW_PARAMETER_LBL        2024
#define IDC_WINDOW_PARAMETER            2026
#define IDC_WINDOW_SKEW_LBL             2028
#define IDC_WINDOW_SKEW                 2030

#define IDC_REACTION_ALIGNMENT_LBL      2032
#define IDC_REACTION_ALIGNMENT          2034

#pragma endregion

#pragma region FFT

#define IDC_FFT_GROUP                   2040

#define IDC_NUM_BINS_LBL                2050
#define IDC_NUM_BINS                    2052
#define IDC_NUM_BINS_PARAMETER_NAME     2054
#define IDC_NUM_BINS_PARAMETER          2056
#define IDC_NUM_BINS_PARAMETER_UNIT     2058

#define IDC_SUMMATION_METHOD_LBL        2060
#define IDC_SUMMATION_METHOD            2062
#define IDC_MAPPING_METHOD_LBL          2064
#define IDC_MAPPING_METHOD              2066

#define IDC_SMOOTH_LOWER_FREQUENCIES    2070
#define IDC_SMOOTH_GAIN_TRANSITION      2072

#define IDC_KERNEL_SIZE_LBL             2080
#define IDC_KERNEL_SIZE                 2082
#define IDC_KERNEL_SIZE_SPIN            2084

#pragma endregion

#pragma region Brown-Puckette CQT

#define IDC_BP_GROUP                    2300

#define IDC_BW_OFFSET_LBL               2310
#define IDC_BW_OFFSET                   2312
#define IDC_BW_CAP_LBL                  2314
#define IDC_BW_CAP                      2316
#define IDC_BW_AMOUNT_LBL               2318
#define IDC_BW_AMOUNT                   2320
#define IDC_GRANULAR_BW                 2322

#define IDC_KERNEL_SHAPE_LBL            2326
#define IDC_KERNEL_SHAPE                2328
#define IDC_KERNEL_SHAPE_PARAMETER_LBL  2330
#define IDC_KERNEL_SHAPE_PARAMETER      2332
#define IDC_KERNEL_ASYMMETRY_LBL        2334
#define IDC_KERNEL_ASYMMETRY            2336

#pragma endregion

#pragma region IIR

#define IDC_IIR_GROUP                   2400

#define IDC_FBO_LBL                     2402
#define IDC_FBO                         2404
#define IDC_TR_LBL                      2406
#define IDC_TR                          2408
#define IDC_IIR_BW_LBL                  2410
#define IDC_IIR_BW                      2412
#define IDC_CONSTANT_Q                  2414
#define IDC_COMPENSATE_BW               2416
#define IDC_PREWARPED_Q                 2418

#pragma endregion

#pragma region Frequencies

#define IDC_FREQUENCIES_GROUP           2100

#define IDC_DISTRIBUTION_LBL            2110
#define IDC_DISTRIBUTION                2112

#define IDC_NUM_BANDS_LBL               2120
#define IDC_NUM_BANDS                   2122
#define IDC_NUM_BANDS_SPIN              2124

#define IDC_RANGE_LBL_1                 2130
#define IDC_LO_FREQUENCY                2132
#define IDC_LO_FREQUENCY_SPIN           2134
#define IDC_RANGE_LBL_2                 2136
#define IDC_HI_FREQUENCY                2138
#define IDC_HI_FREQUENCY_SPIN           2140
#define IDC_RANGE_LBL_3                 2142

#define IDC_MIN_NOTE_LBL                2150
#define IDC_MIN_NOTE                    2152
#define IDC_MIN_NOTE_SPIN               2154
#define IDC_MAX_NOTE_LBL                2156
#define IDC_MAX_NOTE                    2158
#define IDC_MAX_NOTE_SPIN               2160

#define IDC_BANDS_PER_OCTAVE_LBL        2170
#define IDC_BANDS_PER_OCTAVE            2172
#define IDC_BANDS_PER_OCTAVE_SPIN       2174

#define IDC_PITCH_LBL_1                 2180
#define IDC_PITCH                       2182
#define IDC_PITCH_SPIN                  2184
#define IDC_PITCH_LBL_2                 2186

#define IDC_TRANSPOSE_LBL               2200
#define IDC_TRANSPOSE                   2202
#define IDC_TRANSPOSE_SPIN              2204

#define IDC_SCALING_FUNCTION_LBL        2210
#define IDC_SCALING_FUNCTION            2212

#define IDC_SKEW_FACTOR_LBL             2220
#define IDC_SKEW_FACTOR                 2222
#define IDC_SKEW_FACTOR_SPIN            2224

#define IDC_BANDWIDTH_LBL               2230
#define IDC_BANDWIDTH                   2232
#define IDC_BANDWIDTH_SPIN              2234

#pragma endregion

#pragma region Filters

#define IDC_FILTERS_GROUP               2500

#define IDC_ACOUSTIC_FILTER_LBL         2510
#define IDC_ACOUSTIC_FILTER             2512

#define IDC_SLOPE_FN_OFFS_LBL           2520
#define IDC_SLOPE_FN_OFFS               2522
#define IDC_SLOPE_FN_OFFS_SPIN          2524

#define IDC_SLOPE_LBL                   2530
#define IDC_SLOPE                       2532
#define IDC_SLOPE_SPIN                  2534
#define IDC_SLOPE_UNIT                  2536

#define IDC_SLOPE_OFFS_LBL              2540
#define IDC_SLOPE_OFFS                  2542
#define IDC_SLOPE_OFFS_SPIN             2544
#define IDC_SLOPE_OFFS_UNIT             2546

#define IDC_EQ_AMT_LBL                  2550
#define IDC_EQ_AMT                      2552
#define IDC_EQ_AMT_SPIN                 2554

#define IDC_EQ_OFFS_LBL                 2560
#define IDC_EQ_OFFS                     2562
#define IDC_EQ_OFFS_SPIN                2564
#define IDC_EQ_OFFS_UNIT                2566

#define IDC_EQ_DEPTH_LBL                2570
#define IDC_EQ_DEPTH                    2572
#define IDC_EQ_DEPTH_SPIN               2574
#define IDC_EQ_DEPTH_UNIT               2576

#define IDC_WT_AMT_LBL                  2580
#define IDC_WT_AMT                      2582
#define IDC_WT_AMT_SPIN                 2584

#pragma endregion

/** Common **/

#pragma region Common

#define IDC_COMMON                      6000

#define IDC_SMOOTHING_METHOD_LBL        6050
#define IDC_SMOOTHING_METHOD            6052

#define IDC_SMOOTHING_FACTOR_LBL        6060
#define IDC_SMOOTHING_FACTOR            6062

#define IDC_SHOW_TOOLTIPS               6070
#define IDC_SUPPRESS_MIRROR_IMAGE       6072

#pragma endregion

#pragma region Artwork

#define IDC_ARTWORK                     6078

#define IDC_ARTWORK_BACKGROUND          6082

#define IDC_ARTWORK_TYPE_LBL            6124
#define IDC_ARTWORK_TYPE                6126

#define IDC_FIT_MODE_LBL                6084
#define IDC_FIT_MODE                    6086
#define IDC_FIT_WINDOW                  6088

#define IDC_ARTWORK_OPACITY_LBL         6090
#define IDC_ARTWORK_OPACITY             6092
#define IDC_ARTWORK_OPACITY_SPIN        6094
#define IDC_ARTWORK_OPACITY_LBL_2       6096

#define IDC_NUM_ARTWORK_COLORS_LBL      6100
#define IDC_NUM_ARTWORK_COLORS          6102
#define IDC_NUM_ARTWORK_COLORS_SPIN     6104

#define IDC_LIGHTNESS_THRESHOLD_LBL     6106
#define IDC_LIGHTNESS_THRESHOLD         6108
#define IDC_LIGHTNESS_THRESHOLD_SPIN    6110
#define IDC_LIGHTNESS_THRESHOLD_LBL_2   6112

#define IDC_COLOR_ORDER_LBL             6114
#define IDC_COLOR_ORDER                 6116

#define IDC_FILE_PATH_LBL               6120
#define IDC_FILE_PATH                   6122

#pragma endregion

#define IDC_COMPONENT                   6200

#define IDC_LOG_LEVEL_LBL               6202
#define IDC_LOG_LEVEL                   6204

/** Graphs **/

#pragma region Graphs

#define IDC_GRAPH_SETTINGS              5100

#define IDC_ADD_GRAPH                   5102
#define IDC_REMOVE_GRAPH                5104

#define IDC_VERTICAL_LAYOUT             5106

#define IDC_GRAPH_DESCRIPTION_LBL       5108
#define IDC_GRAPH_DESCRIPTION           5109

#define IDC_LAYOUT                      5110

#define IDC_HORIZONTAL_ALIGNMENT_LBL    5112
#define IDC_HORIZONTAL_ALIGNMENT        5113

#define IDC_VERTICAL_ALIGNMENT_LBL      5114
#define IDC_VERTICAL_ALIGNMENT          5115

#define IDC_FLIP_HORIZONTALLY           5116
#define IDC_FLIP_VERTICALLY             5118

#define IDC_CHANNELS                    5120

#pragma endregion

#pragma region X axis

#define IDC_X_AXIS                      4000

#define IDC_X_AXIS_MODE_LBL             4002
#define IDC_X_AXIS_MODE                 4004

#define IDC_X_AXIS_TOP                  4006
#define IDC_X_AXIS_BOTTOM               4008

#pragma endregion

#pragma region Y axis

#define IDC_Y_AXIS                      5000

#define IDC_Y_AXIS_MODE_LBL             5002
#define IDC_Y_AXIS_MODE                 5004

#define IDC_Y_AXIS_LEFT                 5006
#define IDC_Y_AXIS_RIGHT                5008

#define IDC_AMPLITUDE_LBL_1             5010
#define IDC_AMPLITUDE_LO                5012
#define IDC_AMPLITUDE_LO_SPIN           5014
#define IDC_AMPLITUDE_LBL_2             5016
#define IDC_AMPLITUDE_HI                5018
#define IDC_AMPLITUDE_HI_SPIN           5020
#define IDC_AMPLITUDE_LBL_3             5022

#define IDC_AMPLITUDE_STEP_LBL        5030
#define IDC_AMPLITUDE_STEP              5032
#define IDC_AMPLITUDE_STEP_SPIN         5034
#define IDC_AMPLITUDE_STEP_UNIT        5036

#define IDC_USE_ABSOLUTE                5040

#define IDC_GAMMA_LBL                   5050
#define IDC_GAMMA                       5052

#pragma endregion

/** Visualization **/

#pragma region Visualization

#define IDC_VISUALIZATION_LBL           7100
#define IDC_VISUALIZATION               7102

// Peak Indicators
#define IDC_PEAK_INDICATORS             7104

#define IDC_PEAK_MODE_LBL               7108
#define IDC_PEAK_MODE                   7110

#define IDC_HOLD_TIME_LBL               7112
#define IDC_HOLD_TIME                   7114

#define IDC_ACCELERATION_LBL            7116
#define IDC_ACCELERATION                7118

// LEDs

#define IDC_LEDS                        7120

#define IDC_LED_MODE                    7122
#define IDC_LED_SIZE_LBL                7124
#define IDC_LED_SIZE                    7126
#define IDC_LED_GAP_LBL                 7128
#define IDC_LED_GAP                     7130

// Bars

#define IDC_BARS                        7140

// Spectogram

#define IDC_SPECTOGRAM                  7150

#define IDC_SCROLLING_SPECTOGRAM        7152
#define IDC_HORIZONTAL_SPECTOGRAM       7154
#define IDC_SPECTRUM_BAR_METRICS        7156

// Radial Bars

#define IDC_RADIAL_BARS                 7190

#define IDC_INNER_RADIUS_LBL            7192
#define IDC_INNER_RADIUS                7193

#define IDC_OUTER_RADIUS_LBL            7194
#define IDC_OUTER_RADIUS                7195

#define IDC_ANGULAR_VELOCITY_LBL        7196
#define IDC_ANGULAR_VELOCITY            7197

// Peak Meter

#define IDC_PEAK_METER                  7160

#define IDC_HORIZONTAL_PEAK_METER       7162
#define IDC_RMS_PLUS_3                  7164

#define IDC_RMS_WINDOW_LBL              7166
#define IDC_RMS_WINDOW                  7167
#define IDC_RMS_WINDOW_SPIN             7168
#define IDC_RMS_WINDOW_UNIT             7169

#define IDC_GAUGE_GAP_LBL               7170
#define IDC_GAUGE_GAP                   7171

// Level Meter

#define IDC_LEVEL_METER                 7180

#define IDC_CHANNEL_PAIRS_LBL           7182
#define IDC_CHANNEL_PAIRS               7183

#define IDC_HORIZONTAL_LEVEL_METER      7184

#pragma endregion

#pragma region Styles

#define IDC_STYLES                      7500

#define IDC_COLOR_SOURCE_LBL            7502
#define IDC_COLOR_SOURCE                7504

#define IDC_COLOR_INDEX_LBL             7506
#define IDC_COLOR_INDEX                 7508

#define IDC_COLOR_BUTTON_LBL            7510
#define IDC_COLOR_BUTTON                7512

#define IDC_OPACITY_LBL                 7514
#define IDC_OPACITY                     7516
#define IDC_OPACITY_SPIN                7518
#define IDC_OPACITY_UNIT                7520

#define IDC_THICKNESS_LBL               7522
#define IDC_THICKNESS                   7524
#define IDC_THICKNESS_SPIN              7526

#define IDC_COLOR_SCHEME_LBL            7530
#define IDC_COLOR_SCHEME                7532

#define IDC_GRADIENT                    7534
#define IDC_COLOR_LIST                  7536
#define IDC_ADD                         7538
#define IDC_REMOVE                      7540
#define IDC_REVERSE                     7542

#define IDC_POSITION                    7544
#define IDC_POSITION_LBL                7546
#define IDC_SPREAD                      7548

#define IDC_HORIZONTAL_GRADIENT         7550
#define IDC_AMPLITUDE_BASED             7552

#define IDC_FONT_NAME_LBL               7554
#define IDC_FONT_NAME                   7556
#define IDC_FONT_NAME_SELECT            7558

#define IDC_FONT_SIZE_LBL               7560
#define IDC_FONT_SIZE                   7562

#pragma endregion

#pragma region Presets

#define IDC_PRESETS_LBL                 7600
#define IDC_PRESETS_ROOT                7602
#define IDC_PRESETS_ROOT_SELECT         7604
#define IDC_PRESET_NAMES                7606

#define IDC_PRESET_NAME_LBL             7608
#define IDC_PRESET_NAME                 7610

#define IDC_PRESET_LOAD                 7612
#define IDC_PRESET_SAVE                 7614
#define IDC_PRESET_DELETE               7616

#pragma endregion

#define IDC_RESET                       9999

/** Color Dialog **/

#define IDD_CHOOSECOLOR                 1020

#define IDC_ALPHA_SLIDER                 800
#define IDC_ALPHA_VALUE                  801
