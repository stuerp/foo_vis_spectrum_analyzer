
/** $VER: Aurora.h (2026.10.23) **/

#pragma once

#include "FieldEffect.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace Aurora
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
            double CurtainScale;
            double Drift;
            double FoldStrength;
            double FoldFrequency;
            double VerticalFlow;
            double LowEnergy;
            double MidEnergy;
            double HighEnergy;
            double BroadEnergy;
            double BalanceOffset;
            double Horizon;
            bool HasImpulse;
        };

        void Update(double time);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept;

        uint32_t Colorize(double intensity, double fold, double height) const noexcept;

    private:
        work_t _Work {};
        std::atomic<uint32_t> _NextRow { 0 };
    };
}
