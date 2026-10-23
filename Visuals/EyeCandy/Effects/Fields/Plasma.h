
/** $VER: Plasma.h (2026.10.21) **/

#pragma once

#include "FieldEffect.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace Plasma
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
        struct work_t
        {
            double Time;
            double InverseWidth;
            double InverseHeight;
            double CoordinateScale;
            double HorizontalScale;
            double HorizontalFrequency;
            double DiagonalFrequency;
            double RadialFrequency;
            double HorizontalPhase;
            double DiagonalPhase;
            double RadialPhase;
            double DetailPhase;
            double RipplePhase;
            double ColorPhase;
            double BalanceOffset;
            double RadialCenterX;
            double RadialCenterY;
            bool HasImpulse;
        };

        void Update(double time);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept;

        uint32_t Colorize(double value) const noexcept;

    private:
        work_t _Work {};
        std::atomic<uint32_t> _NextRow { 0 };
    };
}
