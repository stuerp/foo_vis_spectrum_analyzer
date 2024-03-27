
/** $VER: Rendering.cpp (2024.03.18) P. Stuer **/

#include "UIElement.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "WIC.h"

#include "Resources.h"
#include "Color.h"
#include "Gradients.h"
#include "StyleManager.h"

#include "ToneGenerator.h"
#include "Log.h"

#pragma hdrstop

/// <summary>
/// Starts the timer.
/// </summary>
void UIElement::StartTimer() noexcept
{
    if (_ThreadPoolTimer != nullptr)
        StopTimer();

    _ThreadPoolTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);

    FILETIME DueTime = { };

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _MainState._RefreshRateLimit, 0);
}

/// <summary>
/// Stops the timer.
/// </summary>
void UIElement::StopTimer() noexcept
{
    if (_ThreadPoolTimer == nullptr)
        return;

    ::SetThreadpoolTimer(_ThreadPoolTimer, nullptr, 0, 0);

    ::WaitForThreadpoolTimerCallbacks(_ThreadPoolTimer, true);

    ::CloseThreadpoolTimer(_ThreadPoolTimer);
    _ThreadPoolTimer = nullptr;
}

/// <summary>
/// Handles a timer tick.
/// </summary>
void CALLBACK UIElement::TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept
{
    ((UIElement *) context)->OnTimer();
}

/// <summary>
/// Handles a timer tick.
/// </summary>
void UIElement::OnTimer()
{
    if (_IsFrozen || !_IsVisible || ::IsIconic(core_api::get_main_window()))
        return;

    LONG64 BarrierState = ::InterlockedIncrement64(&_ThreadState._Barrier);

    if (BarrierState != 1)
    {
        ::InterlockedDecrement64(&_ThreadState._Barrier);

        return;
    }

    if (_CriticalSection.TryEnter())
    {
        ProcessEvents();

        if (IsWindowVisible())
            Render();

        _CriticalSection.Leave();
    }

    // Notify the configuration dialog about the changed artwork colors.
    if (_IsConfigurationChanged)
    {
        if (_CriticalSection.TryEnter())
        {
            _MainState._ArtworkGradientStops = _ThreadState._ArtworkGradientStops;

            _CriticalSection.Leave();

            // Notify the configuration dialog.
            if (_ConfigurationDialog.IsWindow())
                _ConfigurationDialog.PostMessageW(UM_CONFIGURATION_CHANGED, CC_COLORS); // Must be sent outside the critical section.

            _IsConfigurationChanged = false;
        }
    }

    ::InterlockedDecrement64(&_ThreadState._Barrier);
}

/// <summary>
/// Allows the rendering thread to react to events captured in or generated by the UI thread.
/// </summary>
void UIElement::ProcessEvents()
{
    Event::Flags Flags = _Event.GetFlags();

    if (Flags == 0)
        return;

    if (Event::IsRaised(Flags, Event::PlaybackStartedNewTrack))
    {
        _ThreadState._PlaybackTime = 0.;
        _ThreadState._TrackTime = 0.;

        if (_Artwork.Bitmap() == nullptr)
        {
            // Set the default dominant color and gradient for the artwork color scheme.
            _ThreadState._ArtworkGradientStops = GetGradientStops(ColorScheme::Artwork);
            _ThreadState._StyleManager._DominantColor = _ThreadState._ArtworkGradientStops[0].color;

            _ThreadState._StyleManager.SetArtworkDependentParameters(_ThreadState._ArtworkGradientStops, _ThreadState._StyleManager._DominantColor);

            _IsConfigurationChanged = true;
        }

        for (auto & Iter : _Grid)
            Iter._Graph->Reset();
    }

    if (Event::IsRaised(Flags, Event::PlaybackStopped))
    {
        _ThreadState._PlaybackTime = 0.;
        _ThreadState._TrackTime = 0.;

        _Artwork.Release();

        for (auto & Iter : _Grid)
            Iter._Graph->Reset();
    }

    if (Event::IsRaised(Flags, Event::UserInterfaceColorsChanged))
    {
        _ThreadState._StyleManager.UpdateCurrentColors();
        _ThreadState._StyleManager.ReleaseDeviceSpecificResources();
    }
}

/// <summary>
/// Renders a frame.
/// </summary>
void UIElement::Render()
{
    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && !(_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        _FrameCounter.NewFrame();

        UpdateSpectrum();

        _RenderTarget->BeginDraw();

        for (auto & Iter : _Grid)
            Iter._Graph->Render(_RenderTarget, (double) _SampleRate, _Artwork);

        if (_MainState._ShowFrameCounter)
            _FrameCounter.Render(_RenderTarget);

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }
}

