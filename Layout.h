
/** $VER: Layout.h (2024.01.20) P. Stuer - Defines the layout of the configuration dialog. **/

#pragma once

#define H_LBL        8  // Label

#define W_BTN       50 // Button
#define H_BTN       14 // Button

#define H_TBX       14 // Edit box
#define H_CBX       14 // Combo box

#define W_CHB       10 // Check box
#define H_CHB       10 // Check box

#define W_A00      442 // Dialog width (in dialog units)
#define H_A00      308 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

#define W_D01   60
#define H_D01   H_A00 - DY - DY
#define X_D01   DX
#define Y_D01   DY

#pragma region Transform
// Groupbox
#define X_B05   X_D01 + W_D01 + IX
#define Y_B05   Y_D01

    #pragma region Method
    // Label
    #define W_A62    62
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
    // Label
    #define W_C01    62
    #define H_C01    H_LBL
    #define X_C01    X_A62
    #define Y_C01    Y_A63 + H_A63 + IY

    // Combobox
    #define W_C02    82
    #define H_C02    H_CBX
    #define X_C02    X_C01 + W_C01 + IX
    #define Y_C02    Y_C01

    // Label
    #define W_C03    62
    #define H_C03    H_LBL
    #define X_C03    X_C01
    #define Y_C03    Y_C02 + H_C02 + IY

    // Textbox
    #define W_C04    30
    #define H_C04    H_CBX
    #define X_C04    X_C03 + W_C03 + IX
    #define Y_C04    Y_C03

    // Label
    #define W_C05    62
    #define H_C05    H_LBL
    #define X_C05    X_C03
    #define Y_C05    Y_C04 + H_C04 + IY

    // Textbox
    #define W_C06    30
    #define H_C06    H_CBX
    #define X_C06    X_C05 + W_C05 + IX
    #define Y_C06    Y_C05
    #pragma endregion

    #pragma region Channels
    // Button
    #define W_A88    82
    #define H_A88    H_BTN
    #define X_A88    X_C06
    #define Y_A88    Y_C06 + H_C06 + IY
    #pragma endregion

#define W_B05   174
#define H_B05   11 + H_A63 + IY + H_C02 + IY + H_C04 + IY + H_C06 + IY + H_A88 + 7
#pragma endregion

#pragma region FFT
// Groupbox
#define X_B00   X_B05
#define Y_B00   Y_B05 + H_B05 + IY

    #pragma region Size
    // Label
    #define W_A03   60
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
    #define W_A61   60
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
    #define W_A15    60
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
    #define W_A65    60
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
    #define W_A39    60
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

#pragma region Frequencies
// Groupbox
#define X_B01   X_B05 + W_B05 + IX
#define Y_B01   Y_B05

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

#pragma region Common
// Groupbox
#define X_B04   X_D01 + W_D01 + IX
#define Y_B04   Y_D01

    #pragma region Color Scheme
    // Label
    #define W_A09    64
    #define H_A09    H_LBL
    #define X_A09    X_B04 + 5
    #define Y_A09    Y_B04 + 11

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
    #define W_F01    40
    #define H_F01    H_TBX
    #define X_F01    X_A71
    #define Y_F01    Y_A71 + H_A71 + IY

    // Position Label
    #define W_F02    10
    #define H_F02    H_LBL
    #define X_F02    X_F01 + W_F01 + IX
    #define Y_F02    Y_F01

    // Spread
    #define W_F03   W_BTN
    #define H_F03   H_BTN
    #define X_F03   X_F01
    #define Y_F03   Y_F01 + H_F01 + IY

    #pragma endregion

    #pragma region Artwork Colors
    // Label
    #define W_G06    64
    #define H_G06    H_LBL
    #define X_G06    X_A09
    #define Y_G06    Y_A67 + H_A67 + IY

    // Textbox
    #define W_G07    30
    #define H_G07    H_TBX
    #define X_G07    X_G06 + W_G06 + IX
    #define Y_G07    Y_G06
    #pragma endregion

    #pragma region Lightness Threshold
    // Label
    #define W_G08    64
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
    #define W_G11    64
    #define H_G11    H_LBL
    #define X_G11    X_G08
    #define Y_G11    Y_G09 + H_G09 + IY

    // Combobox
    #define W_G12    86
    #define H_G12    H_CBX
    #define X_G12    X_G11 + W_G11 + IX
    #define Y_G12    Y_G11
    #pragma endregion

    #pragma region Smoothing Method
    // Label
    #define W_A17    64
    #define H_A17    H_LBL
    #define X_A17    X_G11
    #define Y_A17    Y_G12 + H_G12 + IY

    // Combobox
    #define W_A18    60
    #define H_A18    H_CBX
    #define X_A18    X_A17 + W_A17 + IX
    #define Y_A18    Y_A17
    #pragma endregion

    #pragma region Smoothing Factor
    // Label
    #define W_A37    64
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
    #define W_A87    80
    #define H_A87    H_CHB
    #define X_A87    X_A38
    #define Y_A87    Y_A38 + H_A38 + IY
    #pragma endregion

    #pragma region Background Mode
    // Label
    #define W_G01    64
    #define H_G01    H_LBL
    #define X_G01    X_A37
    #define Y_G01    Y_A87 + H_A87 + IY

    // Combobox
    #define W_G02    100
    #define H_G02    H_CBX
    #define X_G02    X_G01 + W_G01 + IX
    #define Y_G02    Y_G01
    #pragma endregion

    #pragma region Artwork Opacity
    // Label
    #define W_G03    64
    #define H_G03    H_LBL
    #define X_G03    X_G01
    #define Y_G03    Y_G02 + H_G02 + IY

    // Textbox
    #define W_G04    30
    #define H_G04    H_TBX
    #define X_G04    X_G03 + W_G03 + IX
    #define Y_G04    Y_G03

    // Label
    #define W_G05    10
    #define H_G05    H_LBL
    #define X_G05    X_G04 + W_G04 + IX
    #define Y_G05    Y_G03
    #pragma endregion

