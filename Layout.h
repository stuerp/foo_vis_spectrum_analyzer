
/** $VER: Layout.h (2024.08.18) P. Stuer - Defines the layout of the configuration dialog. **/

#pragma once

#define H_LBL        8 // Label

#define W_BTN       50 // Button
#define H_BTN       14 // Button

#define H_TBX       14 // Edit box
#define H_CBX       14 // Combo box

#define W_CHB       10 // Check box
#define H_CHB       10 // Check box

#define W_A00      442 // Dialog width (in dialog units)
#define H_A00      309 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

#define W_D01   60
#define H_D01   H_A00 - DY - DY
#define X_D01   DX
#define Y_D01   DY

/** Page: Transform **/

#pragma region Transform
// Groupbox
#define X_B05   X_D01 + W_D01 + IX
#define Y_B05   Y_D01

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

    #pragma region Channels
    // Button
    #define W_A88    82
    #define H_A88    H_BTN
    #define X_A88    X_C08
    #define Y_A88    Y_C08 + H_C08 + IY
    #pragma endregion

#define W_B05   178
#define H_B05   11 + H_A63 + IY + H_C02 + IY + H_C04 + IY + H_C06 + IY + H_C08 + IY + H_A88 + 7
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

/** Page: Frequencies **/

#pragma region Frequencies

// Groupbox
#define X_B01   X_D01 + W_D01 + IX
#define Y_B01   Y_D01

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
#pragma endregion

/** Page: Filters **/

#pragma region Filters
// Groupbox
#define X_B09   X_D01 + W_D01 + IX
#define Y_B09   Y_D01

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

#define W_B09   W_B01
#define H_B09   11 + H_H02 + IY + H_H04 + IY + H_H06 + IY + H_H09 + IY + H_H12 + IY + H_H14 + IY + H_H17 + IY + H_H20 + 7
#pragma endregion

/** Page: Common **/

#pragma region Common

// Groupbox
#define X_B04   X_D01 + W_D01 + IX
#define Y_B04   Y_D01

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
    #define W_A87    90
    #define H_A87    H_CHB
    #define X_A87    X_A38
    #define Y_A87    Y_A38 + H_A38 + IY

    #pragma endregion

    #pragma region Suppress mirror image

    // Checkbox
    #define W_G19    90
    #define H_G19    H_CHB
    #define X_G19    X_A87
    #define Y_G19    Y_A87 + H_A87 + IY

    #pragma endregion

#define W_B04  184
#define H_B04   11 + H_A18 + IY + H_A38 + IY + H_A87 + IY + H_G19 + 7

#pragma endregion

#pragma region Artwork
// Groupbox
#define X_B06   X_B04
#define Y_B06   Y_B04 + H_B04 + IY

    #pragma region Artwork Colors
    // Label
    #define W_G06    66
    #define H_G06    H_LBL
    #define X_G06    X_B06 + 5
    #define Y_G06    Y_B06 + 11

    // Textbox
    #define W_G07    30
    #define H_G07    H_TBX
    #define X_G07    X_G06 + W_G06 + IX
    #define Y_G07    Y_G06
    #pragma endregion

    #pragma region Lightness Threshold
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

    // Label
    #define W_G10    10
    #define H_G10    H_LBL
    #define X_G10    X_G09 + W_G09 + IX
    #define Y_G10    Y_G08
    #pragma endregion

    #pragma region Color Order

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

    #pragma endregion

    // Checkbox: Show artwork on background
    #define W_G02    160
    #define H_G02    H_CHB
    #define X_G02    X_B06 + 5
    #define Y_G02    Y_G12 + H_G12 + IY

    #pragma region Fit mode

    // Label: Fit mode
    #define W_G30    66
    #define H_G30    H_LBL
    #define X_G30    X_G02
    #define Y_G30    Y_G02 + H_G02 + IY

    // Combobox: Fit mode
    #define W_G31    86
    #define H_G31    H_CBX
    #define X_G31    X_G30 + W_G30 + IX
    #define Y_G31    Y_G30

    // Checkbox: Fit window
    #define W_G32    60
    #define H_G32    H_CHB
    #define X_G32    X_G31
    #define Y_G32    Y_G31 + H_G31 + IY

    #pragma endregion

    #pragma region Artwork Opacity
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
    #pragma endregion

    #pragma region Script
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
    #pragma endregion

