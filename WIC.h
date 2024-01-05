
/** $VER: WIC.h (2024.01.05) P. Stuer **/

#pragma once

#include "framework.h"

#include <wincodec.h>

class WIC
{
public:
    WIC();

    CComPtr<IWICFormatConverter> Load(const uint8_t * data, size_t size) const;

public:
    CComPtr<IWICImagingFactory> _WICFactory;
};

extern WIC _WIC;