/// <summary>
/// Updates the spectrum of all the graphs using the next audio chunk.
/// </summary>
void UIElement::UpdateSpectrum()
{
    if (_MainState._UseToneGenerator)
    {
        audio_chunk_impl Chunk;

        if (_ToneGenerator.GetChunk(Chunk, _SampleRate))
        {
            for (auto & Iter : _Grid)
                Iter._Graph->Process(Chunk);
        }
    }
    else
    {
        double PlaybackTime; // in seconds

        // Update the graph.
        if (_VisualisationStream.is_valid() && _VisualisationStream->get_absolute_time(PlaybackTime))
        {
            if (PlaybackTime != _ThreadState._PlaybackTime) // Delta Time will 0 when the playback is paused.
            {
                audio_chunk_impl Chunk;

                const bool IsSlidingWindow = _ThreadState._Transform == Transform::SWIFT;
                const double WindowSize = IsSlidingWindow ? PlaybackTime - _ThreadState._PlaybackTime :  (double) _MainState._BinCount / (double) _SampleRate;
                const double Offset = IsSlidingWindow ? _ThreadState._PlaybackTime : PlaybackTime - (WindowSize * (0.5 + _ThreadState._ReactionAlignment));

                if (_VisualisationStream->get_chunk_absolute(Chunk, Offset, WindowSize))
                {
                    for (auto & Iter : _Grid)
                        Iter._Graph->Process(Chunk);
                }

                _ThreadState._PlaybackTime = PlaybackTime;
            }
        }
    }

    // Needs to be called even when no audio is playing to keep animating the decay of the peak indicators after the audio stops.
    if (_MainState._PeakMode != PeakMode::None)
    {
        for (auto & Iter : _Grid)
            Iter._Graph->GetAnalysis().UpdatePeakIndicators();
    }
}

#pragma region DirectX

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT UIElement::CreateDeviceIndependentResources()
{
    HRESULT hr = _FrameCounter.CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void UIElement::ReleaseDeviceIndependentResources()
{
    _FrameCounter.ReleaseDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT UIElement::CreateDeviceSpecificResources()
{
    CRect cr;

    GetClientRect(cr);

    UINT32 Width  = (UINT32) (cr.right  - cr.left);
    UINT32 Height = (UINT32) (cr.bottom - cr.top);

    HRESULT hr = (Width != 0) && (Height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    // Create the render target.
    if (SUCCEEDED(hr) && (_RenderTarget == nullptr))
    {
        D2D1_SIZE_U Size = D2D1::SizeU(Width, Height);

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties
        (
            _MainState._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);

        hr = _Direct2D.Factory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
        {
            _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            _RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            // Resize some elements based on the size of the render target.
            {
                // Reposition the frame counter.
                _FrameCounter.Resize((FLOAT) Size.width, (FLOAT) Size.height);

                // Resize the grid.
                _Grid.Resize((FLOAT) Size.width, (FLOAT) Size.height);

                _ThreadState._StyleManager.ResetGradients();
            }
        }
    }

    // Create the background bitmap from the artwork.
    if (SUCCEEDED(hr) && _Artwork.IsInitialized())
    {
        for (auto & Iter : _Grid)
        {
            Spectrum & s = Iter._Graph->GetSpectrum();

            s.ReleaseDeviceSpecificResources();
        }

        hr = _Artwork.Realize(_RenderTarget);
    }

    // Create the resources that depend on the artwork. Done at least once per artwork because the configuration dialog needs it for the dominant color and ColorScheme::Artwork.
    if (SUCCEEDED(hr) && _Artwork.IsRealized())
        hr = CreateArtworkDependentResources();

    return hr;
}

/// <summary>
/// Creates the DirectX resources that are dependent on the artwork.
/// </summary>
HRESULT UIElement::CreateArtworkDependentResources()
{
    // Get the colors from the artwork.
    HRESULT hr = _Artwork.GetColors(_ThreadState._ArtworkColors, _ThreadState._NumArtworkColors, _ThreadState._LightnessThreshold, _ThreadState._TransparencyThreshold);

    // Sort the colors.
    if (SUCCEEDED(hr))
    {
        _ThreadState._StyleManager._DominantColor = _ThreadState._ArtworkColors[0];

        #pragma warning(disable: 4061) // Enumerator not handled
        switch (_ThreadState._ColorOrder)
        {
            case ColorOrder::None:
                break;

            case ColorOrder::HueAscending:
                Color::SortColorsByHue(_ThreadState._ArtworkColors, true);
                break;

            case ColorOrder::HueDescending:
                Color::SortColorsByHue(_ThreadState._ArtworkColors, false);
                break;

            case ColorOrder::SaturationAscending:
                Color::SortColorsBySaturation(_ThreadState._ArtworkColors, true);
                break;

            case ColorOrder::SaturationDescending:
                Color::SortColorsBySaturation(_ThreadState._ArtworkColors, false);
                break;

            case ColorOrder::LightnessAscending:
                Color::SortColorsByLightness(_ThreadState._ArtworkColors, true);
                break;

            case ColorOrder::LightnessDescending:
                Color::SortColorsByLightness(_ThreadState._ArtworkColors, false);
                break;
        }
        #pragma warning(default: 4061)
    }

    // Create the gradient stops.
    if (SUCCEEDED(hr))
        hr = _Direct2D.CreateGradientStops(_ThreadState._ArtworkColors, _ThreadState._ArtworkGradientStops);

    if (SUCCEEDED(hr))
        _ThreadState._StyleManager.SetArtworkDependentParameters(_ThreadState._ArtworkGradientStops, _ThreadState._StyleManager._DominantColor);

    _IsConfigurationChanged = true;

    _Artwork.SetIdle();

    return S_OK; // Make sure resource creation continues even if something goes wrong while creating the gradient.
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void UIElement::ReleaseDeviceSpecificResources()
{
    _ThreadState._StyleManager.ReleaseDeviceSpecificResources();

    for (auto & Iter : _Grid)
        Iter._Graph->ReleaseDeviceSpecificResources();

    _FrameCounter.ReleaseDeviceSpecificResources();

    _Artwork.Release();

    _RenderTarget.Release();
}

#pragma endregion
