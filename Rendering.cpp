
/** $VER: UIElement.cpp (2024.01.03) P. Stuer **/

#include "UIElement.h"

#include "DirectX.h"
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
    if (!::TryEnterCriticalSection(&_Lock))
        return;

    _FrameCounter.NewFrame();

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && !(_RenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        _RenderTarget->Clear(_Configuration._UseCustomBackColor ? _Configuration._BackColor : _Configuration._DefBackColor);

        {
            double PlaybackTime; // in sec

            if (_VisualisationStream.is_valid() && _VisualisationStream->get_absolute_time(PlaybackTime))
            {
                if (PlaybackTime < _OldPlaybackTime)
                    _OldPlaybackTime = 0.;

//              double WindowSize = 0.92;//(_SampleRate != 0) ? ((double) _FFTSize / (double) _SampleRate) : PlaybackTime - _OldPlaybackTime;//.2;
//              double WindowSize = (double) _FFTSize * 2. / (double) _SampleRate;
//              double WindowSize = .2; // in sec
                double WindowSize = (double) _FFTSize / _Configuration._HiFrequency;

                audio_chunk_impl Chunk;

//              if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime /*- (WindowSize / 2.)*/, WindowSize))
                if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime, WindowSize))
                {
                    ProcessAudioChunk(Chunk);

                    _OldPlaybackTime = PlaybackTime;
                }
            }
            else
                for (auto & Iter : _FrequencyBands)
                    Iter.CurValue = 0.;
        }

        // Update the peak indicators.
        if ((_FFTAnalyzer != nullptr) && (_Configuration._VisualizationType == VisualizationType::Bars) && (_Configuration._PeakMode != PeakMode::None))
            _FFTAnalyzer->UpdatePeakIndicators(_FrequencyBands);

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

    ::LeaveCriticalSection(&_Lock);
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
        #pragma warning (disable: 4061)
        switch (_Configuration._FFTSize)
        {
            default:
                _FFTSize = (size_t) (64. * ::exp2((long) _Configuration._FFTSize));
                break;

            case FFTSize::FFTCustom:
                _FFTSize = (_Configuration._FFTCustom > 0) ? (size_t) _Configuration._FFTCustom : 64;
                break;

            case FFTSize::FFTDuration:
                _FFTSize = (_Configuration._FFTDuration > 0.) ? (size_t) (((double) _SampleRate * _Configuration._FFTDuration) / 1000.) : 64;
                break;
        }
        #pragma warning (default: 4061)

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

        int n = (int) i % 12;

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], (int) i / 12, fb.Ctr);

        fb.BackColor = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10) ? _Configuration._DarkBandColor : _Configuration._LiteBandColor;

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

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);

        hr = _DirectX._Direct2D->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
            _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Resize some elements based on the size of the render target.
        Resize();
    }

    if (SUCCEEDED(hr))
        hr = _FrameCounter.CreateDeviceSpecificResources(_RenderTarget);

    if (SUCCEEDED(hr))
        hr = _Graph.CreateDeviceSpecificResources(_RenderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void UIElement::ReleaseDeviceSpecificResources()
{
    _Graph.ReleaseDeviceSpecificResources();
    _FrameCounter.ReleaseDeviceSpecificResources();

    _RenderTarget.Release();
}
#pragma endregion

