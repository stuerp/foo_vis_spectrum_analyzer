
/** $VER: Analysis.cpp (2024.02.15) P. Stuer **/

#include "Analysis.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void Analysis::Initialize(const State & state) noexcept
{
    if (state._Transform == Transform::FFT)
    {
        switch (state._FrequencyDistribution)
        {
            default:

            case FrequencyDistribution::Linear:
                GenerateLinearFrequencyBands(state);
                break;

            case FrequencyDistribution::Octaves:
                GenerateOctaveFrequencyBands(state);
                break;

            case FrequencyDistribution::AveePlayer:
                GenerateAveePlayerFrequencyBands(state);
                break;
        }
    }
    else
        GenerateOctaveFrequencyBands(state);
}

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void Analysis::GenerateLinearFrequencyBands(const State & state)
{
    const double MinFreq = ScaleF(state._LoFrequency, state._ScalingFunction, state._SkewFactor);
    const double MaxFreq = ScaleF(state._HiFrequency, state._ScalingFunction, state._SkewFactor);

    const double Width = (((state._Transform == Transform::FFT) && (state._MappingMethod == Mapping::TriangularFilterBank)) || (state._Transform == Transform::CQT)) ? state._Bandwidth : 0.5;

    _FrequencyBands.resize(state._BandCount);

    double i = 0.;

    for (FrequencyBand & Iter: _FrequencyBands)
    {
        Iter.Lo  = DeScaleF(Map(i - Width, 0., (double)(state._BandCount - 1), MinFreq, MaxFreq), state._ScalingFunction, state._SkewFactor);
        Iter.Ctr = DeScaleF(Map(i,         0., (double)(state._BandCount - 1), MinFreq, MaxFreq), state._ScalingFunction, state._SkewFactor);
        Iter.Hi  = DeScaleF(Map(i + Width, 0., (double)(state._BandCount - 1), MinFreq, MaxFreq), state._ScalingFunction, state._SkewFactor);

        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);

        Iter.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void Analysis::GenerateOctaveFrequencyBands(const State & state)
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (state._Pitch > 0.) ? ::round((::log2(state._Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = state._Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NoteGroup = 24. / state._BandsPerOctave;

    const double LoNote = ::round(state._MinNote * 2. / NoteGroup);
    const double HiNote = ::round(state._MaxNote * 2. / NoteGroup);

    const double Width = (((state._Transform == Transform::FFT) && (state._MappingMethod == Mapping::TriangularFilterBank)) || (state._Transform == Transform::CQT)) ? state._Bandwidth : 0.5;

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, (i - Width) * NoteGroup + state._Transpose),
            C0 * ::pow(Root24,  i          * NoteGroup + state._Transpose),
            C0 * ::pow(Root24, (i + Width) * NoteGroup + state._Transpose),
        };

        // Pre-calculate the tooltip text and the bar background color.
        {
            int Note = (int) (i * (NoteGroup / 2.));

            int n = Note % 12;

            ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], Note / 12, fb.Ctr);

            fb.HasDarkBackground = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        }

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands of AveePlayer.
/// </summary>
void Analysis::GenerateAveePlayerFrequencyBands(const State & state)
{
    const double Width = (((state._Transform == Transform::FFT) && (state._MappingMethod == Mapping::TriangularFilterBank)) || (state._Transform == Transform::CQT)) ? state._Bandwidth : 0.5;

    _FrequencyBands.resize(state._BandCount);

    double i = 0.;

    for (FrequencyBand & Iter : _FrequencyBands)
    {
        Iter.Lo  = LogSpace(state._LoFrequency, state._HiFrequency, i - Width, state._BandCount - 1, state._SkewFactor);
        Iter.Ctr = LogSpace(state._LoFrequency, state._HiFrequency, i,         state._BandCount - 1, state._SkewFactor);
        Iter.Hi  = LogSpace(state._LoFrequency, state._HiFrequency, i + Width, state._BandCount - 1, state._SkewFactor);

        Iter.HasDarkBackground = true;
        ++i;
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double Analysis::ScaleF(double x, ScalingFunction function, double skewFactor)
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
double Analysis::DeScaleF(double x, ScalingFunction function, double skewFactor)
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
