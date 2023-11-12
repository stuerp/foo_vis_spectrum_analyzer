
/** $VER: SpectrumAnalyzerUI.cpp (2023.11.11) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#include <vector>
#include <algorithm>

#pragma hdrstop

struct FrequencyBand
{
    FrequencyBand() : lo(), ctr(), hi() { }
    FrequencyBand(double l, double c, double h) : lo(l), ctr(c), hi(h) { }

    double lo;
    double ctr;
    double hi;
};

std::vector<FrequencyBand> FrequencyBands;

std::vector<double> spectrum;

std::vector<double> currentSpectrum;

enum SummationMode
{
    Minimum,
    Maximum,
    Sum,
    RMSSum,
    RMS,
    Average,
    Median
};

enum TimeSmootingMethod
{
    MethodAverage,
    MethodPeak
};

enum ScalingFunctions
{
    Logarithmic = 1,
};

enum FrequencyDistribution
{
    Octaves,
    Frequencies,
    AveePlayer,
};

struct Settings
{
    FFTSize _FFTSize = FFTSize::Fft4096;

    FrequencyDistribution _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Common
    size_t _numBands =    320;  // Number of frequency bands, 2 .. 512
    uint32_t _minFreq =    20;  // Hz, 0 .. 96000
    uint32_t _maxFreq = 20000;  // Hz, 0 .. 96000

    // Octaves
    uint32_t _octaves =  12;    // Bands per octave, 1 .. 48
    uint32_t _minNote =   4;    // Minimum note, 0 .. 128
    uint32_t _maxNote = 124;    // Maximum note, 0 .. 128
    uint32_t _detune =    0;    // Detune, -24 ..24
    uint32_t _noteTuning = 440; // Hz, 0 .. 96000, Octave bands tuning (nearest note = tuning frequency in Hz)

    // Frequencies
    ScalingFunctions _fscale = Logarithmic;

    double _hzLinearFactor = 0.0;   // Hz linear factor, 0.0 .. 1.0
    double _bandwidth = 0.5;        // Bandwidth, 0.0 .. 64.0

    TimeSmootingMethod _SmoothingMethod = TimeSmootingMethod::MethodAverage;    // Time smoothing method
    double _SmoothingConstant = 0.0;                                            // Time smoothing constant, 0.0 .. 1.0

    int interpSize = 32;                                    // Lanczos interpolation kernel size, 1 .. 64
    SummationMode _SummationMode = SummationMode::Maximum;  // Band power summation method
    bool smoothInterp = true;                               // Smoother bin interpolation on lower frequencies
    bool smoothSlope = true;                                // Smoother frequency slope on sum modes

    // ascale() Amplitude Scale
    bool _UseDecibels = true;                               // Use decibel scale or logaritmic amplitude

    double _MinDecibels = -90.;                             // Lower amplitude, -120.0 .. 0.0
    double _MaxDecibels =   0.;                             // Upper amplitude, -120.0 .. 0.0

    bool _UseAbsolute = true;                               // Use absolute value

    double _gamma = 1.;                                     // Gamma, 0.5 .. 10


/*
        type: 'fft',
        bandwidthOffset: 1,

        windowFunction: 'hann',
        windowParameter: 1,
        windowSkew: 0,

        timeAlignment: 1,
        downsample: 0,
        useComplex: true,
        holdTime: 30,
        fallRate: 0.5,
        clampPeaks: true,
        peakMode: 'gravity',
        showPeaks: true,

        freeze: false,
        color: 'none',
        showLabels: true,
        showLabelsY: true,
        labelTuning: 440,
        showDC: true,
        showNyquist: true,
        mirrorLabels: true,
        diffLabels: false,
        labelMode : 'decade',
        darkMode: false,
        compensateDelay: false
*/
} _Settings;

struct Complex
{
    double re;
    double im;
};

static bool _IsPlaying = false;

/// <summary>
/// Returns the minimum value of the specified values.
/// </summary>
template <class T>
inline static T Min(T a, T b)
{
    return (a < b) ? a : b;
}

/// <summary>
/// Returns the maximum value of the specified values.
/// </summary>
template <class T>
inline static T Max(T a, T b)
{
    return (a > b) ? a : b;
}

/// <summary>
/// Returns the logarithm of a specified number in a specified base.
/// </summary>
/// <param name="a">The number whose logarithm is to be found.</param>
/// <param name="newBase">The base of the logarithm.</param>
inline static double Log(double a, double newBase)
{
    if (a == NAN)
        return a;

    if (newBase == NAN)
        return newBase;

    if (newBase == 1.0)
        return NAN;

    if (a != 1.0 && (newBase == 0.0 || newBase == INFINITY))
        return NAN;

    return log(a) / log(newBase);
}

