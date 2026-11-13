
/** $VER: AudioGalaxy.h (2026.11.06) **/

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

namespace AudioGalaxy
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
        struct star_t
        {
            double X = 0.;
            double Y = 0.;
            double Z = 0.;

            double PreviousZ   = 0.;
            double PreviousX   = 0.;
            double PreviousY   = 0.;

            double Age         = 0.;
            double Lifetime    = 0.;
            double Size        = 0.; // Size of the star

            double Angle       = 0.;
            double Radius      = 0.; // Distance from the center of the galaxy
            double Speed       = 0.;
            double Brightness  = 0.;
            double HueOffset   = 0.;

            double Height      = 0.;
            double Inclination = 0.;
            double NodeAngle   = 0.;
            size_t Band        = 0;

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
        void DrawStar(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, const D2D1_COLOR_F & color, float radius, float feather) noexcept;
        void RenderFlash(ID2D1RenderTarget * renderTarget) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

        D2D1_COLOR_F GetStarColor(const star_t & star, double depthBrightness) const noexcept;
        
    private:
        std::vector<star_t> _Stars;
        std::atomic<size_t> _NextStar = 0;

        static constexpr double MinBirthAge =  0.; // seconds
        static constexpr double MaxBirthAge = 10.; // seconds

        static constexpr double MinLifeTime =  7.; // seconds
        static constexpr double MaxLifeTime = 18.; // seconds

        static constexpr double MaxRadius       = 1.30;
        static constexpr double MaxAngularSpeed = 2.00;

        double _ScaleX               = 0.;
        double _ScaleY               = 0.;

        double _StarDeltaTime        = 0.;
        double _ForwardSpeed         = 0.;

    private:
        double _TransientPulse       = 0.;
        double _TransientCooldown    = 0.;
        size_t _TransientStarIndex   = 0;
    };
}
