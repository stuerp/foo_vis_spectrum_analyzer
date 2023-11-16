
/** $VER: Layout.h (2023.11.16) P. Stuer - Defines the layout of the configuration dialog. **/

#pragma once

#define H_LA        8  // Label

#define W_BT        50 // Button
#define H_BT        14 // Button

#define H_EB        14 // Edit box
#define H_CB        14 // Combo box

#define W_A00      400 // Dialog width (in dialog units)
#define H_A00      200 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

#pragma region("Frequencies")
// Label
#define X_A01    DX
#define Y_A01    DY
#define W_A01    42
#define H_A01    H_LA

// Combobox
#define W_A02    60
#define H_A02    H_CB
#define X_A02    X_A01 + W_A01 + DX
#define Y_A02    Y_A01
#pragma endregion

#pragma region("FFT Size")
// Label
#define X_A03    X_A01
#define Y_A03    Y_A02 + H_A02 + IY
#define W_A03    42
#define H_A03    H_LA

// Combobox
#define W_A04    60
#define H_A04    H_CB
#define X_A04    X_A03 + W_A03 + DX
#define Y_A04    Y_A03
#pragma endregion

#pragma region("X Axis")
// Label
#define X_A05    X_A01
#define Y_A05    Y_A04 + H_A04 + IY
#define W_A05    42
#define H_A05    H_LA

// Combobox
#define W_A06    60
#define H_A06    H_CB
#define X_A06    X_A05 + W_A05 + DX
#define Y_A06    Y_A05
#pragma endregion

#pragma region("Y Axis")
// Label
#define X_A07    X_A01
#define Y_A07    Y_A06 + H_A06 + IY
#define W_A07    42
#define H_A07    H_LA

// Combobox
#define W_A08    60
#define H_A08    H_CB
#define X_A08    X_A07 + W_A07 + DX
#define Y_A08    Y_A07
#pragma endregion

#pragma region("Color Scheme")
// Label
#define X_A09    X_A07
#define Y_A09    Y_A08 + H_A08 + IY
#define W_A09    42
#define H_A09    H_LA

// Combobox
#define W_A10    80
#define H_A10    H_CB
#define X_A10    X_A09 + W_A09 + DX
#define Y_A10    Y_A09
#pragma endregion

#pragma region("Peak Mode")
// Label
#define X_A11    X_A09
#define Y_A11    Y_A10 + H_A10 + IY
#define W_A11    42
#define H_A11    H_LA

// Combobox
#define W_A12    60
#define H_A12    H_CB
#define X_A12    X_A11 + W_A11 + DX
#define Y_A12    Y_A11
#pragma endregion

// Cancel button (right-most button)
#define X_A98   W_A00 - W_BT - DX
#define Y_A98   H_A00 - H_BT - DY
#define W_A98   W_BT
#define H_A98   H_BT

// OK button
#define X_A99   X_A98 - W_BT - IX
#define Y_A99   Y_A98
#define W_A99   W_BT
#define H_A99   H_BT
