
/** $VER: StylesPageLayout.h (2026.02.22) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "NewConfigurationDialogLayout.h"

#define W_I00    W_A00 - IX - W_D01 - IX - IX
#define H_I00    H_A00 - IY - IY - H_BTN - IY

// Listbox
#define W_I01    112
#define H_I01    H_I00
#define X_I01    0
#define Y_I01    0

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
#define W_C13    70
#define H_C13    H_CHB
#define X_C13    X_F03
#define Y_C13    Y_F03 + H_F03 + IY

// Checkbox: Amplitude-based
#define W_C14    64
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
