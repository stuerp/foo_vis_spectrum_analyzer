
/** $VER: DominantColors.cpp (2024.01.07) P. Stuer - Determines the dominant colors of an image. Based on BetterDominantColors (https://github.com/Humeur/better-dominant-colors) **/

#include "DominantColors.h"

#include "Support.h"
#include "Pixels.h"
#include "PeakFinder.h"

#include <array>
#include <algorithm>

#pragma hdrstop

struct Bin
{
    int Count;      // Number of pixels with this hue
    double AvgS;    // Average saturation of the pixels with this hue
    double AvgV;    // Average value of the pixels with this hue
};

struct HSV
{
    double h;
    double s;
    double v;
};

void Intensify(std::vector<std::pair<Bin, size_t>> & peaks) noexcept;
std::vector<std::pair<Bin, size_t>> GetPeaks(std::array<Bin, 360> & bins, size_t count) noexcept;
std::vector<std::pair<Bin, size_t>> getDistantPeaks(const std::vector<std::pair<Bin, size_t>> & peaks, size_t dominantCount) noexcept;
bool isXDistantFromNToPairArray(size_t x, size_t n, const std::vector<std::pair<Bin, size_t>> & array) noexcept;
HSV ToHSV(uint8_t r, uint8_t g, uint8_t b) noexcept;
D2D1_COLOR_F ToRGB(const HSV & hsv) noexcept;
bool IsEqual(double a, double b, double epsilon = 0.00001) noexcept;

struct ColorConverter
{
    D2D1_COLOR_F operator()(const std::pair<Bin, size_t> & peak)
    {
        return ToRGB({ static_cast<double>(peak.second), peak.first.AvgS, peak.first.AvgV });
    }
};

/// <summary>
/// Gets the dominant colors of the specified image.
/// </summary>
HRESULT DominantColors::Get(CComPtr<IWICBitmapFrameDecode> frame, size_t Count, std::vector<D2D1_COLOR_F> & colors) const noexcept
{
    Pixels Pixels;

    HRESULT hr = Pixels.Initialize(frame);

    if (SUCCEEDED(hr))
    {
        size_t BytesPerPixel = Pixels.GetBitsPerPixel() >> 3;

        std::array<Bin, 360> Bins{};

        Bins.fill({ 0, 100., 100. });

        UINT Width = Pixels.GetWidth();
        UINT Height = Pixels.GetHeight();
        BYTE * Data = Pixels.GetData();
        UINT Stride = Pixels.GetStride();

        #pragma warning(disable: 6011 6385)
        for (UINT y = 0; y < Height; y++)
        {
            const BYTE * Line = Data;

            for (UINT x = 0; x < Width; x += BytesPerPixel)
            {
                HSV hsv = ToHSV(Line[2], Line[1], Line[0]);

                Bin & b = Bins[(size_t) hsv.h];

                b.AvgS = (b.Count * b.AvgS + hsv.s) / (b.Count + 1);
                b.AvgV = (b.Count * b.AvgV + hsv.v) / (b.Count + 1);

                b.Count++;

                Line += BytesPerPixel;
            }

            Data += Stride;
        }
        #pragma warning(default: 6011 6385)

        std::vector<std::pair<Bin, size_t>> Peaks = GetPeaks(Bins, Count);

        const bool enliven = true;

        if (enliven)
            Intensify(Peaks);

        colors.resize(Peaks.size());

        ColorConverter colorConverter;

        std::transform(Peaks.begin(), Peaks.end(), colors.begin(), colorConverter);
    }

    return hr;
}