#define W_B06  184
#define H_B06   11 + H_G07 + IY + H_G09 + IY + H_G12 + IY + H_G02 + IY + H_G31 + IY + H_G32 + IY + H_G04 + IY + H_G14 + 7

#pragma endregion

/** Page: Graphs **/

#pragma region Graphs

// ListBox: Graphs Settings
#define W_G20    70
#define H_G20   160
#define X_G20   X_D01 + W_D01 + IX
#define Y_G20   Y_D01

    // Button: Add
    #define W_G21   20
    #define H_G21   H_BTN
    #define X_G21   X_G20 + W_G20 + IX
    #define Y_G21   Y_G20

    // Button: Remove
    #define W_G22   20
    #define H_G22   H_BTN
    #define X_G22   X_G21
    #define Y_G22   Y_G21 + H_G21 + IY

// Checkbox: Vertical Layout
#define W_G23   70
#define H_G23   H_CHB
#define X_G23   X_G20
#define Y_G23   Y_G20 + H_G20 + IY

// Label: Description
#define W_G24   38
#define H_G24   H_LBL
#define X_G24   X_G21 + W_G21 + IX
#define Y_G24   Y_G21

// Editbox: Description
#define W_G25    80
#define H_G25    H_TBX
#define X_G25    X_G24 + W_G24 + IX
#define Y_G25    Y_G24

#pragma region Layout
// Groupbox
#define X_B16   X_G24
#define Y_B16   Y_G25 + H_G25 + IY

    // Label: Horizontal Alignment
    #define W_G33    66
    #define H_G33    H_LBL
    #define X_G33    X_B16 + 5
    #define Y_G33    Y_B16 + 11

    // Combobox: Horizontal Alignment
    #define W_G34    82
    #define H_G34    H_CBX
    #define X_G34    X_G33 + W_G33 + IX
    #define Y_G34    Y_G33

    // Label: Vertical Alignment
    #define W_G35    66
    #define H_G35    H_LBL
    #define X_G35    X_G33
    #define Y_G35    Y_G34 + H_G34 + IY

    // Combobox: Vertical Alignment
    #define W_G36    82
    #define H_G36    H_CBX
    #define X_G36    X_G35 + W_G35 + IX
    #define Y_G36    Y_G35

    // Checkbox: Flip horizontally
    #define W_G26   80
    #define H_G26   H_CHB
    #define X_G26   X_G33
    #define Y_G26   Y_G34 + H_G34 + IY

    // Checkbox: Flip vertically
    #define W_G27   80
    #define H_G27   H_CHB
    #define X_G27   X_G26 + W_G26 + IX
    #define Y_G27   Y_G26

#define W_B16  170
#define H_B16   11 + H_G34 + IY + H_G26 + 7
#pragma endregion

#pragma region X axis
// Groupbox
#define X_B02   X_B16
#define Y_B02   Y_B16 + H_B16 + IY

    #pragma region X axis

    // Label
    #define W_A05    66
    #define H_A05    H_LBL
    #define X_A05    X_B02 + 5
    #define Y_A05    Y_B02 + 11

    // Combobox
    #define W_A06    82
    #define H_A06    H_CBX
    #define X_A06    X_A05 + W_A05 + IX
    #define Y_A06    Y_A05

    // Checkbox: Top
    #define W_G15    39
    #define H_G15    H_CHB
    #define X_G15    X_A06
    #define Y_G15    Y_A06 + H_A06 + IY

    // Checkbox: Bottom
    #define W_G16    39
    #define H_G16    H_CHB
    #define X_G16    X_G15 + W_G15 + IX
    #define Y_G16    Y_G15

    #pragma endregion

#define W_B02  170
#define H_B02   11 + H_A06 + IY + H_G15 + 7
#pragma endregion

