
/** $VER: InterferenceMoire.h (2026.11.02) **/

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace InterferenceMoire
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
        void AnalyzeAudio(const audio_chunk &);
        void Update(double) noexcept;

        void RenderTexture() noexcept;
        void RenderRows(UINT32, UINT32) noexcept;

        double LineGrid(double, double, double, double, double) const noexcept;
        double CircleGrid(double, double, double, double, double) const noexcept;
        double DotScreen(double, double, double, double, double) const noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker(size_t) noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<double> _Samples;

        static constexpr size_t BandCount = 12;

        std::array<double, BandCount> _BandEnergy{ };
        std::array<double, BandCount> _SmoothedBandEnergy{ };
        std::array<double, BandCount> _PreviousBandEnergy{ };

        std::vector<uint32_t> _Pixels;

        UINT32 _PixelWidth = 0;
        UINT32 _PixelHeight = 0;

        double _PreviousPulse = 0.;
        double _Time = 0.;

        double _AngleA = 0.;
        double _AngleB = .018;
        double _AngleC = -.026;

        double _OffsetAX = 0.;
        double _OffsetAY = 0.;

        double _OffsetBX = 0.;
        double _OffsetBY = 0.;

        double _CirclePhase = 0.;
        double _DotPhase = 0.;
        double _RenderBrightness = 1.;
        double _HueShift = 0.;
        double _PaletteSpread = .22;
        double _Saturation = .78;

    private:
        ComPtr<ID2D1Bitmap> _Bitmap;
    };
}
