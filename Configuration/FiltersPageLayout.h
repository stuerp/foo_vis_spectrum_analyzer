
/** $VER: FiltersPageLayout.h (2026.02.20) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#define H_LBL        8 // Label

#define W_BTN       50 // Button
#define H_BTN       14 // Button

#define H_TBX       14 // Edit box
#define H_CBX       14 // Combo box
#define H_CHB       10 // Check box

#define W_A00      442 // Dialog width (in dialog units)
#define H_A00      309 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

// Groupbox
#define X_B09   0
#define Y_B09   0

    #pragma region Acoustic Filter
    // Label
    #define W_H01    50
    #define H_H01    H_LBL
    #define X_H01    X_B09 + 5
    #define Y_H01    Y_B09 + 11

    // Combobox
    #define W_H02    100
    #define H_H02    H_CBX
    #define X_H02    X_H01 + W_H01 + IX
    #define Y_H02    Y_H01
    #pragma endregion

    #pragma region Slope Function Offset
    // Label
    #define W_H03    68
    #define H_H03    H_LBL
    #define X_H03    X_H01
    #define Y_H03    Y_H02 + H_H02 + IY

    // Textbox
    #define W_H04    46
    #define H_H04    H_TBX
    #define X_H04    X_H03 + W_H03 + IX
    #define Y_H04    Y_H03
    #pragma endregion

    #pragma region Slope
    // Label
    #define W_H05    68
    #define H_H05    H_LBL
    #define X_H05    X_H03
    #define Y_H05    Y_H04 + H_H04 + IY

    // Textbox
    #define W_H06    46
    #define H_H06    H_TBX
    #define X_H06    X_H05 + W_H05 + IX
    #define Y_H06    Y_H05

    // Unit
    #define W_H07    60
    #define H_H07    H_LBL
    #define X_H07    X_H06 + W_H06 + IX
    #define Y_H07    Y_H06
    #pragma endregion

    #pragma region Slope Offset
    // Label
    #define W_H08    68
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

    #pragma region Equalize Amount
    // Label
    #define W_H11    68
    #define H_H11    H_LBL
    #define X_H11    X_H08
    #define Y_H11    Y_H09 + H_H09 + IY

    // Textbox
    #define W_H12    46
    #define H_H12    H_TBX
    #define X_H12    X_H11 + W_H11 + IX
    #define Y_H12    Y_H11
    #pragma endregion

    #pragma region Equalize Offset
    // Label
    #define W_H13    68
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

    #pragma region Equalize Depth
    // Label
    #define W_H16    68
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
    #define W_H19    68
    #define H_H19    H_LBL
    #define X_H19    X_H16
    #define Y_H19    Y_H17 + H_H17 + IY

    // Textbox
    #define W_H20    46
    #define H_H20    H_TBX
    #define X_H20    X_H19 + W_H19 + IX
    #define Y_H20    Y_H19
    #pragma endregion

#define W_B09   186
#define H_B09   11 + H_H02 + IY + H_H04 + IY + H_H06 + IY + H_H09 + IY + H_H12 + IY + H_H14 + IY + H_H17 + IY + H_H20 + 7