/// <summary>
/// 
/// </summary>
std::vector<std::pair<Bin, size_t>> GetPeaks(std::array<Bin, 360> & bins, size_t count) noexcept
{
    std::vector<float> x(360);

    for (size_t i = 0; i < 360; ++i)
        x[i] = (float) bins[i].Count;

    std::vector<int> Peaks;

    PeakFinder::findPeaks(x, Peaks, false, 100.0);

    std::vector<std::pair<Bin, size_t>> PeaksToBin(Peaks.size());

    std::transform(Peaks.cbegin(), Peaks.cend(), PeaksToBin.begin(), [&bins](int index)
    {
        return std::pair<Bin, size_t>{bins[index], (size_t) index};
    });

    std::sort(PeaksToBin.begin(), PeaksToBin.end(), [](const std::pair<Bin, int> & a, const std::pair<Bin, int> & b)
    {
        return a.first.Count > b.first.Count;
    });

    PeaksToBin = getDistantPeaks(PeaksToBin, count);

    return { PeaksToBin.begin(), PeaksToBin.begin() + count };
}

/// <summary>
/// Intensifies the saturation and the value of the peak colors.
/// </summary>
void Intensify(std::vector<std::pair<Bin, size_t>> & peaks) noexcept
{
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        peaks[i].first.AvgS *= 1.5;

        if (peaks[i].first.AvgS > 1)
            peaks[i].first.AvgS = 1;

        peaks[i].first.AvgV *= 1.5;

        if (peaks[i].first.AvgV > 1)
            peaks[i].first.AvgV = 1;
    }
}

/// <summary>
/// 
/// </summary>
std::vector<std::pair<Bin, size_t>> getDistantPeaks(const std::vector<std::pair<Bin, size_t>> & peaks, size_t dominantCount) noexcept
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

/// <summary>
/// 
/// </summary>
bool isXDistantFromNToPairArray(size_t x, size_t n, const std::vector<std::pair<Bin, size_t>> & array) noexcept
{
    for (const auto & e : array)
    {
        size_t value = e.second;

        if ((size_t) std::abs((int) x - (int) value) < n)
            return false;
    }

    return true;
}

/// <summary>
/// Converts RGB color values to HSV.
/// </summary>
HSV ToHSV(uint8_t r, uint8_t g, uint8_t b) noexcept
{
    HSV hsv {};

    // Special case for black.
    if ((r + g + b) == 0)
        return hsv;

    const double R = (double) r / 255.;
    const double G = (double) g / 255.;
    const double B = (double) b / 255.;

    const double MaxC = Max(R, Max(G, B));
    const double MinC = Min(R, Min(G, B));

    const double Delta = MaxC - MinC;

    if (IsEqual(Delta, 0.))
        hsv.h = 0.;
    else
    {
        if (IsEqual(MaxC, R))
            hsv.h = 60. * ((G - B) / Delta);
        else
        if (IsEqual(MaxC, G))
            hsv.h = 60. * ((B - R) / Delta + 2.);
        else
        if (IsEqual(MaxC, B))
            hsv.h = 60. * ((R - G) / Delta + 4.);

        if (hsv.h < 0.)
            hsv.h += 360.;
    }

    hsv.s = IsEqual(MaxC, 0.) ? 0. : Delta / MaxC;
    hsv.v = MaxC;

    return hsv;
}

/// <summary>
/// Converts HSV color values to RGB.
/// </summary>
D2D1_COLOR_F ToRGB(const HSV & hsv) noexcept
{
    double c = hsv.v * hsv.s;
    double x = c * (1. - std::abs(std::fmod(hsv.h / 60., 2) - 1.));
    double m = hsv.v - c;

    double R = 0., G = 0., B = 0.;

    if (hsv.h < 60.)
    {
        R = c; G = x;
    }
    else
    if (hsv.h < 120.)
    {
        R = x; G = c;
    }
    else
    if (hsv.h < 180.)
    {
        G = c; B = x;
    }
    else
    if (hsv.h < 240.)
    {
        G = x; B = c;
    }
    else
    if (hsv.h < 300.)
    {
        R = x; B = c;
    }
    else
    if (hsv.h < 360.)
    {
        R = c; B = x;
    }

    return D2D1::ColorF((float) (R + m), (float) (G + m), (float) (B + m));
}

/// <summary>
/// Returns true if the specified double numbers are (nearly) equal.
/// </summary>
inline bool IsEqual(double a, double b, double epsilon) noexcept
{
    return std::abs(a - b) < epsilon;
}
