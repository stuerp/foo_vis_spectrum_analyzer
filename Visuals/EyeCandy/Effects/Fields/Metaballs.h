
/** $VER: Metaballs.h (2026.10.23) **/

#pragma once

#include "FieldEffect.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Metaballs
{
    using namespace EyeCandy;

    class effect_t final : public Field::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        void Reset() noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t BlobCount = 7;

        struct work_t
        {
            double InverseWidth;
            double InverseHeight;
            double AspectRatio;
            double Threshold;
            double Softness;
            double EdgeWidth;
            double BackgroundGlow;
            double BlobX[BlobCount];
            double BlobY[BlobCount];
            double BlobRadius[BlobCount];
            double BlobWeight[BlobCount];
        };

        void Update(double time);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept;

        uint32_t Colorize(double surface, double interior, double edge, double glow) const noexcept;

    private:
        work_t _Work {};
        std::atomic<uint32_t> _NextRow { 0 };
    };
}
