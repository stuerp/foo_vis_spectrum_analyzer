
/** $VER: Module.h (2026.08.16) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class module_t
{
public:
    module_t(const WCHAR * libraryName)
    {
        _Handle = ::LoadLibraryW(libraryName);
    }

    virtual ~module_t()
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