/// <summary>
/// Converts magnitude to decibel (dB).
/// </summary>
inline static double ToDecibel(double magnitude)
{
    return 20.0 * ::log10(magnitude);
}

/// <summary>
/// Converts decibel (dB) to magnitude.
/// </summary>
inline static double ToMagnitude(double dB)
{
    return ::pow(10.0, dB / 20.0);
}

/// <summary>
/// Converts points to DIPs (Device Independet Pixels).
/// </summary>
inline static FLOAT ToDIPs(FLOAT points)
{
    return (points / 72.0f) * 96.0f; // FIXME: Should 96.0 change on high DPI screens?
}

/// <summary>
/// Constructor
/// </summary>
SpectrumAnalyzerUIElement::SpectrumAnalyzerUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback) : m_callback(callback), _LastRefresh(0), _RefreshInterval(10)
{
    set_configuration(data);
}

#pragma region CWindowImpl

/// <summary>
/// Gets the window class definition.
/// </summary>
CWndClassInfo & SpectrumAnalyzerUIElement::GetWndClassInfo()
{
    static ATL::CWndClassInfo wci =
    {
        {
            sizeof(WNDCLASSEX),
            CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
            StartWindowProc,
            0, 0,
            NULL, // Instance,
            NULL, // Icon
            NULL, // Cursor
            (HBRUSH) NULL, // Background
            NULL, // Menu
            TEXT(STR_SPECTOGRAM_WINDOW_CLASS), // Class name
            NULL // Small Icon
        },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };

    return wci;
}

/// <summary>
/// Creates the window.
/// </summary>
LRESULT SpectrumAnalyzerUIElement::OnCreate(LPCREATESTRUCT cs)
{
    HRESULT hr = S_OK;

    hr = CreateDeviceIndependentResources();

    if (FAILED(hr))
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Unable to create Direct2D device independent resources.";
    }

    try
    {
        static_api_ptr_t<visualisation_manager> VisualisationManager;

        VisualisationManager->create_stream(_VisualisationStream, 0);

        _VisualisationStream->request_backlog(0.8);
    }
    catch (std::exception & ex)
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Exception while creating visualisation stream. " << ex;
    }

    return 0;
}

/// <summary>
/// Destroys the window.
/// </summary>
void SpectrumAnalyzerUIElement::OnDestroy()
{
    if (_SpectrumAnalyzer)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    _VisualisationStream.release();

    ReleaseDeviceSpecificResources();

    _Direct2dFactory.Release();
}

/// <summary>
/// Handles the WM_TIMER message.
/// </summary>
void SpectrumAnalyzerUIElement::OnTimer(UINT_PTR timerID)
{
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void SpectrumAnalyzerUIElement::OnPaint(CDCHandle hDC)
{
    Render();
    ValidateRect(nullptr);

    if (!_IsPlaying)
        return;

    ULONGLONG Now = ::GetTickCount64(); // in ms, with a resolution of the system timer, which is typically in the range of 10ms to 16ms.

    if (_VisualisationStream.is_valid())
    {
        ULONGLONG NextRefresh = _LastRefresh + _RefreshInterval;

        if (NextRefresh < Now)
            NextRefresh = Now;

        // Schedule the next refresh. Limited to USER_TIMER_MINIMUM (10ms / 100Hz).
        SetTimer(ID_REFRESH_TIMER, (UINT)(NextRefresh - Now));
    }

    _LastRefresh = Now;
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
void SpectrumAnalyzerUIElement::OnSize(UINT type, CSize size)
{
    if (!_RenderTarget)
        return;

    _RenderTarget->Resize(D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy));
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::OnContextMenu(CWindow wnd, CPoint point)
{
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
    }
    else
    {
        CMenu Menu;

        Menu.CreatePopupMenu();
        Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Toggle Full-Screen Mode"));
        Menu.AppendMenu((UINT) MF_STRING | (_Configuration._UseHardwareRendering ? MF_CHECKED : 0), IDM_HW_RENDERING_ENABLED, TEXT("Hardware Rendering"));

        Menu.SetMenuDefaultItem(IDM_TOGGLE_FULLSCREEN);

        CMenu RefreshRateLimitMenu;

        RefreshRateLimitMenu.CreatePopupMenu();
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  20) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20,  TEXT("20 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  60) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_60,  TEXT("60 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 100) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_100, TEXT("100 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 200) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_200, TEXT("200 Hz"));

        Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, TEXT("Refresh Rate Limit"));

        int CommandId = Menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

        switch (CommandId)
        {
            case IDM_TOGGLE_FULLSCREEN:
                ToggleFullScreen();
                break;

            case IDM_HW_RENDERING_ENABLED:
                ToggleHardwareRendering();
                break;

            case IDM_REFRESH_RATE_LIMIT_20:
                _Configuration._RefreshRateLimit = 20;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_60:
                _Configuration._RefreshRateLimit = 60;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_100:
                _Configuration._RefreshRateLimit = 100;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_200:
                _Configuration._RefreshRateLimit = 200;
                UpdateRefreshRateLimit();
                break;
        }

        Invalidate();
    }
}