#pragma region Y axis
// Groupbox
#define X_B03   X_B02
#define Y_B03   Y_B02 + H_B02 + IY

    #pragma region Y axis

    // Label
    #define W_A07    66
    #define H_A07    H_LBL
    #define X_A07    X_B03 + 5
    #define Y_A07    Y_B03 + 11

    // Combobox
    #define W_A08    82
    #define H_A08    H_CBX
    #define X_A08    X_A07 + W_A07 + IX
    #define Y_A08    Y_A07

    // Checkbox: Left
    #define W_G17    39
    #define H_G17    H_CHB
    #define X_G17    X_A08
    #define Y_G17    Y_A08 + H_A08 + IY

    // Checkbox: Right
    #define W_G18    39
    #define H_G18    H_CHB
    #define X_G18    X_G17 + W_G17 + IX
    #define Y_G18    Y_G17

    #pragma endregion

    #pragma region Amplitude range: [Lo] - [Hi] dB
    // Label
    #define W_A45    54
    #define H_A45    H_LBL
    #define X_A45    X_A07
    #define Y_A45    Y_G17 + H_G17 + IY

    // Textbox (Lo)
    #define W_A46    40
    #define H_A46    H_TBX
    #define X_A46    X_A45 + W_A45 + IX
    #define Y_A46    Y_A45

    // Label
    #define W_A47    2
    #define H_A47    H_LBL
    #define X_A47    X_A46 + W_A46 + IX
    #define Y_A47    Y_A46

    // Textbox (Hi)
    #define W_A48    40
    #define H_A48    H_TBX
    #define X_A48    X_A47 + W_A47 + IX
    #define Y_A48    Y_A47

    // Label (db)
    #define W_A49    8
    #define H_A49    H_LBL
    #define X_A49    X_A48 + W_A48 + IX
    #define Y_A49    Y_A48
    #pragma endregion

    #pragma region Amplitude increment
    // Label
    #define W_A84    54
    #define H_A84    H_LBL
    #define X_A84    X_A45
    #define Y_A84    Y_A45 + H_A45 + IY + 8

    // Textbox (Lo)
    #define W_A85    40
    #define H_A85    H_TBX
    #define X_A85    X_A84 + W_A84 + IX
    #define Y_A85    Y_A84

    // Label
    #define W_A86    8
    #define H_A86    H_LBL
    #define X_A86    X_A85 + W_A85 + IX
    #define Y_A86    Y_A85
    #pragma endregion

    #pragma region Use absolute
    // Checkbox
    #define W_A50    100
    #define H_A50    H_CHB
    #define X_A50    X_A85
    #define Y_A50    Y_A85 + H_A85 + IY
    #pragma endregion

    #pragma region Gamma
    // Label
    #define W_A41    54
    #define H_A41    H_LBL
    #define X_A41    X_A07
    #define Y_A41    Y_A50 + H_A50 + IY

    // Textbox
    #define W_A42    30
    #define H_A42    H_TBX
    #define X_A42    X_A41 + W_A41 + IX
    #define Y_A42    Y_A41
    #pragma endregion

#define W_B03   170
#define H_B03   11 + H_A08 + IY + H_G17 + IY + H_A85 + IY + H_A46 + IY + H_A50 + IY + H_A42 + 7

// ListBox: Channels
#define W_G28    70
#define H_G28   160
#define X_G28   X_B02 + W_B02 + IX
#define Y_G28   Y_G24
#pragma endregion

#pragma endregion

/** Page: Visualization **/

#pragma region Visualization

// Label
#define W_E01    24
#define H_E01    H_LBL
#define X_E01    X_D01 + W_D01 + IX
#define Y_E01    Y_D01

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

#define W_B07  W_B13
#define H_B07  11 + H_C12 + IY + H_C18 + IY + H_C20 + 7
#pragma endregion

#pragma region Spectogram
// Groupbox
#define X_B08   X_E01
#define Y_B08   Y_B07 + H_B07 + IY

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

#define W_B08   W_B07
#define H_B08   11 + H_C15 + IY + H_C27 + IY + H_C28 + 7
#pragma endregion

#pragma region Radial Bars

// Groupbox
#define X_B15   X_B08
#define Y_B15   Y_B08 + H_B08 + IY

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

#define W_B15   W_B08
#define H_B15   11 + H_C31 + IY + H_C33 + IY + H_C35 + 7

#pragma endregion

#pragma region Level Meter
// Groupbox
#define X_B14   X_B13 + W_B13 + IX
#define Y_B14   Y_B13

    #pragma region Channel Pairs
    // Label
    #define W_C50    46
    #define H_C50    H_LBL
    #define X_C50    X_B14 + 5
    #define Y_C50    Y_B14 + 11

    // Combobox
    #define W_C51    76
    #define H_C51    H_CBX
    #define X_C51    X_C50 + W_C50 + IX
    #define Y_C51    Y_C50
    #pragma endregion

    // Checkbox: Horizontal
    #define W_C52    60
    #define H_C52    H_CHB
    #define X_C52    X_C51
    #define Y_C52    Y_C51 + H_C51 + IY

