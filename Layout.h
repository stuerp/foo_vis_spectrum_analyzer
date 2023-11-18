
/** $VER: Layout.h (2023.11.18) P. Stuer - Defines the layout of the configuration dialog. **/

#pragma once

#define H_LA        8  // Label

#define W_BT        50 // Button
#define H_BT        14 // Button

#define H_EB        14 // Edit box
#define H_CB        14 // Combo box
#define H_CH        10 // Check box

#define W_A00      356 // Dialog width (in dialog units)
#define H_A00      326 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

#pragma region FFT
// Groupbox
#define X_B00   DX
#define Y_B00   DX

    #pragma region FFT Size
    // Label
    #define W_A03   60
    #define H_A03   H_LA
    #define X_A03   X_B00 + 5
    #define Y_A03   Y_B00 + 11

    // Combobox
    #define W_A04   60
    #define H_A04   H_CB
    #define X_A04   X_A03 + W_A03 + IX
    #define Y_A04   Y_A03

    // Label
    #define W_A04b  42
    #define H_A04b  H_LA
    #define X_A04b  X_A04 + W_A04 + IX
    #define Y_A04b  Y_A03
    #pragma endregion

    #pragma region Summation Method
    // Label
    #define W_A15    60
    #define H_A15    H_LA
    #define X_A15    X_A03
    #define Y_A15    Y_A04 + H_A04 + IY

    // Combobox
    #define W_A16    100
    #define H_A16    H_CB
    #define X_A16    X_A15 + W_A15 + IX
    #define Y_A16    Y_A15
    #pragma endregion

    #pragma region Smooth lower frequencies
    // Checkbox
    #define W_A43    100
    #define H_A43    H_CH
    #define X_A43    X_A16
    #define Y_A43    Y_A16 + H_A16 + IY
    #pragma endregion

    #pragma region Smooth gain transition
    // Checkbox
    #define W_A44    100
    #define H_A44    H_CH
    #define X_A44    X_A43
    #define Y_A44    Y_A43 + H_A43 + IY
    #pragma endregion

    #pragma region Lanczos Kernel
    // Label
    #define W_A39    60
    #define H_A39    H_LA
    #define X_A39    X_A15
    #define Y_A39    Y_A44 + H_A44 + IY

    // Textbox
    #define W_A40    30
    #define H_A40    H_EB
    #define X_A40    X_A39 + W_A39 + IX
    #define Y_A40    Y_A39
    #pragma endregion

#define W_B00    5 + W_A03 + IX + W_A04 + IX + W_A04b + 5
#define H_B00   11 + H_A04 + IY + H_A16 + IY + H_A43 + IY + H_A44 + IY + H_A40 + 11
#pragma endregion