/// <summary>
/// Toggles between panel and full screen mode.
/// </summary>
void SpectrumAnalyzerUIElement::OnLButtonDblClk(UINT flags, CPoint point)
{
    ToggleFullScreen();
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::ToggleFullScreen()
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::ToggleHardwareRendering()
{
    _Configuration._UseHardwareRendering = !_Configuration._UseHardwareRendering;
    ReleaseDeviceSpecificResources();
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::UpdateRefreshRateLimit()
{
    _RefreshInterval = pfc::clip_t<DWORD>(1000 / (DWORD)_Configuration._RefreshRateLimit, 5, 1000);
}

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::Render()
{
    // Frame counter
    {
        LARGE_INTEGER Time;

        ::QueryPerformanceCounter(&Time);

        _Times.Add(Time.QuadPart);
    }

    HRESULT hr = CreateDeviceSpecificResources();

    if (SUCCEEDED(hr))
    {
        _RenderTarget->BeginDraw();

        _RenderTarget->SetAntialiasMode(_Configuration._UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the background.
        {
            t_ui_color BackgroundColor = m_callback->query_std_color(ui_color_background);

            //FIXME: Should be dynamic
            BackgroundColor = RGB(0, 0, 0);

            _RenderTarget->Clear(D2D1::ColorF(GetRValue(BackgroundColor) / 255.0f, GetGValue(BackgroundColor) / 255.0f, GetBValue(BackgroundColor) / 255.0f));
        }

        if (_IsPlaying)
        {
            // Draw the visualization.
            if (_VisualisationStream.is_valid())
            {
                double PlaybackTime; // in ms

                if (_VisualisationStream->get_absolute_time(PlaybackTime))
                {
                    double WindowDuration = _Configuration.GetWindowDurationInMs();

                    audio_chunk_impl Chunk;

                    if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - WindowDuration / 2, WindowDuration * (_Configuration._UseZeroTrigger ? 2 : 1)))
                        RenderChunk(Chunk);
                }
            }

            // Draw the text.
//          hr = RenderText();
        }

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            ReleaseDeviceSpecificResources();

            hr = S_OK;
        }
    }

    return hr;
}

inline static double Map(double value, double minValue, double maxValue, double minTarget, double maxTarget)
{
    return minTarget + ((value - minValue) * (maxTarget - minTarget)) / (maxValue - minValue);
}

inline static double clamp(double x, double min, double max)
{
    return Min(Max(x, min), max);
}

/// <summary>
/// Scales the frequency.
/// </summary>
static double fscale(double x, ScalingFunctions function, double freqSkew)
{
    switch (function)
    {
        case Logarithmic:
            return ::log2(x);

        default:
            return x;
    }
}

/// <summary>
/// Scales the amplitude.
/// </summary>
static double ascale(double x)
{
    if (_Settings._UseDecibels)
        return Map(ToDecibel(x), _Settings._MinDecibels, _Settings._MaxDecibels, 0.0, 1.0);
    else
        return Map(::pow(x, (1.0 / _Settings._gamma)),
            (!_Settings._UseAbsolute ? 1.0 : 0.0) * ::pow(ToMagnitude(_Settings._MinDecibels), (1.0 / _Settings._gamma)),
                                                    ::pow(ToMagnitude(_Settings._MaxDecibels), (1.0 / _Settings._gamma)),
//          (!_Settings._UseAbsolute ? 1.0 : 0.0) * ::pow(::pow(10, (_Settings._MinDecibels / 20)), (1.0 / _Settings._gamma)),
//                                                  ::pow(::pow(10, (_Settings._MaxDecibels / 20)), (1.0 / _Settings._gamma)),
            0.0, 1.0);
}

double invFscale(double x, ScalingFunctions function, double freqSkew)
{
    switch (function)
    {
        case Logarithmic:
            return ::exp2(x);

        default:
            return x;
    }
}

void generateOctaveBands(uint32_t bandsPerOctave, uint32_t lowerNote, uint32_t higherNote, uint32_t detune, uint32_t tuningFreq, double bandwidth)
{
    FrequencyBands.clear();

    double tuningNote = ::isfinite(::log2(tuningFreq)) ? ::round((::log2(tuningFreq) - 4.0) * 12.0) * 2.0 : 0.0;
    double root24 = ::exp2(1. / 24.);
    double c0 = tuningFreq * ::pow(root24, -tuningNote); // ~16.35 Hz
    double groupNotes = 24 / bandsPerOctave;

    for (double i = ::round(lowerNote * 2 / groupNotes); i <= ::round(higherNote * 2 / groupNotes); ++i)
    {
        FrequencyBand fb = 
        {
            c0 * ::pow(root24, ((i - bandwidth) * groupNotes + detune)),
            c0 * ::pow(root24,  (i * groupNotes              + detune)),
            c0 * ::pow(root24, ((i + bandwidth) * groupNotes + detune))
        };

        FrequencyBands.push_back(fb);
    }
}

