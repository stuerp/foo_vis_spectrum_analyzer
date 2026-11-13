
/** $VER: Starfield.h (2026.11.06) **/

#pragma once

#include "ParticleSystemEffect.h"
#include "ThreeD.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Starfield
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public ParticleSystem::effect_t, public EyeCandy::ThreeD
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Reset() noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t TrailLayerCount = 3;

        struct star_t
        {
            double X = 0.;
            double Y = 0.;
            double Z = 1.;

            double PreviousZ  = 1.;

            double Speed      = 1.;
            double Size       = 1.;
            double Brightness = 1.;
            double HueOffset  = 0.;

            uint32_t Seed = 0;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk);
        void Update(double deltaTime);

        void GenerateWork(double deltaTime, double forwardSpeed) noexcept;
        void UpdateStar(size_t index, double deltaTime, double forwardSpeed) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void RespawnStar(star_t & star, size_t index, bool randomDepth) noexcept;

        float CurrentThemeHue() const noexcept;

        void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept override final;

        void RenderStars(ID2D1RenderTarget * renderTarget);
        void RenderFlash(ID2D1RenderTarget * renderTarget) noexcept;

        void GenerateStarTrails(std::array<ComPtr<ID2D1PathGeometry>, TrailLayerCount> & geometry);

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;
        
        D2D1_COLOR_F GetStarColor(const star_t & star, double depthBrightness) const noexcept;
        
    private:
        std::vector<star_t> _Stars;
        std::atomic<size_t> _NextStar = 0;

        double _StarDeltaTime = 0.;
        double _ForwardSpeed  = 0.;

        double _MidDirectionCooldown = 0.;

    private:
        ComPtr<ID2D1Factory> _D2DFactory;

        ComPtr<ID2D1SolidColorBrush> _StarBrush;
        ComPtr<ID2D1SolidColorBrush> _TrailBrush;
        ComPtr<ID2D1SolidColorBrush> _ScanlineBrush;
        ComPtr<ID2D1SolidColorBrush> _FrameBrush;

        ComPtr<ID2D1StrokeStyle> _RoundStrokeStyle;
    };
}
