
/** $VER: DirectWrite.h (2024.01.28) P. Stuer **/

#pragma once

#include "framework.h"

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