void generateFreqBands(size_t numBands, uint32_t loFreq, uint32_t hiFreq, ScalingFunctions scalingFunction, double freqSkew, double bandwidth)
{
    FrequencyBands.resize(_Settings._numBands);

    for (double i = 0.0; i < (double) FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = FrequencyBands[(size_t) i];

        Iter.lo  = invFscale(Map(i - bandwidth, 0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
        Iter.ctr = invFscale(Map(i,             0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
        Iter.hi  = invFscale(Map(i + bandwidth, 0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
    }
}

// Hz and FFT bin conversion
double hertzToFFTBin(double x, size_t bufferSize = 4096, uint32_t sampleRate = 44100)
{
    return x * (double) bufferSize / sampleRate;
}

uint32_t fftBinToHertz(size_t x, size_t bufferSize = 4096, uint32_t sampleRate = 44100)
{
    return (uint32_t)((size_t)(x * sampleRate) / bufferSize);
}

double lanzcos(Complex * data, size_t length, double x, int kernelSize = 4, bool useComplex = false)
{
    Complex Sum = { 0., 0. };

    for (int i = -kernelSize + 1; i <= kernelSize; ++i)
    {
        double pos = ::floor(x) + i; // i + x
        double twiddle = x - pos; // -pos + ::round(pos) + i

        double w = ::fabs(twiddle) <= 0 ? 1 : ::sin(twiddle * M_PI) / (twiddle * M_PI) * ::sin(M_PI * twiddle / kernelSize) / (M_PI * twiddle / kernelSize);
        int idx = (int) (((int) pos % length + length) % length);

        Sum.re += data[idx].re * w * (-1 + (i % 2 + 2) % 2 * 2);
        Sum.im += data[idx].im * w * (-1 + (i % 2 + 2) % 2 * 2);
    }

    return ::hypot(Sum.re, Sum.im);
}

double lanzcos(const double * fftCoeffs, size_t length, double x, int kernelSize = 4, bool useComplex = false)
{
    double Sum = 0.;

    for (int i = -kernelSize + 1; i <= kernelSize; ++i)
    {
        double pos = ::floor(x) + i;
        double twiddle = x - pos; // -pos + ::round(pos) + i

        double w = ::fabs(twiddle) <= 0 ? 1 : ::sin(twiddle * M_PI) / (twiddle * M_PI) * ::sin(M_PI * twiddle / kernelSize) / (M_PI * twiddle / kernelSize);
        int idx = (int) (((int) pos % length + length) % length);

        Sum += fftCoeffs[idx] * w;
    }

    return Sum;
}

double median(std::vector<double> & data)
{
    if (data.size())
        return NAN;

    if (data.size() <= 1)
        return data[0];

    std::vector<double> SortedData = data;

    sort(SortedData.begin(), SortedData.end());

    size_t Half = data.size() / 2;

    return (data.size() % 2) ? SortedData[Half] : (SortedData[Half - 1] + SortedData[Half]) / 2;
}

// Calculates bandpower from FFT (foobar2000 flavored, can be enhanced by using complex FFT coefficients instead of magnitude-only FFT data)
void calcSpectrum(const double * fftCoeffs, size_t length, std::vector<FrequencyBand> & freqBands, int interpSize, SummationMode summationMode, bool useComplex, bool smoothInterp, bool smoothGainTransition, uint32_t sampleRate)
{
    size_t j = 0;

    for (auto x : freqBands)
    {
        int minIdx = (int)  ::ceil(hertzToFFTBin(Min(x.hi, x.lo), length, sampleRate));
        int maxIdx = (int) ::floor(hertzToFFTBin(Max(x.hi, x.lo), length, sampleRate));

        int minIdx2 = (int) (smoothInterp ? ::round(hertzToFFTBin(Min(x.hi, x.lo), length, sampleRate)) + 1 : minIdx);
        int maxIdx2 = (int) (smoothInterp ? ::round(hertzToFFTBin(Max(x.hi, x.lo), length, sampleRate)) - 1 : maxIdx);

        double bandGain = smoothGainTransition && (summationMode == Sum || summationMode == RMSSum) ? ::hypot(1, ::pow(((x.hi - x.lo) * (double) length / sampleRate), (1 - (int) (summationMode == RMS || summationMode == RMSSum) / 2))) : 1.;

        if (minIdx2 > maxIdx2)
        {
            spectrum[j] = ::fabs(lanzcos(fftCoeffs, length, x.ctr * (double) length / sampleRate, interpSize, useComplex)) * bandGain;
        }
        else
        {
            double sum = (summationMode == Minimum) ? DBL_MAX : 0.;
            double diff = 0.;

            int overflowCompensation = Max(maxIdx - minIdx - (int) length, 0);

            bool isAverage = (summationMode == Average || summationMode == RMS) || ((summationMode == Sum || summationMode == RMSSum) && smoothGainTransition);
            bool isRMS = summationMode == RMS || summationMode == RMSSum;

            std::vector<double> medianData;

            for (int i = minIdx; i <= maxIdx - overflowCompensation; ++i)
            {
                int binIdx = (int)((i % length + length) % length);

                double data = fftCoeffs[binIdx];

                switch (summationMode)
                {
                    case SummationMode::Maximum:
                        sum = Max(data, sum);
                        break;

                    case SummationMode::Minimum:
                        sum = Min(data, sum);
                        break;

                    case SummationMode::Average:
                    case SummationMode::RMS:
                    case SummationMode::Sum:
                    case SummationMode::RMSSum:
                        sum += ::pow(data, (1 + isRMS));
                        break;

                    case SummationMode::Median:
                        medianData.push_back(data);
                        break;

                    default:
                        sum = data;
                }

                diff++;
            }

            if (summationMode == Median)
                sum = median(medianData);
            else
                sum /= isAverage ? diff : 1;

            spectrum[j] = (isRMS ? ::sqrt(sum) : sum) * bandGain;
        }

        ++j;
    }
}

/// <summary>
/// Applies a time smoothing factor.
/// </summary>
void calcSmoothingTimeConstant(std::vector<double> & dst, const std::vector<double> & src, double factor)
{
    if (factor != 0.0)
    {
        for (size_t i = 0; i < src.size(); ++i)
            dst[i] = (!::isnan(dst[i]) ? dst[i] * factor : 0.0) + (!::isnan(src[i]) ? src[i] * (1.0 - factor) : 0.0);
    }
    else
        dst = src;
}

void calcPeakDecay(std::vector<double> & dst, const std::vector<double> & src, double factor)
{
    for (size_t i = 0; i < src.size(); ++i)
        dst[i] = Max(!::isnan(dst[i]) ? dst[i] * factor : 0.0, !::isnan(src[i]) ? src[i] : 0.0);
}

/// <summary>
/// Render an audio chunk.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderChunk(const audio_chunk & chunk)
{
    HRESULT hr = S_OK;

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t SampleRate   = chunk.get_sample_rate();

    if (_SpectrumAnalyzer == nullptr)
    {
        _SpectrumAnalyzer = new SpectrumAnalyzer(ChannelCount, _Settings._FFTSize, SampleRate);
    }

    // Add the samples to the spectrum analyzer.
    {
        const audio_sample * Samples = chunk.get_data();

        t_size SampleCount  = chunk.get_sample_count();

        _SpectrumAnalyzer->Add(Samples, SampleCount);
    }

    // Initialize the bands.
    switch (_Settings._FrequencyDistribution)
    {
        case Octaves:
            generateOctaveBands(_Settings._octaves, _Settings._minNote, _Settings._maxNote, _Settings._detune, _Settings._noteTuning, _Settings._bandwidth);
            break;

        case AveePlayer:
//          generateAveePlayerFreqs(_Settings._numBands, _Settings._minFreq, _Settings._maxFreq, _Settings._hzLinearFactor, _Settings._bandwidth);
            break;

        case Frequencies:
        default:
            generateFreqBands(_Settings._numBands, _Settings._minFreq, _Settings._maxFreq, _Settings._fscale, _Settings._hzLinearFactor, _Settings._bandwidth);
    }

    spectrum.resize(FrequencyBands.size());
    std::fill(spectrum.begin(), spectrum.end(), 0.0);

    currentSpectrum.resize(FrequencyBands.size());
    std::fill(currentSpectrum.begin(), currentSpectrum.end(), 0.0);

    {
        // Get the frequency data.
        double * FreqData = new double[(size_t) _Settings._FFTSize];

        _SpectrumAnalyzer->GetFrequencyData(FreqData, (size_t) _Settings._FFTSize);

        // Calculate the spectrum 
        calcSpectrum(FreqData, (size_t) _Settings._FFTSize / 2, FrequencyBands, _Settings.interpSize, _Settings._SummationMode, false, _Settings.smoothInterp, _Settings.smoothSlope, SampleRate);

        switch (_Settings._SmoothingMethod)
        {
            case MethodAverage:
                calcSmoothingTimeConstant(currentSpectrum, spectrum, _Settings._SmoothingConstant);
                break;

            case MethodPeak:
                calcPeakDecay(currentSpectrum, spectrum, _Settings._SmoothingConstant);
                break;
        
            default:
                currentSpectrum = spectrum;
        }

        if (FreqData)
            delete[] FreqData;
    }

    hr = RenderBands();

    return hr;
}

/// <summary>
/// Renders the X axis.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderXAxis(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, int octave)
{
    _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // Draw the label.
    {
        WCHAR Text[16] = { };

        ::StringCchPrintfW(Text, _countof(Text), L"C%d", octave);

        D2D1_RECT_F TextRect = { x1, y1, x2, y2 };

        _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

        _LabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        _LabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        _LabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
        _RenderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _LabelTextFormat, TextRect, _TextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        #pragma warning(default: 6385)
    }

    return S_OK;
}

const FLOAT YAxisWidth = 30.f;

/// <summary>
/// Renders the Y axis.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderYAxis()
{
    const double Amplitudes[] = { 0, -6, -12, -18, -24, -30, -36, -42, -48, -54, -60, -66, -72, -78, -84, -90 }; // FIXME: Should be based on MindB and MaxdB

    const FLOAT Width  = (FLOAT) _RenderTargetProperties.pixelSize.width;
    const FLOAT Height = (FLOAT) _RenderTargetProperties.pixelSize.height - _LabelTextMetrics.height;

    const FLOAT StrokeWidth = 1.0f;

    for (size_t i = 0; i < _countof(Amplitudes); ++i)
    {
        FLOAT y = (FLOAT) Map(ascale(ToMagnitude(Amplitudes[i])), 0.0, 1.0, Height, 0.0);

        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the horizontal grid line.
        {
            _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

            _RenderTarget->DrawLine(D2D1_POINT_2F(YAxisWidth, y), D2D1_POINT_2F(Width, y), _TextBrush, StrokeWidth, nullptr);
        }

        // Draw the label.
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitudes[i]);

            FLOAT LineHeight = _LabelTextMetrics.height;

            D2D1_RECT_F TextRect = { 0.f, y - LineHeight / 2, YAxisWidth - 2.f, y + LineHeight / 2 };

            _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

            _LabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _LabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
            _RenderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _LabelTextFormat, TextRect, _TextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            #pragma warning(default: 6385)
        }
    }

    return S_OK;
}

/// <summary>
/// Renders the bands.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderBands()
{
    const FLOAT PaddingY = 1.f;

    const FLOAT RightMargin = 1.f;

    FLOAT Width = (FLOAT) _RenderTargetProperties.pixelSize.width - YAxisWidth;

    FLOAT BandWidth = Max((Width / (FLOAT) currentSpectrum.size()), 1.f);

    FLOAT x1 = YAxisWidth + (Width - ((FLOAT) currentSpectrum.size() * BandWidth)) / 2.f;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = PaddingY;
    const FLOAT y2 = (FLOAT) _RenderTargetProperties.pixelSize.height - _LabelTextMetrics.height - PaddingY;

    RenderYAxis();

    int Note = _Settings._minNote;

    for (auto Iter : currentSpectrum)
    {
        {
            if (Note % 12 == 0)
            {
                RenderXAxis(x1, y2, x2, y2 + _LabelTextMetrics.height, Note / 12);

                // Draw the vertical grid line.
                {
                    _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                    _RenderTarget->DrawLine(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x1, y2), _TextBrush, 1.f, nullptr);
                }
            }

            Note++;
        }

        D2D1_RECT_F Rect = { x1, y1, x2 - RightMargin, y2 };
#ifdef Original
        // Draw the background.
        {
            _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 0.3f)); // #1E90FF
            _RenderTarget->FillRectangle(Rect, _TextBrush);
        }

        // Draw the foreground.
        if (Iter > 0.0)
        {
            Rect.top = Max((FLOAT)(y2 - ((y2 - y1) * ascale(Iter))), y1);

            _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 1.0f)); // #1E90FF
            _RenderTarget->FillRectangle(Rect, _TextBrush);
        }
#endif
        // Draw the foreground.
        if (Iter > 0.0)
        {
            Rect.top = Max((FLOAT)(y2 - ((y2 - y1) * ascale(Iter))), y1);

            _RenderTarget->FillRectangle(Rect, _GradientBrush);
        }

        x1  = x2;
        x2 += BandWidth;
    }

    return S_OK;
}

