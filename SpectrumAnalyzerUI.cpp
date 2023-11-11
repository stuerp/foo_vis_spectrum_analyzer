
/** $VER: SpectrumAnalyzerUI.cpp (2023.11.11) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "SpectrumAnalyzerUI.h"

#include <vector>

#pragma hdrstop

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

#define ToDecibel(x)    (20.0 * ::log10(x))   // Converts the magnitude to decibel (dB)

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
    if (_FrequencyBands)
    {
        delete[] _FrequencyBands,
        _FrequencyBands = nullptr;
    }

    if (_SpectrumAnalyzer)
    {
        delete _SpectrumAnalyzer;
        _SpectrumAnalyzer = nullptr;
    }

    _VisualisationStream.release();

    _StrokeBrush.Release();
    _RenderTarget.Release();
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
    if (_RenderTarget)
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
        Menu.AppendMenu((UINT) MF_STRING | (_Config._UseHardwareRendering ? MF_CHECKED : 0), IDM_HW_RENDERING_ENABLED, TEXT("Hardware Rendering"));

        Menu.SetMenuDefaultItem(IDM_TOGGLE_FULLSCREEN);

        CMenu RefreshRateLimitMenu;

        RefreshRateLimitMenu.CreatePopupMenu();
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Config._RefreshRateLimit ==  20) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20,  TEXT("20 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Config._RefreshRateLimit ==  60) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_60,  TEXT("60 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Config._RefreshRateLimit == 100) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_100, TEXT("100 Hz"));
        RefreshRateLimitMenu.AppendMenu((UINT) MF_STRING | ((_Config._RefreshRateLimit == 200) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_200, TEXT("200 Hz"));

        Menu.AppendMenu((UINT) MF_STRING, RefreshRateLimitMenu, TEXT("Refresh Rate Limit"));

        // Visualization specific menu items

        /** None **/

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
                _Config._RefreshRateLimit = 20;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_60:
                _Config._RefreshRateLimit = 60;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_100:
                _Config._RefreshRateLimit = 100;
                UpdateRefreshRateLimit();
                break;

            case IDM_REFRESH_RATE_LIMIT_200:
                _Config._RefreshRateLimit = 200;
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
    _Config._UseHardwareRendering = !_Config._UseHardwareRendering;
    ReleaseDeviceSpecificResources();
}

/// <summary>
/// 
/// </summary>
void SpectrumAnalyzerUIElement::UpdateRefreshRateLimit()
{
    _RefreshInterval = pfc::clip_t<DWORD>(1000 / (DWORD)_Config._RefreshRateLimit, 5, 1000);
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

        _RenderTarget->SetAntialiasMode(_Config._UseLowQuality ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
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
                    double WindowDuration = _Config.GetWindowDurationInMs();

                    audio_chunk_impl Chunk;

                    if (_VisualisationStream->get_chunk_absolute(Chunk, PlaybackTime - WindowDuration / 2, WindowDuration * (_Config._UseZeroTrigger ? 2 : 1)))
                        RenderChunk(Chunk);
                }
            }

            // Draw the text.
            hr = RenderText();
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

double map(double x, double min, double max, double targetMin, double targetMax)
{
    return (x - min) / (max - min) * (targetMax - targetMin) + targetMin;
}

double clamp(double x, double min, double max)
{
    return Min(Max(x, min), max);
}

enum ScalingFunctions
{
    Logarithmic = 1,
};

