
/** $VER: VisualizationPageLayout.h (2026.02.20) P. Stuer - Defines the layout of a configuration dialog page. **/

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

// Label
#define W_E01    24
#define H_E01    H_LBL
#define X_E01    0
#define Y_E01    0

// Combobox
#define W_E02    82
#define H_E02    H_CBX
#define X_E02    X_E01 + W_E01 + IX
#define Y_E02    Y_E01

#pragma region Peak Indicators

// Groupbox
#define X_B13   X_E01
#define Y_B13   Y_E02 + H_E02 + IY

    #pragma region Peak Mode
    // Label
    #define W_A11    42
    #define H_A11    H_LBL
    #define X_A11    X_B13 + 5
    #define Y_A11    Y_B13 + 11

    // Combobox
    #define W_A12    60
    #define H_A12    H_CBX
    #define X_A12    X_A11 + W_A11 + IX
    #define Y_A12    Y_A11
    #pragma endregion

    #pragma region Hold time (ms)
    // Label
    #define W_A51    42
    #define H_A51    H_LBL
    #define X_A51    X_A11
    #define Y_A51    Y_A12 + H_A12 + IY

    // Textbox
    #define W_A52    30
    #define H_A52    H_TBX
    #define X_A52    X_A51 + W_A51 + IX
    #define Y_A52    Y_A51
    #pragma endregion

    #pragma region Acceleration (db/s2)
    // Label
    #define W_A53    42
    #define H_A53    H_LBL
    #define X_A53    X_A51
    #define Y_A53    Y_A52 + H_A52 + IY

    // Textbox
    #define W_A54    30
    #define H_A54    H_TBX
    #define X_A54    X_A53 + W_A53 + IX
    #define Y_A54    Y_A53
    #pragma endregion

#define W_B13   116
#define H_B13   11 + H_A12 + IY + H_A52 + IY + H_A54 + 7

#pragma endregion

#pragma region LEDs

// Groupbox
#define X_B07   X_B13
#define Y_B07   Y_B13 + H_B13 + IY

    #pragma region LED mode
    // Checkbox
    #define W_C12    42
    #define H_C12    H_CHB
    #define X_C12    X_B07 + 5
    #define Y_C12    Y_B07 + 11
    #pragma endregion

    #pragma region LED size
    // Label
    #define W_C17    42
    #define H_C17    H_LBL
    #define X_C17    X_C12
    #define Y_C17    Y_C12 + H_C12 + IY

    // Textbox
    #define W_C18    30
    #define H_C18    H_TBX
    #define X_C18    X_C17 + W_C17 + IX
    #define Y_C18    Y_C17
    #pragma endregion

    #pragma region LED gap
    // Label
    #define W_C19    42
    #define H_C19    H_LBL
    #define X_C19    X_C17
    #define Y_C19    Y_C18 + H_C18 + IY

    // Textbox
    #define W_C20    30
    #define H_C20    H_TBX
    #define X_C20    X_C19 + W_C19 + IX
    #define Y_C20    Y_C19
    #pragma endregion

    #pragma region LED integral size
    // Checkbox
    #define W_C29    60
    #define H_C29    H_CHB
    #define X_C29    X_C12
    #define Y_C29    Y_C20 + H_C20 + IY
    #pragma endregion

#define W_B07  W_B13
#define H_B07  11 + H_C12 + IY + H_C18 + IY + H_C20 + IY + H_C29 + 7
#pragma endregion

#pragma region Bars (Not used yet)

// Groupbox
#define X_B17   X_B07
#define Y_B17   Y_B07 + H_B07 + IY

#define W_B17   W_B07
#define H_B17   0

#pragma endregion

#pragma region Radial Bars

// Groupbox
#define X_B15   X_B07
#define Y_B15   Y_B07 + H_B07 + IY

    #pragma region Inner radius
    // Label
    #define W_C30    52
    #define H_C30    H_LBL
    #define X_C30    X_B15 + 5
    #define Y_C30    Y_B15 + 11

    // Textbox
    #define W_C31    30
    #define H_C31    H_TBX
    #define X_C31    X_C30 + W_C30 + IX
    #define Y_C31    Y_C30
    #pragma endregion

    #pragma region Outer radius
    // Label
    #define W_C32    W_C30
    #define H_C32    H_LBL
    #define X_C32    X_C30
    #define Y_C32    Y_C31 + H_C31 + IY

    // Textbox
    #define W_C33    W_C31
    #define H_C33    H_TBX
    #define X_C33    X_C32 + W_C32 + IX
    #define Y_C33    Y_C32
    #pragma endregion

    #pragma region Angular velocity
    // Label
    #define W_C34    W_C32
    #define H_C34    H_LBL
    #define X_C34    X_C32
    #define Y_C34    Y_C33 + H_C33 + IY

    // Textbox
    #define W_C35    W_C33
    #define H_C35    H_TBX
    #define X_C35    X_C34 + W_C34 + IX
    #define Y_C35    Y_C34
    #pragma endregion

#define W_B15   W_B17
#define H_B15   11 + H_C31 + IY + H_C33 + IY + H_C35 + 7

#pragma endregion

#pragma region Spectrogram
// Groupbox
#define X_B08   X_B13 + W_B13 + IX
#define Y_B08   Y_B13

    #pragma region Scrolling
    // Checkbox
    #define W_C15    80
    #define H_C15    H_CHB
    #define X_C15    X_B08 + 5
    #define Y_C15    Y_B08 + 11
    #pragma endregion

    #pragma region Horizontal
    // Checkbox
    #define W_C27    80
    #define H_C27    H_CHB
    #define X_C27    X_C15
    #define Y_C27    Y_C15 + H_C15 + IY
    #pragma endregion

    #pragma region Spectrum Bar Metrics
    // Checkbox
    #define W_C28    90
    #define H_C28    H_CHB
    #define X_C28    X_C27
    #define Y_C28    Y_C27 + H_C27 + IY
    #pragma endregion