/// <summary>
/// Renders some information about the visulizer.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderText()
{
    float FPS = 0.0f;

    {
        LARGE_INTEGER Frequency;

        ::QueryPerformanceFrequency(&Frequency);

        FPS = (float)((_Times.GetCount() - 1) * Frequency.QuadPart) / (float) (_Times.GetLast() - _Times.GetFirst());
    }

    HRESULT hr = S_OK;

    WCHAR Text[512] = { };

    switch (_Settings._FrequencyDistribution)
    {
        case FrequencyDistribution::Frequencies:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"%uHz - %uHz, %u bands, FFT %u, FPS: %.2f\n",
                    _Settings._minFreq, _Settings._maxFreq, (uint32_t) _Settings._numBands, _Settings._FFTSize, FPS);
            break;

        case FrequencyDistribution::Octaves:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"Note %u - %u, %u bands per octave, Tuning %uHz, FFT %u, FPS: %.2f\n",
                    _Settings._minNote, _Settings._maxNote, (uint32_t) _Settings._octaves, _Settings._noteTuning, _Settings._FFTSize, FPS);
            break;

        case FrequencyDistribution::AveePlayer:
            break;
    }

    if (SUCCEEDED(hr))
    {
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        static const D2D1_RECT_F TextRect = { 4.f, 4.f, 768.f, 50.f };
        static const float TextRectInset = 10.f;

        {
            _TextBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

            _RenderTarget->FillRoundedRectangle(D2D1::RoundedRect(TextRect, TextRectInset, TextRectInset), _TextBrush);
        }

        {
            _TextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

            #pragma warning(disable: 6385) // Reading invalid data from 'Text': false positive.
            _RenderTarget->DrawText
            (
                Text,
                (UINT) ::wcsnlen(Text, _countof(Text)),
                _TextFormat,
                D2D1::RectF(TextRect.left + TextRectInset, TextRect.top + TextRectInset, TextRect.right - TextRectInset, TextRect.bottom - TextRectInset),
                _TextBrush,
                D2D1_DRAW_TEXT_OPTIONS_NONE
            );
            #pragma warning(default: 6385)
        }
    }

    return hr;
}

