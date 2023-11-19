
/** $VER: CQTProvider.h (2023.11.19) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include <vector>
#include <complex>

using namespace std;

#include "FrequencyBand.h"
#include "Math.h"

/// <summary>
/// Implements a Constant-Q Transform provider.
/// </summary>
class CQTProvider
{
public:
    CQTProvider() = delete;
    CQTProvider(size_t channelCount, uint32_t sampleRate);

    CQTProvider(const CQTProvider &) = delete;
    CQTProvider & operator=(const CQTProvider &) = delete;
    CQTProvider(CQTProvider &&) = delete;
    CQTProvider & operator=(CQTProvider &&) = delete;

    virtual ~CQTProvider();

    bool GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & freqBand, double bandwidthOffset, double alignment, double downSample) const;

private:
    static double ApplyWindowFunction(double n);
    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
    double _SampleRate;
};

/// <summary>
/// Initializes a new instance of the class.
/// </summary>
/// <param name="channelCount">Number of channels of the input data</param>
inline CQTProvider::CQTProvider(size_t channelCount, uint32_t sampleRate)
{
    if (channelCount == 0)
        throw; // FIXME

    _ChannelCount = channelCount;
    _SampleRate = (double) sampleRate;
}

/// <summary>
/// Destroys this instance.
/// </summary>
inline CQTProvider::~CQTProvider()
{
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands.
/// </summary>
bool inline CQTProvider::GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & freqBand, double bandwidthOffset, double alignment, double downSample) const
{
    for (FrequencyBand & Iter : freqBand)
    {
        double Bandwidth = ::fabs(Iter.Hi - Iter.Lo) + (_SampleRate / (double) sampleCount) * bandwidthOffset;
        double TLen = Min(1.0 / Bandwidth, (double) sampleCount / _SampleRate);
        double DownsampleAmount = Max(1.0, ::trunc((_SampleRate * downSample) / (Iter.Ctr + TLen)));
        double Coeff = 2. * ::cos(2. * M_PI * Iter.Ctr / _SampleRate * DownsampleAmount);

        double f1 = 0.;
        double f2 = 0.;
        double Sine = 0.;
        double Offset = ::trunc(((double) sampleCount - TLen * _SampleRate) * (0.5 + alignment / 2.));

        double LoIdx = Offset;
        double HiIdx = ::trunc(TLen * _SampleRate) + Offset - 1.;
        double Norm = 0.;

        for (double Idx = ::trunc(LoIdx / DownsampleAmount); Idx <= ::trunc(HiIdx / DownsampleAmount); ++Idx)
        {
            double posX = ((Idx * DownsampleAmount - LoIdx) / (HiIdx - LoIdx) * 2. - 1.);
            double w = ApplyWindowFunction(posX);

            Norm += w;

            // Goertzel transform
            Sine = AverageSamples(sampleData, (size_t)(Idx * DownsampleAmount), _ChannelCount) * w + Coeff * f1 - f2;

            f2 = f1;
            f1 = Sine;
        }

        Iter.NewValue = ::sqrt((f1 * f1) + (f2 * f2) - Coeff * f1 * f2) / Norm;
    }

    return true;
}

/// <summary>
/// Applies a window function.
/// </summary>
inline double CQTProvider::ApplyWindowFunction(double n)
{
    return ::pow(::cos(n * M_PI / 2.0), 2.0);
}
/*
function applyWindow(posX, windowType = 'Hann', windowParameter = 1, truncate = true, windowSkew = 0)
{
    let x = windowSkew > 0 ? ((posX / 2 - 0.5) / (1 - (posX / 2 - 0.5) * 10 * (windowSkew ** 2))) / (1 / (1 + 10 * (windowSkew ** 2))) * 2 + 1 : ((posX / 2 + 0.5) / (1 + (posX / 2 + 0.5) * 10 * (windowSkew ** 2))) / (1 / (1 + 10 * (windowSkew ** 2))) * 2 - 1;

    if (truncate && Math.abs(x) > 1)
        return 0;

    switch (windowType.toLowerCase())
    {
        default:
            return 1;

        case 'hanning':
        case 'cosine squared':
        case 'hann':
            return Math.cos(x * Math.PI / 2) ** 2;

        case 'raised cosine':
        case 'hamming':
            return 0.54 + 0.46 * Math.cos(x * Math.PI);

        case 'power of sine':
            return Math.cos(x * Math.PI / 2) ** windowParameter;

        case 'tapered cosine':
        case 'tukey':
            return Math.abs(x) <= 1 - windowParameter ? 1 : (x > 0 ? (-Math.sin((x - 1) * Math.PI / windowParameter / 2)) ** 2 : Math.sin((x + 1) * Math.PI / windowParameter / 2) ** 2);

        case 'blackman':
            return 0.42 + 0.5 * Math.cos(x * Math.PI) + 0.08 * Math.cos(x * Math.PI * 2);

        case 'nuttall':
            return 0.355768 + 0.487396 * Math.cos(x * Math.PI) + 0.144232 * Math.cos(2 * x * Math.PI) + 0.012604 * Math.cos(3 * x * Math.PI);

        case 'flat top':
        case 'flattop':
            return 0.21557895 + 0.41663158 * Math.cos(x * Math.PI) + 0.277263158 * Math.cos(2 * x * Math.PI) + 0.083578947 * Math.cos(3 * x * Math.PI) + 0.006947368 * Math.cos(4 * x * Math.PI);

        case 'kaiser':
            return Math.cosh(Math.sqrt(1 - (x ** 2)) * (windowParameter ** 2)) / Math.cosh(windowParameter ** 2);

        case 'gauss':
        case 'gaussian':
            return Math.exp(-(windowParameter ** 2) * (x ** 2));

        case 'bartlett':
        case 'triangle':
        case 'triangular':
            return 1 - Math.abs(x);

        case 'poisson':
        case 'exponential':
            return Math.exp(-Math.abs(x * (windowParameter ** 2)));

        case 'hyperbolic secant':
        case 'sech':
            return 1 / Math.cosh(x * (windowParameter ** 2));

        case 'quadratic spline':
            return Math.abs(x) <= 0.5 ? -((x * Math.sqrt(2)) ** 2) + 1 : (Math.abs(x * Math.sqrt(2)) - Math.sqrt(2)) ** 2;

        case 'parzen':
            return Math.abs(x) > 0.5 ? -2 * ((-1 + Math.abs(x)) ** 3) : 1 - 24 * (Math.abs(x / 2) ** 2) + 48 * (Math.abs(x / 2) ** 3);

        case 'welch':
            return 1 - (x ** 2);
    }
}
*/
/// <summary>
/// Calculates the average of the specified samples.
/// </summary>
audio_sample inline CQTProvider::AverageSamples(const audio_sample * samples, size_t i, size_t channelCount)
{
    switch (channelCount)
    {
        case 1:
            return samples[i];

        case 2:
            return (samples[i] + samples[i + 1]) / (audio_sample) 2.0;

        case 3:
            return (samples[i] + samples[i + 1] + samples[i + 2]) / (audio_sample) 3.0;

        case 4:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3]) / (audio_sample) 4.0;

        case 5:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4]) / (audio_sample) 5.0;

        case 6:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4] + samples[i + 5]) / (audio_sample) 6.0;

        default:
        {
            audio_sample Average = 0.;

            for (size_t j = 0; j < channelCount; ++j)
                Average += samples[i + j];

            return Average / (audio_sample) channelCount;
        }
    }
}
