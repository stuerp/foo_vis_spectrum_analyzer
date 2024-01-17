
/** $VER: SafeModuleHandle.h (2024.01.01) P. Stuer **/

#pragma once

#include "framework.h"

class SafeModuleHandle
{
public:
    SafeModuleHandle(const WCHAR * libraryName)
    {
        _Handle = ::LoadLibraryW(libraryName);
    }

    virtual ~SafeModuleHandle()
    {
        if (_Handle != nullptr)
        {
            ::FreeLibrary(_Handle);
            _Handle = nullptr;
        }
    }

    void * GetFunctionAddress(const char * functionName)
    {
        return (_Handle != nullptr) ? ::GetProcAddress(_Handle, functionName) : nullptr;
    }

private:
    HMODULE _Handle;
};
