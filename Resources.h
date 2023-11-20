
/** $VER: Resources.h (2023.11.20) P. Stuer **/

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
#define NUM_FILE_MINOR          3
#define NUM_FILE_PATCH          0
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR       0
#define NUM_PRODUCT_MINOR       3
#define NUM_PRODUCT_PATCH       0
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

#define IDC_TRANSFORM                   2000
#define IDC_TRANSFORM_TYPE              IDC_TRANSFORM + 1

#define IDC_DISTRIBUTION                IDC_TRANSFORM_TYPE + 1

#define IDC_FFT_SIZE                    IDC_DISTRIBUTION + 1
#define IDC_FFT_SIZE_PARAMETER_NAME     IDC_FFT_SIZE + 1
#define IDC_FFT_SIZE_PARAMETER          IDC_FFT_SIZE_PARAMETER_NAME + 1
#define IDC_FFT_SIZE_PARAMETER_UNIT     IDC_FFT_SIZE_PARAMETER + 1

#define IDC_X_AXIS                      IDC_FFT_SIZE_PARAMETER_UNIT + 1
#define IDC_Y_AXIS                      IDC_X_AXIS + 1

#define IDC_COLOR_SCHEME                IDC_Y_AXIS + 1
#define IDC_PEAK_MODE                   IDC_COLOR_SCHEME + 1

#define IDC_SCALING_FUNCTION            IDC_PEAK_MODE + 1
#define IDC_SUMMATION_METHOD            IDC_SCALING_FUNCTION + 1
#define IDC_MAPPING_METHOD              IDC_SUMMATION_METHOD + 1
#define IDC_SMOOTHING_METHOD            IDC_MAPPING_METHOD + 1

#define IDC_NUM_BANDS                   IDC_SMOOTHING_METHOD + 1
#define IDC_MIN_FREQUENCY               IDC_NUM_BANDS + 1
#define IDC_MAX_FREQUENCY               IDC_MIN_FREQUENCY + 1

#define IDC_MIN_NOTE                    IDC_MAX_FREQUENCY + 1
#define IDC_MAX_NOTE                    IDC_MIN_NOTE + 1
#define IDC_BANDS_PER_OCTAVE            IDC_MAX_NOTE + 1
#define IDC_PITCH                       IDC_BANDS_PER_OCTAVE + 1
#define IDC_TRANSPOSE                   IDC_PITCH + 1

#define IDC_SKEW_FACTOR                 IDC_TRANSPOSE + 1
#define IDC_BANDWIDTH                   IDC_SKEW_FACTOR + 1
#define IDC_SMOOTHING_FACTOR            IDC_BANDWIDTH + 1
#define IDC_KERNEL_SIZE                 IDC_SMOOTHING_FACTOR + 1
#define IDC_GAMMA                       IDC_KERNEL_SIZE + 1

#define IDC_SMOOTH_LOWER_FREQUENCIES    IDC_GAMMA + 1
#define IDC_SMOOTH_GAIN_TRANSITION      IDC_SMOOTH_LOWER_FREQUENCIES + 1

#define IDC_MIN_DECIBEL                 IDC_SMOOTH_GAIN_TRANSITION + 1
#define IDC_MAX_DECIBEL                 IDC_MIN_DECIBEL + 1
#define IDC_USE_ABSOLUTE                IDC_MAX_DECIBEL + 1

#define IDC_HOLD_TIME                   IDC_USE_ABSOLUTE + 1
#define IDC_ACCELERATION                IDC_HOLD_TIME + 1

#define IDC_DRAW_BAND_BACKGROUND        IDC_ACCELERATION + 1

#define IDC_RESET                       IDC_DRAW_BAND_BACKGROUND + 1
