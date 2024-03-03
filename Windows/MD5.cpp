
/** $VER: MD5.cpp (2024.03.03) P. Stuer **/

#include "MD5.h"

#include <Wincrypt.h>

bool MD5::GetHash(const uint8_t * data, size_t size) noexcept
{
    if ((data == nullptr) || (size == 0))
        return false;

    bool Result = false;

    HCRYPTPROV hProv = 0;

    if (::CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        HCRYPTHASH hHash = 0;

        if (::CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            if (::CryptHashData(hHash, data, size, 0))
            {
                DWORD Size = _countof(Value);

                Result = ::CryptGetHashParam(hHash, HP_HASHVAL, Value, &Size, 0);
            }

            ::CryptDestroyHash(hHash);
        }

        ::CryptReleaseContext(hProv, 0);
    }

    return Result;
}
