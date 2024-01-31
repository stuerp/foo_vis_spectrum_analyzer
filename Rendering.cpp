
/** $VER: Rendering.cpp (2024.01.31) P. Stuer **/

#include "UIElement.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "WIC.h"

#include "Resources.h"
#include "Gradients.h"
#include "StyleManager.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Creates the timer.
/// </summary>
void UIElement::CreateTimer() noexcept
{
    _ThreadPoolTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);
}

/// <summary>
/// Starts the timer.
/// </summary>
void UIElement::StartTimer() const noexcept
{
    if (_ThreadPoolTimer == nullptr)
        return;

    FILETIME DueTime = { };

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 1000 / (DWORD) _Configuration._RefreshRateLimit, 0);
}

/// <summary>
/// Stops the timer.
/// </summary>
void UIElement::StopTimer() const noexcept
{
    if (_ThreadPoolTimer == nullptr)
        return;

    FILETIME DueTime = { };

    ::SetThreadpoolTimer(_ThreadPoolTimer, &DueTime, 0, 0);
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
    if (!_CriticalSection.TryEnter())
        return;

    _FrameCounter.NewFrame();

    ProcessPlaybackEvent();
    UpdateSpectrum();
    Render();

    _CriticalSection.Leave();
}

/// <summary>
/// Allows the rendering thread to react to playback events in the main thread.
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
            if (_Artwork.Bitmap() == nullptr)
            {
                _Configuration._ArtworkGradientStops = GetGradientStops(ColorScheme::Artwork); // Get the default colors for the Artwork gradient.
                _Configuration._DominantColor = _Configuration._ArtworkGradientStops[0].color;

                if (_Configuration._ColorScheme == ColorScheme::Artwork)
                {
                    _Configuration._GradientStops = _Configuration._ArtworkGradientStops;

                    Spectrum & s = _Graph.GetSpectrum();

                    s.Initialize(&_Configuration);

                    if (_ConfigurationDialog.IsWindow())
                        _ConfigurationDialog.PostMessageW(WM_CONFIGURATION_CHANGED, CC_GRADIENT_STOPS);
                }
            }
            break;
        }

        case PlaybackEvent::Stop:
        {
            _Artwork.Release();

            for (auto & Iter : _FrequencyBands)
                Iter.CurValue = 0.;
            break;
        }
    }

    _PlaybackEvent = PlaybackEvent::None;
}

/// <summary>
/// Updates the spectrum using the next audio chunk.
/// </summary>
void UIElement::UpdateSpectrum()
{
    double PlaybackTime; // in seconds

    // Update the graph.
    if (_VisualisationStream.is_valid() && _VisualisationStream->get_absolute_time(PlaybackTime))
    {
        double WindowSize = (double) _FFTSize / (double) _SampleRate;

        audio_chunk_impl Chunk;

        if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - (WindowSize / 2.), WindowSize))
            ProcessAudioChunk(Chunk);
    }

    // Update the peak indicators.
    if ((_FFTAnalyzer != nullptr) && (_Configuration._PeakMode != PeakMode::None))
        _FFTAnalyzer->UpdatePeakIndicators(_FrequencyBands);
}

