
/** $VER: Rendering.cpp (2024.01.17) P. Stuer **/

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
            if ((_FFTAnalyzer != nullptr) && (_Configuration._VisualizationType == VisualizationType::Bars) && (_Configuration._PeakMode != PeakMode::None))
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

/// <summary>
/// Analyzes an an audio chunk.
/// </summary>
void UIElement::ProcessAudioChunk(const audio_chunk & chunk) noexcept
{
    _SampleRate = chunk.get_sample_rate();

    GetAnalyzer(chunk);

    // Add the samples to the analyzer.
    {
        const audio_sample * Samples = chunk.get_data();

        if (Samples == nullptr)
            return;

        size_t SampleCount = chunk.get_sample_count();

        if (_Configuration._Transform == Transform::FFT)
        {
            _FFTAnalyzer->Add(Samples, SampleCount, _Configuration._SelectedChannels);

            _FFTAnalyzer->GetFrequencyCoefficients(_FrequencyCoefficients);

            if (_Configuration._MappingMethod == Mapping::Standard)
                _FFTAnalyzer->GetFrequencyBands(_FrequencyCoefficients, _SampleRate, _Configuration._SummationMethod, _FrequencyBands);
            else
                _FFTAnalyzer->GetFrequencyBands(_FrequencyCoefficients, _SampleRate, _FrequencyBands);
        }
        else
            _CQTAnalyzer->GetFrequencyBands(Samples, SampleCount, _Configuration._SelectedChannels, _FrequencyBands);
    }

    // Smooth the spectrum.
    switch (_Configuration._SmoothingMethod)
    {
        default:

        case SmoothingMethod::Average:
        {
            ApplyAverageSmoothing(_Configuration._SmoothingFactor);
            break;
        }

        case SmoothingMethod::Peak:
        {
            ApplyPeakSmoothing(_Configuration._SmoothingFactor);
            break;
        }
    }
}

/// <summary>
/// Gets an analyzer and any supporting objects.
/// </summary>
void UIElement::GetAnalyzer(const audio_chunk & chunk) noexcept
{
    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    if (_WindowFunction == nullptr)
        _WindowFunction = WindowFunction::Create(_Configuration._WindowFunction, _Configuration._WindowParameter, _Configuration._WindowSkew, _Configuration._Truncate);

    if (_FFTAnalyzer == nullptr)
    {
        _FFTAnalyzer = new FFTAnalyzer(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, _FFTSize, &_Configuration);

        _FrequencyCoefficients.resize(_FFTSize);
    }

    if (_CQTAnalyzer == nullptr)
        _CQTAnalyzer = new CQTAnalyzer(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, 1.0, 1.0, 0.0, &_Configuration);
}

/// <summary>
/// Generates frequency bands.
/// </summary>
void UIElement::GenerateLinearFrequencyBands()
{
    const double MinFreq = ScaleF(_Configuration._LoFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);
    const double MaxFreq = ScaleF(_Configuration._HiFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);

    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = DeScaleF(Map((double) i - _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Ctr = DeScaleF(Map((double) i,              0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Hi  = DeScaleF(Map((double) i + _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);

        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void UIElement::GenerateOctaveFrequencyBands()
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (_Configuration._Pitch > 0.) ? ::round((::log2(_Configuration._Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = _Configuration._Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NotesGroup = 24. / _Configuration._BandsPerOctave;

    const double LoNote = ::round(_Configuration._MinNote * 2. / NotesGroup);
    const double HiNote = ::round(_Configuration._MaxNote * 2. / NotesGroup);

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - _Bandwidth) * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24,  (i               * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24, ((i + _Bandwidth) * NotesGroup + _Configuration._Transpose)),
        };

        int Note = (int) (i * (NotesGroup / 2.));

        int n = Note % 12;

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], Note / 12, fb.Ctr);

        fb.BackColor = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10) ? _Configuration._DarkBandColor : _Configuration._LightBandColor;

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands of AveePlayer.
/// </summary>
void UIElement::GenerateAveePlayerFrequencyBands()
{
    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i - _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Ctr = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i,              _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Hi  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i + _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double UIElement::ScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::log2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::Mel:
            return ::log2(1.0 + x / 700.0);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * x) / (1960.0 + x) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * x) / (::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * x);

        case ScalingFunction::Cams:
            return ::log2((x / 1000.0 + 0.312) / (x / 1000.0 + 14.675));

        case ScalingFunction::HyperbolicSine:
            return ::asinh(x / ::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return ::pow(x, (1.0 / (11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::exp2(-x / ::exp2(7 + skewFactor * 8));

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Descales the frequency.
/// </summary>
double UIElement::DeScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case ScalingFunction::Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case ScalingFunction::AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case ScalingFunction::Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case ScalingFunction::HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case ScalingFunction::NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Smooths the spectrum using averages.
/// </summary>
void UIElement::ApplyAverageSmoothing(double factor)
{
    if (factor != 0.0)
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = (::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0) + (::isfinite(Iter.NewValue) ? Iter.NewValue * (1.0 - factor) : 0.0);

    }
    else
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = Iter.NewValue;
    }
}

/// <summary>
/// Smooths the spectrum using peak decay.
/// </summary>
void UIElement::ApplyPeakSmoothing(double factor)
{
    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue = Max(::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0, ::isfinite(Iter.NewValue) ? Iter.NewValue : 0.0);
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
