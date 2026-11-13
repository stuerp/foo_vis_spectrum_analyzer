#include "pch.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "FFTAnalyzer.h"

class fft_analyzer_test_t
{
public:
    static void Run()
    {
        TestAboveBinToneMovesUpwardAndPreservesPower();
        TestBelowBinToneMovesDownwardAndPreservesPower();
        TestBinsMayBeEvacuatedWithoutLosingPower();
        TestSilenceProducesSilence();
    }

    static constexpr double Pi    = std::numbers::pi;
    static constexpr double TwoPi = 2. * Pi;
    static constexpr double Pi2   = Pi * 0.5;

private:
    static void Require(bool Condition, const char* Message)
    {
        if (!Condition)
            throw std::runtime_error(Message);
    }

    static void RequireNear(double Actual, double Expected, double Tolerance, const char* Message)
    {
        if (!std::isfinite(Actual) || !std::isfinite(Expected) || !std::isfinite(Tolerance) || (Tolerance < 0.) || (std::abs(Actual - Expected) > Tolerance))
        {
            throw std::runtime_error(
                std::string(Message) +
                ": expected " + std::to_string(Expected) +
                ", actual " + std::to_string(Actual) +
                ", tolerance " + std::to_string(Tolerance));
        }
    }

    static void PrintTest(const char* Name)
    {
        std::cout << "[ RUN      ] " << Name << '\n';
    }

    static void PrintSuccess(const char* Name)
    {
        std::cout << "[       OK ] " << Name << '\n';
    }

    static double GetAcceptedPositivePower(const fft_analyzer_t& Analyzer)
    {
        const size_t BinCount = (Analyzer._FFTSize / 2) + 1;

        double MaximumPower = 0.;

        for (size_t i = 0; i < BinCount; ++i)
            MaximumPower = std::max(MaximumPower, std::norm(Analyzer._Coefficients[i]));

        if (MaximumPower == 0.)
            return 0.;

        const double MinimumPower = MaximumPower * 1e-10;

        double Power = 0.;

        for (size_t i = 0; i < BinCount; ++i)
        {
            const double BinPower = std::norm(Analyzer._Coefficients[i]);

            if (BinPower >= MinimumPower)
                Power += BinPower;
        }

        return Power;
    }

    static double GetReassignedPositivePower(const fft_analyzer_t& Analyzer)
    {
        const size_t BinCount = (Analyzer._FFTSize / 2) + 1;

        double Power = 0.;

        for (size_t i = 0; i < BinCount; ++i)
            Power += Analyzer._ReassignedPower[i];

        return Power;
    }

    static double GetLocalReassignedPower(
        const fft_analyzer_t& Analyzer,
        size_t FirstBin,
        size_t LastBin)
    {
        const size_t BinCount = (Analyzer._FFTSize / 2) + 1;

        FirstBin = std::min(FirstBin, BinCount - 1);
        LastBin = std::min(LastBin, BinCount - 1);

        double Power = 0.;

        for (size_t i = FirstBin; i <= LastBin; ++i)
            Power += Analyzer._ReassignedPower[i];

        return Power;
    }

    static double GetLocalReassignedCenter(
        const fft_analyzer_t& Analyzer,
        size_t FirstBin,
        size_t LastBin)
    {
        const size_t BinCount = (Analyzer._FFTSize / 2) + 1;

        FirstBin = std::min(FirstBin, BinCount - 1);
        LastBin = std::min(LastBin, BinCount - 1);

        double WeightedBin = 0.;
        double Power = 0.;

        for (size_t i = FirstBin; i <= LastBin; ++i)
        {
            const double BinPower = Analyzer._ReassignedPower[i];

            WeightedBin += (double)i * BinPower;
            Power += BinPower;
        }

        Require(Power > 0., "The reassigned test region contains no power");

        return WeightedBin / Power;
    }

    static size_t GetStrongestOriginalBin(
        const fft_analyzer_t& Analyzer,
        size_t FirstBin,
        size_t LastBin)
    {
        const size_t BinCount = (Analyzer._FFTSize / 2) + 1;

        FirstBin = std::min(FirstBin, BinCount - 1);
        LastBin = std::min(LastBin, BinCount - 1);

        size_t StrongestBin = FirstBin;
        double StrongestPower = std::norm(Analyzer._Coefficients[FirstBin]);

        for (size_t i = FirstBin + 1; i <= LastBin; ++i)
        {
            const double Power = std::norm(Analyzer._Coefficients[i]);

            if (Power > StrongestPower)
            {
                StrongestBin = i;
                StrongestPower = Power;
            }
        }

        return StrongestBin;
    }