/// <summary>
/// FIXME: Temporary method to transfer configuration parameters to styles.
/// </summary>
void UIElement::UpdateStyles() noexcept
{
    {
        Style & style = _StyleManager.GetStyle(VisualElement::Background);

        style._CustomGradientStops = _Configuration._GradientStops;

        if ((_Configuration._BackgroundMode == BackgroundMode::ArtworkAndDominantColor) && (_Configuration._ArtworkGradientStops.size() > 0))
        {
            style._ColorSource = ColorSource::DominantColor;
            style._Color = _Configuration._DominantColor;
        }
        else
        if (!_Configuration._UseCustomBackColor)
        {
            style._ColorSource = ColorSource::Host;
            style._Color = D2D1::ColorF(_Configuration._DefBackColor);
        }
        else
        {
            style._CustomColor = _Configuration._BackColor;

            style._ColorSource = ColorSource::Solid;
            style._Color = style._CustomColor;
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::XAxisLine);

        style._CustomColor = _Configuration._XLineColor;
        style._CustomGradientStops = _Configuration._GradientStops;

            if (!_Configuration._UseCustomXLineColor)
            {
                style._ColorSource = ColorSource::Host;
                style._Color = D2D1::ColorF(_Configuration._DefTextColor);
            }
            else
            {
                style._ColorSource = ColorSource::Solid;
                style._Color = style._CustomColor;
            }

        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::XAxisText);

        style._CustomColor = _Configuration._XTextColor;
        style._CustomGradientStops = _Configuration._GradientStops;

            if (!_Configuration._UseCustomXTextColor)
            {
                style._ColorSource = ColorSource::Host;
                style._Color = D2D1::ColorF(_Configuration._DefTextColor);
            }
            else
            {
                style._ColorSource = ColorSource::Solid;
                style._Color = style._CustomColor;
            }

        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisLine);

        style._CustomColor = _Configuration._YLineColor;
        style._CustomGradientStops = _Configuration._GradientStops;

            if (!_Configuration._UseCustomYLineColor)
            {
                style._ColorSource = ColorSource::Host;
                style._Color = D2D1::ColorF(_Configuration._DefTextColor);
            }
            else
            {
                style._ColorSource = ColorSource::Solid;
                style._Color = style._CustomColor;
            }

        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisText);

        style._CustomColor = _Configuration._YTextColor;
        style._CustomGradientStops = _Configuration._GradientStops;

            if (!_Configuration._UseCustomYTextColor)
            {
                style._ColorSource = ColorSource::Host;
                style._Color = D2D1::ColorF(_Configuration._DefTextColor);
            }
            else
            {
                style._ColorSource = ColorSource::Solid;
                style._Color = style._CustomColor;
            }

        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveLine);

        style._CustomColor = _Configuration._LineColor;
        style._CustomGradientStops = _Configuration._GradientStops;

        if (_Configuration._UseCustomLineColor)
        {
            style._ColorSource = ColorSource::Solid;
            style._Color = style._CustomColor;
        }
        else
        {
            style._ColorSource = ColorSource::Gradient;
            style._Color = D2D1::ColorF(0, 0.f);
            style._GradientStops = style._CustomGradientStops;
        }

        style._Thickness = _Configuration._LineWidth;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveArea);

        style._CustomGradientStops = _Configuration._GradientStops;

            style._ColorSource = ColorSource::Gradient;
            style._Opacity = _Configuration._AreaOpacity;

        style._Color = style._CustomColor;
        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::PeakLine);

        style._CustomColor = _Configuration._PeakLineColor;
        style._CustomGradientStops = _Configuration._GradientStops;

            if (!_Configuration._UseCustomPeakLineColor)
            {
                style._ColorSource = ColorSource::Gradient;
            }
            else
            {
                style._ColorSource = ColorSource::Solid;
            }

        style._Color = style._CustomColor;
        style._GradientStops = style._CustomGradientStops;
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::PeakArea);

        style._Color = style._CustomColor;
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
        UpdateStyles(); // FIXME

        _RenderTarget->BeginDraw();

        _Graph.RenderBackground(_RenderTarget, _Artwork, _Configuration._DominantColor);
        _Graph.RenderForeground(_RenderTarget, _FrequencyBands, (double) _SampleRate);

        if (_Configuration._ShowFrameCounter)
            _FrameCounter.Render(_RenderTarget);

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }
}

#pragma region DirectX

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT UIElement::CreateDeviceIndependentResources()
{
    HRESULT hr = _FrameCounter.CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
        hr = _Graph.CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void UIElement::ReleaseDeviceIndependentResources()
{
    _Graph.ReleaseDeviceIndependentResources();

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
            _Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);

        hr = _Direct2D.Factory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
        {
            _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            // Resize some elements based on the size of the render target.
            Resize();
        }
    }

    // Create the background bitmap from the artwork.
    if (SUCCEEDED(hr) && _NewArtwork)
    {
        Spectrum & s = _Graph.GetSpectrum();

        s.ReleaseDeviceSpecificResources();

        hr = _Artwork.Realize(_RenderTarget);

        _NewArtwork = false;
        _NewArtworkGradient = true;
    }

    // Create the gradient stops based on the artwork. Done at least once per artwork because the configuration dialog needs it when ColorScheme::Artwork is selected.
    if (SUCCEEDED(hr) && ((_Artwork.Bitmap() != nullptr) && _NewArtworkGradient))
    {
        hr = CreateArtworkGradient();

        _NewArtworkGradient = false;
    }

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr))
        hr = _Graph.CreateDeviceSpecificResources(_RenderTarget);

    return hr;
}

/// <summary>
/// Creates the DirectX resources that are dependent on the artwork.
/// </summary>
HRESULT UIElement::CreateArtworkGradient()
{
    // Get the colors from the artwork.
    std::vector<D2D1_COLOR_F> Colors;

    HRESULT hr = _Artwork.GetColors(Colors, _Configuration._NumArtworkColors, _Configuration._LightnessThreshold, _Configuration._TransparencyThreshold);

    // Sort the colors.
    if (SUCCEEDED(hr))
    {
        _Configuration._DominantColor = Colors[0];

        #pragma warning(disable: 4061) // Enumerator not handled
        switch (_Configuration._ColorOrder)
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
        hr = _Direct2D.CreateGradientStops(Colors, _Configuration._ArtworkGradientStops);

    if (SUCCEEDED(hr))
    {
        if (_Configuration._ColorScheme == ColorScheme::Artwork)
        {
            // Inform the other elements about the change.
            _Configuration._GradientStops = _Configuration._ArtworkGradientStops;

            Spectrum & s = _Graph.GetSpectrum();

            s.Initialize(&_Configuration);

            if (_ConfigurationDialog.IsWindow())
                _ConfigurationDialog.SendMessageW(WM_CONFIGURATION_CHANGED, CC_GRADIENT_STOPS);
        }
    }

    return S_OK; // Make sure resource creation continues even if something goes wrong while creating the gradient.
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void UIElement::ReleaseDeviceSpecificResources()
{
    _Graph.ReleaseDeviceSpecificResources();
    _FrameCounter.ReleaseDeviceSpecificResources();

    _Artwork.Release();

    _RenderTarget.Release();
}

#pragma endregion
