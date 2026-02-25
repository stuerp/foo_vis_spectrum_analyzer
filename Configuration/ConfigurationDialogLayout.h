
/** $VER: ConfigurationDialogLayout.h (2026.02.22) P. Stuer - Defines the layout of the configuration dialog. **/

#pragma once

#define H_LBL        8 // Label

#define W_BTN       50 // Button
#define H_BTN       14 // Button

#define H_TBX       14 // Edit box
#define H_CBX       14 // Combo box

#define W_CHB       10 // Check box
#define H_CHB       10 // Check box

#define W_A00      442 // Dialog width (in dialog units)
#define H_A00      320 // Dialog height (in dialog units)

#define DX           7
#define DY           7

#define IX           4 // Spacing between two related controls
#define IY           4

// Menu List
#define W_D01   60
#define H_D01   H_A00 - DY - DY
#define X_D01   DX
#define Y_D01   DY

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
