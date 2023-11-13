
/** $VER: SpectrumAnalyzerUI.cpp (2023.11.13) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#include <vector>
#include <algorithm>

#include "Configuration.h"

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
/// Returns the input value clamped between min and max.
/// </summary>
template <class T>
inline static T Clamp(T value, T minValue, T maxValue)
{
    return Min(Max(value, minValue), maxValue);
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

    return ::log(a) / ::log(newBase);
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

    const D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

    _RenderTarget->Resize(Size);

    _RenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, Size);
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
        CMenu RefreshRateLimitMenu;

        {
            Menu.CreatePopupMenu();
            Menu.AppendMenu((UINT) MF_STRING, IDM_CONFIGURE, TEXT("Configure"));
            Menu.AppendMenu((UINT) MF_SEPARATOR);
            Menu.AppendMenu((UINT) MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Full-Screen Mode"));
//          Menu.AppendMenu((UINT) MF_STRING | (_Configuration._UseHardwareRendering ? MF_CHECKED : 0), IDM_TOGGLE_HARDWARE_RENDERING, TEXT("Hardware Rendering"));

            {
                RefreshRateLimitMenu.CreatePopupMenu();
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  20) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20,  TEXT("20 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit ==  60) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_60,  TEXT("60 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 100) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_100, TEXT("100 Hz"));
                RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Configuration._RefreshRateLimit == 200) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_200, TEXT("200 Hz"));

                Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, TEXT("Refresh Rate Limit"));
            }

            Menu.SetMenuDefaultItem(IDM_CONFIGURE);
        }

        int CommandId = Menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

        switch (CommandId)
        {
            case IDM_TOGGLE_FULLSCREEN:
                ToggleFullScreen();
                break;

            case IDM_TOGGLE_HARDWARE_RENDERING:
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

            case IDM_CONFIGURE:
                Configure();
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

#pragma endregion

/// <summary>
/// Toggles full screen mode.
/// </summary>
void SpectrumAnalyzerUIElement::ToggleFullScreen() noexcept
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
}

/// <summary>
/// Toggles hardware/software rendering.
/// </summary>
void SpectrumAnalyzerUIElement::ToggleHardwareRendering() noexcept
{
    _Configuration._UseHardwareRendering = !_Configuration._UseHardwareRendering;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Updates the refresh rate.
/// </summary>
void SpectrumAnalyzerUIElement::UpdateRefreshRateLimit() noexcept
{
    _RefreshInterval = pfc::clip_t<DWORD>(1000 / (DWORD) _Configuration._RefreshRateLimit, 5, 1000);
}

/// <summary>
/// Shows the Options dialog.
/// </summary>
void SpectrumAnalyzerUIElement::Configure() noexcept
{
    if (!_ConfigurationDialog.IsWindow())
    {
        if (_ConfigurationDialog.Create(m_hWnd, (LPARAM) &_Configuration) == NULL)
            return;

        _ConfigurationDialog.ShowWindow(SW_SHOW);
    }
    else
        _ConfigurationDialog.BringWindowToTop();
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

/// <summary>
/// Scales the frequency.
/// </summary>
static double fscale(double x, ScalingFunctions function, double factor)
{
    switch (function)
    {
        default:

        case Linear:
            return x;

        case Logarithmic:
            return ::log2(x);

        case ShiftedLogarithmic:
            return ::log2(::pow(10, factor * 4.0) + x);

        case Mel:
            return ::log2(1.0 + x / 700.0);

        case Bark: // "Critical bands"
            return (26.81 * x) / (1960.0 + x) - 0.53;

        case AdjustableBark:
            return (26.81 * x) / (::pow(10, factor * 4.0) + x);

        case ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * x);

        case Cams:
            return ::log2((x / 1000.0 + 0.312) / (x / 1000.0 + 14.675));

        case HyperbolicSine:
            return ::asinh(x / ::pow(10, factor * 4));

        case NthRoot:
            return ::pow(x, (1.0 / (11.0 - factor * 10.0)));

        case NegativeExponential:
            return -::exp2(-x / ::exp2(7 + factor * 8));

        case Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double invFscale(double x, ScalingFunctions function, double factor)
{
    switch (function)
    {
        default:

        case Linear:
            return x;

        case Logarithmic:
            return ::exp2(x);

        case ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, factor * 4.0);

        case Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case AdjustableBark:
            return ::pow(10.0, (factor * 4.0)) / (26.81 / x - 1.0);

        case ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, factor * 4);

        case NthRoot:
            return ::pow(x, ((11.0 - factor * 10.0)));

        case NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + factor * 8.0);

        case Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Scales the amplitude.
/// </summary>
static double ascale(double x)
{
    if (_Configuration._UseDecibels)
        return Map(ToDecibel(x), _Configuration._MinDecibels, _Configuration._MaxDecibels, 0.0, 1.0);
    else
    {
        double Exponent = 1.0 / _Configuration._gamma;

        return Map(::pow(x, Exponent), _Configuration._UseAbsolute ? 0.0 : ::pow(ToMagnitude(_Configuration._MinDecibels), Exponent), ::pow(ToMagnitude(_Configuration._MaxDecibels), Exponent), 0.0, 1.0);
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void generateOctaveBands(uint32_t bandsPerOctave, uint32_t loNote, uint32_t hiNote, int detune, double pitch, double bandwidth, uint32_t sampleRate)
{
    const double Root24 = ::exp2(1. / 24.);
    const double NyquistFrequency = (double) sampleRate / 2.0;

    const double Pitch = (pitch > 0.0) ? ::round((::log2(pitch) - 4.0) * 12.0) * 2.0 : 0.0;
    const double C0 = pitch * ::pow(Root24, -Pitch); // ~16.35 Hz
    const double groupNotes = 24. / bandsPerOctave;

    const double LoNote = ::round(loNote * 2 / groupNotes);
    const double HiNote = ::round(hiNote * 2 / groupNotes);

    FrequencyBands.clear();

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - bandwidth) * groupNotes + detune)),
            C0 * ::pow(Root24,  (i              * groupNotes + detune)),
            C0 * ::pow(Root24, ((i + bandwidth) * groupNotes + detune)),
        };

        FrequencyBands.push_back((fb.ctr < NyquistFrequency) ? fb : FrequencyBand(NyquistFrequency, NyquistFrequency, NyquistFrequency));
    }
}

void generateFreqBands(size_t numBands, uint32_t loFreq, uint32_t hiFreq, ScalingFunctions scalingFunction, double freqSkew, double bandwidth)
{
    FrequencyBands.resize(_Configuration._numBands);

    for (double i = 0.0; i < (double) FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = FrequencyBands[(size_t) i];

        Iter.lo  = invFscale(Map(i - bandwidth, 0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
        Iter.ctr = invFscale(Map(i,             0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
        Iter.hi  = invFscale(Map(i + bandwidth, 0., (double)(numBands - 1), fscale(loFreq, scalingFunction, freqSkew), fscale(hiFreq, scalingFunction, freqSkew)), scalingFunction, freqSkew);
    }
}

double HzToFFTIndex(double x, size_t bufferSize, uint32_t sampleRate)
{
    return x * (double) bufferSize / sampleRate;
}

uint32_t FFTIndexToHz(size_t x, size_t bufferSize, uint32_t sampleRate)
{
    return (uint32_t)((size_t)(x * sampleRate) / bufferSize);
}

double Lanzcos(const std::vector<double> & fftCoeffs, double x, int kernelSize = 4, bool useComplex = false)
{
    double Sum = 0.;

    for (int i = -kernelSize + 1; i <= kernelSize; ++i)
    {
        double pos = ::floor(x) + i;
        double twiddle = x - pos; // -pos + ::round(pos) + i

        double w = ::fabs(twiddle) <= 0 ? 1 : ::sin(twiddle * M_PI) / (twiddle * M_PI) * ::sin(M_PI * twiddle / kernelSize) / (M_PI * twiddle / kernelSize);

        size_t CoefIdx = ((size_t) pos % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

        Sum += fftCoeffs[CoefIdx] * w;
    }

    return Sum;
}

double Lanzcos(Complex * data, size_t length, double x, int kernelSize = 4, bool useComplex = false)
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
void calcSpectrum(const std::vector<double> & fftCoeffs, std::vector<FrequencyBand> & freqBands, int interpSize, SummationMode summationMode, bool useComplex, bool smoothInterp, bool smoothGainTransition, uint32_t sampleRate)
{
    size_t j = 0;

    for (const FrequencyBand & Iter : freqBands)
    {
        const double LoHz = HzToFFTIndex(Min(Iter.hi, Iter.lo), fftCoeffs.size(), sampleRate);
        const double HiHz = HzToFFTIndex(Max(Iter.hi, Iter.lo), fftCoeffs.size(), sampleRate);

        int minIdx1 = (int)                  ::ceil(LoHz);
        int maxIdx1 = (int)                 ::floor(HiHz);

        int minIdx2 = (int) (smoothInterp ? ::round(LoHz) + 1 : minIdx1);
        int maxIdx2 = (int) (smoothInterp ? ::round(HiHz) - 1 : maxIdx1);

        double bandGain = smoothGainTransition && (summationMode == Sum || summationMode == RMSSum) ? ::hypot(1, ::pow(((Iter.hi - Iter.lo) * (double) fftCoeffs.size() / sampleRate), (1 - (int) (summationMode == RMS || summationMode == RMSSum) / 2))) : 1.;

        if (minIdx2 > maxIdx2)
        {
            spectrum[j] = ::fabs(Lanzcos(fftCoeffs, Iter.ctr * (double) fftCoeffs.size() / sampleRate, interpSize, useComplex)) * bandGain;
        }
        else
        {
            double sum = (summationMode == Minimum) ? DBL_MAX : 0.;
            double diff = 0.;

            int overflowCompensation = Max(maxIdx1 - minIdx1 - (int) fftCoeffs.size(), 0);

            bool isAverage = (summationMode == Average || summationMode == RMS) || ((summationMode == Sum || summationMode == RMSSum) && smoothGainTransition);
            bool isRMS = summationMode == RMS || summationMode == RMSSum;

            std::vector<double> medianData;

            for (int i = minIdx1; i <= maxIdx1 - overflowCompensation; ++i)
            {
                size_t CoefIdx = ((size_t) i % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

                double data = fftCoeffs[CoefIdx];

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
        _SpectrumAnalyzer = new SpectrumAnalyzer(ChannelCount, _Configuration._FFTSize, SampleRate);
    }

    // Add the samples to the spectrum analyzer.
    {
        const audio_sample * Samples = chunk.get_data();

        size_t SampleCount  = chunk.get_sample_count();

        _SpectrumAnalyzer->Add(Samples, SampleCount);
    }

    // Initialize the bands.
    switch (_Configuration._FrequencyDistribution)
    {
        case Octaves:
            generateOctaveBands(_Configuration._octaves, _Configuration._minNote, _Configuration._maxNote, _Configuration._detune, _Configuration._Pitch, _Configuration._bandwidth, SampleRate);
            break;

        case AveePlayer:
//          generateAveePlayerFreqs(_Configuration._numBands, _Configuration._minFreq, _Configuration._maxFreq, _Configuration._hzLinearFactor, _Configuration._bandwidth);
            break;

        case Frequencies:
        default:
            generateFreqBands(_Configuration._numBands, _Configuration._minFreq, _Configuration._maxFreq, _Configuration._fscale, _Configuration._hzLinearFactor, _Configuration._bandwidth);
    }

    spectrum.resize(FrequencyBands.size());
    std::fill(spectrum.begin(), spectrum.end(), 0.0);

    currentSpectrum.resize(FrequencyBands.size());
    std::fill(currentSpectrum.begin(), currentSpectrum.end(), 0.0);

    {
        // Get the frequency data.
        std::vector<double> FreqData((size_t) _Configuration._FFTSize, 0.0);

        _SpectrumAnalyzer->GetFrequencyData(&FreqData[0], FreqData.size());

        // Calculate the spectrum 
        calcSpectrum(FreqData, FrequencyBands, _Configuration.interpSize, _Configuration._SummationMode, false, _Configuration.smoothInterp, _Configuration.smoothSlope, SampleRate);

        switch (_Configuration._SmoothingMethod)
        {
            case MethodAverage:
                calcSmoothingTimeConstant(currentSpectrum, spectrum, _Configuration._SmoothingConstant);
                break;

            case MethodPeak:
                calcPeakDecay(currentSpectrum, spectrum, _Configuration._SmoothingConstant);
                break;
        
            default:
                currentSpectrum = spectrum;
        }
    }

    hr = RenderBands();

    return hr;
}

/// <summary>
/// Renders the bands.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderBands()
{
    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    FLOAT Width = (FLOAT) _RenderTargetProperties.pixelSize.width - YAxisWidth;

    FLOAT BandWidth = Max((Width / (FLOAT) currentSpectrum.size()), 1.f);

    FLOAT x1 = YAxisWidth + (Width - ((FLOAT) currentSpectrum.size() * BandWidth)) / 2.f;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = PaddingY;
    const FLOAT y2 = (FLOAT) _RenderTargetProperties.pixelSize.height - _LabelTextMetrics.height;

    RenderYAxis();

    uint32_t Note = _Configuration._minNote;

    for (const auto Iter : currentSpectrum)
    {
        D2D1_RECT_F Rect = { x1, y1, x2 - PaddingX, y2 - PaddingY };

        switch (_Configuration._FrequencyDistribution)
        {
            case FrequencyDistribution::Frequencies:
            {
                break;
            }

            case FrequencyDistribution::Octaves:
            {
                if (Note % 12 == 0)
                {
                    RenderXAxis(x1, y2, x2, y2 + _LabelTextMetrics.height, Note / 12u);

                    // Draw the vertical grid line.
                    {
                        _TextBrush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                        _RenderTarget->DrawLine(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x1, y2), _TextBrush, 1.f, nullptr);
                    }
                }

                Note++;
                break;
            }

            case FrequencyDistribution::AveePlayer:
                break;
        }

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
            Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * ascale(Iter))), y1, y2);

            _RenderTarget->FillRectangle(Rect, _GradientBrush);
        }

        x1  = x2;
        x2 += BandWidth;
    }

    return S_OK;
}

/// <summary>
/// Renders the X axis.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderXAxis(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, uint32_t octave)
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

    switch (_Configuration._FrequencyDistribution)
    {
        case FrequencyDistribution::Frequencies:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"%uHz - %uHz, %u bands, FFT %u, FPS: %.2f\n",
                    _Configuration._minFreq, _Configuration._maxFreq, (uint32_t) _Configuration._numBands, _Configuration._FFTSize, FPS);
            break;

        case FrequencyDistribution::Octaves:
            hr = ::StringCchPrintfW(Text, _countof(Text), L"Note %u - %u, %u bands per octave, Tuning %.2fHz, FFT %u, FPS: %.2f\n",
                    _Configuration._minNote, _Configuration._maxNote, (uint32_t) _Configuration._octaves, _Configuration._Pitch, _Configuration._FFTSize, FPS);
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
    Configuration Config;

    ui_element_config_builder Builder;

    Config.Write(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::initialize_window(HWND p_parent)
{
    this->Create(p_parent, nullptr, nullptr, 0, WS_EX_STATICEDGE);
}

/// <summary>
/// Alters element's current configuration. Specified ui_element_config's GUID must be the same as this element's GUID.
/// </summary>
void SpectrumAnalyzerUIElement::set_configuration(ui_element_config::ptr data)
{
    ui_element_config_parser Parser(data);

    _Configuration.Read(Parser);

    UpdateRefreshRateLimit();
}

/// <summary>
/// Retrieves element's current configuration. Returned object's GUID must be set to your element's GUID so your element can be re-instantiated with stored settings.
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::get_configuration()
{
    ui_element_config_builder Builder;

    _Configuration.Write(Builder);

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
    _Configuration.Read();

    // Make sure the spectrum analyzer is recreated. The audio chunks may have another configuration than the ones from the previous track.
    if (_SpectrumAnalyzer != nullptr)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

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
