
/** $VER: Resources.h (2024.01.01) P. Stuer **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

/** Component specific **/

#define STR_COMPONENT_NAME          "Spectrum Analyzer"
#define STR_COMPONENT_VERSION       TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE)
#define STR_COMPONENT_BASENAME      "foo_vis_spectrum_analyzer"
#define STR_COMPONENT_FILENAME      STR_COMPONENT_BASENAME ".dll"
#define STR_COMPONENT_COMPANY_NAME  ""
#define STR_COMPONENT_COPYRIGHT     "Copyright (c) 2023-2024 P. Stuer. All rights reserved."
#define STR_COMPONENT_COMMENTS      ""
#define STR_COMPONENT_DESCRIPTION   "A spectrum analyzer for foobar2000"
#define STR_COMPONENT_COMMENT       "Based on the Audio Spectrum project (https://codepen.io/TF3RDL/pen/poQJwRW)"

/** Generic **/

#define STR_COMPANY_NAME        TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT           TEXT(STR_COMPONENT_COPYRIGHT)

#define NUM_FILE_MAJOR          0
#define NUM_FILE_MINOR          6
#define NUM_FILE_PATCH          0
#define NUM_FILE_PRERELEASE     1

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR       0
#define NUM_PRODUCT_MINOR       6
#define NUM_PRODUCT_PATCH       0
#define NUM_PRODUCT_PRERELEASE  1

#define STR_PRODUCT_NAME        STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/") STR_COMPONENT_BASENAME
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Window **/

#define GUID_UI_ELEMENT_SPECTRUM_ANALYZER   {0x3247c894,0xe585,0x4025,{0xa8,0x66,0xc7,0xd4,0x93,0x3f,0xb2,0xe3}} // {3247c894-e585-4025-a866-c7d4933fb2e3}
#define STR_SPECTOGRAM_WINDOW_CLASS         "{08e851a2-ec49-467e-a336-775d79ee26de}"

#define WM_CONFIGURATION_CHANGING           WM_USER + 1

/** Configuration **/

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

#define IDC_CHANNELS                    2036

#define IDM_CHANNELS                    9000

#define IDM_CHANNELS_FIRST              9100
#define IDM_CHANNELS_LAST               9118

#pragma endregion

#pragma region FFT

#define IDC_FFT_GROUP                   2040

#define IDC_FFT_SIZE_LBL                2050
#define IDC_FFT_SIZE                    2052
#define IDC_FFT_SIZE_PARAMETER_NAME     2054
#define IDC_FFT_SIZE_PARAMETER          2056
#define IDC_FFT_SIZE_PARAMETER_UNIT     2058

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

#pragma region Common

#define IDC_COMMON                      6000

#define IDC_COLOR_SCHEME_LBL            6010
#define IDC_COLOR_SCHEME                6012

#define IDC_GRADIENT                    6022
#define IDC_COLOR_LIST                      6024
#define IDC_ADD                         6026
#define IDC_REMOVE                      6028
#define IDC_REVERSE                     6030

#define IDC_POSITION                    6040
#define IDC_POSITION_LBL                6042
#define IDC_SPREAD                      6044

#define IDC_SMOOTHING_METHOD_LBL        6050
#define IDC_SMOOTHING_METHOD            6052

#define IDC_SMOOTHING_FACTOR_LBL        6060
#define IDC_SMOOTHING_FACTOR            6062

#define IDC_SHOW_TOOLTIPS               6070

#pragma endregion

#pragma region X axis

#define IDC_X_AXIS                      4000

#define IDC_X_AXIS_MODE_LBL             4002
#define IDC_X_AXIS_MODE                 4004

#pragma endregion

#pragma region Y axis

#define IDC_Y_AXIS                      5000

#define IDC_Y_AXIS_MODE_LBL             5002
#define IDC_Y_AXIS_MODE                 5004

#define IDC_AMPLITUDE_LBL_1             5010
#define IDC_AMPLITUDE_LO                5012
#define IDC_AMPLITUDE_LO_SPIN           5014
#define IDC_AMPLITUDE_LBL_2             5016
#define IDC_AMPLITUDE_HI                5018
#define IDC_AMPLITUDE_HI_SPIN           5020
#define IDC_AMPLITUDE_LBL_3             5022

#define IDC_AMPLITUDE_STEP_LBL_1        5030
#define IDC_AMPLITUDE_STEP              5032
#define IDC_AMPLITUDE_STEP_SPIN         5034
#define IDC_AMPLITUDE_STEP_LBL_2        5036

#define IDC_USE_ABSOLUTE                5040

#define IDC_GAMMA_LBL                   5050
#define IDC_GAMMA                       5052

#pragma endregion

#pragma region Colors

#define IDC_COLORS_GROUP                7000

#define IDC_BACK_COLOR_LBL              7010
#define IDC_BACK_COLOR                  7012
#define IDC_BACK_COLOR_DEF              7014

#define IDC_X_TEXT_COLOR_LBL            7016
#define IDC_X_TEXT_COLOR                7018
#define IDC_X_TEXT_COLOR_DEF            7020

#define IDC_X_LINE_COLOR_LBL            7022
#define IDC_X_LINE_COLOR                7024
#define IDC_X_LINE_COLOR_DEF            7026

#define IDC_Y_TEXT_COLOR_LBL            7028
#define IDC_Y_TEXT_COLOR                7030
#define IDC_Y_TEXT_COLOR_DEF            7032

#define IDC_Y_LINE_COLOR_LBL            7034
#define IDC_Y_LINE_COLOR                7036
#define IDC_Y_LINE_COLOR_DEF            7038

#pragma endregion

#pragma region Visualization

#define IDC_VISUALIZATION_LBL           7100
#define IDC_VISUALIZATION               7102

// Bars

#define IDC_BARS                        7110

#define IDC_DRAW_BAND_BACKGROUND        7112
#define IDC_LED_MODE                    7114
#define IDC_HORIZONTAL_GRADIENT         7116

#define IDC_PEAK_MODE_LBL               7120
#define IDC_PEAK_MODE                   7122

#define IDC_HOLD_TIME_LBL               7130
#define IDC_HOLD_TIME                   7132

#define IDC_ACCELERATION_LBL            7140
#define IDC_ACCELERATION                7142

#define IDC_WHITE_KEYS_LBL              7150
#define IDC_WHITE_KEYS                  7152

#define IDC_BLACK_KEYS_LBL              7154
#define IDC_BLACK_KEYS                  7156

// Curve

#define IDC_CURVE                       7200

#define IDC_LINE_WIDTH_LBL              7210
#define IDC_LINE_WIDTH                  7212
#define IDC_LINE_WIDTH_SPIN             7214

#define IDC_AREA_OPACITY_LBL            7220
#define IDC_AREA_OPACITY                7222
#define IDC_AREA_OPACITY_SPIN           7224
#define IDC_AREA_OPACITY_LBL_2          7226

#pragma endregion

#define IDC_RESET                       9999

/** Color Dialog **/

#define IDD_CHOOSECOLOR         1020

#define IDC_ALPHA_SLIDER        800
#define IDC_ALPHA_VALUE         801
