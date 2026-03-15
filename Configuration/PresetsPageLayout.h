
/** $VER: PresetsPageLayout.h (2026.02.22) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

#define W_J00    W_A00 - IX - W_D01 - IX - IX
#define H_J00    H_A00 - IY - IY - H_BTN - IY

// Label: Location
#define W_J01   34
#define H_J01   H_LBL
#define X_J01   0
#define Y_J01   0

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
#define H_J04   231
#define X_J04   X_J02
#define Y_J04   Y_J02 + H_J02 + IY

// Label: Name
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
