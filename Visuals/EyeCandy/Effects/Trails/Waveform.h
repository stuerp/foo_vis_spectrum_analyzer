
/** $VER: Waveform.h (2026.10.14) P. Stuer - Implements a waveform visual effect. **/

#pragma once

#include "Effect.h"

namespace Waveform
{
    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t() = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept override final;

        HRESULT CreateDeviceDependentResources(ComPtr<ID2D1RenderTarget> renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        ComPtr<ID2D1SolidColorBrush> _SignalBrush;
        ComPtr<ID2D1SolidColorBrush> _GridBrush;
    };
}
