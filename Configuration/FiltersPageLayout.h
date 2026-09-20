
/** $VER: FiltersPageLayout.h (2026.09.09) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

// Groupbox
#define X_H00   0
#define Y_H00   0

    #pragma region Acoustic Filter
    // Label
    #define W_H01    50
    #define H_H01    H_LBL
    #define X_H01    X_H00 + 5
    #define Y_H01    Y_H00 + 11

    // Combobox
    #define W_H02    100
    #define H_H02    H_CBX
    #define X_H02    X_H01 + W_H01 + IX
    #define Y_H02    Y_H01
    #pragma endregion

    #pragma region Frequency Shift
    // Label
    #define W_H03    100
    #define H_H03    H_LBL
    #define X_H03    X_H01
    #define Y_H03    Y_H02 + H_H02 + IY

    // Textbox
    #define W_H04    46
    #define H_H04    H_TBX
    #define X_H04    X_H03 + W_H03 + IX
    #define Y_H04    Y_H03

    // Unit
    #define W_H21    36
    #define H_H21    H_LBL
    #define X_H21    X_H04 + W_H04 + IX
    #define Y_H21    Y_H04
    #pragma endregion

    #pragma region Frequency Tilt
    // Label
    #define W_H05    W_H03
    #define H_H05    H_LBL
    #define X_H05    X_H03
    #define Y_H05    Y_H04 + H_H04 + IY

    // Textbox
    #define W_H06    46
    #define H_H06    H_TBX
    #define X_H06    X_H05 + W_H05 + IX
    #define Y_H06    Y_H05

    // Unit
    #define W_H07    36
    #define H_H07    H_LBL
    #define X_H07    X_H06 + W_H06 + IX
    #define Y_H07    Y_H06
    #pragma endregion

    #pragma region Frequency Tilt Pivot
    // Label
    #define W_H08    W_H03
    #define H_H08    H_LBL
    #define X_H08    X_H05
    #define Y_H08    Y_H06 + H_H06 + IY

    // Textbox
    #define W_H09    46
    #define H_H09    H_TBX
    #define X_H09    X_H08 + W_H08 + IX
    #define Y_H09    Y_H08

    // Unit
    #define W_H10    20
    #define H_H10    H_LBL
    #define X_H10    X_H09 + W_H09 + IX
    #define Y_H10    Y_H09
    #pragma endregion

    #pragma region Equalization Amount
    // Label
    #define W_H11    W_H03
    #define H_H11    H_LBL
    #define X_H11    X_H08
    #define Y_H11    Y_H09 + H_H09 + IY

    // Textbox
    #define W_H12    46
    #define H_H12    H_TBX
    #define X_H12    X_H11 + W_H11 + IX
    #define Y_H12    Y_H11
    #pragma endregion

    #pragma region Equalization Frequency Scale
    // Label
    #define W_H13    W_H03
    #define H_H13    H_LBL
    #define X_H13    X_H11
    #define Y_H13    Y_H12 + H_H12 + IY

    // Textbox
    #define W_H14    46
    #define H_H14    H_TBX
    #define X_H14    X_H13 + W_H13 + IX
    #define Y_H14    Y_H13

    // Unit
    #define W_H15    20
    #define H_H15    H_LBL
    #define X_H15    X_H14 + W_H14 + IX
    #define Y_H15    Y_H14
    #pragma endregion

    #pragma region Equalization Depth
    // Label
    #define W_H16    W_H03
    #define H_H16    H_LBL
    #define X_H16    X_H13
    #define Y_H16    Y_H14 + H_H14 + IY

    // Textbox
    #define W_H17    46
    #define H_H17    H_TBX
    #define X_H17    X_H16 + W_H16 + IX
    #define Y_H17    Y_H16

    // Unit
    #define W_H18    20
    #define H_H18    H_LBL
    #define X_H18    X_H17 + W_H17 + IX
    #define Y_H18    Y_H17
    #pragma endregion

    #pragma region Weight Amount
    // Label
    #define W_H19    W_H03
    #define H_H19    H_LBL
    #define X_H19    X_H16
    #define Y_H19    Y_H17 + H_H17 + IY

    // Textbox
    #define W_H20    46
    #define H_H20    H_TBX
    #define X_H20    X_H19 + W_H19 + IX
    #define Y_H20    Y_H19
    #pragma endregion

#define W_H00   198
#define H_H00   11 + H_H02 + IY + H_H04 + IY + H_H06 + IY + H_H09 + IY + H_H12 + IY + H_H14 + IY + H_H17 + IY + H_H20 + 7

// Groupbox Crossover Filter
#define X_I00   X_H00
#define Y_I00   Y_H00 + H_H00 + IY

    // Label: Crossover Mode
    #define W_I01    50
    #define H_I01    H_LBL
    #define X_I01    X_I00 + 5
    #define Y_I01    Y_I00 + 11

    // Combobox: Crossover Mode
    #define W_I02    100
    #define H_I02    H_CBX
    #define X_I02    X_I01 + W_I01 + IX
    #define Y_I02    Y_I01
    #pragma endregion

    // Label: Low band
    #define W_I03    W_I01
    #define H_I03    H_LBL
    #define X_I03    X_I01
    #define Y_I03    Y_I02 + H_I02 + IY

    // Textbox: Low band
    #define W_I04    36
    #define H_I04    H_TBX
    #define X_I04    X_I03 + W_I03 + IX
    #define Y_I04    Y_I03

    // Label: High band
    #define W_I05    W_I01
    #define H_I05    H_LBL
    #define X_I05    X_I04 + W_I04 + IX
    #define Y_I05    Y_I04

    // Textbox: High band
    #define W_I06    36
    #define H_I06    H_TBX
    #define X_I06    X_I05 + W_I05 + IX
    #define Y_I06    Y_I05

#define W_I00   W_H00
#define H_I00   11 + H_I02 + IY + H_I04 + 7