#define W_B14  5 + W_C50 + IX + W_C51 + 5
#define H_B14  11 + H_C51 + IY + H_C52 + 7
#pragma endregion

#pragma region Peak Meter
// Groupbox
#define X_B12   X_B14
#define Y_B12   Y_B14 + H_B14 + IY

    // Checkbox: Horizontal
    #define W_C16    80
    #define H_C16    H_CHB
    #define X_C16    X_B12 + 5
    #define Y_C16    Y_B12 + 11

   // Checkbox: RMS +3
    #define W_C24    80
    #define H_C24    H_CHB
    #define X_C24    X_C16
    #define Y_C24    Y_C16 + 11

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

    // Label: Gauge gap
    #define W_C25   46
    #define H_C25   H_LBL
    #define X_C25   X_C21
    #define Y_C25   Y_C22 + H_C22 + IY

    // Text Box
    #define W_C26   34
    #define H_C26   H_TBX
    #define X_C26   X_C25 + W_C25 + IX
    #define Y_C26   Y_C25

#define W_B12  W_B14
#define H_B12  11 + H_C16 + IY + H_C24 + IY + H_C22 + IY + H_C26 + 7
#pragma endregion

#pragma endregion

/** Page: Styles **/

#pragma region Styles

// Listbox
#define W_I01    112
#define H_I01    H_D01
#define X_I01    X_D01 + W_D01 + IX
#define Y_I01    Y_D01

// Label: Source
#define W_I02    48
#define H_I02    H_LBL
#define X_I02    X_I01 + W_I01 + IX
#define Y_I02    Y_I01

// Combobox: Source
#define W_I03    82
#define H_I03    H_CBX
#define X_I03    X_I02 + W_I02 + IX
#define Y_I03    Y_I02

// Label: Index
#define W_I04    48
#define H_I04    H_LBL
#define X_I04    X_I02
#define Y_I04    Y_I03 + H_I03 + IY

// Combobox: Index
#define W_I05    82
#define H_I05    H_CBX
#define X_I05    X_I04 + W_I04 + IX
#define Y_I05    Y_I04

// Label: Color
#define W_I06   48
#define H_I06   H_LBL
#define X_I06   X_I04
#define Y_I06   Y_I05 + H_I05 + IY

// Button: Color
#define W_I07   40
#define H_I07   H_BTN
#define X_I07   X_I06 + W_I06 + IX
#define Y_I07   Y_I06

#pragma region Color Scheme
// Label
#define W_A09    48
#define H_A09    H_LBL
#define X_A09    X_I06
#define Y_A09    Y_I07 + H_I07 + IY

// Combobox
#define W_A10    86
#define H_A10    H_CBX
#define X_A10    X_A09 + W_A09 + IX
#define Y_A10    Y_A09
#pragma endregion

#pragma region Gradient
// Label
#define W_A67   14
#define H_A67   100
#define X_A67   X_A10 - W_A67 - IX
#define Y_A67   Y_A10 + H_A10 + IY

// Listbox
#define W_A68   45
#define H_A68   100
#define X_A68   X_A10
#define Y_A68   Y_A67

// Button
#define W_A69   W_BTN
#define H_A69   H_BTN
#define X_A69   X_A68 + W_A68 + IY
#define Y_A69   Y_A68

// Button
#define W_A70   W_BTN
#define H_A70   H_BTN
#define X_A70   X_A69
#define Y_A70   Y_A69 + H_A69 + IY

// Button
#define W_A71   W_BTN
#define H_A71   H_BTN
#define X_A71   X_A70
#define Y_A71   Y_A70 + H_A70 + IY

// Position
#define W_F01   40
#define H_F01   H_TBX
#define X_F01   X_A71
#define Y_F01   Y_A71 + H_A71 + IY

// Position Label
#define W_F02   10
#define H_F02   H_LBL
#define X_F02   X_F01 + W_F01 + IX
#define Y_F02   Y_F01

// Spread
#define W_F03   W_BTN
#define H_F03   H_BTN
#define X_F03   X_F01
#define Y_F03   Y_F01 + H_F01 + IY

