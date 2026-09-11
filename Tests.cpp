
/** $VER: Tests.cpp (2026.09.09) P. Stuer - Various unit tests **/

#include "pch.h"

#ifdef _DEBUG

#include "Analyzers/Analysis.h"
#include "Analyzers/FrequencyScaler.h"
#include "Analyzers/WindowFunctions.h"
#include "Log.h"

static void TestWeighting()
{
    constexpr double WeightTolerance  = 1.e-9;
    constexpr double DecibelTolerance = 1.e-6;

    analysis_t Analysis;

    struct weighting_test_t
    {
        double Frequency;
        double ExpectedWeight;
        double ExpectedDecibels;
    };

    constexpr weighting_test_t AWeightingTests[] =
    {
        {       20., 0.003021740854364, -50.394855673 },
        {      100., 0.110342379727598, -19.145153079 },
        {    1'000., 0.999993407716264,  -0.000057260 },
        {    4'000., 1.117300404192550,   0.963399118 },
        {    8'000., 0.876286444332179,  -1.147078131 },
        {   10'000., 0.750605235629460,  -2.491768212 },
        {   12'500., 0.612759671202555,  -4.254196511 },
        {   16'000., 0.462044856449356,  -6.706317200 },
        {   20'000., 0.340918345911634,  -9.346992547 },
        {   22'050., 0.294666762978363, -10.613376955 }
    };

    for (auto & Test : AWeightingTests)
    {
        const double Weight = Analysis.GetAcousticWeight(Test.Frequency, WeightingType::AWeighting, 1.);

        const double Decibels = 20. * std::log10(Weight);

        assert(std::abs(Weight   - Test.ExpectedWeight)   < WeightTolerance);
        assert(std::abs(Decibels - Test.ExpectedDecibels) < DecibelTolerance);
    }

    constexpr weighting_test_t BWeightingTests[] =
    {
        {       20., 0.061945880288931, -24.159751422 },
        {      100., 0.521951593253106,  -5.647395448 },
        {    1'000., 1.000039043321642,   0.000339119 },
        {    4'000., 0.919948172289609,  -0.724732782 },
        {    8'000., 0.712770611671976,  -2.941004301 },
        {   10'000., 0.609637005298333,  -4.298573579 },
        {   12'500., 0.497206407676075,  -6.069265662 },
        {   16'000., 0.374665736290569,  -8.527120439 },
        {   20'000., 0.276342931213370, -11.171032802 },
        {   22'050., 0.238824071836831, -12.438438028 }
    };

    for (auto & Test : BWeightingTests)
    {
        const double Weight = Analysis.GetAcousticWeight(Test.Frequency, WeightingType::BWeighting, 1.);

        const double Decibels = 20. * std::log10(Weight);

        assert(std::abs(Weight   - Test.ExpectedWeight)   < WeightTolerance);
        assert(std::abs(Decibels - Test.ExpectedDecibels) < DecibelTolerance);
    }

    constexpr weighting_test_t CWeightingTests[] =
    {
        {       20., 0.488587005734043,  -6.221161751 },
        {      100., 0.965876039258696,  -0.301572149 },
        {    1'000., 0.999780785464601,  -0.001904282 },
        {    4'000., 0.909084064953623,  -0.827919096 },
        {    8'000., 0.703938966400541,  -3.049299877 },
        {   10'000., 0.602040720003176,  -4.407482671 },
        {   12'500., 0.490988847556463,  -6.178567449 },
        {   16'000., 0.369968958083791,  -8.636694270 },
        {   20'000., 0.272873899712495, -11.280760049 },
        {   22'050., 0.235824715060297, -12.548213633 }
    };

    for (auto & Test : CWeightingTests)
    {
        const double Weight = Analysis.GetAcousticWeight(Test.Frequency, WeightingType::CWeighting, 1.);

        const double Decibels = 20. * std::log10(Weight);

        assert(std::abs(Weight   - Test.ExpectedWeight)   < WeightTolerance);
        assert(std::abs(Decibels - Test.ExpectedDecibels) < DecibelTolerance);
    }

    constexpr weighting_test_t DWeightingTests[] =
    {
        {       20., 0.093040046288330, -20.626601644 },
        {      100., 0.436351784271307,  -7.203264877 },
        {    1'000., 1.000000000000001,   0.000000000 },
        {    4'000., 3.590888586341657,  11.104038612 },
        {    8'000., 1.875687842405231,   5.463211269 },
        {   10'000., 1.485438479750624,   3.437093399 },
        {   12'500., 1.179055775054986,   1.430686996 },
        {   16'000., 0.915612192737624,  -0.765768654 },
        {   20'000., 0.729865992657882,  -2.735137427 },
        {   22'050., 0.661242984785963,  -3.592778448 }
    };

    for (auto & Test : DWeightingTests)
    {
        const double Weight = Analysis.GetAcousticWeight(Test.Frequency, WeightingType::DWeighting, 1.);

        const double Decibels = 20. * std::log10(Weight);

        assert(std::abs(Weight   - Test.ExpectedWeight)   < WeightTolerance);
        assert(std::abs(Decibels - Test.ExpectedDecibels) < DecibelTolerance);
    }

    constexpr weighting_test_t MWeightingTests[] =
    {
        {       20., 0.0202609969281888, -33.866783786 },
        {      100., 0.1012873424294580, -19.888896474 },
        {    1'000., 0.9955572537679230,  -0.038675179 },
        {    4'000., 3.3488312656300400,  10.497865314 },
        {    8'000., 3.6854119108895500,  11.329720702 },
        {   10'000., 2.5399209406451300,   8.096403974 },
        {   12'500., 0.9938189010351010,  -0.053854957 },
        {   16'000., 0.2588781981336750, -11.738090457 },
        {   20'000., 0.0774867544332273, -22.215450587 },
        {   22'050., 0.0462349277052746, -26.700596342 }
    };

    for (auto & Test : MWeightingTests)
    {
        const double Weight = Analysis.GetAcousticWeight(Test.Frequency, WeightingType::MWeighting, 1.);

        const double Decibels = 20. * std::log10(Weight);

        assert(std::abs(Weight   - Test.ExpectedWeight)   < WeightTolerance);
        assert(std::abs(Decibels - Test.ExpectedDecibels) < DecibelTolerance);
    }
}

static void TestWindowFunctions()
{
//  constexpr double Epsilon = 1.0e-12;
    constexpr double Epsilon = std::numeric_limits<double>::epsilon();

    {
        const BoxCar w{0., true};

        assert(w( 0.)   == 1.);
        assert(w(-1.)   == 1.);
        assert(w( 1.)   == 1.);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    {
        const Hann w{0., true};

        assert(std::abs(w( 0.) - 1.) < Epsilon);
        assert(std::abs(w(-1.))      < Epsilon);
        assert(std::abs(w( 1.))      < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Hann
    {
        const Hann w{0., false};

        const double Expected = (1. + std::cos(1.5 * std::numbers::pi)) / 2.;

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    {
        const Hamming w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.) - 0.08) < Epsilon);
        assert(std::abs(w( 1.) - 0.08) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Hamming
    {
        const Hamming w{0., false};

        const double Expected = 0.54 + (0.46 * std::cos(1.5 * std::numbers::pi));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Blackmann
    {
        const Blackman w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Blackmann
    {
        const Blackman w{0., false};

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = 0.42 + (0.5 * std::cos(Phase)) + (0.08 * std::cos(2. * Phase));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Nuttall
    {
        const Nuttall w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Nuttall
    {
        const Nuttall w{0., false};

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = 0.355768 + (0.487396 * std::cos(Phase)) + (0.144232 * std::cos(2. * Phase)) + (0.012604 * std::cos(3. * Phase));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Flat Top
    {
        constexpr double A0 = 0.215578950;
        constexpr double A1 = 0.416631580;
        constexpr double A2 = 0.277263158;
        constexpr double A3 = 0.083578947;
        constexpr double A4 = 0.006947368;

        constexpr double Normalization = A0 + A1 + A2 + A3 + A4;

        constexpr double Endpoint = (A0 - A1 + A2 - A3 + A4) / Normalization;

        const FlatTop w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.) - Endpoint) < Epsilon);
        assert(std::abs(w( 1.) - Endpoint) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Flat Top
    {
        const FlatTop w{0., false};

        constexpr double A0 = 0.215578950;
        constexpr double A1 = 0.416631580;
        constexpr double A2 = 0.277263158;
        constexpr double A3 = 0.083578947;
        constexpr double A4 = 0.006947368;

        constexpr double Normalization = A0 + A1 + A2 + A3 + A4;

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = (A0 + (A1 * std::cos(Phase)) + (A2 * std::cos(2. * Phase)) + (A3 * std::cos(3. * Phase)) + (A4 * std::cos(4. * Phase))) / Normalization;

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Bartlett
    {
        const Bartlett w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test known intermediate values.
        assert(std::abs(w(-0.25) - 0.75) < Epsilon);
        assert(std::abs(w( 0.25) - 0.75) < Epsilon);
        assert(std::abs(w(-0.75) - 0.25) < Epsilon);
        assert(std::abs(w( 0.75) - 0.25) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Bartlett
    {
        const Bartlett w{0., false};

        const double Expected = 1. - std::abs(1.5);

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Welch
    {
        const Welch w(0., true, 1.);

        // Test maximum at the center.
        assert(std::fabs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::fabs(w(-1.)) < Epsilon);
        assert(std::fabs(w( 1.)) < Epsilon);

        // Test known intermediate values.
        assert(std::fabs(w(-0.5) - 0.75) < Epsilon);
        assert(std::fabs(w( 0.5) - 0.75) < Epsilon);

        assert(std::fabs(w(-0.25) - 0.9375) < Epsilon);
        assert(std::fabs(w( 0.25) - 0.9375) < Epsilon);

        // Test symmetry around zero.
        assert(std::fabs(w(-0.125) - w(0.125)) < Epsilon);
        assert(std::fabs(w(-0.625) - w(0.625)) < Epsilon);
        assert(std::fabs(w(-0.875) - w(0.875)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);

        assert(w(-2.) == 0.);
        assert(w( 2.) == 0.);
    }

    {
        const Welch w(0., false, 1.);

        assert(std::fabs(w(-1.5) + 1.25) < Epsilon);
        assert(std::fabs(w( 1.5) + 1.25) < Epsilon);

        assert(std::fabs(w(-2.) + 3.) < Epsilon);
        assert(std::fabs(w( 2.) + 3.) < Epsilon);
    }

    // Tukey
    {
        const Tukey w(0., true, 0.5);

        // Test maximum at the center.
        assert(std::fabs(w(0.) - 1.) < Epsilon);

        // Test central region.
        assert(std::fabs(w(-0.5)  - 1.) < Epsilon);
        assert(std::fabs(w(-0.25) - 1.) < Epsilon);
        assert(std::fabs(w( 0.25) - 1.) < Epsilon);
        assert(std::fabs(w( 0.5)  - 1.) < Epsilon);

        // Test endpoints.
        assert(std::fabs(w(-1.)) < Epsilon);
        assert(std::fabs(w( 1.)) < Epsilon);

        // Halfway through each taper, the value is 0.5.
        assert(std::fabs(w(-0.75) - 0.5) < Epsilon);
        assert(std::fabs(w( 0.75) - 0.5) < Epsilon);

        // Verify another known value in the taper.
        const double Expected = (1. + std::cos(std::numbers::pi / 2.)) / 2.;

        assert(std::fabs(w(0.75) - Expected) < Epsilon);

        // The non-skewed Tukey window is symmetric.
        constexpr double Samples[]
        {
            0.,
            0.125,
            0.25,
            0.5,
            0.625,
            0.75,
            0.875,
            1.
        };

        for (const double x : Samples)
            assert(std::fabs(w(-x) - w(x)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
        assert(w(-2.)   == 0.);
        assert(w( 2.)   == 0.);
    }

    // Ogg Vorbis
    {
        const OggVorbis w{0., true};

        assert(std::abs(w( 0.) - 1.) < Epsilon);
        assert(std::abs(w(-1.))      < Epsilon);
        assert(std::abs(w( 1.))      < Epsilon);

        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Cascaded sine
    {
        const CascadedSine w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated cascaded sine
    {
        const CascadedSine w{0., false};

        const double HalfPi = std::numbers::pi / 2.;

        const double Sine = std::sin(1.5 * HalfPi);

        const double Expected = 1. - std::sin(HalfPi * Sine * Sine);

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // 4-term Blackman-Harris
    {
        const BlackmanHarris4 w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        constexpr double Endpoint = 0.00006;

        assert(std::abs(w(-1.) - Endpoint) < Epsilon);
        assert(std::abs(w( 1.) - Endpoint) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated 4-term Blackman-Harris
    {
        const BlackmanHarris4 w{0., false};

        constexpr double a0 = 0.35875;
        constexpr double a1 = 0.48829;
        constexpr double a2 = 0.14128;
        constexpr double a3 = 0.01168;

        const double t = 1.5;

        const double Expected = a0 + (a1 * std::cos(std::numbers::pi * t)) + (a2 * std::cos(2. * std::numbers::pi * t)) + (a3 * std::cos(3. * std::numbers::pi * t));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }
}

static void TestFrequencyScaler()
{
    constexpr double Epsilon = 1.0e-5;

    for (double f = MinFrequency; f <= MaxFrequency; f += 10.)
    {
        double Scale = ScaleFrequency(f, ScalingFunction::Linear, 0.);
        double Frequency = DescaleFrequency(Scale, ScalingFunction::Linear, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::Logarithmic, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::Logarithmic, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::ShiftedLogarithmic, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::ShiftedLogarithmic, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::Mel, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::Mel, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::Bark, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::Bark, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::AdjustableBark, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::AdjustableBark, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::ERB, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::ERB, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::Cams, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::Cams, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::HyperbolicSine, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::HyperbolicSine, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::NthRoot, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::NthRoot, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::NegativeExponential, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::NegativeExponential, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);

        Scale = ScaleFrequency(f, ScalingFunction::Period, 0.);
        Frequency = DescaleFrequency(Scale, ScalingFunction::Period, 0.);

        assert(std::abs(Frequency - f) <= Epsilon);
    }
}

void RunTests()
{
    Log.Write("Running tests...");

    Log.Write("Testing frequency scalers...");

    TestFrequencyScaler();

    Log.Write("Testing window functions...");

    TestWindowFunctions();

    Log.Write("Testing weighting functions...");

    TestWeighting();

    Log.Write("Finished running tests.");
}

#endif
