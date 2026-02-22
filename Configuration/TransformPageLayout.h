
/** $VER: TransformPageLayout.h (2026.02.22) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "NewConfigurationDialogLayout.h"

#pragma region Transform

// Groupbox
#define X_B05   0
#define Y_B05   0

    #pragma region Method
    // Label
    #define W_A62    64
    #define H_A62    H_LBL
    #define X_A62    X_B05 + 5
    #define Y_A62    Y_B05 + 11

    // Combobox
    #define W_A63    82
    #define H_A63    H_CBX
    #define X_A63    X_A62 + W_A62 + IX
    #define Y_A63    Y_A62
    #pragma endregion

    #pragma region Window Function

    // Label: Window function
    #define W_C01    64
    #define H_C01    H_LBL
    #define X_C01    X_A62
    #define Y_C01    Y_A63 + H_A63 + IY

    // Combobox: Window function
    #define W_C02    82
    #define H_C02    H_CBX
    #define X_C02    X_C01 + W_C01 + IX
    #define Y_C02    Y_C01

    // Label: Window parameter
    #define W_C03    64
    #define H_C03    H_LBL
    #define X_C03    X_C01
    #define Y_C03    Y_C02 + H_C02 + IY

    // Textbox: Window parameter
    #define W_C04    30
    #define H_C04    H_CBX
    #define X_C04    X_C03 + W_C03 + IX
    #define Y_C04    Y_C03

    // Label: Window Skew
    #define W_C05    64
    #define H_C05    H_LBL
    #define X_C05    X_C03
    #define Y_C05    Y_C04 + H_C04 + IY

    // Textbox: Window Skew
    #define W_C06    30
    #define H_C06    H_CBX
    #define X_C06    X_C05 + W_C05 + IX
    #define Y_C06    Y_C05

    #pragma endregion

    // Label: Reaction Alignment
    #define W_C07    64
    #define H_C07    H_LBL
    #define X_C07    X_C05
    #define Y_C07    Y_C06 + H_C06 + IY

    // Textbox: Reaction Alignment
    #define W_C08    30
    #define H_C08    H_CBX
    #define X_C08    X_C07 + W_C07 + IX
    #define Y_C08    Y_C07

#define W_B05   178
#define H_B05   11 + H_A63 + IY + H_C02 + IY + H_C04 + IY + H_C06 + IY + H_C08 + 7
#pragma endregion

#pragma region FFT
// Groupbox
#define X_B00   X_B05
#define Y_B00   Y_B05 + H_B05 + IY

    #pragma region Size
    // Label
    #define W_A03   64
    #define H_A03   H_LBL
    #define X_A03   X_B00 + 5
    #define Y_A03   Y_B00 + 11

    // Combobox
    #define W_A04   82
    #define H_A04   H_CBX
    #define X_A04   X_A03 + W_A03 + IX
    #define Y_A04   Y_A03
    #pragma endregion

    #pragma region Size Parameter
    // Label
    #define W_A61   64
    #define H_A61   H_LBL
    #define X_A61   X_A03
    #define Y_A61   Y_A04 + H_A04 + IY

    // Textbox
    #define W_A59    30
    #define H_A59    H_TBX
    #define X_A59    X_A61 + W_A61 + IY
    #define Y_A59    Y_A61

    // Label
    #define W_A60   42
    #define H_A60   H_LBL
    #define X_A60   X_A59 + W_A59 + IX
    #define Y_A60   Y_A59
    #pragma endregion

    #pragma region Summation Method
    // Label
    #define W_A15    64
    #define H_A15    H_LBL
    #define X_A15    X_A03
    #define Y_A15    Y_A59 + H_A59 + IY

    // Combobox
    #define W_A16    100
    #define H_A16    H_CBX
    #define X_A16    X_A15 + W_A15 + IX
    #define Y_A16    Y_A15
    #pragma endregion

    #pragma region Mapping
    // Label
    #define W_A65    64
    #define H_A65    H_LBL
    #define X_A65    X_A15
    #define Y_A65    Y_A16 + H_A16 + IY

    // Combobox
    #define W_A66    100
    #define H_A66    H_CBX
    #define X_A66    X_A65 + W_A65 + IX
    #define Y_A66    Y_A65
    #pragma endregion

    #pragma region Smooth lower frequencies
    // Checkbox
    #define W_A43    100
    #define H_A43    H_CHB
    #define X_A43    X_A66
    #define Y_A43    Y_A66 + H_A66 + IY
    #pragma endregion

    #pragma region Smooth gain transition
    // Checkbox
    #define W_A44    100
    #define H_A44    H_CHB
    #define X_A44    X_A43
    #define Y_A44    Y_A43 + H_A43 + IY
    #pragma endregion

    #pragma region Lanczos Kernel
    // Label
    #define W_A39    64
    #define H_A39    H_LBL
    #define X_A39    X_A15
    #define Y_A39    Y_A44 + H_A44 + IY

    // Textbox
    #define W_A40    30
    #define H_A40    H_TBX
    #define X_A40    X_A39 + W_A39 + IX
    #define Y_A40    Y_A39
    #pragma endregion

#define W_B00   W_B05
#define H_B00   11 + H_A04 + IY + H_A59 + IY + H_A16 + IY + H_A43 + IY + H_A66 + IY + H_A44 + IY + H_A40 + 7
#pragma endregion

#pragma region Brown-Puckette CQT
// Groupbox
#define X_B10   X_B05 + W_B05 + IY
#define Y_B10   Y_B05

    // Label: Bandwidth Offset
    #define W_F04    78
    #define H_F04    H_LBL
    #define X_F04    X_B10 + 5
    #define Y_F04    Y_B10 + 11

    // Textbox
    #define W_F05    30
    #define H_F05    H_TBX
    #define X_F05    X_F04 + W_F04 + IX
    #define Y_F05    Y_F04

    // Label: Bandwidth Cap
    #define W_F06    78
    #define H_F06    H_LBL
    #define X_F06    X_F04
    #define Y_F06    Y_F05 + H_F05 + IX

    // Textbox
    #define W_F07    30
    #define H_F07    H_TBX
    #define X_F07    X_F06 + W_F06 + IX
    #define Y_F07    Y_F06

    // Label: Bandwidth Amount
    #define W_F08    78
    #define H_F08    H_LBL
    #define X_F08    X_F06
    #define Y_F08    Y_F07 + H_F07 + IX

    // Textbox
    #define W_F09    30
    #define H_F09    H_TBX
    #define X_F09    X_F08 + W_F08 + IX
    #define Y_F09    Y_F08

    // Checkbox: Granular Bandwidth
    #define W_F10    80
    #define H_F10    H_LBL
    #define X_F10    X_F09
    #define Y_F10    Y_F09 + H_F09 + IX

    // Label: Kernel Shape
    #define W_A89    78
    #define H_A89    H_LBL
    #define X_A89    X_F08
    #define Y_A89    Y_F10 + H_F10 + IY

    // Combobox
    #define W_A90    82
    #define H_A90    H_CBX
    #define X_A90    X_A89 + W_A89 + IX
    #define Y_A90    Y_A89

    // Label: Kernel Shape Parameter
    #define W_A91    78
    #define H_A91    H_LBL
    #define X_A91    X_A89
    #define Y_A91    Y_A90 + H_A90 + IY

    // Textbox
    #define W_A92    30
    #define H_A92    H_CBX
    #define X_A92    X_A91 + W_A91 + IX
    #define Y_A92    Y_A91

    // Label: Kernel Asymmetry
    #define W_A93    78
    #define H_A93    H_LBL
    #define X_A93    X_A91
    #define Y_A93    Y_A92 + H_A92 + IY

    // Textbox
    #define W_A94    30
    #define H_A94    H_CBX
    #define X_A94    X_A93 + W_A93 + IX
    #define Y_A94    Y_A93

#define W_B10   W_B05
#define H_B10   11 + H_F05 + IY + H_F07 + IY + H_F09 + IY + H_F10 + IY + H_A90 + IY + H_A92 + IY + H_A94 + 7

#pragma endregion

#pragma region IIR

// Groupbox
#define X_B11   X_B10
#define Y_B11   Y_B10 + H_B10 + IY

    // Label: Filter bank order
    #define W_F11    78
    #define H_F11    H_LBL
    #define X_F11    X_B11 + 5
    #define Y_F11    Y_B11 + 11

    // Textbox
    #define W_F12    30
    #define H_F12    H_TBX
    #define X_F12    X_F11 + W_F11 + IX
    #define Y_F12    Y_F11

    // Label: Time resolution
    #define W_F13    78
    #define H_F13    H_LBL
    #define X_F13    X_F11
    #define Y_F13    Y_F12 + H_F12 + IY

    // Textbox
    #define W_F14    30
    #define H_F14    H_TBX
    #define X_F14    X_F13 + W_F13 + IX
    #define Y_F14    Y_F13

    // Label: Bandwidth
    #define W_F15    78
    #define H_F15    H_LBL
    #define X_F15    X_F13
    #define Y_F15    Y_F14 + H_F14 + IY

    // Textbox
    #define W_F16    30
    #define H_F16    H_TBX
    #define X_F16    X_F15 + W_F15 + IX
    #define Y_F16    Y_F15

    // Checkbox: Constant-Q
    #define W_F17    100
    #define H_F17    H_CBX
    #define X_F17    X_F15
    #define Y_F17    Y_F16 + H_F16 + IY

    // Checkbox: Compensate bandwidth
    #define W_F18    100
    #define H_F18    H_CBX
    #define X_F18    X_F17
    #define Y_F18    Y_F17 + H_F17 + IY

    // Checkbox: Use prewarped Q
    #define W_F19    100
    #define H_F19    H_CBX
    #define X_F19    X_F18
    #define Y_F19    Y_F18 + H_F18 + IY

#define W_B11   W_B10
#define H_B11   11 + H_F12 + IY + H_F14 + IY + H_F16 + IY + H_F17 + IY + H_F18 + IY + H_F19 + 7

#pragma endregion
