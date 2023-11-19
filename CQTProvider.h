
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
    CQTProvider(size_t channelCount);

    CQTProvider(const CQTProvider &) = delete;
    CQTProvider & operator=(const CQTProvider &) = delete;
    CQTProvider(CQTProvider &&) = delete;
    CQTProvider & operator=(CQTProvider &&) = delete;

    virtual ~CQTProvider();

    bool GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & freqBand, uint32_t sampleRate, double bandwidthOffset, double alignment, double downSample) const;

private:
    static double ApplyWindowFunction(double n);
    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
};

/// <summary>
/// Initializes a new instance of the class.
/// </summary>
/// <param name="channelCount">Number of channels of the input data</param>
/// <param name="fftSize">The number of bands to use</param>
inline CQTProvider::CQTProvider(size_t channelCount)
{
    if (channelCount == 0)
        throw; // FIXME

    _ChannelCount = channelCount;
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
bool inline CQTProvider::GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & freqBand, uint32_t sampleRate, double bandwidthOffset, double alignment, double downSample) const
{
    for (FrequencyBand & Iter : freqBand)
    {
        double Bandwidth = ::fabs(Iter.Hi - Iter.Lo) + ((double) sampleRate / (double) sampleCount) * bandwidthOffset;
        double tlen = Min(1.0 / Bandwidth, (double) sampleCount / sampleRate);
        double downsampleAmount = Max(1.0, ::trunc((sampleRate * downSample) / (Iter.Ctr + tlen)));
        double coeff = 2. * ::cos(2 * M_PI * Iter.Ctr / sampleRate * downsampleAmount);

        double f1 = 0.;
        double f2 = 0.;
        double sine = 0.;
        double offset = ::trunc(((double) sampleCount - tlen * sampleRate) * (0.5 + alignment / 2));

        double LoIdx = offset;
        double HiIdx = ::trunc(tlen * sampleRate) + offset - 1;
        double Norm = 0.;

        for (double i = ::trunc(LoIdx / downsampleAmount); i <= ::trunc(HiIdx / downsampleAmount); ++i)
        {
            double posX = ((i * downsampleAmount - LoIdx) / (HiIdx - LoIdx) * 2.0 - 1.0);
            double w = ApplyWindowFunction(posX);

            Norm += w;

            // Goertzel transform
//          sine = sampleData[(size_t)(i * downsampleAmount) * _ChannelCount] * w + coeff * f1 - f2;
            sine = AverageSamples(sampleData, (size_t)(i * downsampleAmount), _ChannelCount) * w + coeff * f1 - f2;

            f2 = f1;
            f1 = sine;
        }

        Iter.NewValue = ::sqrt((f1 * f1) + (f2 * f2) - coeff * f1 * f2) / Norm;
    }

    return true;
}

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
            return Math.abs(x) <= 1 - windowParameter ? 1 :
                (x > 0 ?
                    (-Math.sin((x - 1) * Math.PI / windowParameter / 2)) ** 2 :
                    Math.sin((x + 1) * Math.PI / windowParameter / 2) ** 2);
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
