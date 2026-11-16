
/** $VER: FieldEffect.cpp (2026.11.12) - Base class for a field effect */

#include <pch.h>

#include "FieldEffect.h"

namespace Field
{
    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        const auto RasterSize = D2D1::SizeU(_RasterWidth, _RasterHeight);

        if (_BackBuffer)
        {
            const auto BitmapSize = _BackBuffer->GetPixelSize();

            if (BitmapSize == RasterSize)
                return S_OK;
        }

        HRESULT hr = renderTarget->CreateBitmap(RasterSize, nullptr, 0, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), _BackBuffer.ReleaseAndGetAddressOf());

        return hr;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _BackBuffer.Reset();
    }
}