#define W_B04  180 // 5 + W_A09 + IX + W_A10  + 5
#define H_B04   11 + H_A10 + IY + H_A68 + IY + H_G07 + IY + H_G09 + IY + H_G12 + IY + H_A18 + IY + H_A38 + IY + H_A87 + IY + H_G02 + IY + H_G04 + 7
#pragma endregion

#pragma region X axis
// Groupbox
#define X_B02   X_B04 + W_B04 + IX
#define Y_B02   DY

    #pragma region X axis
    // Label
    #define W_A05    60
    #define H_A05    H_LBL
    #define X_A05    X_B02 + 5
    #define Y_A05    Y_B02 + 11

    // Combobox
    #define W_A06    82
    #define H_A06    H_CBX
    #define X_A06    X_A05 + W_A05 + IX
    #define Y_A06    Y_A05
    #pragma endregion

#define W_B02  176 // 5 + W_A05 + IX + W_A06 + 5
#define H_B02   11 + H_A06 + 7
#pragma endregion

#pragma region Y axis
// Groupbox
#define X_B03   X_B02
#define Y_B03   Y_B02 + H_B02 + IY

    #pragma region Y axis
    // Label
    #define W_A07    60
    #define H_A07    H_LBL
    #define X_A07    X_B03 + 5
    #define Y_A07    Y_B03 + 11

    // Combobox
    #define W_A08    82
    #define H_A08    H_CBX
    #define X_A08    X_A07 + W_A07 + IX
    #define Y_A08    Y_A07
    #pragma endregion

    #pragma region Amplitude range: [Lo] - [Hi] dB
    // Label
    #define W_A45    60
    #define H_A45    H_LBL
    #define X_A45    X_A07
    #define Y_A45    Y_A08 + H_A08 + IY

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
    #define W_A84    60
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
    #define W_A41    60
    #define H_A41    H_LBL
    #define X_A41    X_A07
    #define Y_A41    Y_A50 + H_A50 + IY

    // Textbox
    #define W_A42    30
    #define H_A42    H_TBX
    #define X_A42    X_A41 + W_A41 + IX
    #define Y_A42    Y_A41
    #pragma endregion

#define W_B03   176 //  5 + W_A45 + IX + W_A46 + IX + W_A47 + IX + W_A48 + IX + W_A49 + 5
#define H_B03   11 + H_A08 + IY + H_A85 + IY + H_A46 + IY + H_A50 + IY + H_A42 + 7
#pragma endregion