    static void ClearInput(fft_analyzer_t& Analyzer)
    {
        Analyzer._Next = 0;

        for (auto& Sample : Analyzer._InputRing)
            Sample = (audio_sample)0.;
    }

    static void GenerateCosine(
        fft_analyzer_t& Analyzer,
        double FrequencyBin,
        double Amplitude = 1.)
    {
        const size_t FFTSize = Analyzer._FFTSize;

        Analyzer._Next = 0;

        Require(
            Analyzer._InputRing.size() >= FFTSize,
            "The input ring buffer is smaller than the FFT");

        for (size_t i = 0; i < FFTSize; ++i)
        {
            const double Phase = 2. * Pi * FrequencyBin * (double)i / (double)FFTSize;

            Analyzer._InputRing[i] =
                (audio_sample)(Amplitude * std::cos(Phase));
        }
    }

    static void PrepareOrdinaryCoefficients(
        fft_analyzer_t& Analyzer,
        double norm)
    {
        const size_t FFTSize = Analyzer._FFTSize;
        const size_t Size = Analyzer._InputRing.size();

        size_t i = Analyzer._Next;

        const audio_sample* const Sample = Analyzer._InputRing.data();

        for (size_t j = 0; j < FFTSize; ++j)
        {
            const double x = msc::Map(
                j,
                (size_t)0,
                FFTSize - 1,
                -1.,
                1.);

            const double WindowFactor = Analyzer.WindowFunction(x);

            Analyzer._Samples[j] = std::complex<double>(
                Sample[i] * WindowFactor,
                0.);

            if (++i == Size)
                i = 0;
        }

        // Normalize the samples in exactly the same way as the analyzer.
        {
            const double Factor = (double)FFTSize / norm;

            for (auto& Sample : Analyzer._Samples)
                Sample *= Factor;
        }

        // Transform the ordinary windowed data.
        Analyzer._FFT.Transform(
            Analyzer._Samples,
            Analyzer._Coefficients);

        // Normalize the coefficients in exactly the same way as the analyzer.
        {
            const double Factor = 2. / (double)FFTSize;

            for (auto& Coefficient : Analyzer._Coefficients)
                Coefficient *= Factor;
        }

        // Correct the DC and Nyquist bins.
        Analyzer._Coefficients.front() *= 0.5;

        if ((FFTSize % 2) == 0)
            Analyzer._Coefficients[FFTSize / 2] *= 0.5;
    }

    static void PrepareTone(
        fft_analyzer_t& Analyzer,
        double FrequencyBin,
        double norm)
    {
        GenerateCosine(Analyzer, FrequencyBin);
        PrepareOrdinaryCoefficients(Analyzer, norm);
    }

    static void TestAboveBinToneMovesUpwardAndPreservesPower()
    {
        const char* const Name =
            "Above-bin tone moves upward and preserves power";

        PrintTest(Name);

        constexpr size_t FFTSize = 4096;
        constexpr size_t SourceBin = 300;
        constexpr double Offset = 0.25;

        const double ExpectedBin = (double)SourceBin + Offset;
        const double norm = (double)FFTSize;

        fft_analyzer_t Analyzer(FFTSize);

        PrepareTone(Analyzer, ExpectedBin, norm);

        const size_t StrongestOriginalBin = GetStrongestOriginalBin(
            Analyzer,
            SourceBin - 8,
            SourceBin + 8);

        const double AcceptedPower = GetAcceptedPositivePower(Analyzer);

        Analyzer.Reassign(norm);

        const double ReassignedPower =
            GetReassignedPositivePower(Analyzer);

        const double ReassignedCenter = GetLocalReassignedCenter(
            Analyzer,
            SourceBin - 8,
            SourceBin + 8);

        RequireNear(
            ReassignedPower,
            AcceptedPower,
            std::max(1., AcceptedPower) * 1e-12,
            "Reassignment did not preserve accepted power");

        Require(
            ReassignedCenter > (double)StrongestOriginalBin,
            "An above-bin tone was not reassigned upward");

        RequireNear(
            ReassignedCenter,
            ExpectedBin,
            0.05,
            "The reassigned center is inaccurate");

        PrintSuccess(Name);
    }

