
/** $VER: HarmonicHarp.h (2026.11.02) */

#pragma once

#include "Effect.h"
#include "ThreeD.h"

#include <array>
#include <atomic>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace HarmonicHarp
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t, public EyeCandy::ThreeD
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget *, double, audio_chunk &) noexcept override final;
        void OnMouseWheel(short wheelDelta) noexcept override final;
        void Reset() noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t StringCount = 32;
        static constexpr size_t PointCount = 48;

        struct point_t
        {
            double X = 0.;
            double Y = 0.;
            double Z = 0.;
        };
        struct string_t
        {
            std::array<point_t, PointCount> Points{ };
            double Frequency = 0.;
            double Energy = 0.;
            double SmoothedEnergy = 0.;
            double Phase = 0.;
            double HueOffset = 0.;
            double ColorBurst = 0.;
        };
        void AnalyzeAudio(const audio_chunk &);
        void Update(double) noexcept;
        void UpdateDragRotation(ID2D1RenderTarget *) noexcept;
        void GenerateWork() noexcept;
        void UpdateString(size_t) noexcept;
        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;
        void RenderScene(ID2D1RenderTarget *) noexcept;
        void RenderString(ID2D1RenderTarget *, const string_t &, const ThreeD::prepared_projection_t &) noexcept;
        void Fade(ID2D1RenderTarget *, double) noexcept;
        D2D1_COLOR_F StringColor(const string_t &, double) const noexcept;
        D2D1_COLOR_F BurstColor(const string_t &) const noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::array<string_t, StringCount> _Strings{ };
        std::vector<double> _Samples;
        std::atomic<size_t> _NextString{ 0 };

        double _SampleRate = 44100.;
        double _BeatEnvelope = 0.;
        double _PreviousRMS = 0.;
        double _SceneJump = 0.;
        double _SceneJumpVelocity = 0.;
        double _MousePitch = 25. * Pi / 180.;
        double _MouseYaw = 0.;
        double _DragStartPitch = 25. * Pi / 180.;
        double _DragStartYaw = 0.;
        double _ZoomFieldOfView = 52.;

    private:
        LONG _DragStartX = 0;
        LONG _DragStartY = 0;
        bool _IsDragging = false;
        double _Hue = .56;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
