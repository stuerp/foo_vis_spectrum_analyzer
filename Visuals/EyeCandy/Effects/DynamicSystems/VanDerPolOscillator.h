
/** $VER: VanDerPolOscillator.h (2026.11.04) */

#pragma once

#include "DynamicSystemEffect.h"
#include "ThreeD.h"

#include <string>

#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace VanDerPolOscillator
{
    using namespace EyeCandy;

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
        void Update(double deltaTime);
        void UpdateAttractor() noexcept;
        void UpdateScene(double deltaTime) noexcept;
        void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;

        point3d_t Integrate(point3d_t point, double step, double a, double b, double c) noexcept;
        point3d_t Derive(point3d_t point, double mu, double forcing, double omega) noexcept;

    private:
        point3d_t _AttractorState { .15, 0., 0. };
        bool _AttractorSettled = false;

        double _AudioOrbit = 0.;
        double _AudioLift = 0.;
        double _AudioScale = 1.;
    };
}
