
/** $VER: Rotozoomer.h (2026.11.02) **/

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Rotozoomer
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

        void RenderTexture() noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker(size_t) noexcept;

        void RenderRows(UINT32, UINT32) noexcept;

        double SamplePattern(size_t, double, double, double, double, int64_t, int64_t, uint64_t) const noexcept;
        D2D1_COLOR_F SamplePalette(size_t, double) const noexcept;
        uint32_t SampleTexture(double, double, double, int64_t, double) const noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<double> _Samples;

        std::array<double, BandCount> _BandEnergy {};
        std::array<double, BandCount> _SmoothedBandEnergy {};
        std::array<double, BandCount> _PreviousBandEnergy {};
        std::vector<uint32_t> _Pixels;

        UINT32 _PixelWidth  = 0;
        UINT32 _PixelHeight = 0;

        double _RotationVelocity   = 0.035;
        double _RotationCS         = 1.;
        double _RotationSN         = 0.;
        double _ColorPhase         = 0.;
        double _SmoothedDeltaTime  = 1. / 60.;
        double _ZoomPhase          = 0.;
        double _ZoomVelocity       = 0.18;
        double _ZoomResponse       = 0.18;
        double _WarpPhase          = 0.;
        double _PreviousPulse      = 0.;
        double _TextureTime        = 0.;
        double _RenderCS           = 1.;
        double _RenderSN           = 0.;
        double _RenderZoom         = 1.;
        double _RenderAspect       = 1.;
        double _RenderWarp         = 0.;
        double _RenderPhase        = 0.;
        double _RenderZoomFraction = 0.;

        int64_t _RenderDetailLevel = 0;
        size_t _RenderTextureA     = 0;
        size_t _RenderTextureB     = 1;
        size_t _RenderPaletteA     = 0;
        size_t _RenderPaletteB     = 1;
        double _RenderTextureBlend = 0.;
        double _RenderPaletteBlend = 0.;

    private:
        ComPtr<ID2D1Bitmap> _Bitmap;
    };
}
