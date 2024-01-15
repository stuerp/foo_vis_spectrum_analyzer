
/** $VER: DirectWrite.h (2024.01.15) P. Stuer **/

#pragma once

#include "framework.h"

class DirectWrite
{
public:
    DirectWrite();

private:
    HRESULT Initialize();

public:
    CComPtr<IDWriteFactory> Factory;
};

extern DirectWrite _DirectWrite;