/// <summary>
/// Create resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::CreateDeviceIndependentResources()
{
    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_Direct2dFactory);

    if (SUCCEEDED(hr))
        hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_DirectWriteFactory), reinterpret_cast<IUnknown **>(&_DirectWriteFactory));

    if (SUCCEEDED(hr))
    {
        static const PCWSTR FontFamilyName = L"Calibri";
        static const FLOAT FontSize = 20.0f; // In DIP

        hr = _DirectWriteFactory->CreateTextFormat(FontFamilyName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
    }

    if (SUCCEEDED(hr))
    {
        static const PCWSTR FontFamilyName = L"Segoe UI";
        static const FLOAT FontSize = ToDIPs(6.0f); // In DIP

        hr = _DirectWriteFactory->CreateTextFormat(FontFamilyName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_LabelTextFormat);
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectWriteFactory->CreateTextLayout(L"-90dB", 1, _LabelTextFormat, 100, 100, &TextLayout);

        if (SUCCEEDED(hr))
            TextLayout->GetMetrics(&_LabelTextMetrics);
    }

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT SpectrumAnalyzerUIElement::CreateDeviceSpecificResources()
{
    if (_Direct2dFactory == nullptr)
        return E_FAIL;

    HRESULT hr = S_OK;

    // Create the render target.
    if (_RenderTarget == nullptr)
    {
        CRect ClientRect;

        GetClientRect(ClientRect);

        D2D1_SIZE_U ClientSize = D2D1::SizeU((UINT32) ClientRect.Width(), (UINT32) ClientRect.Height());

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Configuration._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);

        _RenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, ClientSize);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, _RenderTargetProperties, &_RenderTarget);
    }

    // Create the brushes.
    if (SUCCEEDED(hr) && (_StrokeBrush == nullptr))
    {
        t_ui_color TextColor = m_callback->query_std_color(ui_color_text);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(TextColor) / 255.0f, GetGValue(TextColor) / 255.0f, GetBValue(TextColor) / 255.0f), &_StrokeBrush);
    }

    if (SUCCEEDED(hr) && (_GradientBrush == nullptr))
    {
        CComPtr<ID2D1GradientStopCollection> Collection;

        // Prism / foo_musical_spectrum
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 5.f, D2D1::ColorF(0xFD0000, 1.f) },
            { 1.f / 5.f, D2D1::ColorF(0xFF8000, 1.f) },
            { 2.f / 5.f, D2D1::ColorF(0xFFFF01, 1.f) },
            { 3.f / 5.f, D2D1::ColorF(0x7EFF77, 1.f) },
            { 4.f / 5.f, D2D1::ColorF(0x0193A2, 1.f) },
            { 5.f / 5.f, D2D1::ColorF(0x002161, 1.f) },
        };
