
/** $VER: AizawaAttractor.h (2026.10.20) https://en.wikipedia.org/wiki/Aizawa_attractor */

#pragma once

#include "DynamicSystemEffect.h"

#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace AizawaAttractor
{
    using namespace EyeCandy;

    class effect_t final : public DynamicSystem::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;
        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct field_t
        {
            size_t PointCount = 0;

            double A = 0.95;
            double B = 0.70;
            double C = 0.60;
            double D = 3.50;
            double E = 0.25;
            double F = 0.10;
            double Step = 0.0060;
        };

        struct range_t
        {
            size_t First = 0;
            size_t Last = 0;
            point3d_t Seed {};
        };

    private:
        void Reset() noexcept override final;

        void Update(double deltaTime);

        void UpdateAttractor() noexcept;
        void UpdateScene(double deltaTime) noexcept;

        void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker(size_t workerIndex) noexcept;
        void GenerateRange(size_t workerIndex, const field_t & field) noexcept;

        static point3d_t Derive(point3d_t point, double a, double b, double c, double d, double e, double f) noexcept;
        static point3d_t Integrate(point3d_t point, double step, double a, double b, double c, double d, double e, double f) noexcept;

    private:
        field_t _Field {};

        double _AudioOrbit = 0.;
        double _AudioLift = 0.;
        double _AudioScale = 1.;

    private:
        std::vector<range_t> _WorkerRanges;
    };
}
