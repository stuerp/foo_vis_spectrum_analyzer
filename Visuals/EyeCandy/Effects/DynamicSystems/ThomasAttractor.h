
/** $VER: ThomasAttractor.h (2026.10.20) https://en.wikipedia.org/wiki/Thomas%27_cyclically_symmetric_attractor */

#pragma once

#include "DynamicSystemEffect.h"

#include <string>

#include <d2d1.h>
#include <wrl/client.h>

namespace ThomasAttractor
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public DynamicSystem::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override final = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        void Reset() noexcept override final;

        void Update(double) noexcept;

        void UpdateScene(double) noexcept;
        void UpdateAttractor() noexcept;

        void RenderCurve(ID2D1RenderTarget *) noexcept;

        static point3d_t Derive(point3d_t, double) noexcept;
        static point3d_t Integrate(point3d_t, double, double) noexcept;

    private:
        double _AudioOrbit = 0.;
        double _AudioLift  = 0.;
        double _AudioScale = 1.;
    };
}
