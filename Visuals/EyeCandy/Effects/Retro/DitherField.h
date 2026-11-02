
/** $VER: DitherField.h (2026.11.02) */

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace DitherField
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget *, double, audio_chunk &) noexcept override;
        void Reset() noexcept override;

        std::wstring GetStatus() noexcept override;

    private:
        static constexpr size_t BandCount = 12;
        static constexpr size_t DitherModeCount = 10;
        void AnalyzeAudio(const audio_chunk &);
        void Update(double) noexcept;
        void BuildEnergyField() noexcept;
        void BuildErrorDiffusion() noexcept;
        void RenderTexture() noexcept;
        void StartWorkers();
        void StopWorkers() noexcept;
        void WorkerLoop(size_t) noexcept;
        void RenderRows(UINT32, UINT32) noexcept;
        double OrderedThreshold(size_t, UINT32, UINT32) const noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<double> _Samples;
        std::array<double, BandCount> _BandEnergy{ };
        std::array<double, BandCount> _SmoothedBandEnergy{ };
        std::array<double, BandCount> _PreviousBandEnergy{ };
        std::vector<float> _EnergyField;
        std::vector<float> _ErrorBuffer;
        std::vector<uint8_t> _DiffusedField;
        std::vector<uint32_t> _Pixels;

        UINT32 _PixelWidth = 0;
        UINT32 _PixelHeight = 0;

        double _PreviousPulse = 0.;
        double _Time = 0.;
        double _FieldPhase = 0.;
        double _NoisePhase = 0.;
        double _HueShift = 0.;
        double _PaletteSpread = .2;
        double _RadialAudio = 0.;
        double _RadialScale = 1.;
        double _RadialTwist = 0.;
        double _DitherBlend = 0.;
        size_t _DitherModeA = 0;
        size_t _DitherModeB = 1;
        double _RenderBrightness = 1.;

    private:
        ComPtr<ID2D1Bitmap> _Bitmap;
    };
}
