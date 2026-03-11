
/** $VER: CommonPageLayout.h (2026.02.22) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

// Groupbox
#define X_B04   0
#define Y_B04   0

    #pragma region Smoothing Method
    // Label
    #define W_A17    66
    #define H_A17    H_LBL
    #define X_A17    X_B04 + 5
    #define Y_A17    Y_B04 + 11

    // Combobox
    #define W_A18    60
    #define H_A18    H_CBX
    #define X_A18    X_A17 + W_A17 + IX
    #define Y_A18    Y_A17
    #pragma endregion

    #pragma region Smoothing Factor
    // Label
    #define W_A37    66
    #define H_A37    H_LBL
    #define X_A37    X_A17
    #define Y_A37    Y_A18 + H_A18 + IY

    // Textbox
    #define W_A38    30
    #define H_A38    H_TBX
    #define X_A38    X_A37 + W_A37 + IX
    #define Y_A38    Y_A37
    #pragma endregion

    #pragma region Tool tips

    // Checkbox
    #define W_A87    86
    #define H_A87    H_CHB
    #define X_A87    X_A17
    #define Y_A87    Y_A38 + H_A38 + IY

    #pragma endregion

    #pragma region Suppress mirror image

    // Checkbox
    #define W_G19    86
    #define H_G19    H_CHB
    #define X_G19    X_A87
    #define Y_G19    Y_A87 + H_A87 + IY

    #pragma endregion

    #pragma region Visualize during pause

    // Checkbox
    #define W_A95    86
    #define H_A95    H_CHB
    #define X_A95    X_A87 + W_A87 + IX
    #define Y_A95    Y_A87

    #pragma endregion

#pragma region Artwork
// Groupbox
#define X_B06   X_B04
#define Y_B06   Y_B04 + H_B04 + IY

    /** Artwork Type **/

    // Label
    #define W_G40    66
    #define H_G40    H_LBL
    #define X_G40    X_B06 + 5
    #define Y_G40    Y_B06 + 11

    // Combobox
    #define W_G41    86
    #define H_G41    H_CBX
    #define X_G41    X_G40 + W_G40 + IX
    #define Y_G41    Y_G40

    /** Artwork Colors **/

    // Label
    #define W_G06    66
    #define H_G06    H_LBL
    #define X_G06    X_G40
    #define Y_G06    Y_G41 + H_G41 + IY

    // Textbox
    #define W_G07    30
    #define H_G07    H_TBX
    #define X_G07    X_G06 + W_G06 + IX
    #define Y_G07    Y_G06

    /** Lightness Threshold **/

    // Label
    #define W_G08    66
    #define H_G08    H_LBL
    #define X_G08    X_G06
    #define Y_G08    Y_G07 + H_G07 + IY

    // Textbox
    #define W_G09    30
    #define H_G09    H_TBX
    #define X_G09    X_G08 + W_G08 + IX
    #define Y_G09    Y_G08

    // Label "%"
    #define W_G10    10
    #define H_G10    H_LBL
    #define X_G10    X_G09 + W_G09 + IX
    #define Y_G10    Y_G08

    /** Color Order **/

    // Label
    #define W_G11    66
    #define H_G11    H_LBL
    #define X_G11    X_G08
    #define Y_G11    Y_G09 + H_G09 + IY

    // Combobox
    #define W_G12    86
    #define H_G12    H_CBX
    #define X_G12    X_G11 + W_G11 + IX
    #define Y_G12    Y_G11

    /** Checkbox: Show artwork on background **/

    // Checkbox
    #define W_G02    160
    #define H_G02    H_CHB
    #define X_G02    X_B06 + 5
    #define Y_G02    Y_G12 + H_G12 + IY

    /** Fit mode **/

    // Label
    #define W_G30    66
    #define H_G30    H_LBL
    #define X_G30    X_G02
    #define Y_G30    Y_G02 + H_G02 + IY

    // Combobox
    #define W_G31    86
    #define H_G31    H_CBX
    #define X_G31    X_G30 + W_G30 + IX
    #define Y_G31    Y_G30

    /** Fit window **/

    // Checkbox
    #define W_G32    60
    #define H_G32    H_CHB
    #define X_G32    X_G31
    #define Y_G32    Y_G31 + H_G31 + IY

    /** Artwork Opacity **/

    // Label
    #define W_G03    66
    #define H_G03    H_LBL
    #define X_G03    X_G30
    #define Y_G03    Y_G32 + H_G32 + IY

    // Textbox
    #define W_G04    30
    #define H_G04    H_TBX
    #define X_G04    X_G03 + W_G03 + IX
    #define Y_G04    Y_G03

    // Label: Unit
    #define W_G05    10
    #define H_G05    H_LBL
    #define X_G05    X_G04 + W_G04 + IX
    #define Y_G05    Y_G03

    /** Script **/

    // Label
    #define W_G13    66
    #define H_G13    H_LBL
    #define X_G13    X_G03
    #define Y_G13    Y_G04 + H_G04 + IY

    // Textbox
    #define W_G14    100
    #define H_G14    H_TBX
    #define X_G14    X_G13 + W_G13 + IX
    #define Y_G14    Y_G13

#define W_B06  184
#define H_B06   11 + H_G41 + IY + H_G07 + IY + H_G09 + IY + H_G12 + IY + H_G02 + IY + H_G31 + IY + H_G32 + IY + H_G04 + IY + H_G14 + 7

#pragma endregion

#pragma region Component
// Groupbox
#define X_K00   X_B06
#define Y_K00   Y_B06 + H_B06 + IY
#define W_K00   W_B06

    // Label: Log level
    #define X_K01    X_K00 +  5
    #define Y_K01    Y_K00 + 11
    #define W_K01    34
    #define H_K01    H_LBL

    // Combobox: Log level
    #define X_K02    X_K01 + W_K01 + IX
    #define Y_K02    Y_K01
    #define W_K02    48
    #define H_K02    H_CBX

#define H_K00   11 + H_K02 + 7

#pragma endregion

#define W_B04  184
#define H_B04   11 + H_A18 + IY + H_A38 + IY + H_A87 + IY + H_G19 + 7
