
/** $VER: DirectWrite.cpp (2024.01.28) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "DirectWrite.h"

#include "COMException.h"

#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT DirectWrite::Initialize()
{
    HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(Factory), (IUnknown **) &Factory);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create DirectWrite factory.");

    return hr;
}

void DirectWrite::Terminate()
{
    Factory.Release();
}

DirectWrite _DirectWrite;
