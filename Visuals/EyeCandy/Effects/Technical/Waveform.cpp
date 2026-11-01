
/** $VER: Waveform.h (2026.10.14) P. Stuer - Implements a waveform visual effect. **/

#include <pch.h>

#include "Waveform.h"

#include <algorithm>
#include <vector>

namespace Waveform
{
    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        const float _Center = (float) _Height / 2.f;

        renderTarget->Clear(D2D1::ColorF(0.035f, 0.045f, 0.06f));

        // Draw the static content.
        {
            renderTarget->DrawLine(D2D1::Point2F(0.f, _Center), D2D1::Point2F(_Width, _Center), _GridBrush.Get(), 1.f);

            for (int i = 1; i < 8; ++i)
            {
                const float x = (_Width * (float) i) / 8.f;

                renderTarget->DrawLine(D2D1::Point2F(x, 0.f), D2D1::Point2F(x, _Height), _GridBrush.Get(), 0.5f);
            }
        }

        const size_t FrameCount  = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const audio_sample * Data = chunk.get_data();

        if ((Data == nullptr) || (FrameCount == 0) || (ChannelCount == 0) || (_Width == 0.f))
            return;

        // Convert a frame to mono.
        const auto ToMono = [ChannelCount](const audio_sample * Frame) noexcept -> audio_sample
        {
            audio_sample Mono = 0.f;

            for (size_t i = 0; i < ChannelCount; ++i)
                Mono += Frame[i];

            return Mono / (audio_sample) ChannelCount;
        };

        D2D1_POINT_2F PreviousPoint = {};

        bool HasPreviousPoint = false;

        for (uint32_t x = 0; x < (uint32_t) _Width; ++x)
        {
            audio_sample Mono = 0.f;

            // Map this horizontal pixel proportionally onto the frame range.
            const size_t BeginFrame = (size_t) ((uint64_t)  x      * FrameCount) / (uint32_t) _Width;
            const size_t EndFrame   = (size_t) ((uint64_t) (x + 1) * FrameCount) / (uint32_t) _Width;

            if (BeginFrame >= EndFrame)
            {
                // There are more pixels than frames. Reuse the nearest frame.
                const size_t FrameIndex = std::min(BeginFrame, FrameCount - 1);

                Mono = ToMono(Data + (FrameIndex * ChannelCount));
            }
            else
            {
                const audio_sample * Frame = Data + (BeginFrame * ChannelCount);

                // Average all frames that belong to this horizontal pixel.
                for (size_t i = BeginFrame; i < EndFrame; ++i, Frame += ChannelCount)
                    Mono += ToMono(Frame);

                Mono /= (audio_sample) (EndFrame - BeginFrame);
            }

            const float y = _Center - (float) (Mono * _Height);

            const D2D1_POINT_2F Point = D2D1::Point2F((float) x, y);

            if (HasPreviousPoint)
                renderTarget->DrawLine(PreviousPoint, Point, _SignalBrush.Get(), 1.f);

            PreviousPoint = Point;
            HasPreviousPoint = true;
        }

        // Extend the final segment to the full requested width.
        if (HasPreviousPoint && (_Width > 1.f))
        {
            const D2D1_POINT_2F FinalPoint = D2D1::Point2F(_Width, PreviousPoint.y);

            renderTarget->DrawLine(PreviousPoint, FinalPoint, _SignalBrush.Get(), 1.f);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ComPtr<ID2D1RenderTarget> renderTarget) noexcept
    {
        if (renderTarget == nullptr)
            return E_INVALIDARG;

        if (!_SignalBrush)
        {
            HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.95f, 0.65f), _SignalBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        if (!_GridBrush)
        {
            HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.25f, 0.29f, 0.34f), _GridBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _SignalBrush.Reset();
        _GridBrush.Reset();
    }
};