#define W_B08   232
#define H_B08   11 + H_C15 + IY + H_C27 + IY + H_C28 + 7
#pragma endregion

#pragma region Peak Meter

// Groupbox
#define X_B12   X_B08
#define Y_B12   Y_B08 + H_B08 + IY

    // Checkbox: Horizontal
    #define W_C16    80
    #define H_C16    H_CHB
    #define X_C16    X_B12 + 5
    #define Y_C16    Y_B12 + 11

   // Checkbox: RMS +3
    #define W_C24    80
    #define H_C24    H_CHB
    #define X_C24    X_C16
    #define Y_C24    Y_C16 + H_C16 + IY

    // Checkbox: Center scale
    #define W_C74    80
    #define H_C74    H_CHB
    #define X_C74    X_C16 + W_C16 + IX
    #define Y_C74    Y_C16 

    // Checkbox: Scale lines
    #define W_C77    80
    #define H_C77    H_CHB
    #define X_C77    X_C74
    #define Y_C77    Y_C74 + H_C74 + IY 

    #pragma region RMS window
    // Label: RMS window
    #define W_C21   46
    #define H_C21   H_LBL
    #define X_C21   X_C24
    #define Y_C21   Y_C24 + H_C24 + IY

    // Text Box
    #define W_C22   34
    #define H_C22   H_TBX
    #define X_C22   X_C21 + W_C21 + IX
    #define Y_C22   Y_C21

    // Label: Unit
    #define W_C23   10
    #define H_C23   H_LBL
    #define X_C23   X_C22 + W_C22 + IX
    #define Y_C23   Y_C22
    #pragma endregion

    // Label: Bar gap
    #define W_C25   46
    #define H_C25   H_LBL
    #define X_C25   X_C21
    #define Y_C25   Y_C22 + H_C22 + IY

    // Text Box
    #define W_C26   34
    #define H_C26   H_TBX
    #define X_C26   X_C25 + W_C25 + IX
    #define Y_C26   Y_C25

    // Label: Max. bar size
    #define W_C75   46
    #define H_C75   H_LBL
    #define X_C75   X_C26 + W_C26 + IX
    #define Y_C75   Y_C25

    // Text Box
    #define W_C76   34
    #define H_C76   H_TBX
    #define X_C76   X_C75 + W_C75 + IX
    #define Y_C76   Y_C75

#define W_B12  W_B08
#define H_B12  11 + H_C16 + IY + H_C24 + IY + H_C22 + IY + H_C26 + 7

#pragma endregion

#pragma region Level Meter

// Groupbox
#define X_B14   X_B12
#define Y_B14   Y_B12 + H_B12 + IY

    // Checkbox: Horizontal
    #define W_C52    60
    #define H_C52    H_CHB
    #define X_C52    X_C16
    #define Y_C52    Y_B14 + 11

#define W_B14  W_B12
#define H_B14  11 + H_C52 + 7

#pragma endregion

#pragma region Oscilloscope

// Groupbox
#define X_B18   X_B14
#define Y_B18   Y_B14 + H_B14 + IY

    // Checkbox: X-Y mode
    #define W_C54    62
    #define H_C54    H_CHB
    #define X_C54    X_B18 +  5
    #define Y_C54    Y_B18 + 11

    // Label: X gain
    #define W_C56    34
    #define H_C56    H_LBL
    #define X_C56    X_C54 + W_C54 + IX
    #define Y_C56    Y_C54

    // Textbox: X gain
    #define W_C58    30
    #define H_C58    H_TBX
    #define X_C58    X_C56 + W_C56 + IX
    #define Y_C58    Y_C56

    // Label: Y gain
    #define W_C60    48
    #define H_C60    H_LBL
    #define X_C60    X_C58 + W_C58 + IX
    #define Y_C60    Y_C58

    // Textbox: Y gain
    #define W_C62    30
    #define H_C62    H_TBX
    #define X_C62    X_C60 + W_C60 + IX
    #define Y_C62    Y_C60

    // Label: Rotation
    #define W_C78    34
    #define H_C78    H_LBL
    #define X_C78    X_C56
    #define Y_C78    Y_C62 + H_C62 + IY

    // Textbox: Rotation
    #define W_C79    30
    #define H_C79    H_TBX
    #define X_C79    X_C78 + W_C78 + IX
    #define Y_C79    Y_C78

    // Checkbox: Phosphor decay effect
    #define W_C64    62
    #define H_C64    H_CHB
    #define X_C64    X_C54
    #define Y_C64    Y_C79 + H_C79 + IY

    // Label: Blur sigma
    #define W_C66    34
    #define H_C66    H_LBL
    #define X_C66    X_C64 + W_C64 + IX
    #define Y_C66    Y_C64

    // Textbox: Blur sigma
    #define W_C68    30
    #define H_C68    H_TBX
    #define X_C68    X_C66 + W_C66 + IX
    #define Y_C68    Y_C66

    // Label: Decay factor
    #define W_C70    48
    #define H_C70    H_LBL
    #define X_C70    X_C68 + W_C68 + IX
    #define Y_C70    Y_C68

    // Textbox: Decay factor
    #define W_C72    30
    #define H_C72    H_TBX
    #define X_C72    X_C70 + W_C70 + IX
    #define Y_C72    Y_C70

#define W_B18  W_B14
#define H_B18  11 + H_C56 + IY + H_C79 + IY + H_C68 + IY + 7

#pragma endregion
