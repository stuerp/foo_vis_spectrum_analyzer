
/** $VER: MD5.h (2024.03.03) P. Stuer **/

#pragma once

#include "framework.h"

class MD5
{
public:
    uint8_t Value[16];

public:
    bool GetHash(const uint8_t * data, size_t size) noexcept;
};
