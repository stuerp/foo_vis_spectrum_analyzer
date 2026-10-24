
/** $VER: CliffordAttractor.h (2026.10.20) https://paulbourke.net/fractals/clifford/ */

#pragma once

#include "DynamicSystemEffect.h"

#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace CliffordAttractor
{
    using namespace EyeCandy;

    class effect_t final : public DynamicSystem::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override final = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        std::wstring GetStatus() noexcept override final;

    private:
        struct parameters_t
        {
            double A = -1.4;
            double B =  1.6;
            double C =  1.0;
            double D =  0.7;
        };

        struct point_t
        {
            double X = 0.;
            double Y = 0.;
        };

    private:
        void Reset() noexcept override final;
        void Update(double deltaTime) noexcept;

        void UpdateAttractor(double deltaTime) noexcept;
        void AccumulatePoint(point_t point, double amount) noexcept;
        void UpdateBitmap() noexcept;
        void ClearDensity() noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

        static point_t Iterate(point_t point, const parameters_t & parameters) noexcept;

    private:
        parameters_t _Parameters {};
        point_t _Point { 0.1, 0.1 };

        std::vector<double> _Densities;
        std::vector<uint32_t> _Pixels;

        UINT32 _BitmapWidth = 0;
        UINT32 _BitmapHeight = 0;

        double _Exposure = 1.;
        double _AccumulationRemainder = 0.;

    private:
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