/*
        // Prism 2
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 9.f, D2D1::ColorF(0xAA3355, 1.f) },
            { 1.f / 9.f, D2D1::ColorF(0xCC6666, 1.f) },
            { 2.f / 9.f, D2D1::ColorF(0xEE9944, 1.f) },
            { 3.f / 9.f, D2D1::ColorF(0xEEDD00, 1.f) },
            { 4.f / 9.f, D2D1::ColorF(0x99DD55, 1.f) },
            { 5.f / 9.f, D2D1::ColorF(0x44DD88, 1.f) },
            { 6.f / 9.f, D2D1::ColorF(0x22CCBB, 1.f) },
            { 7.f / 9.f, D2D1::ColorF(0x00BBCC, 1.f) },
            { 8.f / 9.f, D2D1::ColorF(0x0099CC, 1.f) },
            { 9.f / 9.f, D2D1::ColorF(0x3366BB, 1.f) },
        };
*//*
        // Prism 3
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 4.f, D2D1::ColorF(0xFF0000, 1.f) }, // hsl(  0, 100%, 50%)
            { 1.f / 4.f, D2D1::ColorF(0xFFFF00, 1.f) }, // hsl( 60, 100%, 50%)
            { 2.f / 4.f, D2D1::ColorF(0x00FF00, 1.f) }, // hsl(120, 100%, 50%)
            { 3.f / 4.f, D2D1::ColorF(0x00FFFF, 1.f) }, // hsl(180, 100%, 50%)
            { 4.f / 4.f, D2D1::ColorF(0x0000FF, 1.f) }, // hsl(240, 100%, 50%)
        };
*//*
        // foobar2000 Dark Mode
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 1.f, D2D1::ColorF(0x0080FF, 1.f) },
            { 1.f / 1.f, D2D1::ColorF(0xFFFFFF, 1.f) },
        };
*//*
        // foobar2000
        const D2D1_GRADIENT_STOP GradientStops[] =
        {
            { 0.f / 1.f, D2D1::ColorF(0x0066CC, 1.f) }, 
            { 1.f / 1.f, D2D1::ColorF(0x000000, 1.f) },
        };
*/
        hr = _RenderTarget->CreateGradientStopCollection(GradientStops, _countof(GradientStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

        if (SUCCEEDED(hr))
        {
            D2D1_SIZE_F Size = _RenderTarget->GetSize();

            hr = _RenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_GradientBrush);
        }
    }

    if (SUCCEEDED(hr) && (_TextBrush == nullptr))
    {
        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_TextBrush);
    }

    return hr;
}