// Checkbox: Horizontal gradient
#define W_C13    80
#define H_C13    H_CHB
#define X_C13    X_F03
#define Y_C13    Y_F03 + H_F03 + IY

// Checkbox: Amplitude-based
#define W_C14    80
#define H_C14    H_CHB
#define X_C14    X_C13 + W_C13 + IX
#define Y_C14    Y_C13

#pragma endregion

// Label: Opacity
#define W_I08   48
#define H_I08   H_LBL
#define X_I08   X_I06
#define Y_I08   Y_A68 + H_A68 + IY

// Button: Opacity
#define W_I09   40
#define H_I09   H_BTN
#define X_I09   X_I08 + W_I08 + IX
#define Y_I09   Y_I08

// lABEL: Opacity Unit
#define W_I10   10
#define H_I10   H_LBL
#define X_I10   X_I09 + W_I09 + IX
#define Y_I10   Y_I09

// Label: Thickness
#define W_I11   48
#define H_I11   H_LBL
#define X_I11   X_I08
#define Y_I11   Y_I09 + H_I09 + IY

// Button: Thickness
#define W_I12   40
#define H_I12   H_BTN
#define X_I12   X_I11 + W_I11 + IX
#define Y_I12   Y_I11

// Label: Font Name
#define W_I13   48
#define H_I13   H_LBL
#define X_I13   X_I11
#define Y_I13   Y_I12 + H_I12 + IY

// Button: Font Name
#define W_I14   100
#define H_I14   H_BTN
#define X_I14   X_I13 + W_I13 + IX
#define Y_I14   Y_I13

// Button: Font Select
#define W_I15   16
#define H_I15   H_BTN
#define X_I15   X_I14 + W_I14 + IX
#define Y_I15   Y_I13

// Label: Font Size
#define W_I16   48
#define H_I16   H_LBL
#define X_I16   X_I13
#define Y_I16   Y_I14 + H_I14 + IY

// Button: Font Size
#define W_I17   40
#define H_I17   H_BTN
#define X_I17   X_I16 + W_I16 + IX
#define Y_I17   Y_I16

#pragma endregion

/** Page: Presets **/

#pragma region Presets

// Label: Directory
#define W_J01   34
#define H_J01   H_LBL
#define X_J01   X_D01 + W_D01 + IX
#define Y_J01   Y_D01

// Edit Box
#define W_J02   200
#define H_J02   H_TBX
#define X_J02   X_J01 + W_J01 + IX
#define Y_J02   Y_J01

// Button
#define W_J03   16
#define H_J03   H_BTN
#define X_J03   X_J02 + W_J02 + IX
#define Y_J03   Y_J02

// List Box
#define W_J04   W_J02
#define H_J04   200
#define X_J04   X_J02
#define Y_J04   Y_J02 + H_J02 + IY

// Label: File Name
#define W_J05   W_J01
#define H_J05   H_LBL
#define X_J05   X_J01
#define Y_J05   Y_J04 + H_J04 + IY

// Edit Box
#define W_J06   W_J04
#define H_J06   H_TBX
#define X_J06   X_J05 + W_J05 + IX
#define Y_J06   Y_J05

// Button: Delete
#define W_J09   W_BTN
#define H_J09   H_BTN
#define X_J09   X_J06 + W_J06 - W_BTN
#define Y_J09   Y_J06 + H_J06 + IY

// Button: Save
#define W_J08   W_BTN
#define H_J08   H_BTN
#define X_J08   X_J09 - IX - W_J09
#define Y_J08   Y_J09

// Button: Load
#define W_J07   W_BTN
#define H_J07   H_BTN
#define X_J07   X_J08 - IX - W_J08
#define Y_J07   Y_J08

#pragma endregion

// Cancel button (right-most button)
#define W_A99   W_BTN
#define H_A99   H_BTN
#define X_A99   W_A00 - W_A99 - DX
#define Y_A99   H_A00 - H_A99 - DY

// OK button
#define W_A98   W_BTN
#define H_A98   H_BTN
#define X_A98   X_A99 - W_A99 - IX
#define Y_A98   Y_A99

// Reset button
#define W_A97   W_BTN
#define H_A97   H_BTN
#define X_A97   X_A98 - W_A98 - IX
#define Y_A97   Y_A98
