
/** $VER: Resources.cpp (2023.11.12) P. Stuer **/

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

/** Generic **/

#define STR_COMPANY_NAME        TEXT(STR_COMPONENT_COMPANY_NAME)
#define STR_INTERNAL_NAME       TEXT(STR_COMPONENT_NAME)
#define STR_COMMENTS            TEXT(STR_COMPONENT_COMMENTS)
#define STR_COPYRIGHT           TEXT(STR_COMPONENT_COPYRIGHT)

#define NUM_FILE_MAJOR          0
#define NUM_FILE_MINOR          1
#define NUM_FILE_PATCH          0
#define NUM_FILE_PRERELEASE     0

#define STR_FILE_NAME           TEXT(STR_COMPONENT_FILENAME)
#define STR_FILE_VERSION        TOSTRING(NUM_FILE_MAJOR) TEXT(".") TOSTRING(NUM_FILE_MINOR) TEXT(".") TOSTRING(NUM_FILE_PATCH) TEXT(".") TOSTRING(NUM_FILE_PRERELEASE)
#define STR_FILE_DESCRIPTION    TEXT(STR_COMPONENT_DESCRIPTION)

#define NUM_PRODUCT_MAJOR       0
#define NUM_PRODUCT_MINOR       1
#define NUM_PRODUCT_PATCH       0
#define NUM_PRODUCT_PRERELEASE  0

#define STR_PRODUCT_NAME        STR_INTERNAL_NAME
#define STR_PRODUCT_VERSION     TOSTRING(NUM_PRODUCT_MAJOR) TEXT(".") TOSTRING(NUM_PRODUCT_MINOR) TEXT(".") TOSTRING(NUM_PRODUCT_PATCH) TEXT(".") TOSTRING(NUM_PRODUCT_PRERELEASE)

#define STR_ABOUT_NAME          STR_INTERNAL_NAME
#define STR_ABOUT_WEB           TEXT("https://github.com/stuerp/") STR_COMPONENT_BASENAME
#define STR_ABOUT_EMAIL         TEXT("mailto:peter.stuer@outlook.com")

/** Preferences **/

#define IDD_CONFIG                      101
#define IDC_LOW_ENERGY                  1001
#define IDC_HIGH_ENERGY                 1002
#define IDC_BLEND_LINEAR                1006
#define IDC_BLEND_CCW                   1007
#define IDC_BLEND_CW                    1008
#define IDC_REDRAW_PER_SECOND           1009
#define IDC_LINES_PER_SECOND            1010
#define IDC_STYLELINE                   1011
#define IDC_COMBOBOXEX1                 1012
#define IDC_LOW_POWER_CENTIBEL          1014
#define IDC_HIGH_POWER_CENTIBEL         1015
#define IDC_HIGH_POWER_LABEL            1016
#define IDC_LOW_POWER_LABEL             1017

/** Spectogram **/

#define GUID_UI_ELEMENT_SPECTOGRAM      {0x3247c894,0xe585,0x4025,{0xa8,0x66,0xc7,0xd4,0x93,0x3f,0xb2,0xe3}} // {3247c894-e585-4025-a866-c7d4933fb2e3}
#define STR_SPECTOGRAM_WINDOW_CLASS     "{08e851a2-ec49-467e-a336-775d79ee26de}"
/*
#define GUID_UI_ELEMENT_SPECTOGRAM_OLD  {0x51df46c8,0x741e,0x4b36,{0xaf,0xd7,0x75,0xa8,0xa5,0xac,0x4a,0x22}} // {51df46c8-741e-4b36-afd7-75a8a5ac4a22}
#define STR_SPECTOGRAM_OLD_WINDOW_CLASS "{405be56f-7f7e-4a0d-a81e-00ea8ddbb98d}"
*/
#define NUM_CONFIG_VERSION      1
