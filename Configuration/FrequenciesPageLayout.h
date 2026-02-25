
/** $VER: FrequenciesPageLayout.h (2026.02.2) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

// Groupbox
#define X_B01   0
#define Y_B01   0

    #pragma region Distribution
    // Label
    #define W_A01   60
    #define H_A01   H_LBL
    #define X_A01   X_B01 + 5
    #define Y_A01   Y_B01 + 11

    // Combobox
    #define W_A02   60
    #define H_A02   H_CBX
    #define X_A02   X_A01 + W_A01 + IX
    #define Y_A02   Y_A01
    #pragma endregion

    #pragma region Number of Bands
    // Label
    #define W_A19    60
    #define H_A19    H_LBL
    #define X_A19    X_A01
    #define Y_A19    Y_A02 + H_A02 + IX

    // Textbox
    #define W_A20    40
    #define H_A20    H_TBX
    #define X_A20    X_A19 + W_A19 + IX
    #define Y_A20    Y_A19
    #pragma endregion

    #pragma region Frequencies: [Lo] - [Hi] Hz
    // Label
    #define W_A21    60
    #define H_A21    H_LBL
    #define X_A21    X_A19
    #define Y_A21    Y_A20 + H_A20 + IY

    // Textbox (Lo)
    #define W_A22    46
    #define H_A22    H_TBX
    #define X_A22    X_A21 + W_A21 + IX
    #define Y_A22    Y_A21

    // Label
    #define W_A23    2
    #define H_A23    H_LBL
    #define X_A23    X_A22 + W_A22 + IX
    #define Y_A23    Y_A22

    // Textbox (Hi)
    #define W_A24    46
    #define H_A24    H_TBX
    #define X_A24    X_A23 + W_A23 + IX
    #define Y_A24    Y_A23

    // Label (Hz)
    #define W_A24b   8
    #define H_A24b   H_LBL
    #define X_A24b   X_A24 + W_A24 + IX
    #define Y_A24b   Y_A24
    #pragma endregion

    #pragma region Note range: [Lo] - [Hi]
    // Label
    #define W_A25    60
    #define H_A25    H_LBL
    #define X_A25    X_A21
    #define Y_A25    Y_A24 + H_A24 + IY

    // Textbox (Lo)
    #define W_A26    36
    #define H_A26    H_TBX
    #define X_A26    X_A25 + W_A25 + IX
    #define Y_A26    Y_A25

    // Label
    #define W_A27    2
    #define H_A27    H_LBL
    #define X_A27    X_A26 + W_A26 + IX
    #define Y_A27    Y_A26

    // Textbox (Hi)
    #define W_A28    36
    #define H_A28    H_TBX
    #define X_A28    X_A27 + W_A27 + IX
    #define Y_A28    Y_A27
    #pragma endregion

    #pragma region Bands per octave
    // Label
    #define W_A55    60
    #define H_A55    H_LBL
    #define X_A55    X_A25
    #define Y_A55    Y_A28 + H_A28 + IY

    // Textbox
    #define W_A56    30
    #define H_A56    H_TBX
    #define X_A56    X_A55 + W_A55 + IX
    #define Y_A56    Y_A55
    #pragma endregion

    #pragma region Pitch
    // Label
    #define W_A29    60
    #define H_A29    H_LBL
    #define X_A29    X_A55
    #define Y_A29    Y_A56 + H_A56 + IY

    // Textbox
    #define W_A30    40
    #define H_A30    H_TBX
    #define X_A30    X_A29 + W_A29 + IX
    #define Y_A30    Y_A29

    // Label (Hz)
    #define W_A64    8
    #define H_A64    H_LBL
    #define X_A64    X_A30 + W_A30 + IX
    #define Y_A64    Y_A30
    #pragma endregion

    #pragma region Transpose
    // Label
    #define W_A31    60
    #define H_A31    H_LBL
    #define X_A31    X_A29
    #define Y_A31    Y_A30 + H_A30 + IY

    // Textbox
    #define W_A32    30
    #define H_A32    H_TBX
    #define X_A32    X_A31 + W_A31 + IX
    #define Y_A32    Y_A31
    #pragma endregion

    #pragma region Scaling Function
    // Label
    #define W_A13    60
    #define H_A13    H_LBL
    #define X_A13    X_A31
    #define Y_A13    Y_A32 + H_A32 + IY

    // Combobox
    #define W_A14    80
    #define H_A14    H_CBX
    #define X_A14    X_A13 + W_A13 + IX
    #define Y_A14    Y_A13
    #pragma endregion

    #pragma region Skew Factor
    // Label
    #define W_A33    60
    #define H_A33    H_LBL
    #define X_A33    X_A13
    #define Y_A33    Y_A14 + H_A14 + IY

    // Textbox
    #define W_A34    32
    #define H_A34    H_TBX
    #define X_A34    X_A33 + W_A33 + IX
    #define Y_A34    Y_A33
    #pragma endregion

    #pragma region Bandwidth
    // Label
    #define W_A35    60
    #define H_A35    H_LBL
    #define X_A35    X_A33
    #define Y_A35    Y_A34 + H_A34 + IY

    // Textbox
    #define W_A36    32
    #define H_A36    H_TBX
    #define X_A36    X_A35 + W_A35 + IX
    #define Y_A36    Y_A35
    #pragma endregion

#define W_B01   186
#define H_B01   11 + H_A02 + IY + H_A20 + IY + H_A22 + IY + H_A26 + IY + H_A56 + IY + H_A30 + IY + H_A32 + IY + H_A14 + IY + H_A34 + IY + H_A36 + 7