double fscale(double x, ScalingFunctions function, double freqSkew)
{
    switch (function)
    {
        case Logarithmic:
            return ::log2(x);

        default:
            return x;
    }
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

struct Band
{
    double lo;
    double ctr;
    double hi;
} FrequencyBands[320] = { };

double spectrum[320] = { };

double currentSpectrum[320] = { };

void generateFreqBands(size_t N = _countof(FrequencyBands), uint32_t low = 20, uint32_t high = 20000, ScalingFunctions freqScale = Logarithmic, double freqSkew = 0.0, double bandwidth = 0.5)
{
    for (size_t i = 0; i < N; ++i)
    {
        FrequencyBands[i].lo  = invFscale(map((double)((double) i - bandwidth), 0., (double)(N - 1), fscale(low, freqScale, freqSkew), fscale(high, freqScale, freqSkew)), freqScale, freqSkew);
        FrequencyBands[i].ctr = invFscale(map((double)(i),                      0., (double)(N - 1), fscale(low, freqScale, freqSkew), fscale(high, freqScale, freqSkew)), freqScale, freqSkew);
        FrequencyBands[i].hi  = invFscale(map((double)((double) i + bandwidth), 0., (double)(N - 1), fscale(low, freqScale, freqSkew), fscale(high, freqScale, freqSkew)), freqScale, freqSkew);
    }
}

double ascale(double x)
{
    static bool UseDecibels = false;

    const double MinDecibels = -90.;
    const double MaxDecibels =   0.;

    const double gamma = 1.;
    const bool UseAbsolute = true;

    if (UseDecibels)
        return map(20 * ::log10(x), MinDecibels, MaxDecibels, 0, 1);
    else
        return map(::pow(x, (1 / gamma)), !UseAbsolute * ::pow(::pow(10, (MinDecibels / 20)), (1 / gamma)), ::pow(::pow(10, (MaxDecibels / 20)), (1 / gamma)), 0., 1.);
}

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

TimeSmootingMethod smoothingMode = TimeSmootingMethod::MethodAverage;

double smoothingTimeConstant = 0.;

// Hz and FFT bin conversion
double hertzToFFTBin(double x, size_t bufferSize = 4096, uint32_t sampleRate = 44100)
{
    return x * (double) bufferSize / sampleRate;
}

uint32_t fftBinToHertz(size_t x, size_t bufferSize = 4096, uint32_t sampleRate = 44100)
{
    return (uint32_t)((size_t)(x * sampleRate) / bufferSize);
}

struct Complex
{
    double re;
    double im;
};

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

// Calculates bandpower from FFT (foobar2000 flavored, can be enhanced by using complex FFT coefficients instead of magnitude-only FFT data)
void calcSpectrum(const double * fftCoeffs, size_t length, Band * freqBands, int interpSize, SummationMode summationMode, bool useComplex, bool smoothInterp, bool smoothGainTransition, size_t bufferSize, uint32_t sampleRate)
{
    for (size_t j = 0; j < 320; ++j)
    {
        Band& x = freqBands[j];

        int minIdx = (int)  ::ceil(hertzToFFTBin(Min(x.hi, x.lo), bufferSize, sampleRate));
        int maxIdx = (int) ::floor(hertzToFFTBin(Max(x.hi, x.lo), bufferSize, sampleRate));

        int minIdx2 = (int) (smoothInterp ? ::round(hertzToFFTBin(Min(x.hi, x.lo), bufferSize, sampleRate)) + 1 : minIdx);
        int maxIdx2 = (int) (smoothInterp ? ::round(hertzToFFTBin(Max(x.hi, x.lo), bufferSize, sampleRate)) - 1 : maxIdx);

        double bandGain = smoothGainTransition && (summationMode == Sum || summationMode == RMSSum) ? ::hypot(1, ::pow(((x.hi - x.lo) * (double) bufferSize / sampleRate), (1 - (int) (summationMode == RMS || summationMode == RMSSum) / 2))) : 1.;

        if (minIdx2 > maxIdx2)
        {
            spectrum[j] = ::fabs(lanzcos(fftCoeffs, length, x.ctr * (double) bufferSize / sampleRate, interpSize, useComplex)) * bandGain;
        }
        else
        {
            double sum = (summationMode == Minimum) ? DBL_MAX : 0.;
            double diff = 0.;

            int overflowCompensation = Max(maxIdx - minIdx - (int) bufferSize, 0);

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
//FIXME         sum = median(medianData);
                sum = 0.;
            else
                sum /= isAverage ? diff : 1;

            spectrum[j] = (isRMS ? ::sqrt(sum) : sum) * bandGain;
        }
    }
}

void calcSmoothingTimeConstant(double * targetArr, double * sourceArr, double factor = 0.5)
{
    for (size_t i = 0; i < 320; ++i)
        targetArr[i] = (!::isnan(targetArr[i]) ? targetArr[i] * factor : 0)  + (!::isnan(sourceArr[i]) ? sourceArr[i] * (1 - factor) : 0);
}

void calcPeakDecay(double * targetArr, double * sourceArr, double factor = 0.5)
{
    for (size_t i = 0; i < 320; ++i)
        targetArr[i] = Max(!::isnan(targetArr[i]) ? targetArr[i] * factor : 0, !::isnan(sourceArr[i]) ? sourceArr[i] : 0);
}
/*
double median(std::vector<double>& data)
{
    if (data.size())
        return NAN;

    if (data.size() <= 1)
        return data[0];

    const
        sortedData = data.slice().sort((x, y) => x - y),
        half = Math.trunc(data.length / 2);

    if (data.length % 2)
        return sortedData[half];

    return (sortedData[half - 1] + sortedData[half]) / 2;
}
*/
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
        _SpectrumAnalyzer = new SpectrumAnalyzer(ChannelCount, _FFTSize, SampleRate);

        _FrequencyBands = new FrequencyBand[_BandCount];

        ::memset(_FrequencyBands, 0, sizeof(_FrequencyBands[0]) * _BandCount);
    }

    // Add the samples to the spectrum provider.
    {
        const audio_sample * Samples = chunk.get_data();

        t_size SampleCount  = chunk.get_sample_count();

        _SpectrumAnalyzer->Add(Samples, SampleCount);
    }

    // Initialize the bands.
    generateFreqBands();

    {
        // Get the frequency data.
        double * FreqData = new double[(size_t) _FFTSize];

        _SpectrumAnalyzer->GetFrequencyData(FreqData, (size_t) _FFTSize);

        ::memset(spectrum, 0, sizeof(spectrum));

        calcSpectrum(FreqData, (size_t) _FFTSize / 2, FrequencyBands, 32, SummationMode::Maximum, false, true, true, (size_t) _FFTSize / 2, SampleRate);

        switch (smoothingMode)
        {
            case MethodAverage:
                calcSmoothingTimeConstant(currentSpectrum, spectrum, smoothingTimeConstant / 100);
                break;

            case MethodPeak:
                calcPeakDecay(currentSpectrum, spectrum, smoothingTimeConstant / 100);
                break;
        
            default:
                ::memcpy(currentSpectrum, spectrum, sizeof(currentSpectrum));
        }

        if (FreqData)
            delete[] FreqData;
    }

    hr = RenderBands();

    return hr;
}