#pragma region Frequencies
// Groupbox
#define X_B01   DX
#define Y_B01   Y_B00 + H_B00 + IX

    #pragma region Distribution
    // Label
    #define W_A01   60
    #define H_A01   H_LA
    #define X_A01   X_B01 + 5
    #define Y_A01   Y_B01 + 11

    // Combobox
    #define W_A02   60
    #define H_A02   H_CB
    #define X_A02   X_A01 + W_A01 + IX
    #define Y_A02   Y_A01
    #pragma endregion

    #pragma region Number of Bands
    // Label
    #define W_A19    60
    #define H_A19    H_LA
    #define X_A19    X_A01
    #define Y_A19    Y_A02 + H_A02 + IX

    // Textbox
    #define W_A20    60
    #define H_A20    H_EB
    #define X_A20    X_A19 + W_A19 + IX
    #define Y_A20    Y_A19
    #pragma endregion

    #pragma region Frequencies: [Lo] - [Hi] Hz
    // Label
    #define W_A21    60
    #define H_A21    H_LA
    #define X_A21    X_A19
    #define Y_A21    Y_A20 + H_A20 + IY

    // Textbox (Lo)
    #define W_A22    30
    #define H_A22    H_EB
    #define X_A22    X_A21 + W_A21 + IX
    #define Y_A22    Y_A21

    // Label
    #define W_A23    2
    #define H_A23    H_LA
    #define X_A23    X_A22 + W_A22 + IX
    #define Y_A23    Y_A22

    // Textbox (Hi)
    #define W_A24    30
    #define H_A24    H_EB
    #define X_A24    X_A23 + W_A23 + IX
    #define Y_A24    Y_A23

    // Label (Hz)
    #define W_A24b   8
    #define H_A24b   H_LA
    #define X_A24b   X_A24 + W_A24 + IX
    #define Y_A24b   Y_A24
    #pragma endregion

    #pragma region Note range: [Lo] - [Hi]
    // Label
    #define W_A25    60
    #define H_A25    H_LA
    #define X_A25    X_A21
    #define Y_A25    Y_A24 + H_A24 + IY

    // Textbox (Lo)
    #define W_A26    30
    #define H_A26    H_EB
    #define X_A26    X_A25 + W_A25 + IX
    #define Y_A26    Y_A25

    // Label
    #define W_A27    2
    #define H_A27    H_LA
    #define X_A27    X_A26 + W_A26 + IX
    #define Y_A27    Y_A26

    // Textbox (Hi)
    #define W_A28    30
    #define H_A28    H_EB
    #define X_A28    X_A27 + W_A27 + IX
    #define Y_A28    Y_A27
    #pragma endregion

    #pragma region Bands per octave
    // Label
    #define W_A55    60
    #define H_A55    H_LA
    #define X_A55    X_A25
    #define Y_A55    Y_A28 + H_A28 + IY

    // Textbox
    #define W_A56    30
    #define H_A56    H_EB
    #define X_A56    X_A55 + W_A55 + IX
    #define Y_A56    Y_A55
    #pragma endregion

    #pragma region Pitch
    // Label
    #define W_A29    60
    #define H_A29    H_LA
    #define X_A29    X_A55
    #define Y_A29    Y_A56 + H_A56 + IY

    // Textbox
    #define W_A30    30
    #define H_A30    H_EB
    #define X_A30    X_A29 + W_A29 + IX
    #define Y_A30    Y_A29

    // Label (Hz)
    #define W_A30b   8
    #define H_A30b   H_LA
    #define X_A30b   X_A30 + W_A30 + IX
    #define Y_A30b   Y_A30
    #pragma endregion

    #pragma region Transpose
    // Label
    #define W_A31    60
    #define H_A31    H_LA
    #define X_A31    X_A29
    #define Y_A31    Y_A30 + H_A30 + IY

    // Textbox
    #define W_A32    30
    #define H_A32    H_EB
    #define X_A32    X_A31 + W_A31 + IX
    #define Y_A32    Y_A31
    #pragma endregion

    #pragma region("Scaling Function")
    // Label
    #define W_A13    60
    #define H_A13    H_LA
    #define X_A13    X_A31
    #define Y_A13    Y_A32 + H_A32 + IY

    // Combobox
    #define W_A14    80
    #define H_A14    H_CB
    #define X_A14    X_A13 + W_A13 + IX
    #define Y_A14    Y_A13
    #pragma endregion

    #pragma region Skew Factor
    // Label
    #define W_A33    60
    #define H_A33    H_LA
    #define X_A33    X_A13
    #define Y_A33    Y_A14 + H_A14 + IY

    // Textbox
    #define W_A34    30
    #define H_A34    H_EB
    #define X_A34    X_A33 + W_A33 + IX
    #define Y_A34    Y_A33
    #pragma endregion

    #pragma region Bandwidth
    // Label
    #define W_A35    60
    #define H_A35    H_LA
    #define X_A35    X_A33
    #define Y_A35    Y_A34 + H_A34 + IY

    // Textbox
    #define W_A36    30
    #define H_A36    H_EB
    #define X_A36    X_A35 + W_A35 + IX
    #define Y_A36    Y_A35
    #pragma endregion

#define W_B01   W_B00 // 5 + W_A01 + IX + W_A02 + 5
#define H_B01   11 + H_A02 + IY + H_A19 + IY + H_A22 + IY + H_A26 + IY + H_A56 + IY + H_A30 + IY + H_A32 + IY + H_A14 + IY + H_A34 + IY + H_A36 + 11
#pragma endregion

#pragma region X axis
// Groupbox
#define X_B02   X_B00 + W_B00 + IX
#define Y_B02   DY

    #pragma region X axis
    // Label
    #define W_A05    60
    #define H_A05    H_LA
    #define X_A05    X_B02 + 5
    #define Y_A05    Y_B02 + 11

    // Combobox
    #define W_A06    82
    #define H_A06    H_CB
    #define X_A06    X_A05 + W_A05 + IX
    #define Y_A06    Y_A05
    #pragma endregion

#define W_B02    5 + W_A05 + IX + W_A06 + 5
#define H_B02   11 + H_A06 + 11
#pragma endregion

