
/** $VER: SafeModuleHandle.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

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
