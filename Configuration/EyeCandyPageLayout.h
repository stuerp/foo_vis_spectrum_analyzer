
/** $VER: EyeCandyPageLayout.h (2026.10.23) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

#define W_B00    W_A00 - IX - W_D01 - IX - IX
#define H_B00    H_A00 - IY - IY - H_BTN - IY

/* Effect Category */

// Label
#define W_B01    34
#define H_B01    H_LBL
#define X_B01    0
#define Y_B01    0

// Combobox
#define W_B02    86
#define H_B02    H_CBX
#define X_B02    X_B01 + W_B01 + IX
#define Y_B02    Y_B01

/* Effect */

// Label
#define W_B03    24
#define H_B03    H_LBL
#define X_B03    X_B02 + W_B02 + IX
#define Y_B03    Y_B02

// Combobox
#define W_B04    86
#define H_B04    H_CBX
#define X_B04    X_B03 + W_B03 + IX
#define Y_B04    Y_B03

/* Quality */

// Label
#define W_B05    34
#define H_B05    H_LBL
#define X_B05    X_B01
#define Y_B05    Y_B02 + H_B02 + IY

// Combobox
#define W_B06    86
#define H_B06    H_CBX
#define X_B06    X_B05 + W_B05 + IX
#define Y_B06    Y_B05

/* Speed */

// Label
#define W_B07    34
#define H_B07    H_LBL
#define X_B07    X_B05
#define Y_B07    Y_B06 + H_B06 + IY

// Combobox
#define W_B08    86
#define H_B08    H_CBX
#define X_B08    X_B07 + W_B07 + IX
#define Y_B08    Y_B07

/* Brightness */

// Label
#define W_B09    34
#define H_B09    H_LBL
#define X_B09    X_B07
#define Y_B09    Y_B08 + H_B08 + IY

// Combobox
#define W_B10    86
#define H_B10    H_CBX
#define X_B10    X_B09 + W_B09 + IX
#define Y_B10    Y_B09

/* Density */

// Label
#define W_B11    34
#define H_B11    H_LBL
#define X_B11    X_B09
#define Y_B11    Y_B10 + H_B10 + IY

// Combobox
#define W_B12    86
#define H_B12    H_CBX
#define X_B12    X_B11 + W_B11 + IX
#define Y_B12    Y_B11