#pragma region Colors
// Groupbox
#define X_B06   X_B03
#define Y_B06   Y_B03 + H_B03 + IY

    // Label (Back)
    #define W_A72   60
    #define H_A72   H_LBL
    #define X_A72   X_B06 + 5
    #define Y_A72   Y_B06 + 11

    // Button
    #define W_A73   40
    #define H_A73   H_BTN
    #define X_A73   X_A72 + W_A72 + IX
    #define Y_A73   Y_A72

    // Checkbox
    #define W_C07   W_CHB
    #define H_C07   H_CHB
    #define X_C07   X_A73 + W_A73 + IX
    #define Y_C07   Y_A73 + 2

    // Label (X Text)
    #define W_A74   60
    #define H_A74   H_LBL
    #define X_A74   X_A72
    #define Y_A74   Y_A73 + H_A73 + IY

    // Button
    #define W_A75   40
    #define H_A75   H_BTN
    #define X_A75   X_A74 + W_A74 + IX
    #define Y_A75   Y_A74

    // Checkbox
    #define W_C08   W_CHB
    #define H_C08   H_CHB
    #define X_C08   X_A75 + W_A75 + IX
    #define Y_C08   Y_A75 + 2

    // Label (X Line)
    #define W_A76   60
    #define H_A76   H_LBL
    #define X_A76   X_A74
    #define Y_A76   Y_A75 + H_A75 + IY

    // Button
    #define W_A77   40
    #define H_A77   H_BTN
    #define X_A77   X_A76 + W_A76 + IX
    #define Y_A77   Y_A76

    // Checkbox
    #define W_C09   W_CHB
    #define H_C09   H_CHB
    #define X_C09   X_A77 + W_A77 + IX
    #define Y_C09   Y_A77 + 2

    // Label (Y Text)
    #define W_A78   60
    #define H_A78   H_LBL
    #define X_A78   X_A76
    #define Y_A78   Y_A77 + H_A77 + IY

    // Button
    #define W_A79   40
    #define H_A79   H_BTN
    #define X_A79   X_A78 + W_A78 + IX
    #define Y_A79   Y_A78

    // Checkbox
    #define W_C10   W_CHB
    #define H_C10   H_CHB
    #define X_C10   X_A79 + W_A79 + IX
    #define Y_C10   Y_A79 + 2

    // Label (Y Line)
    #define W_A80   60
    #define H_A80   H_LBL
    #define X_A80   X_A78
    #define Y_A80   Y_A79 + H_A79 + IY

    // Button
    #define W_A81   40
    #define H_A81   H_BTN
    #define X_A81   X_A80 + W_A80 + IX
    #define Y_A81   Y_A80

    // Checkbox
    #define W_C11   W_CHB
    #define H_C11   H_CHB
    #define X_C11   X_A81 + W_A81 + IX
    #define Y_C11   Y_A81 + 2

#define W_B06  176 // 5 + W_A72 + IX + W_A73 + IX + W_CHB + 5
#define H_B06  11 + H_A73 + IY + H_A75 + IY + H_A77 + IY + H_A79 + IY + H_A81 + 5
#pragma endregion

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

#pragma region Horizontal gradient
// Checkbox
#define W_C13    80
#define H_C13    H_CHB
#define X_C13    X_E01 + 5
#define Y_C13    Y_E02 + H_E02 + IY
#pragma endregion

#pragma region Bars
// Groupbox
#define X_B07   X_E01
#define Y_B07   Y_C13 + H_C13 + IY

    #pragma region Draw band background
    // Checkbox
    #define W_A57    80
    #define H_A57    H_CHB
    #define X_A57    X_B07 + 5
    #define Y_A57    Y_B07 + 11
    #pragma endregion

    #pragma region LED mode
    // Checkbox
    #define W_C12    80
    #define H_C12    H_CHB
    #define X_C12    X_A57
    #define Y_C12    Y_A57 + H_A57 + IY
    #pragma endregion

    #pragma region Peak Mode
    // Label
    #define W_A11    42
    #define H_A11    H_LBL
    #define X_A11    X_C12
    #define Y_A11    Y_C12 + H_C12 + IY

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

    #pragma region White & Black keys
    // Label (White keys)
    #define W_A82   42
    #define H_A82   H_LBL
    #define X_A82   X_A53
    #define Y_A82   Y_A54 + H_A54 + IY

    // Button
    #define W_A83   40
    #define H_A83   H_BTN
    #define X_A83   X_A82 + W_A82 + IX
    #define Y_A83   Y_A82

    // Label (Black keys)
    #define W_C14   42
    #define H_C14   H_LBL
    #define X_C14   X_A82
    #define Y_C14   Y_A83 + H_A83 + IY

    // Button
    #define W_C15   40
    #define H_C15   H_BTN
    #define X_C15   X_C14 + W_C14 + IX
    #define Y_C15   Y_C14
    #pragma endregion

#define W_B07  176 // 5 + W_A09 + IX + W_A10  + 5
#define H_B07   11 + H_A57 + IY + H_C12 + IY + H_A12 + IY + H_A52 + IY + H_A54 + IY + H_A83 + IY + H_C15 + 7
#pragma endregion

#pragma region Curve
// Groupbox
#define X_B08   X_B07
#define Y_B08   Y_B07 + H_B07 + IY

    #pragma region Line Width
    // Label
    #define W_E03    42
    #define H_E03    H_LBL
    #define X_E03    X_B08 + 5
    #define Y_E03    Y_B08 + 11

    // Textbox
    #define W_E04    30
    #define H_E04    H_TBX
    #define X_E04    X_E03 + W_E03 + IX
    #define Y_E04    Y_E03
    #pragma endregion

    #pragma region Area Opacity
    // Label
    #define W_E05    42
    #define H_E05    H_LBL
    #define X_E05    X_E03
    #define Y_E05    Y_E04 + H_E04 + IY

    // Textbox
    #define W_E06    30
    #define H_E06    H_TBX
    #define X_E06    X_E05 + W_E05 + IX
    #define Y_E06    Y_E05

    // Label
    #define W_E07    10
    #define H_E07    H_LBL
    #define X_E07    X_E06 + W_E06 + IX
    #define Y_E07    Y_E06
    #pragma endregion

#define W_B08  W_B07
#define H_B08   11 + H_E04 + IY + H_E06 + 7
#pragma endregion

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