    static void TestBelowBinToneMovesDownwardAndPreservesPower()
    {
        const char* const Name =
            "Below-bin tone moves downward and preserves power";

        PrintTest(Name);

        constexpr size_t FFTSize = 4096;
        constexpr size_t SourceBin = 300;
        constexpr double Offset = -0.25;

        const double ExpectedBin = (double)SourceBin + Offset;
        const double norm = (double)FFTSize;

        fft_analyzer_t Analyzer(FFTSize);

        PrepareTone(Analyzer, ExpectedBin, norm);

        const size_t StrongestOriginalBin = GetStrongestOriginalBin(
            Analyzer,
            SourceBin - 8,
            SourceBin + 8);

        const double AcceptedPower = GetAcceptedPositivePower(Analyzer);

        Analyzer.Reassign(norm);

        const double ReassignedPower =
            GetReassignedPositivePower(Analyzer);

        const double ReassignedCenter = GetLocalReassignedCenter(
            Analyzer,
            SourceBin - 8,
            SourceBin + 8);

        RequireNear(
            ReassignedPower,
            AcceptedPower,
            std::max(1., AcceptedPower) * 1e-12,
            "Reassignment did not preserve accepted power");

        Require(
            ReassignedCenter < (double)StrongestOriginalBin,
            "A below-bin tone was not reassigned downward");

        RequireNear(
            ReassignedCenter,
            ExpectedBin,
            0.05,
            "The reassigned center is inaccurate");

        PrintSuccess(Name);
    }

    static void TestBinsMayBeEvacuatedWithoutLosingPower()
    {
        const char* const Name =
            "Bins may be evacuated without losing power";

        PrintTest(Name);

        constexpr size_t FFTSize = 4096;
        constexpr size_t SourceBin = 300;

        const double FrequencyBin = (double)SourceBin + 0.25;
        const double norm = (double)FFTSize;

        fft_analyzer_t Analyzer(FFTSize);

        PrepareTone(Analyzer, FrequencyBin, norm);

        const double AcceptedPower = GetAcceptedPositivePower(Analyzer);

        Analyzer.Reassign(norm);

        const double ReassignedPower =
            GetReassignedPositivePower(Analyzer);

        const double LocalPower = GetLocalReassignedPower(
            Analyzer,
            SourceBin - 16,
            SourceBin + 16);

        size_t NearlyEmptyBinCount = 0;

        for (size_t i = SourceBin - 16; i <= SourceBin + 16; ++i)
        {
            if (Analyzer._ReassignedPower[i] <
                std::max(1., LocalPower) * 1e-12)
            {
                ++NearlyEmptyBinCount;
            }
        }

        RequireNear(
            ReassignedPower,
            AcceptedPower,
            std::max(1., AcceptedPower) * 1e-12,
            "Evacuated bins caused a loss of accepted power");

        Require(
            NearlyEmptyBinCount > 0,
            "The reassigned spectrum did not contain any evacuated bins");

        PrintSuccess(Name);
    }

    static void TestSilenceProducesSilence()
    {
        const char* const Name =
            "Silence produces a silent reassigned spectrum";

        PrintTest(Name);

        constexpr size_t FFTSize = 4096;

        const double norm = (double)FFTSize;

        fft_analyzer_t Analyzer(FFTSize);

        ClearInput(Analyzer);
        PrepareOrdinaryCoefficients(Analyzer, norm);

        Analyzer.Reassign(norm);

        const double ReassignedPower =
            GetReassignedPositivePower(Analyzer);

        RequireNear(
            ReassignedPower,
            0.,
            0.,
            "Silence produced reassigned power");

        for (const auto& Coefficient : Analyzer._ReassignedCoefficients)
        {
            Require(
                Coefficient == std::complex<double>{},
                "Silence produced a nonzero reassigned coefficient");
        }

        PrintSuccess(Name);
    }
};

int main()
{
    try
    {
        fft_analyzer_test_t::Run();

        std::cout << "[  PASSED  ] All reassignment tests\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "[  FAILED  ] " << Exception.what() << '\n';

        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "[  FAILED  ] Unknown exception\n";

        return EXIT_FAILURE;
    }
}