/// <summary>
/// Renders the bands.
/// </summary>
HRESULT SpectrumAnalyzerUIElement::RenderBands()
{
    const FLOAT PaddingY = 1.f;
    const FLOAT Radix = 1.f;

    const FLOAT RightMargin = 1.f;

    FLOAT Width  = (FLOAT) _RenderTargetProperties.pixelSize.width;
//  FLOAT Height = (FLOAT) _RenderTargetProperties.pixelSize.height;

    FLOAT BandWidth = Width / (FLOAT) _BandCount;

    FLOAT x1 = (Width - ((FLOAT) _BandCount * BandWidth)) / 2.f;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = PaddingY;
    const FLOAT y2 = (FLOAT) _RenderTargetProperties.pixelSize.height - PaddingY;

    for (size_t BandIndex = 0; BandIndex < _BandCount; ++BandIndex)
    {
        // Draw the background.
        D2D1_RECT_F Rect = { x1, y1, x2 - RightMargin, y2 };

        _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 0.3f)); // #1E90FF
        _RenderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Radix, Radix), _TextBrush);

        // Draw the foreground.
        if (currentSpectrum[BandIndex] > 0.0)
        {
            Rect.top = Max((FLOAT)(y2 - ((y2 - y1) * ascale(currentSpectrum[BandIndex] / 64))), y1);

            _TextBrush->SetColor(D2D1::ColorF(30.f / 255.f, 144.f / 255.f, 255.f / 255.f, 1.0f)); // #1E90FF
            _RenderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Radix, Radix), _TextBrush);
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

    WCHAR Text[512] = { };

    HRESULT hr = ::StringCchPrintfW(Text, _countof(Text),
        L"%uHz - %uHz, %u bands, FFT %u, FPS: %.2f\n",
        _MinFrequency, _MaxFrequency, (uint32_t) _BandCount, _FFTSize, FPS
    );

    if (SUCCEEDED(hr))
    {
        _RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        static const D2D1_RECT_F TextRect = { 4.f, 4.f, 512.f, 50.f };
        static const float TextRectInset = 10.f;

        _TextBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

        _RenderTarget->FillRoundedRectangle(D2D1::RoundedRect(TextRect, TextRectInset, TextRectInset), _TextBrush);

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
        static const PCWSTR FontName = L"Calibri";
        static const float FontSize = 20.0f;

        hr = _DirectWriteFactory->CreateTextFormat
        (
            FontName, NULL,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            static_cast<FLOAT>(FontSize),
            L"",
            &_TextFormat
        );
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

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_Config._UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);

        _RenderTargetProperties = D2D1::HwndRenderTargetProperties(m_hWnd, ClientSize);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, _RenderTargetProperties, &_RenderTarget);
    }

    // Create the brushes.
    if (SUCCEEDED(hr) && (_StrokeBrush == nullptr))
    {
        t_ui_color TextColor = m_callback->query_std_color(ui_color_text);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(TextColor) / 255.0f, GetGValue(TextColor) / 255.0f, GetBValue(TextColor) / 255.0f), &_StrokeBrush);
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
    Config config;

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
    Config config;

    config.Parse(Parser);

    _Config = config;

//  UpdateChannelMode();
    UpdateRefreshRateLimit();
}

/// <summary>
/// 
/// </summary>
ui_element_config::ptr SpectrumAnalyzerUIElement::get_configuration()
{
    ui_element_config_builder Builder;

    _Config.Build(Builder);

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
