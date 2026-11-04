
/** $VER: GrayScottReactionDiffusion.h (2026.11.04) */

#pragma once

#include "FieldEffect.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace GrayScottReactionDiffusion
{
    using namespace EyeCandy;

    class effect_t final : public Field::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Reset() noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void OnMouseWheel(short wheelDelta) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct work_t
        {
            double Feed = .0367;
            double Kill = .0649;
            double DiffusionU = 1.;
            double DiffusionV = .50;
            double TimeStep = 1.;
        };

        void Update();
        void SeedPattern() noexcept;
        void InjectAudio(double time) noexcept;
        void DispatchStep(const work_t & work);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;
        void GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept;

        void Colorize() noexcept;
        uint32_t Colorize(double u, double v, double edge) const noexcept;

        size_t Index(uint32_t x, uint32_t y) const noexcept
        {
            return ((size_t) y * _RasterWidth) + x;
        }

    private:
        work_t _Work {};
        std::atomic<uint32_t> _NextRow { 0 };

        std::vector<float> _U;
        std::vector<float> _V;
        std::vector<float> _NextU;
        std::vector<float> _NextV;

        double _InjectionPhase = 0.;
        double _LastImpulse = 0.;
    };
}
