
/** $VER: Resources.h (2023.12.03) P. Stuer **/

#pragma once

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

/** Component specific **/

#define STR_COMPONENT_NAME          "Spectrum Analyzer"
#define STR_COMPONENT_VERSION       TOSTRING(NUM_FILE_MAJOR) "." TOSTRING(NUM_FILE_MINOR) "." TOSTRING(NUM_FILE_PATCH) "." TOSTRING(NUM_FILE_PRERELEASE)
#define STR_COMPONENT_BASENAME      "foo_vis_spectrum_analyzer"
#define STR_COMPONENT_FILENAME      STR_COMPONENT_BASENAME ".dll"
#define STR_COMPONENT_COMPANY_NAME  ""
#define STR_COMPONENT_COPYRIGHT     "Copyright (c) 2023 P. Stuer. All rights reserved."
#define STR_COMPONENT_COMMENTS      ""
#define STR_COMPONENT_DESCRIPTION   "A spectrum analyzer for foobar2000"
#define STR_COMPONENT_COMMENT       "Based on the Audio Spectrum project (https://codepen.io/TF3RDL/pen/poQJwRW)"

/** Generic **/

#define STR_COMPANY_NAME        TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT           TEXT(STR_COMPONENT_COPYRIGHT)

#define NUM_FILE_MAJOR          0
#define NUM_FILE_MINOR          4
#define NUM_FILE_PATCH          2
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR       0
#define NUM_PRODUCT_MINOR       4
#define NUM_PRODUCT_PATCH       2
#define NUM_PRODUCT_PRERELEASE  0

#define STR_PRODUCT_NAME        STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/") STR_COMPONENT_BASENAME
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Window **/

#define GUID_UI_ELEMENT_SPECTOGRAM      {0x3247c894,0xe585,0x4025,{0xa8,0x66,0xc7,0xd4,0x93,0x3f,0xb2,0xe3}} // {3247c894-e585-4025-a866-c7d4933fb2e3}
#define STR_SPECTOGRAM_WINDOW_CLASS     "{08e851a2-ec49-467e-a336-775d79ee26de}"

#define WM_CONFIGURATION_CHANGING       WM_USER + 1
#define WM_CONFIGURATION_CHANGED        WM_USER + 2

/** Configuration **/

#define IDD_CONFIGURATION               1000

// Transform

#define IDC_TRANSFORM                   2000
#define IDC_TRANSFORM_TYPE              2002

#define IDC_WINDOW_FUNCTION             2004
#define IDC_WINDOW_PARAMETER            2006
#define IDC_WINDOW_SKEW                 2008

#define IDC_CHANNELS                    2010

// FFT

#define IDC_FFT_SIZE                    2020
#define IDC_FFT_SIZE_PARAMETER_NAME     2022
#define IDC_FFT_SIZE_PARAMETER          2024
#define IDC_FFT_SIZE_PARAMETER_UNIT     2026

#define IDC_SUMMATION_METHOD            2028
#define IDC_MAPPING_METHOD              2030

#define IDC_KERNEL_SIZE                 2032
#define IDC_KERNEL_SIZE_SPIN            2034

#define IDC_SMOOTH_LOWER_FREQUENCIES    2036
#define IDC_SMOOTH_GAIN_TRANSITION      2038

// Frequencies

#define IDC_DISTRIBUTION                2100

#define IDC_NUM_BANDS                   2110
#define IDC_NUM_BANDS_SPIN              2112
#define IDC_LO_FREQUENCY                2114
#define IDC_LO_FREQUENCY_SPIN           2116
#define IDC_HI_FREQUENCY                2118
#define IDC_HI_FREQUENCY_SPIN           2120

#define IDC_MIN_NOTE                    2130
#define IDC_MIN_NOTE_SPIN               2132
#define IDC_MAX_NOTE                    2134
#define IDC_MAX_NOTE_SPIN               2136
#define IDC_BANDS_PER_OCTAVE            2138
#define IDC_BANDS_PER_OCTAVE_SPIN       2140
#define IDC_PITCH                       2142
#define IDC_PITCH_SPIN                  2144
#define IDC_TRANSPOSE                   2146
#define IDC_TRANSPOSE_SPIN              2148

#define IDC_SCALING_FUNCTION            2150

#define IDC_SKEW_FACTOR                 2152
#define IDC_SKEW_FACTOR_SPIN            2154
#define IDC_BANDWIDTH                   2156
#define IDC_BANDWIDTH_SPIN              2158

// X axis

#define IDC_X_AXIS                      2200

// Y axis

#define IDC_Y_AXIS                      2300

#define IDC_AMPLITUDE_LO                2302
#define IDC_AMPLITUDE_LO_SPIN           2304
#define IDC_AMPLITUDE_HI                2306
#define IDC_AMPLITUDE_HI_SPIN           2308
#define IDC_AMPLITUDE_STEP              2310
#define IDC_AMPLITUDE_STEP_SPIN         2312

#define IDC_USE_ABSOLUTE                2314

#define IDC_GAMMA                       2316

// Bands

#define IDC_BANDS                       6000

#define IDC_COLOR_SCHEME                6010
#define IDC_DRAW_BAND_BACKGROUND        6011
#define IDC_SHOW_TOOLTIPS               6012

#define IDC_GRADIENT                    6020
#define IDC_COLORS                      6021
#define IDC_ADD                         6022
#define IDC_REMOVE                      6023
#define IDC_REVERSE                     6024

#define IDC_SMOOTHING_METHOD            6030
#define IDC_SMOOTHING_METHOD_LBL        6031
#define IDC_SMOOTHING_FACTOR            6032
#define IDC_SMOOTHING_FACTOR_LBL        6033

#define IDC_PEAK_MODE                   6040
#define IDC_PEAK_MODE_LBL               6041

#define IDC_HOLD_TIME                   6050
#define IDC_HOLD_TIME_LBL               6051
#define IDC_ACCELERATION                6052
#define IDC_ACCELERATION_LBL            6053

// Colors

#define IDC_COLORS_GROUP                7000

#define IDC_BACK_COLOR                  7010
#define IDC_X_TEXT_COLOR                7012
#define IDC_X_LINE_COLOR                7014
#define IDC_Y_TEXT_COLOR                7016
#define IDC_Y_LINE_COLOR                7018
#define IDC_BAND_BACK_COLOR             7020

#define IDC_RESET                       8000

#define IDM_CHANNELS                    9000

#define IDM_CHANNELS_FIRST              9100
#define IDM_CHANNELS_LAST               9118
