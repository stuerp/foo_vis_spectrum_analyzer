#include <pch.h>

#include "DSP.h"

#include <cassert>
#include <cmath>
#include <numbers>

/// <summary>
/// Configure this instance.
/// </summary>
void biquad_t::Configure(PassType type, double frequency, double sampleRate, double q) noexcept
{
    constexpr double TwoPi = 2. * std::numbers::pi_v<double>;

    // Prevent invalid or unstable cutoff frequencies. The cutoff frequency is between 10Hz and the Nyquist.
    frequency = std::clamp(frequency, 10., sampleRate * 0.49);

    // Normalized angular frequency in radians/sample.
    const double w = TwoPi * frequency / sampleRate;

    const double Cos = std::cos(w);
    const double Sin = std::sin(w);

    // Controls resonance/bandwidth around the cutoff frequency.
    const double alpha = Sin / (2. * q);

    // Common denominator term used by all normalized coefficients.
    const double a0 = 1. + alpha;

    double b0, b1, b2;

    if (type == PassType::LowPass)
    {
        // RBJ cookbook low-pass coefficients before normalization.
        b1 = (1. - Cos);
        b0 = b1 * .5;
        b2 = b0;
    }
    else
    {
        // RBJ cookbook high-pass coefficients before normalization.
        b0 =  (1. + Cos) * .5;
        b1 = -(1. + Cos);
        b2 = b0;
    }

    // Normalize feed-forward coefficients by a0 so the filter can be evaluated without dividing each sample.
    _B0 = b0 / a0;
    _B1 = b1 / a0;
    _B2 = b2 / a0;

    /// Normalize feedback coefficients. Direct Form II Transposed implementation expects these signs.
    _A1 = (-2 * Cos) / a0;
    _A2 = (1 - alpha) / a0;
}

/// <summary>
/// Processes a sample.
/// </summary>
double biquad_t::Process(double x)
{
    // Direct Form II Transposed structure. Compute current output using the input sample and first state.
    const double y = _B0 * x + _Z1;

    // Update filter states for the next sample. z1 and z2 store the delayed internal accumulators, minimizing memory usage compared to Direct Form I.
    _Z1 = _B1 * x - _A1 * y + _Z2;
    _Z2 = _B2 * x - _A2 * y;

    return y;
}

/// <summary>
/// Configure this instance.
/// </summary>
void lr4_crossover_t::Configure(double hz, double sampleRate) noexcept
{
    _LP1.Configure(biquad_t::PassType::LowPass, hz, sampleRate);
    _LP2.Configure(biquad_t::PassType::LowPass, hz, sampleRate);

    _HP1.Configure(biquad_t::PassType::HighPass, hz, sampleRate);
    _HP2.Configure(biquad_t::PassType::HighPass, hz, sampleRate);
}

/// <summary>
/// Splits the input signal into low and high.
/// </summary>
void lr4_crossover_t::Process(double x, double & low, double & high)
{
    low  = _LP2.Process(_LP1.Process(x)); // Two cascaded 2nd-order Butterworth low-pass filters implemented as a biquad.
    high = _HP2.Process(_HP1.Process(x)); // Two cascaded 2nd-order Butterworth high-pass filters implemented as a biquad.
}

/// <summary>
/// Resets this instance.
/// </summary>
void lr4_crossover_t::Reset()
{
    _LP1.Reset();
    _LP2.Reset();

    _HP1.Reset();
    _HP2.Reset();
}

