
/** $VER: DirectWrite.cpp (2024.03.09) P. Stuer **/

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
