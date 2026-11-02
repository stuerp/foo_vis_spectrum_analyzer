
/** $VER: HiddenLineLandscape.h (2026.11.02) **/

#pragma once

#include "Effect.h"

#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace HiddenLineLandscape
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override final;
        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;
        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Reset() noexcept override final;
        std::wstring GetStatus() noexcept override final;
    private:
        struct peak_t
        {
            double X = 0.;
            double Z = 1.;
            double Height = 0.;
            double Width = 1.;
            double Strength = 1.;
        };
        struct projected_point_t
        {
            D2D1_POINT_2F Position{ };
            float InverseDepth = 0.f;
            bool IsVisible = false;
        };
    private:
        void AnalyzeAudio(const audio_chunk & chunk);
        void Update(double deltaTime);
        void SpawnPeak(double strength) noexcept;
        double TerrainHeight(double x, double z) const noexcept;
        projected_point_t Project(double x, double y, double z) const noexcept;
        void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept override final;
        void RenderLandscape(ID2D1RenderTarget * renderTarget) noexcept;
        void RasterizeDepthTriangle(const projected_point_t & a, const projected_point_t & b, const projected_point_t & c, size_t width, size_t height, size_t firstY, size_t lastY) noexcept;
        void InitializeWorkers();
        void StopWorkers() noexcept;
        void WorkerLoop(size_t workerIndex) noexcept;
        void DispatchDepthPrepass(size_t rowCount, size_t samplesPerRow, size_t width, size_t height) noexcept;
        void RasterizeDepthBand(size_t workerIndex) noexcept;
        void DrawDepthClippedLine(ID2D1RenderTarget * renderTarget, const projected_point_t & start, const projected_point_t & end, float strokeWidth, size_t width, size_t height) noexcept;
        void RenderHorizonGlow(ID2D1RenderTarget * renderTarget) noexcept;
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;
    private:
        std::vector<peak_t> _Peaks;
        std::vector<float> _DepthBuffer;
        std::vector<projected_point_t> _ProjectedGrid;
        size_t _DepthRowCount = 0;
        size_t _DepthSamplesPerRow = 0;
        size_t _DepthBufferWidth = 0;
        size_t _DepthBufferHeight = 0;
        size_t _ActiveWorkerCount = 0;
        double _PreviousBass = 0.;
        double _PeakSpawnCooldown = 0.;
        double _TerrainTravel = 0.;
        double _CameraSway = 0.;
        double _CameraElevation = 0.;
        double _AudioMotion = 0.;
        uint32_t _RandomState = 0x1D872B41u;
    private:
        ComPtr<ID2D1SolidColorBrush> _LandscapeBrush;
        ComPtr<ID2D1SolidColorBrush> _GlowBrush;
        ComPtr<ID2D1StrokeStyle> _RoundStrokeStyle;
    };
}
