
/** $VER: Rendering.cpp (2024.01.28) P. Stuer **/

#include "UIElement.h"

#include "DXGI.h"
#include "Direct3D.h"
#include "Direct2D.h"
#include "DirectWrite.h"
#include "WIC.h"

#include "Resources.h"
#include "Gradients.h"

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
    ((UIElement *) context)->Render();
}

/// <summary>
/// Renders a frame.
/// </summary>
void UIElement::Render()
{
    if (!_CriticalSection.TryEnter())
        return;

    _FrameCounter.NewFrame();

    ProcessPlaybackEvent();

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr))
    {
        _DC->BeginDraw();

        RenderBackground();
        RenderForeground();

        hr = _DC->EndDraw();

        // Present the swap chain to the composition engine.
        hr = _SwapChain->Present(1, 0);

        if (!SUCCEEDED(hr) && (hr != DXGI_STATUS_OCCLUDED))
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }

    _CriticalSection.Leave();
}

/// <summary>
/// Renders the background.
/// </summary>
void UIElement::RenderBackground() const
{
    if ((_Configuration._BackgroundMode == BackgroundMode::ArtworkAndDominantColor) && (_Configuration._ArtworkGradientStops.size() > 0))
        _DC->Clear(_DominantColor);
    else
    if (_Configuration._BackgroundMode == BackgroundMode::None)
        _DC->Clear(D2D1::ColorF(0, 0.f));
    else
        _DC->Clear(_Configuration._UseCustomBackColor ? _Configuration._BackColor : _Configuration._DefBackColor);

    // Render the album art if there is any.
    if ((_Artwork.Bitmap() == nullptr) || !((_Configuration._BackgroundMode == BackgroundMode::Artwork) || (_Configuration._BackgroundMode == BackgroundMode::ArtworkAndDominantColor)))
        return;

    D2D1_SIZE_F Size = _Artwork.Size();
    D2D1_RECT_F Rect = _Graph.GetBounds();

    FLOAT MaxWidth  = Rect.right  - Rect.left;
    FLOAT MaxHeight = Rect.bottom - Rect.top;

    // Fit big images (Free / FitBig / FitWidth / FitHeight)
    {
        // Fit big images.
        FLOAT HScalar = (Size.width  > MaxWidth)  ? (FLOAT) MaxWidth  / (FLOAT) Size.width  : 1.f;
        FLOAT VScalar = (Size.height > MaxHeight) ? (FLOAT) MaxHeight / (FLOAT) Size.height : 1.f;

        FLOAT Scalar = (std::min)(HScalar, VScalar);

        Size.width  *= Scalar;
        Size.height *= Scalar;
    }

    Rect.left   = (MaxWidth  - Size.width)  / 2.f;
    Rect.top    = (MaxHeight - Size.height) / 2.f;
    Rect.right  = Rect.left + Size.width;
    Rect.bottom = Rect.top  + Size.height;

    _DC->DrawBitmap(_Artwork.Bitmap(), Rect, _Configuration._ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void UIElement::RenderForeground()
{
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
        else
            for (auto & Iter : _FrequencyBands)
                Iter.CurValue = 0.;

        // Update the peak indicators.
        if ((_FFTAnalyzer != nullptr) && (_Configuration._PeakMode != PeakMode::None))
            _FFTAnalyzer->UpdatePeakIndicators(_FrequencyBands);
    }

    _Graph.Render(_DC, _FrequencyBands, (double) _SampleRate);

    if (_Configuration._ShowFrameCounter)
        _FrameCounter.Render(_DC);
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
                _DominantColor = _Configuration._ArtworkGradientStops[0].color;

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

#pragma region DirectX

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT UIElement::CreateDeviceIndependentResources()
{
    HRESULT hr = _Direct3D.GetDXGIDevice(&_DXGIDevice);

    // Create the Direct2D device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = _Direct2D.Factory->CreateDevice(_DXGIDevice, &_D2DDevice);

    // Create the DirectComposition device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = ::DCompositionCreateDevice(_DXGIDevice, __uuidof(_CompositionDevice), (void **) &_CompositionDevice);

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceIndependentResources();

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
    HRESULT hr = S_OK;

#ifdef old
    // Create the render target.
    if (_RenderTarget == nullptr)
    {
        CRect rc;

        GetClientRect(rc);

        D2D1_SIZE_U Size = D2D1::SizeU((UINT32) rc.Width(), (UINT32) rc.Height());

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

            // Resize some elements based on the size of the render target.
            Resize();
        }
    }
#endif

    UINT Width  = 0;
    UINT Height = 0;

    // Create the Direct2D device context that is the actual render target and exposes drawing commands.
    if (_DC == nullptr)
    {
        CRect cr;

        GetClientRect(cr);

        Width  = (UINT) (cr.right  - cr.left);
        Height = (UINT) (cr.bottom - cr.top);

        hr = (Width != 0) && (Height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

        if (SUCCEEDED(hr))
        {
            hr = _D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_DC);

            // Set the DPI of the device context based on that of the target window.
            if (SUCCEEDED(hr))
            {
                UINT DPI;

                GetDPI(m_hWnd, DPI);

                _DC->SetDpi((FLOAT) DPI, (FLOAT) DPI);
                _DC->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }
        }
    }

    // Create the swap chain.
    if (SUCCEEDED(hr) && (_SwapChain == nullptr))
    {
        hr = _DXGI.CreateSwapChain(_DXGIDevice, Width, Height, &_SwapChain);

        if (SUCCEEDED(hr))
            hr = CreateSwapChainBuffers(_DC, _SwapChain);
    }

    // Create the composition target.
    if (SUCCEEDED(hr) && (_CompositionTarget == nullptr))
    {
        hr = _CompositionDevice->CreateTargetForHwnd(m_hWnd, true, &_CompositionTarget);

        if (SUCCEEDED(hr) && (_CompositionVisual == nullptr))
            hr = _CompositionDevice->CreateVisual(&_CompositionVisual);

        if (SUCCEEDED(hr))
            hr = _CompositionTarget->SetRoot(_CompositionVisual);
    }

    if (SUCCEEDED(hr))
        hr = _CompositionVisual->SetContent(_SwapChain);

    if (SUCCEEDED(hr))
        hr = _CompositionDevice->Commit();

    // Create the background bitmap from the artwork.
    if (SUCCEEDED(hr) && _NewArtwork)
    {
        Spectrum & s = _Graph.GetSpectrum();

        s.ReleaseDeviceSpecificResources();

        hr = _Artwork.Realize(_DC);

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
        hr = _FrameCounter.CreateDeviceSpecificResources(_DC);

    if (SUCCEEDED(hr))
        hr = _Graph.CreateDeviceSpecificResources(_DC);

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
        _DominantColor = Colors[0];

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

    _SwapChain.Release();
    _DC.Release();
}

/// <summary>
/// Resizes the swap chain buffers.
/// </summary>
void UIElement::ResizeSwapChain(UINT width, UINT height) noexcept
{
    // Release the reference to the swap chain before resizing its buffers.
    _DC->SetTarget(nullptr);

    HRESULT hr = (width != 0) && (height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    if (SUCCEEDED(hr))
        hr = _SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    if (SUCCEEDED(hr))
        CreateSwapChainBuffers(_DC, _SwapChain);
    else
        ReleaseDeviceSpecificResources();
}

/// <summary>
/// Creates the swap chain buffers.
/// </summary>
HRESULT UIElement::CreateSwapChainBuffers(ID2D1DeviceContext * dc, IDXGISwapChain1 * swapChain) noexcept
{
    // Retrieve the swap chain's back buffer.
    CComPtr<IDXGISurface2> Surface;

    HRESULT hr = swapChain->GetBuffer(0, __uuidof(Surface), (void **) &Surface);

    // Create a Direct2D bitmap that points to the swap chain surface.
    CComPtr<ID2D1Bitmap1> SurfaceBitmap;

    if (SUCCEEDED(hr) && (SurfaceBitmap == nullptr))
    {
        D2D1_BITMAP_PROPERTIES1 Properties = {};

        Properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        Properties.pixelFormat.format    = DXGI_FORMAT_B8G8R8A8_UNORM;
        Properties.bitmapOptions         = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = _DC->CreateBitmapFromDxgiSurface(Surface, Properties, &SurfaceBitmap);

        // Set the surface bitmap as the target of the device context for rendering.
        if (SUCCEEDED(hr))
            _DC->SetTarget(SurfaceBitmap);
    }

    return hr;
}

#pragma endregion
