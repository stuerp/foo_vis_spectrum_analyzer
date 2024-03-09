
/** $VER: DirectWrite.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dwrite.h>
#include <atlbase.h>

class DirectWrite
{
public:
    DirectWrite() { }

    HRESULT Initialize();
    void Terminate();

public:
    CComPtr<IDWriteFactory> Factory;
};

extern DirectWrite _DirectWrite;
