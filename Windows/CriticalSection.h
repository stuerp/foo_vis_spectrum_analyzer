
/** $VER: CriticalSection.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class CriticalSection
{
public:
    CriticalSection() noexcept
    {
        ::InitializeCriticalSection(&_cs);
    }

    CriticalSection(const CriticalSection &) = delete;
    CriticalSection & operator=(const CriticalSection &) = delete;
    CriticalSection(CriticalSection &&) = delete;
    CriticalSection & operator=(CriticalSection &&) = delete;

    ~CriticalSection() noexcept
    {
        ::DeleteCriticalSection(&_cs);
    }

    void Enter() noexcept
    {
        ::EnterCriticalSection(&_cs);
    }

    bool TryEnter() noexcept
    {
        return ::TryEnterCriticalSection(&_cs) != 0;
    }

    void Leave() noexcept
    {
        ::LeaveCriticalSection(&_cs);
    }

    operator CRITICAL_SECTION *()
    {
        return &_cs;
    }

private:
    CRITICAL_SECTION _cs;
};
