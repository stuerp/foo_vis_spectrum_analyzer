
/** $VER: GraphsPageLayout.h (2026.03.18) P. Stuer - Defines the layout of a configuration dialog page. **/

#pragma once

#include "ConfigurationDialogLayout.h"

// ListBox: Graphs Settings
#define W_G20    70
#define H_G20   160
#define X_G20   0
#define Y_G20   0

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

/** Layout Groupbox **/

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
    #define W_G26   76
    #define H_G26   H_CHB
    #define X_G26   X_G33
    #define Y_G26   Y_G34 + H_G34 + IY

    // Checkbox: Flip vertically
    #define W_G27   76
    #define H_G27   H_CHB
    #define X_G27   X_G26 + W_G26 + IX
    #define Y_G27   Y_G26

#define W_B16  170
#define H_B16   11 + H_G34 + IY + H_G26 + 7

/** X axis Groupbox **/

#define X_B02   X_B16
#define Y_B02   Y_B16 + H_B16 + IY

    // Label: Mode
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

    // Label
    #define W_A51    66
    #define H_A51    H_LBL
    #define X_A51    X_A05
    #define Y_A51    Y_G15 + H_G15 + IY

    // Textbox: Decimals
    #define W_A52    20
    #define H_A52    H_TBX
    #define X_A52    X_A51 + W_A51 + IX
    #define Y_A52    Y_A51

#define W_B02  170
#define H_B02   11 + H_A06 + IY + H_G15 + IY + H_A52 + 7

/** Y axis Groupbox **/

#define X_B03   X_B02
#define Y_B03   Y_B02 + H_B02 + IY

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

    /** Amplitude range: [Lo] - [Hi] dB **/

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

    /** Amplitude increment **/

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

    /** Use absolute **/

    // Checkbox
    #define W_A50    100
    #define H_A50    H_CHB
    #define X_A50    X_A85
    #define Y_A50    Y_A85 + H_A85 + IY

    /** Gamma **/

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

#define W_B03   170
#define H_B03   11 + H_A08 + IY + H_G17 + IY + H_A85 + IY + H_A46 + IY + H_A50 + IY + H_A42 + 7

// Label: Channels
#define W_G28    46
#define H_G28    H_LBL
#define X_G28    X_B02 + W_B02 + IX
#define Y_G28    Y_G24

// ListBox: Channels
#define W_G29    80
#define H_G29   160
#define X_G29   X_G28
#define Y_G29   Y_G28 + H_G28 + IY

// Button: All Channels
#define W_G52   30
#define H_G52   H_BTN
#define X_G52   X_G29
#define Y_G52   Y_G29 + H_G29 + IY

// Button: No Channels
#define W_G53   30
#define H_G53   H_BTN
#define X_G53   X_G29 + W_G29 - W_G53
#define Y_G53   Y_G52

// Label: Channel Pairs
#define W_C50   46
#define H_C50   H_LBL
#define X_C50   X_G52
#define Y_C50   Y_G52 + H_G52 + IY

// Combobox: Channel Pairs
#define W_C51   76
#define H_C51   H_CBX
#define X_C51   X_C50
#define Y_C51   Y_C50 + H_C50 + IY

// Checkbox: Swap channels
#define W_C53   60
#define H_C53   H_CBX
#define X_C53   X_C51
#define Y_C53   Y_C51 + H_C51 + IY
