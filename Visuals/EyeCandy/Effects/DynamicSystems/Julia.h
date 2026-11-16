
/** $VER: Julia.h (2026.11.13) **/

#pragma once

#include "Effect.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Julia
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget *, double, audio_chunk &) noexcept override final;
        void Reset() noexcept override final;
        void OnMouseWheel(short) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        void AnalyzeAudio(const audio_chunk &) noexcept;
        void Update(double) noexcept;
        void RenderJulia() noexcept;
        void RenderRow(size_t) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        UINT32 Colorize(double amount, bool escaped) const noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<std::thread> _Workers;
        std::atomic<size_t> _NextRow = 0;
        std::mutex _WorkerMutex;
        std::condition_variable _WorkerCondition;
        std::condition_variable _CompletionCondition;
        size_t _Generation = 0;
        size_t _PendingWorkers = 0;
        bool _IsStopping = false;

        double _Time          = 0.;
        double _Zoom          = 1.;
        double _ZoomOffset    = 0.;
        double _Rotation      = 0.;
        double _PaletteOffset = 0.;
        double _Real          = -0.70;
        double _Imaginary     =  0.27015;

        ComPtr<ID2D1Bitmap> _Bitmap;
    };
}
