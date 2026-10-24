
/** $VER: RosslerAttractor.h (2026.10.20) https://en.wikipedia.org/wiki/R%C3%B6ssler_attractor */

#pragma once

#include "DynamicSystemEffect.h"

#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace RosslerAttractor
{
    using namespace EyeCandy;

    class effect_t final : public DynamicSystem::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct field_t
        {
            size_t PointCount = 0;

            double A = 0.2;
            double B = 0.2;
            double C = 5.7;

            double Step = .0085;
        };

        struct range_t
        {
            size_t First = 0;
            size_t Last = 0;
            point3d_t Seed {};
        };

        void Reset() noexcept override final;

        void Update(double deltaTime);

        void UpdateAttractor() noexcept;
        void UpdateScene(double deltaTime) noexcept;

        void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker(size_t workerIndex) noexcept;
        void GenerateRange(size_t workerIndex, const field_t & field) noexcept;

        static point3d_t Derive(point3d_t point, double a, double b, double c) noexcept;
        static point3d_t Integrate(point3d_t point, double step, double a, double b, double c) noexcept;

    private:
        field_t _Field {};

        double _AudioOrbit = 0.;
        double _AudioLift = 0.;
        double _AudioScale = 1.;

        std::vector<range_t> _WorkerRanges;
    };
}
