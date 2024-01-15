
/** $VER: DirectWrite.cpp (2024.01.08) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "DirectWrite.h"

#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
DirectWrite::DirectWrite()
{
    Initialize();
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT DirectWrite::Initialize()
{
    HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(Factory), (IUnknown **) &Factory);

    return hr;
}

DirectWrite _DirectWrite;
