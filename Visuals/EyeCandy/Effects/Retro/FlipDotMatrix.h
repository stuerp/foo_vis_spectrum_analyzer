
/** $VER: FlipDotMatrix.h (2026.11.02) **/

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace FlipDotMatrix
{
    using Microsoft::WRL::ComPtr;

    using namespace EyeCandy;

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
        void AnalyzeAudio(const audio_chunk &);
        void Update(double) noexcept;
        void UpdateTargets() noexcept;
        void UpdateDots(double) noexcept;
        void RenderTexture() noexcept;
        void StartWorkers();
        void StopWorkers() noexcept;
        void WorkerLoop(size_t) noexcept;
        void RenderRows(UINT32, UINT32) noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<double> _Samples;

        std::array<double, BandCount> _BandEnergy{ };
        std::array<double, BandCount> _SmoothedBandEnergy{ };
        std::array<double, BandCount> _PreviousBandEnergy{ };

        std::vector<float> _DotState;
        std::vector<float> _DotVelocity;
        std::vector<uint8_t> _DotTarget;
        std::vector<uint32_t> _Pixels;

        UINT32 _PixelWidth = 0;
        UINT32 _PixelHeight = 0;
        UINT32 _Columns = 0;
        UINT32 _Rows = 0;
        size_t _RefreshCursor = 0;

        double _RefreshAccumulator = 0.;
        double _PreviousPulse = 0.;
        double _Time = 0.;
        double _WavePhase = 0.;
        double _RipplePhase = 0.;
        double _HueShift = 0.;
        double _PaletteSpread = .18;
        double _ColorIntensity = .85;
        double _HighlightIntensity = .24;
        double _RenderBrightness = 1.;

    private:
        ComPtr<ID2D1Bitmap> _Bitmap;
    };
}
