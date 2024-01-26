
/** $VER: COMException.h (2024.01.17) P. Stuer **/

#pragma once

#include <atlbase.h>

#include <string>

struct COMException
{
    HRESULT hResult;
    std::wstring Message;

    COMException(HRESULT const value, const WCHAR * message = nullptr) : hResult(value), Message(message) {}
};
