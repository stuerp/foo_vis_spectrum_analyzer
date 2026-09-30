
/** $VER: Module.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <string>
#include <system_error>
#include <utility>

class module_t final
{
public:
    explicit module_t(const wchar_t * libraryName) : _Handle(::LoadLibraryW(libraryName))
    {
        if (_Handle == nullptr)
            throw_last_error("LoadLibraryW failed");
    }

    explicit module_t(const std::wstring & libraryName) : module_t(libraryName.c_str())
    {
    }

    ~module_t() noexcept
    {
        Reset();
    }

    module_t(const module_t &) = delete;
    module_t& operator=(const module_t &) = delete;

    module_t(module_t && other) noexcept : _Handle(std::exchange(other._Handle, nullptr))
    {
    }

    module_t & operator=(module_t && other) noexcept
    {
        if (this != &other)
        {
            Reset();

            _Handle = std::exchange(other._Handle, nullptr);
        }

        return *this;
    }

    [[nodiscard]]
    FARPROC GetFunctionAddress(const char * functionName) const noexcept
    {
        if ((_Handle == nullptr) || (functionName == nullptr))
            return nullptr;

        return ::GetProcAddress(_Handle, functionName);
    }

    template<typename Function>
    [[nodiscard]]
    Function GetFunction(const char * functionName) const
    {
        const auto Address = GetFunctionAddress(functionName);

        if (Address == nullptr)
            throw_last_error("GetProcAddress failed");

        return reinterpret_cast<Function>(Address);
    }

    [[nodiscard]]
    HMODULE NativeHandle() const noexcept
    {
        return _Handle;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return _Handle != nullptr;
    }

private:
    void Reset() noexcept
    {
        if (_Handle != nullptr)
        {
            ::FreeLibrary(_Handle);
            _Handle = nullptr;
        }
    }

    [[noreturn]]
    static void throw_last_error(const char * message)
    {
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), message);
    }

    HMODULE _Handle = nullptr;
};
