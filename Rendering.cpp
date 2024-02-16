
/** $VER: Rendering.cpp (2024.02.16) P. Stuer **/

#include "UIElement.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "WIC.h"

#include "Resources.h"
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

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _RenderState._RefreshRateLimit, 0);
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
    if (_IsFrozen || !_CriticalSection.TryEnter())
        return;

    // Stop the timer to prevent overlapping callbacks.
    ::SetThreadpoolTimer(_ThreadPoolTimer, nullptr, 0, 0);

    ProcessPlaybackEvent();

    if (IsWindowVisible())
        Render();

    // Start the timer again.
    FILETIME DueTime = { };

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _RenderState._RefreshRateLimit, 0);

    _CriticalSection.Leave();

    // Notify the configuration dialog about the changed gradient colors.
    if (_IsConfigurationChanged && _ConfigurationDialog.IsWindow())
    {
        _ConfigurationDialog.SendMessageW(WM_CONFIGURATION_CHANGED, CC_GRADIENT_STOPS); // Must be sent outside the critical section.

        _IsConfigurationChanged = false;
    }
}

/// <summary>
/// Allows the rendering thread to react to playback events captured in the main thread.
/// </summary>
void UIElement::ProcessPlaybackEvent()
{
    switch (_PlaybackEvent)
    {
        default:

        case PlaybackEvent::None:
            break;

        case PlaybackEvent::NewTrack:
        {
            _OldPlaybackTime = 0.;

            if (_Artwork.Bitmap() == nullptr)
            {
                // Set the default dominant color and gradient for the artwork color scheme.
                _RenderState._ArtworkGradientStops = GetGradientStops(ColorScheme::Artwork);
                _RenderState._DominantColor = _RenderState._ArtworkGradientStops[0].color;

                _RenderState._StyleManager.SetArtworkDependentParameters(_RenderState._ArtworkGradientStops, _RenderState._DominantColor);

                _IsConfigurationChanged = true;
            }
            break;
        }

        case PlaybackEvent::Stop:
        {
            _Artwork.Release();

            for (FrequencyBand & Iter : _Analyses[0]->_FrequencyBands)
                Iter.CurValue = 0.;
            break;
        }
    }

    _PlaybackEvent = PlaybackEvent::None;
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

        if (_RenderState._PeakMode != PeakMode::None)
            UpdatePeakIndicators();

        _RenderTarget->BeginDraw();

        for (Graph * Iter : _Graphs)
            Iter->Render(_RenderTarget, _Analyses[0]->_FrequencyBands, (double) _SampleRate, _Artwork);

        if (_RenderState._ShowFrameCounter)
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
/// Updates the spectrum using the next audio chunk.
/// </summary>
void UIElement::UpdateSpectrum()
{
    if (_RenderState._UseToneGenerator)
    {
        audio_chunk_impl Chunk;

        if (_ToneGenerator.GetChunk(Chunk, _SampleRate))
            ProcessAudioChunk(Chunk);
    }
    else
    {
        double PlaybackTime; // in seconds

        // Update the graph.
        if (_VisualisationStream.is_valid() && _VisualisationStream->get_absolute_time(PlaybackTime))
        {
            audio_chunk_impl Chunk;
/* Old code
            const double WindowSize = (double) _NumBins / (double) _SampleRate;
            const double Offset = (_RenderState._Transform != Transform::SWIFT) ? PlaybackTime - (WindowSize / (0.5 + _RenderState._ReactionAlignment)) : PlaybackTime;

            if (_VisualisationStream->get_chunk_absolute(Chunk, Offset, WindowSize))
                ProcessAudioChunk(Chunk);
*/
            const bool IsSlidingWindow = _RenderState._Transform == Transform::SWIFT;
            const double WindowSize = IsSlidingWindow ? PlaybackTime - _OldPlaybackTime :  (double) _BinCount / (double) _SampleRate;
            const double Offset = IsSlidingWindow ? _OldPlaybackTime : PlaybackTime - (WindowSize * (0.5 + _RenderState._ReactionAlignment));

            if (_VisualisationStream->get_chunk_absolute(Chunk, Offset, WindowSize))
              ProcessAudioChunk(Chunk);

            _OldPlaybackTime = PlaybackTime;
        }
    }
}

/// <summary>
/// Updates the position of the peak indicators.
/// </summary>
void UIElement::UpdatePeakIndicators() noexcept
{
    for (FrequencyBand & Iter : _Analyses[0]->_FrequencyBands)
    {
        double Amplitude = Clamp(_RenderState.ScaleA(Iter.CurValue), 0., 1.);

        if (Amplitude >= Iter.Peak)
        {
            if ((_RenderState._PeakMode == PeakMode::AIMP) || (_RenderState._PeakMode == PeakMode::FadingAIMP))
                Iter.HoldTime = (::isfinite(Iter.HoldTime) ? Iter.HoldTime : 0.) + (Amplitude - Iter.Peak) * _RenderState._HoldTime;
            else
                Iter.HoldTime = _RenderState._HoldTime;

            Iter.Peak = Amplitude;
            Iter.DecaySpeed = 0.;
            Iter.Opacity = 1.;
        }
        else
        {
            if (Iter.HoldTime >= 0.)
            {
                if ((_RenderState._PeakMode == PeakMode::AIMP) || (_RenderState._PeakMode == PeakMode::FadingAIMP))
                    Iter.Peak += (Iter.HoldTime - Max(Iter.HoldTime - 1., 0.)) / _RenderState._HoldTime;

                Iter.HoldTime -= 1.;

                if ((_RenderState._PeakMode == PeakMode::AIMP) || (_RenderState._PeakMode == PeakMode::FadingAIMP))
                    Iter.HoldTime = Min(Iter.HoldTime, _RenderState._HoldTime);
            }
            else
            {
                switch (_RenderState._PeakMode)
                {
                    default:

                    case PeakMode::None:
                        break;

                    case PeakMode::Classic:
                        Iter.DecaySpeed = _RenderState._Acceleration / 256.;
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::Gravity:
                        Iter.DecaySpeed += _RenderState._Acceleration / 256.;
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::AIMP:
                        Iter.DecaySpeed = (_RenderState._Acceleration / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::FadeOut:
                        Iter.DecaySpeed += _RenderState._Acceleration / 256.;
                        Iter.Opacity -= Iter.DecaySpeed;

                        if (Iter.Opacity <= 0.)
                            Iter.Peak = Amplitude;
                        break;

                    case PeakMode::FadingAIMP:
                        Iter.DecaySpeed = (_RenderState._Acceleration / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        Iter.Peak -= Iter.DecaySpeed;
                        Iter.Opacity -= Iter.DecaySpeed;

                        if (Iter.Opacity <= 0.)
                            Iter.Peak = Amplitude;
                        break;
                }
            }

            Iter.Peak = Clamp(Iter.Peak, 0., 1.);
        }
    }
}

/// <summary>
/// Resizes all visual elements.
/// </summary>
void UIElement::Resize()
{
    if (_RenderTarget == nullptr)
        return;

    D2D1_SIZE_F Size = _RenderTarget->GetSize();

    // Reposition the frame counter.
    _FrameCounter.Resize(Size.width, Size.height);

    // Resize the graph area.
    const D2D1_RECT_F Bounds(0.f, 0.f, Size.width, Size.height);

    for (auto * Iter : _Graphs)
        Iter->Move(Bounds);

    // Adjust the tracking tool tip.
    {
        if (_TrackingToolInfo != nullptr)
        {
            _ToolTipControl.DelTool(_TrackingToolInfo);

            delete _TrackingToolInfo;
        }

        _TrackingToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, (UINT_PTR) m_hWnd, nullptr, nullptr);

        ::SetRect(&_TrackingToolInfo->rect, (int) Bounds.left, (int) Bounds.top, (int) Bounds.right, (int) Bounds.bottom);

        _ToolTipControl.AddTool(_TrackingToolInfo);
    }
}

/// <summary>
/// Deletes some analysis resources.
/// </summary>
void UIElement::DeleteResources()
{
    // Forces the recreation of the Brown-Puckette window function.
    if (_BrownPucketteKernel != nullptr)
    {
        delete _BrownPucketteKernel;
        _BrownPucketteKernel = nullptr;
    }

    // Forces the recreation of the window function.
    if (_WindowFunction != nullptr)
    {
        delete _WindowFunction;
        _WindowFunction = nullptr;
    }

    // Forces the recreation of the spectrum analyzer.
    if (_FFTAnalyzer != nullptr)
    {
        delete _FFTAnalyzer;
        _FFTAnalyzer = nullptr;
    }

    // Forces the recreation of the Constant-Q transform.
    if (_CQTAnalyzer != nullptr)
    {
        delete _CQTAnalyzer;
        _CQTAnalyzer = nullptr;
    }

    // Forces the recreation of the Sliding Window Infinite Fourier transform.
    if (_SWIFTAnalyzer != nullptr)
    {
        delete _SWIFTAnalyzer;
        _SWIFTAnalyzer = nullptr;
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
            _RenderState._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);

        hr = _Direct2D.Factory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
        {
            _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            _RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            // Resize some elements based on the size of the render target.
            Resize();
        }
    }

    // Create the background bitmap from the artwork.
    if (SUCCEEDED(hr) && _NewArtwork)
    {
        for (Graph * Iter : _Graphs)
        {
            Spectrum & s = Iter->GetSpectrum();

            s.ReleaseDeviceSpecificResources();
        }

        hr = _Artwork.Realize(_RenderTarget);
        _NewArtworkGradient = true;

        _NewArtwork = false;
    }

    // Create the resources that depend on the artwork. Done at least once per artwork because the configuration dialog needs it for the dominant color and ColorScheme::Artwork.
    if (SUCCEEDED(hr) && ((_Artwork.Bitmap() != nullptr) && _NewArtworkGradient))
    {
        hr = CreateArtworkDependentResources();

        _NewArtworkGradient = false;
    }

    return hr;
}

/// <summary>
/// Creates the DirectX resources that are dependent on the artwork.
/// </summary>
HRESULT UIElement::CreateArtworkDependentResources()
{
    // Get the colors from the artwork.
    std::vector<D2D1_COLOR_F> Colors;

    HRESULT hr = _Artwork.GetColors(Colors, _RenderState._NumArtworkColors, _RenderState._LightnessThreshold, _RenderState._TransparencyThreshold);

    // Sort the colors.
    if (SUCCEEDED(hr))
    {
        _RenderState._DominantColor = Colors[0];

        #pragma warning(disable: 4061) // Enumerator not handled
        switch (_RenderState._ColorOrder)
        {
            case ColorOrder::None:
                break;

            case ColorOrder::HueAscending:
                _Direct2D.SortColorsByHue(Colors, true);
                break;

            case ColorOrder::HueDescending:
                _Direct2D.SortColorsByHue(Colors, false);
                break;

            case ColorOrder::SaturationAscending:
                _Direct2D.SortColorsBySaturation(Colors, true);
                break;

            case ColorOrder::SaturationDescending:
                _Direct2D.SortColorsBySaturation(Colors, false);
                break;

            case ColorOrder::LightnessAscending:
                _Direct2D.SortColorsByLightness(Colors, true);
                break;

            case ColorOrder::LightnessDescending:
                _Direct2D.SortColorsByLightness(Colors, false);
                break;
        }
        #pragma warning(default: 4061)
    }

    // Create the gradient stops.
    if (SUCCEEDED(hr))
        hr = _Direct2D.CreateGradientStops(Colors, _RenderState._ArtworkGradientStops);

    if (SUCCEEDED(hr))
        _RenderState._StyleManager.SetArtworkDependentParameters(_RenderState._ArtworkGradientStops, _RenderState._DominantColor);

    _IsConfigurationChanged = true;

    return S_OK; // Make sure resource creation continues even if something goes wrong while creating the gradient.
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void UIElement::ReleaseDeviceSpecificResources()
{
    _RenderState._StyleManager.ReleaseDeviceSpecificResources();

    for (Graph * Iter : _Graphs)
        Iter->ReleaseDeviceSpecificResources();

    _FrameCounter.ReleaseDeviceSpecificResources();

    _Artwork.Release();

    _RenderTarget.Release();
}

#pragma endregion
