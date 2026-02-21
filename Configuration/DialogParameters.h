
/** $VER: DialogParameters.h (2026.02.20) P. Stuer - Implements the configuration dialog. **/

#pragma once

#include "pch.h"

#include "State.h"

struct dialog_parameters_t
{
    HWND _hWnd;
    state_t * _State;
};