/// <summary>
/// Configures this instance.
/// </summary>
void audio_crossover_t::Configure(double loFreq, double hiFreq, double sampleRate) noexcept
{
    constexpr double TwoPi = 2. * std::numbers::pi_v<double>;

    assert(loFreq < hiFreq);

    _LoFreq = loFreq;
    _HiFreq = hiFreq;
    _SampleRate = sampleRate;

    _LowMid.Configure(loFreq, sampleRate);
    _MidHigh.Configure(hiFreq, sampleRate);

    _LoAlpha = 1.0 - std::exp(-TwoPi * _LoFreq / _SampleRate);
    _HiAlpha = 1.0 - std::exp(-TwoPi * _HiFreq / _SampleRate);

    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void audio_crossover_t::Reset()
{
    _LoLP = _HiLP = 0.;

    _LowMid.Reset();
    _MidHigh.Reset();
}

/// <summary>
/// Processes a sample.
/// </summary>
void audio_crossover_t::Process(double x, double & low, double & mid, double & high)
{
    if (_CrossoverMode == CrossoverMode::OnePole)
    {
        // 6 dB/octave slopes, large overlap due to the shallow slopes, substantial phase shift, broad mid band.
        _LoLP += _LoAlpha * (x - _LoLP);
        _HiLP += _HiAlpha * (x - _HiLP);

        low  = _LoLP;
        high = x - _HiLP;
        mid  = _HiLP - _LoLP;

        return;
    }

    // 24 dB/octave slopes, much better isolation, phase-aligned summation.
    double AboveLow = 0.;

    _LowMid.Process(x, low, AboveLow);
    _MidHigh.Process(AboveLow, mid, high);
}

/// <summary>
/// Configures the audio processor.
/// </summary>
void audio_processor_t::Configure(uint32_t sampleRate)
{
    if ((sampleRate == 0) || (sampleRate == _SampleRate))
        return;

    _SampleRate = sampleRate;

    _CrossoverL.Configure(220., 2500., (double) sampleRate);
    _CrossoverR.Configure(220., 2500., (double) sampleRate);

    _CrossoverL.SetCrossoverMode(_CrossoverMode);
    _CrossoverR.SetCrossoverMode(_CrossoverMode);
}

/// <summary>
/// 
/// </summary>
void audio_processor_t::SetCrossoverMode(CrossoverMode mode)
{
    _CrossoverMode = mode;

    _CrossoverL.SetCrossoverMode(mode);
    _CrossoverR.SetCrossoverMode(mode);
}

/// <summary>
/// Adds a point vertex.
/// </summary>
void audio_processor_t::AddPoint(std::vector<vertex_t> & vertices, double l, double r, const D2D1_COLOR_F color, double energy) noexcept
{
    // Classic stereometer rotation: X is side, Y is mid. Clockwise 45-degree rotation.
    constexpr double InverseSqrt2 = 1. / std::numbers::sqrt2_v<double>;

    const auto x = (float) std::clamp((l - r) * InverseSqrt2, -1., 1.); // Center X
    const auto y = (float) std::clamp((l + r) * InverseSqrt2, -1., 1.); // Center Y

    // Triangle list to represent the square point.
    struct point_t
    {
        float x;
        float y;
    };

    static constexpr point_t QuadCorners[] =
    {
        // Triangle 1
        { -1.f, -1.f },
        { -1.f,  1.f },
        {  1.f,  1.f },

        // Triangle 2
        { -1.f, -1.f },
        {  1.f,  1.f },
        {  1.f, -1.f }
    };

    for (const auto & Corner : QuadCorners)
        vertices.push_back({ { x, y }, { Corner.x, Corner.y }, { color.r, color.g, color.b }, (float) energy });
}

/// <summary>
/// Processes the audio frames.
/// </summary>
std::vector<vertex_t> audio_processor_t::Process(const audio_chunk_impl & chunk, double energy, uint32_t selectedChannels, uint32_t pairedChannels) noexcept
{
    Configure(chunk.get_sample_rate());

    const size_t FrameCount          = chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount      = chunk.get_channel_count();
    const uint32_t AvailableChannels = chunk.get_channel_config();  // Mask containing the channels in the audio chunk.

    const uint32_t ChannelMask = AvailableChannels & selectedChannels & pairedChannels;

    if ((FrameCount == 0) || (ChannelCount < 2) || (ChannelMask == 0))
        return { };

    size_t Channel1 = (size_t)       std::countr_zero(ChannelMask);     // Index of the channel 1 sample in the audio chunk.
    size_t Channel2 = (size_t) (31 - std::countl_zero(ChannelMask));    // Index of the channel 2 sample in the audio chunk.

    std::vector<vertex_t> Vertices;

    Vertices.reserve(FrameCount * (_ColorMode == ColorMode::MultiBand ? 6 * 3 : 6));

    double ll = 0., rr = 0., lr = 0.;

    const audio_sample * Frames = chunk.get_data();

    for (size_t i = 0; i < FrameCount; i += ChannelCount)
    {
        auto Frame = &Frames[i];

        ll += Frame[Channel1] * Frame[Channel1];
        rr += Frame[Channel2] * Frame[Channel2];
        lr += Frame[Channel1] * Frame[Channel2];

        double LoL = 0., MiL = 0., HiL = 0., LoR = 0., MiR = 0., HiR = 0.;

        _CrossoverL.Process(Frame[Channel1], LoL, MiL, HiL);
        _CrossoverR.Process(Frame[Channel2], LoR, MiR, HiR);

        if (_ColorMode == ColorMode::Static)
        {
            const auto Color = D2D1::ColorF(.10f, 1.f, .45f);

            AddPoint(Vertices, Frame[Channel1], Frame[Channel2], Color, energy);
        }
        else
        {
            const double EnergyL = (LoL * LoL) + (LoR * LoR);
            const double EnergyM = (MiL * MiL) + (MiR * MiR);
            const double EnergyH = (HiL * HiL) + (HiR * HiR);

            if (_ColorMode == ColorMode::RGB)
            {
                const auto Total = (float) (EnergyL + EnergyM + EnergyH + 1e-12);

                const auto Color = D2D1::ColorF((float) std::sqrt(EnergyL / Total), (float) std::sqrt(EnergyM / Total), (float) std::sqrt(EnergyH / Total));

                AddPoint(Vertices, Frame[Channel1], Frame[Channel2], Color, energy);
            }
            else
            {
            //  static constexpr double EnergyThreshold = 1e-9; // -90 dB
            //  static constexpr double EnergyThreshold = 1e-8; // -80 dB
                static constexpr double EnergyThreshold = 1e-6; // -60 dB

                if (EnergyL > EnergyThreshold)
                {
                    const auto Color = D2D1::ColorF(1.f, .12f, .04f);

                    AddPoint(Vertices, LoL, LoR, Color, energy * 1.30); // 1.30 gain
                }

                if (EnergyM > EnergyThreshold)
                {
                    const auto Color = D2D1::ColorF(.08f, 1.f, .25f);

                    AddPoint(Vertices, MiL, MiR, Color, energy * 1.10); // Visual compensation weakest, 1.10 gain
                }

                if (EnergyH > EnergyThreshold)
                {
                    const auto Color = D2D1::ColorF(.08f, .35f, 1.f);

                    AddPoint(Vertices, HiL, HiR, Color, energy * 1.45); // Visual compensation brightest, 1.45 gain
                }
            }
        }
    }

    {
        // Calculate the cross-correlation.
        const double d = std::sqrt(ll * rr);

        const double InstantCorrelation = (d > 1e-20) ? std::clamp(lr / d, -1., 1.) : 0.; // Normalized cross-correlation coefficient

        _Correlation += (InstantCorrelation - _Correlation) * .16; // Avoid jitter by smoothing the value.
    }

    return Vertices;
}
