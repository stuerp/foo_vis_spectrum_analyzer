
/** $VER: Rendering.cpp (2024.01.21) P. Stuer **/

#include "UIElement.h"

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
    ((UIElement *) context)->RenderFrame();
}

/// <summary>
/// Renders a frame.
/// </summary>
void UIElement::RenderFrame()
{
    if (!_CriticalSection.TryEnter())
        return;

    _FrameCounter.NewFrame();

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && !(_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        if (_IsStopping)
        {
            _Artwork.Release();

            for (auto & Iter : _FrequencyBands)
                Iter.CurValue = 0.;

            _IsStopping = false;
        }

        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        RenderBackground();

        {
            double PlaybackTime; // in sec

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

        _Graph.Render(_RenderTarget, _FrequencyBands, (double) _SampleRate);

        if (_Configuration._ShowFrameCounter)
            _FrameCounter.Render(_RenderTarget);

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
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
        _RenderTarget->Clear(_DominantColor);
    else
        _RenderTarget->Clear(_Configuration._UseCustomBackColor ? _Configuration._BackColor : _Configuration._DefBackColor);

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

    _RenderTarget->DrawBitmap(_Artwork.Bitmap(), Rect, _Configuration._ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
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
    HRESULT hr = S_OK;

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

    // Create the background bitmap from the artwork.
    if (SUCCEEDED(hr) && _NewArtwork)
    {
        hr = _Artwork.Realize(_RenderTarget);

        if (SUCCEEDED(hr))
            _Configuration._ArtworkGradientStops.clear();

        _NewArtwork = false;
    }

    bool NewArtworkGradientStops = false;

    // Create the gradient stops based on the artwork. Done at least once per artwork because the configuration dialog needs it when ColorScheme::Artwork is selected.
    if (SUCCEEDED(hr) && (_Artwork.Bitmap() != nullptr) && ((_Configuration._ArtworkGradientStops.size() == 0) || _Configuration._NewArtworkParameters))
    {
        _Configuration._NewArtworkParameters = false;

        // Get the colors from the artwork.
        std::vector<D2D1_COLOR_F> Colors;

        hr = _Artwork.GetColors(Colors, _Configuration._NumArtworkColors, _Configuration._LightnessThreshold, _Configuration._TransparencyThreshold);

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
            NewArtworkGradientStops = true;

            if (_Configuration._ColorScheme == ColorScheme::Artwork)
            {
                _Configuration._GradientStops = _Configuration._ArtworkGradientStops;

                // Notify the configuration dialog about the change.
                if (_ConfigurationDialog.IsWindow())
                    _ConfigurationDialog.SendMessageW(WM_COLORS_CHANGED);
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr))
        hr = _Graph.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr) && NewArtworkGradientStops)
    {
        Spectrum & s = _Graph.GetSpectrum();

        s.Initialize(&_Configuration); // FIXME
    }

    return hr;
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
