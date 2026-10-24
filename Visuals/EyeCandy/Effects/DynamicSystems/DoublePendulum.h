
/** $VER: DoublePendulum.h (2026.10.20) Implements a double pendulum visual effect. https://en.wikipedia.org/wiki/Double_pendulum */

#pragma once

#include "DynamicSystemEffect.h"

#include <cstdint>
#include <string>

#include <d2d1.h>
#include <wrl/client.h>

namespace DoublePendulum
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public DynamicSystem::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override final = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;
        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct state_t
        {
            double Theta1 = 1.95;
            double Omega1 = 0.;
            double Theta2 = 1.15;
            double Omega2 = 0.;
        };

        struct field_t
        {
            size_t PointCount = 0;

            double Gravity    = 9.81;
            double Length1    = 1.;
            double Length2    = 0.92;
            double Mass1      = 1.;
            double Mass2      = 1.;
            double Damping    = 0.0015;
            double Step       = 0.0035;
        };

    private:
        void Reset() noexcept override final;

        void Update(double deltaTime) noexcept;

        void UpdateScene(double deltaTime) noexcept;
        void UpdatePendulum() noexcept;

        void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderPendulum(ID2D1RenderTarget * renderTarget) noexcept;

        void DrawAmbientGlow(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F center, uint32_t color, float intensity, float radius) noexcept;

        static state_t Derive(state_t state, const field_t & field) noexcept;
        static state_t Integrate(state_t state, const field_t & field) noexcept;

    private:
        state_t _PendulumState {};
        field_t _Field {};

        point3d_t _PivotWorld {};
        point3d_t _Bob1World {};
        point3d_t _Bob2World {};

        double _AudioOrbit = 0.;
        double _AudioLift = 0.;
        double _AudioScale = 1.;
    };
}