/// <summary>
/// Release the device specific resources.
/// </summary>
void SpectrumAnalyzerUIElement::ReleaseDeviceSpecificResources()
{
    _TextBrush.Release();
    _GradientBrush.Release();
    _StrokeBrush.Release();
    _RenderTarget.Release();
}

#pragma endregion

#pragma region ui_element_instance

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::g_get_name(pfc::string_base & p_out)
{
    p_out = STR_COMPONENT_NAME;
}

/// <summary>
/// 
/// </summary>
const char * SpectrumAnalyzerUIElement::g_get_description()
{
    return "Spectogram visualization using Direct2D";
}

/// <summary>
/// 
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_guid()
{
    static const GUID guid = GUID_UI_ELEMENT_SPECTOGRAM;

    return guid;
}

/// <summary>
/// 
/// </summary>
GUID SpectrumAnalyzerUIElement::g_get_subclass()
{
    return ui_element_subclass_playback_visualisation;
}

/// <summary>
/// 
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::g_get_default_configuration()
{
    ui_element_config_builder builder;
    Configuration config;

    config.Build(builder);

    return builder.finish(g_get_guid());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::initialize_window(HWND p_parent)
{
    this->Create(p_parent, nullptr, nullptr, 0, WS_EX_STATICEDGE);
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::set_configuration(ui_element_config::ptr data)
{
    ui_element_config_parser Parser(data);
    Configuration config;

    config.Parse(Parser);

    _Configuration = config;

    UpdateRefreshRateLimit();
}

/// <summary>
/// 
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::get_configuration()
{
    ui_element_config_builder Builder;

    _Configuration.Build(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::notify(const GUID & what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
    if (what == ui_element_notify_colors_changed)
    {
        _StrokeBrush.Release();

        Invalidate();
    }
}

#pragma endregion

#pragma region play_callback_impl_base

/// <summary>
/// Playback advanced to new track.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_new_track(metadb_handle_ptr track)
{
    _IsPlaying = true;

    Invalidate();
}

/// <summary>
/// Playback stopped.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_stop(play_control::t_stop_reason reason)
{
    _IsPlaying = false;

    Invalidate();
}

/// <summary>
/// Playback paused/resumed.
/// </summary>
void SpectrumAnalyzerUIElement::on_playback_pause(bool)
{
    _IsPlaying = !_IsPlaying;

    Invalidate();
}

#pragma endregion

static service_factory_single_t<ui_element_impl_visualisation<SpectrumAnalyzerUIElement>> _Factory;
