
/** $VER: DominantColors.cpp (2024.01.07) P. Stuer - Determines the dominant colors of an image. Based on BetterDominantColors (https://github.com/Humeur/better-dominant-colors) **/

#include "DominantColors.h"

#include "Support.h"
#include "Pixels.h"
#include "PeakFinder.h"

#include <algorithm>

#pragma hdrstop

struct Bin
{
    int Count;
    double sAverage;
    double vAverage;
};

struct HSV
{
    double h;
    double s;
    double v;
};

bool double_equals(double a, double b, double epsilon = 0.00001)
{
    return std::abs(a - b) < epsilon;
}

HSV RGBToHSV(uint8_t r, uint8_t g, uint8_t b)
{
    double NormalizedR = (double) r / 255.;
    double NormalizedG = (double) g / 255.;
    double NormalizedB = (double) b / 255.;

    double cMax = Max(NormalizedR, Max(NormalizedG, NormalizedB));
    double cMin = Min(NormalizedR, Min(NormalizedG, NormalizedB));

    double delta = cMax - cMin;

    HSV hsv {};

    if (double_equals(delta, 0.))
        hsv.h = 0.;
    else
    if (double_equals(cMax, NormalizedR))
        hsv.h = 60. * ((NormalizedG - NormalizedB) / delta);
    else
    if (double_equals(cMax, NormalizedG))
        hsv.h = 60. * ((NormalizedB - NormalizedR) / delta + 2.);
    else
    if (double_equals(cMax, NormalizedB))
        hsv.h = 60. * ((NormalizedR - NormalizedG) / delta + 4.);

    if (hsv.h < 0.)
        hsv.h += 360.;

    if (double_equals(cMax, 0.))
        hsv.s = 0.;
    else
        hsv.s = delta / cMax;

    hsv.v = cMax;

    return hsv;
}

uint32_t HSVToRGB(const HSV & hsv)
{
    double c = hsv.v * hsv.s;
    double x = c * (1. - std::abs(std::fmod(hsv.h / 60., 2) - 1.));
    double m = hsv.v - c;

    double NormalizedR = 0., NormalizedG = 0., NormalizedB = 0.;

    if (hsv.h < 60.)
    {
        NormalizedR = c; NormalizedG = x;
    }
    else
    if (hsv.h < 120.)
    {
        NormalizedR = x; NormalizedG = c;
    }
    else
    if (hsv.h < 180.)
    {
        NormalizedG = c; NormalizedB = x;
    }
    else
    if (hsv.h < 240.)
    {
        NormalizedG = x; NormalizedB = c;
    }
    else
    if (hsv.h < 300.)
    {
        NormalizedR = x; NormalizedB = c;
    }
    else
    if (hsv.h < 360.)
    {
        NormalizedR = c; NormalizedB = x;
    }

    return (uint32_t) ((NormalizedG + m) * 255.) << 16U | (uint32_t)((NormalizedG + m) * 255.) << 8U | (uint32_t)((NormalizedR + m) * 255.);
}

void enlivenPeaksColors(std::vector<std::pair<Bin, size_t>> & peaks)
{
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        peaks[i].first.sAverage *= 1.5;
        peaks[i].first.vAverage *= 1.5;

        if (peaks[i].first.sAverage > 1)
            peaks[i].first.sAverage = 1;

        if (peaks[i].first.vAverage > 1)
            peaks[i].first.vAverage = 1;
    }
}

bool isXDistantFromNToPairArray(size_t x, size_t n, const std::vector<std::pair<Bin, size_t>> & array)
{
    for (const auto & e : array)
    {
        size_t value = e.second;

        if ((size_t) std::abs((int) x - (int) value) < n)
            return false;
    }

    return true;
}

std::vector<std::pair<Bin, size_t>> getDistantPeaks(const std::vector<std::pair<Bin, size_t>> & peaks, size_t dominantCount)
{
    size_t n = 40;

    std::vector<std::pair<Bin, size_t>> peaksTmp(peaks);
    std::vector<std::pair<Bin, size_t>> distantPeaks;

    while (distantPeaks.size() < dominantCount)
    {
        if (peaksTmp.empty())
        {
            std::copy(peaks.begin(), peaks.end(), std::back_inserter(peaksTmp));

            n = (size_t) (n / 1.5);
        }

        if (isXDistantFromNToPairArray(peaksTmp[0].second, n, distantPeaks))
            distantPeaks.emplace_back(peaksTmp[0]);

        peaksTmp.erase(peaksTmp.begin());
    }

    return distantPeaks;
}

std::vector<std::pair<Bin, size_t>> GetPeaks(Bin * bins, size_t count)
{
    std::vector<float> x(360);

    for (size_t i = 0; i < 360; ++i)
        x[i] = bins[i].Count;

    std::vector<int> Peaks;

    PeakFinder::findPeaks(x, Peaks, false, 100.0);

    std::vector<std::pair<Bin, size_t>> PeaksToBin(Peaks.size());

    std::transform(Peaks.cbegin(), Peaks.cend(), PeaksToBin.begin(), [&bins](int index)
    {
        return std::pair<Bin, size_t>{bins[index], index};
    });

    std::sort(PeaksToBin.begin(), PeaksToBin.end(), [](const std::pair<Bin, int> & a, const std::pair<Bin, int> & b)
    {
        return a.first.Count > b.first.Count;
    });

    PeaksToBin = getDistantPeaks(PeaksToBin, count);

    return { PeaksToBin.begin(), PeaksToBin.begin() + count };
}

struct ColorConverter
{
    uint32_t operator()(const std::pair<Bin, size_t> & peak)
    {
        return HSVToRGB({ static_cast<double>(peak.second), peak.first.sAverage, peak.first.vAverage });
    }
};

/// <summary>
/// Gets the dominant colors of the specified image.
/// </summary>
HRESULT DominantColors::Get(CComPtr<IWICBitmapFrameDecode> frame, size_t Count, std::vector<uint32_t> & colors) const noexcept
{
    Pixels Pixels;

    HRESULT hr = Pixels.Initialize(frame);

    if (SUCCEEDED(hr))
    {
        size_t BytesPerPixel = Pixels.GetBitsPerPixel() >> 3;

        Bin Bins[360] = { };

        for (size_t i = 0; i < 360; ++i)
        {
            Bins[i].Count = 0;
            Bins[i].sAverage = 100.;
            Bins[i].vAverage = 100.;
        }

        #pragma warning(disable: 6011 6385)
        UINT Width = Pixels.GetWidth();
        UINT Height = Pixels.GetHeight();
        BYTE * Data = Pixels.GetData();
        UINT Stride = Pixels.GetStride();

        for (UINT y = 0; y < Height; y++)
        {
            const BYTE * Line = Data;

            for (UINT x = 0; x < Width; x += BytesPerPixel)
            {
                HSV hsv = RGBToHSV(Line[0], Line[1], Line[2]);

                Bin & b = Bins[(int) hsv.h];

                b.sAverage = (b.Count * b.sAverage + hsv.s) / (b.Count + 1);
                b.vAverage = (b.Count * b.vAverage + hsv.s) / (b.Count + 1);

                b.Count++;

                Line += BytesPerPixel;
            }

            Data += Stride;
        }
        #pragma warning(default: 6011 6385)

        std::vector<std::pair<Bin, size_t>> Peaks = GetPeaks(Bins, Count);

        const bool enliven = true;

        if (enliven)
            enlivenPeaksColors(Peaks);

        colors.resize(Peaks.size());

        ColorConverter colorConverter;

        std::transform(Peaks.begin(), Peaks.end(), colors.begin(), colorConverter);
    }

    return hr;
}