#pragma region Y axis
// Groupbox
#define X_B03   X_B00 + W_B00 + IX
#define Y_B03   Y_B02 + H_B02 + IY

    #pragma region Y axis
    // Label
    #define W_A07    60
    #define H_A07    H_LA
    #define X_A07    X_B03 + 5
    #define Y_A07    Y_B03 + 11

    // Combobox
    #define W_A08    82
    #define H_A08    H_CB
    #define X_A08    X_A07 + W_A07 + IX
    #define Y_A08    Y_A07
    #pragma endregion

    #pragma region Amplitude: [Lo] - [Hi] dB
    // Label
    #define W_A45    60
    #define H_A45    H_LA
    #define X_A45    X_A07
    #define Y_A45    Y_A08 + H_A08 + DY

    // Textbox (Lo)
    #define W_A46    30
    #define H_A46    H_EB
    #define X_A46    X_A45 + W_A45 + IX
    #define Y_A46    Y_A45

    // Label
    #define W_A47    2
    #define H_A47    H_LA
    #define X_A47    X_A46 + W_A46 + IX
    #define Y_A47    Y_A46

    // Textbox (Hi)
    #define W_A48    30
    #define H_A48    H_EB
    #define X_A48    X_A47 + W_A47 + IX
    #define Y_A48    Y_A47

    // Label (db)
    #define W_A49    8
    #define H_A49    H_LA
    #define X_A49    X_A48 + W_A48 + IX
    #define Y_A49    Y_A48
    #pragma endregion

    #pragma region Use absolute
    // Checkbox
    #define W_A50    100
    #define H_A50    H_CH
    #define X_A50    X_A46
    #define Y_A50    Y_A48 + H_A48 + IY
    #pragma endregion

    #pragma region Gamma
    // Label
    #define W_A41    60
    #define H_A41    H_LA
    #define X_A41    X_A07
    #define Y_A41    Y_A50 + H_A50 + IY

    // Textbox
    #define W_A42    30
    #define H_A42    H_EB
    #define X_A42    X_A41 + W_A41 + IX
    #define Y_A42    Y_A41
    #pragma endregion

#define W_B03    5 + W_A45 + IX + W_A46 + IX + W_A47 + IX + W_A48 + IX + W_A49 + 5
#define H_B03   11 + H_A08 + IY + H_A46 + IY + H_A50 + IY + H_A42 + 11
#pragma endregion

#pragma region Bands
// Groupbox
#define X_B04   X_B03
#define Y_B04   Y_B03 + H_B03 + IY

    #pragma region Color Scheme
    // Label
    #define W_A09    60
    #define H_A09    H_LA
    #define X_A09    X_B04 + 5
    #define Y_A09    Y_B04 + 11

    // Combobox
    #define W_A10    82
    #define H_A10    H_CB
    #define X_A10    X_A09 + W_A09 + IX
    #define Y_A10    Y_A09
    #pragma endregion

    #pragma region Draw band background
    // Checkbox
    #define W_A57    80
    #define H_A57    H_CH
    #define X_A57    X_A10
    #define Y_A57    Y_A10 + H_A10 + IY
    #pragma endregion

    #pragma region Smoothing Method
    // Label
    #define W_A17    60
    #define H_A17    H_LA
    #define X_A17    X_A09
    #define Y_A17    Y_A57 + H_A57 + IY

    // Combobox
    #define W_A18    60
    #define H_A18    H_CB
    #define X_A18    X_A17 + W_A17 + IX
    #define Y_A18    Y_A17
    #pragma endregion

    #pragma region Smoothing Factor
    // Label
    #define W_A37    60
    #define H_A37    H_LA
    #define X_A37    X_A17
    #define Y_A37    Y_A18 + H_A18 + IY

    // Textbox
    #define W_A38    30
    #define H_A38    H_EB
    #define X_A38    X_A37 + W_A37 + IX
    #define Y_A38    Y_A37
    #pragma endregion

    #pragma region Peak Mode
    // Label
    #define W_A11    60
    #define H_A11    H_LA
    #define X_A11    X_A37
    #define Y_A11    Y_A38 + H_A38 + IY

    // Combobox
    #define W_A12    60
    #define H_A12    H_CB
    #define X_A12    X_A11 + W_A11 + IX
    #define Y_A12    Y_A11
    #pragma endregion

    #pragma region Hold time (ms)
    // Label
    #define W_A51    60
    #define H_A51    H_LA
    #define X_A51    X_A11
    #define Y_A51    Y_A12 + H_A12 + IY

    // Textbox
    #define W_A52    30
    #define H_A52    H_EB
    #define X_A52    X_A51 + W_A51 + IX
    #define Y_A52    Y_A51
    #pragma endregion

    #pragma region Acceleration (db/s2)
    // Label
    #define W_A53    60
    #define H_A53    H_LA
    #define X_A53    X_A51
    #define Y_A53    Y_A52 + H_A52 + IY

    // Textbox
    #define W_A54    30
    #define H_A54    H_EB
    #define X_A54    X_A53 + W_A53 + IX
    #define Y_A54    Y_A53
    #pragma endregion

#define W_B04    5 + W_A09 + IX + W_A10  + 5
#define H_B04   11 + H_A10 + IY + H_A57 + IY + H_A18 + IY + H_A38 + IY + H_A12 + IY + H_A52 + IY + H_A54 + 11 + 24
#pragma endregion




// Cancel button (right-most button)
#define W_A99   W_BT
#define H_A99   H_BT
#define X_A99   W_A00 - W_A99 - DX
#define Y_A99   H_A00 - H_A99 - DY

// OK button
#define W_A98   W_BT
#define H_A98   H_BT
#define X_A98   X_A99 - W_A99 - IX
#define Y_A98   Y_A99

// Reset button
#define W_A97   W_BT
#define H_A97   H_BT
#define X_A97   X_A98 - W_A98 - IX
#define Y_A97   Y_A98
