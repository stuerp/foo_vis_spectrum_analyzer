
/** $VER: Tests.cpp (2026.09.06) P. Stuer - Various unit tests **/

#include "pch.h"

#include "Analyzers/WindowFunctions.h"
#include "Log.h"

#ifdef _DEBUG

static void TestWindowFunctions();

void RunTests()
{
    Log.Write("Running tests...");

    TestWindowFunctions();

    Log.Write("Finished running tests.");
}

void TestWindowFunctions()
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

#endif
