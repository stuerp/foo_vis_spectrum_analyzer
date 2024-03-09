
/** $VER: ColorThief.cpp (2024.03.09) P. Stuer - Based on Fast ColorThief, https://github.com/bedapisl/fast-colorthief **/

#include "ColorThief.h"

#include "Raster.h"
#include "WIC.h"

#include <cmath>
#include <optional>
#include <tuple>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

#pragma hdrstop

namespace ColorThief
{
std::vector<color_t> GetPaletteInternal(const uint8_t * pixels, uint32_t width, uint32_t height, uint32_t stride, uint32_t colorCount, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold) noexcept;

/// <summary>
/// Use the median cut algorithm to cluster similar colors.
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <param name="colorCount">The color count.</param>
/// <param name="quality">0 is the highest quality settings. 10 is the default. There is a trade-off between quality and speed. The bigger the number,
/// the faster a color will be returned but the greater the likelihood that it will not be the visually most dominant color.</param>
/// <param name="ignoreWhite">if set to <c>true</c> [ignore white].</param>
HRESULT GetPalette(IWICBitmapSource * bitmapSource, std::vector<color_t> & palette, uint32_t colorCount, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold)
{
    if ((bitmapSource == nullptr) || (colorCount < 2) || (colorCount > 256) || (quality == 0))
        return E_INVALIDARG;

    CComPtr<IWICFormatConverter> Converter;

    HRESULT hr = _WIC.Factory->CreateFormatConverter(&Converter);

    if (SUCCEEDED(hr))
        hr = Converter->Initialize(bitmapSource, GUID_WICPixelFormat32bppPRGBA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);

    Raster r;

    if (SUCCEEDED(hr))
        hr = r.Initialize(Converter);

    if (SUCCEEDED(hr))
        palette = GetPaletteInternal(r.Data(), r.Width(), r.Height(), r.Stride(), colorCount, quality, ignoreLightColors, lightnessThreshold, transparancyThreshold);

    return hr;
}

/// <summary>
/// Use the median cut algorithm to cluster similar colors and return the base color from the largest cluster.
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <param name="quality">0 is the highest quality settings. 10 is the default. There is a trade-off between quality and speed. The bigger the number,
/// the faster a color will be returned but the greater the likelihood that it will not be the visually most dominant color.</param>
/// <param name="ignoreWhite">if set to <c>true</c> [ignore white].</param>
HRESULT GetDominantColor(IWICBitmapSource * bitmapSource, color_t & color, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold)
{
    std::vector<color_t> Palette;

    HRESULT hr = GetPalette(bitmapSource, Palette, DefaultColorCount, quality, ignoreLightColors, lightnessThreshold, transparancyThreshold);

    if (SUCCEEDED(hr))
        color = Palette[0];

    return S_OK;
}

#pragma region Private

const int32_t SignificantBits = 5;
const int32_t Shift = 8 - SignificantBits;
const size_t MaxIterations = 1000;
const double FractByPopulations = 0.75;

inline size_t GetColorIndex(int32_t r, int32_t g, int32_t b) noexcept
{
    return (size_t) ((r << (2 * SignificantBits)) + (g << SignificantBits) + b);
}

/// <summary>
/// Implements a color box.
/// </summary>
class VBox
{
public:
    VBox(int32_t r1, int32_t r2, int32_t g1, int32_t g2, int32_t b1, int32_t b2, const std::vector<int32_t> & histogram) : r1(r1), r2(r2), g1(g1), g2(g2), b1(b1), b2(b2), _Histogram(histogram), _Count(), _HasCount(false), _Average(), _HasAverage(false)
    {
    }

    #pragma warning(disable: 5267)
    VBox & operator=(const VBox & other)
    {
        r1 = other.r1;
        g1 = other.g1;
        b1 = other.b1;
        r2 = other.r2;
        g2 = other.g2;
        b2 = other.b2;

        _Count = other._Count;
        _HasCount = other._HasCount;

        _Average = other._Average;
        _HasAverage = other._HasAverage;

        return *this;
    }

    int32_t Count() noexcept
    {
        if (!_HasCount)
            GetCount();

        return _Count;
    }

    int32_t Volume() const noexcept
    {
        return (r2 - r1 + 1) * (g2 - g1 + 1) * (b2 - b1 + 1);
    }

    color_t Average() noexcept
    {
        if (!_HasAverage)
            GetAverage();

        return _Average;
    }

    VBox Clone() const noexcept
    {
        return VBox(r1, r2, g1, g2, b1, b2, _Histogram);
    }

private:
    void GetCount() noexcept
    {
        _Count = 0;

        for (int32_t i = r1; i < r2 + 1; ++i)
        {
            for (int32_t j = g1; j < g2 + 1; ++j)
            {
                for (int32_t k = b1; k < b2 + 1; ++k)
                    _Count += _Histogram[GetColorIndex(i, j, k)];
            }
        }

        _HasCount = true;
    }

    void GetAverage() noexcept
    {
        uint64_t Total = 0;

        double SumR = 0.;
        double SumG = 0.;
        double SumB = 0.;

        const int32_t Scale = 1 << Shift;

        for (int32_t i = r1; i < r2 + 1; ++i)
        {
            for (int32_t j = g1; j < g2 + 1; ++j)
            {
                for (int32_t k = b1; k < b2 + 1; ++k)
                {
                    int32_t Count = _Histogram[GetColorIndex(i, j, k)];

                    Total += Count;

                    SumR += Count * (i + 0.5) * Scale;
                    SumG += Count * (j + 0.5) * Scale;
                    SumB += Count * (k + 0.5) * Scale;
                }
            }
        }

        uint8_t AvgR;
        uint8_t AvgG;
        uint8_t AvgB;

        if (Total != 0)
        {
            AvgR = (uint8_t) (SumR / (double) Total);
            AvgG = (uint8_t) (SumG / (double) Total);
            AvgB = (uint8_t) (SumB / (double) Total);
        }
        else
        {
            AvgR = (uint8_t) (Scale * (r1 + r2 + 1) / 2.);
            AvgG = (uint8_t) (Scale * (g1 + g2 + 1) / 2.);
            AvgB = (uint8_t) (Scale * (b1 + b2 + 1) / 2.);
        }

        #pragma warning(disable: 5246)
        _Average = { AvgR, AvgG, AvgB };

        _HasAverage = true;
    }

public:
    int32_t r1;
    int32_t r2;
    int32_t g1;
    int32_t g2;
    int32_t b1;
    int32_t b2;

private:
    const std::vector<int32_t> & _Histogram;

    int32_t _Count;
    bool _HasCount;

    color_t _Average;
    bool _HasAverage;
};

/// <summary>
/// Implements a priority queue.
/// </summary>
template<typename T, typename U>
class PriorityQueue
{
public:
    PriorityQueue(U * predicate) : _Contents(), _Predicate(predicate), _IsSorted(false)
    {
    }

    void Sort()
    {
        std::sort(_Contents.begin(), _Contents.end(), _Predicate);
        _IsSorted = true;
    }

    void Push(const T & o)
    {
        _Contents.push_back(o);
        _IsSorted = false;
    }

    T Pop()
    {
        if (!_IsSorted)
            Sort();

        T Result = _Contents.back();

        _Contents.pop_back();

        return Result;
    }

    std::vector<T> Contents() const noexcept
    {
        return _Contents;
    }

    size_t Size() const
    {
        return _Contents.size();
    }

private:
    std::vector<T> _Contents;
    U * _Predicate;
    bool _IsSorted;
};

enum class PrimaryColor
{
    Red, Green, Blue
};

inline bool CompareCount(VBox & a, VBox & b) noexcept;
inline bool CompareProduct(VBox & a, VBox & b) noexcept;

std::tuple<std::vector<int32_t>, color_t, color_t> GetHistogram(const uint8_t * pixels, uint32_t width, uint32_t height, uint32_t stride, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold) noexcept;
std::vector<color_t> Quantize(const std::vector<int32_t> & histogram, VBox & box, uint32_t colorCount) noexcept;

void Iterate(PriorityQueue<VBox, decltype(CompareCount)> & pq, size_t targetCount, const std::vector<int32_t> & histogram) noexcept;
std::tuple<std::optional<VBox>, std::optional<VBox>> MedianCut(const std::vector<int32_t> & histogram, VBox & box) noexcept;
std::tuple<std::unordered_map<int32_t, uint64_t>, uint64_t> ComputePartialCounts(const std::vector<int32_t> & histogram, const VBox & box, PrimaryColor color) noexcept;

/// <summary>
/// Gets the palette from the RGBA data.
/// </summary>
std::vector<color_t> GetPaletteInternal(const uint8_t * pixels, uint32_t width, uint32_t height, uint32_t stride, uint32_t colorCount, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold) noexcept
{
    std::tuple<std::vector<int32_t>, color_t, color_t> Result = GetHistogram(pixels, width, height, stride, quality, ignoreLightColors, lightnessThreshold, transparancyThreshold);
 
    std::vector<int32_t> Histogram = std::get<0>(Result);

    color_t MinColor = std::get<1>(Result);
    color_t MaxColor = std::get<2>(Result);

    VBox Box = VBox(MinColor[0], MaxColor[0], MinColor[1], MaxColor[1], MinColor[2], MaxColor[2], Histogram);

    return Quantize(Histogram, Box, colorCount);
}

/// <summary>
/// Gets the histogram of the image.
/// </summary>
std::tuple<std::vector<int32_t>, color_t, color_t> GetHistogram(const uint8_t * pixels, uint32_t width, uint32_t height, uint32_t stride, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparancyThreshold) noexcept
{
    std::vector<int32_t> Histogram((size_t) std::pow(2, 3 * SignificantBits), 0);

    color_t MinColor{ 255, 255, 255 };
    color_t MaxColor{   0,   0,   0 };

    struct Pixel
    {
        uint8_t Red;
        uint8_t Green;
        uint8_t Blue;
        uint8_t Alpha;

        bool IsLight(uint8_t threshold) const { return (Red > threshold) && (Green > threshold) && (Blue > threshold); }
        bool IsTransparent(uint8_t threshold) const { return Alpha < threshold; }
    };

    #pragma loop(hint_parallel(4)) // Don't forget /Qpar compiler switch.
    for (uint32_t y = 0; y < height; ++y)
    {
        const Pixel * p = (const Pixel *) pixels;

        for (uint32_t x = 0; x < width; x += quality, p += quality)
        {
            if ((ignoreLightColors && p->IsLight(lightnessThreshold)) || p->IsTransparent(transparancyThreshold))
                continue;

            uint8_t Channel = (uint8_t) (p->Red >> Shift);

            MaxColor[0] = (std::max)(MaxColor[0], Channel);
            MinColor[0] = (std::min)(MinColor[0], Channel);

            size_t Index = (size_t) Channel << (2 * SignificantBits);

            Channel = (uint8_t) (p->Green >> Shift);

            MaxColor[1] = (std::max)(MaxColor[1], Channel);
            MinColor[1] = (std::min)(MinColor[1], Channel);

            Index += (size_t) Channel << SignificantBits;

            Channel = (uint8_t) (p->Blue >> Shift);

            MaxColor[2] = (std::max)(MaxColor[2], Channel);
            MinColor[2] = (std::min)(MinColor[2], Channel);

            Index += (size_t) Channel;

            ++Histogram[Index];
        }

        pixels += stride;
    }

    return { Histogram, MinColor, MaxColor };
}

/// <summary>
/// Quantizes the color box.
/// </summary>
std::vector<color_t> Quantize(const std::vector<int32_t> & histogram, VBox & box, uint32_t colorCount) noexcept
{
    std::vector<color_t> Colors;

    if (colorCount < 2 || colorCount > 256)
        return Colors;

    PriorityQueue<VBox, decltype(CompareCount)> pq1(CompareCount);

    pq1.Push(box);

    Iterate(pq1, (size_t) std::ceil(FractByPopulations * colorCount), histogram);

    PriorityQueue<VBox, decltype(CompareProduct)> pq2(CompareProduct);

    while (pq1.Size() > 0)
        pq2.Push(pq1.Pop());

    if (pq2.Size() < colorCount)
    {
        Iterate(pq2, (size_t) colorCount - pq2.Size(), histogram);

        pq2.Sort();
    }

    for (auto & vbox : pq2.Contents())
        Colors.push_back(vbox.Average());

    // In the queue the boxes are sorted from small to big. Now we want to return the most important color (= biggest box) first.
    std::reverse(Colors.begin(), Colors.end());

    return Colors;
}

/// <summary>
/// Iterates throught the boxes.
/// </summary>
void Iterate(PriorityQueue<VBox, decltype(CompareCount)> & pq, size_t targetCount, const std::vector<int32_t> & histogram) noexcept
{
    size_t ColorCount = 1;

    size_t i = 0;

    while (i < MaxIterations)
    {
        VBox Box = pq.Pop();

        if (Box.Count() == 0)
        {
            pq.Push(Box);
            i += 1;
            continue;
        }

        auto [b1, b2] = MedianCut(histogram, Box);

        if (!b1)
            return; // throw std::runtime_error("Box 1 is not defined; shouldnt happen!");

        pq.Push(b1.value());

        if (b2)
        {
            pq.Push(b2.value());
            ColorCount += 1;
        }

        if (ColorCount >= targetCount)
            return;

        i += 1;
    }
}

/// <summary>
/// Cuts the color box.
/// </summary>
std::tuple<std::optional<VBox>, std::optional<VBox>> MedianCut(const std::vector<int32_t> & histogram, VBox & box) noexcept
{
    int32_t rw = box.r2 - box.r1 + 1;
    int32_t gw = box.g2 - box.g1 + 1;
    int32_t bw = box.b2 - box.b1 + 1;

    int32_t Max = (std::max)(rw, (std::max)(gw, bw));

    PrimaryColor CutColor = (rw == Max) ? PrimaryColor::Red : ((gw == Max) ? PrimaryColor::Green : PrimaryColor::Blue);

    if (box.Count() == 1)
        return { { box.Clone() }, {} };

    std::unordered_map<int32_t, uint64_t> LookAheadCount;

    auto [PartialCount, Total] = ComputePartialCounts(histogram, box, CutColor);

    for (auto & [i, t] : PartialCount)
        LookAheadCount[i] = Total - t;

    int32_t x1;
    int32_t x2;

    if (CutColor == PrimaryColor::Red)
    {
        x1 = box.r1;
        x2 = box.r2;
    }
    else
    if (CutColor == PrimaryColor::Green)
    {
        x1 = box.g1;
        x2 = box.g2;
    }
    else
    {
        x1 = box.b1;
        x2 = box.b2;
    }

    for (int32_t i = x1; i < x2 + 1; ++i)
    {
        if (PartialCount[i] > Total / 2)
        {
            VBox Box1 = box.Clone();
            VBox Box2 = box.Clone();

            int32_t Left = i - x1;
            int32_t Right = x2 - i;

            int32_t d = (Left <= Right) ? (std::min)(x2 - 1, (int32_t) (i + Right / 2.0)) : (std::max)(x1, (int32_t) (i - 1 - Left / 2.0));

            while (!(PartialCount.count(d) > 0 && PartialCount[d] > 0))
                d += 1;

            uint64_t Count = LookAheadCount[d];

            while ((Count == 0) && (PartialCount.count(d - 1) > 0) && (PartialCount[d - 1] > 0))
                d -= 1;

            if (CutColor == PrimaryColor::Red)
            {
                Box1.r2 = d;
                Box2.r1 = Box1.r2 + 1;
            }
            else
            if (CutColor == PrimaryColor::Green)
            {
                Box1.g2 = d;
                Box2.g1 = Box1.g2 + 1;
            }
            else
            {
                Box1.b2 = d;
                Box2.b1 = Box1.b2 + 1;
            }

            return { Box1, Box2 };
        }
    }

    return { {}, {} };
}

/// <summary>
/// Computes the partials sums.
/// </summary>
std::tuple<std::unordered_map<int32_t, uint64_t>, uint64_t> ComputePartialCounts(const std::vector<int32_t> & histogram, const VBox & vbox, PrimaryColor color) noexcept
{
    std::unordered_map<int32_t, uint64_t> PartialCount;
    uint64_t Total = 0;

    switch (color)
    {
        case PrimaryColor::Red:
        {
            for (int32_t i = vbox.r1; i < vbox.r2 + 1; ++i)
            {
                uint64_t Sum = 0;

                for (int32_t j = vbox.g1; j < vbox.g2 + 1; ++j)
                {
                    for (int32_t k = vbox.b1; k < vbox.b2 + 1; ++k)
                        Sum += histogram[GetColorIndex(i, j, k)];
                }

                Total += Sum;
                PartialCount[i] = Total;
            }

            break;
        }

        case PrimaryColor::Green:
        {
            for (int32_t i = vbox.g1; i < vbox.g2 + 1; ++i)
            {
                uint64_t Sum = 0;

                for (int32_t j = vbox.r1; j < vbox.r2 + 1; ++j)
                {
                    for (int32_t k = vbox.b1; k < vbox.b2 + 1; ++k)
                        Sum += histogram[GetColorIndex(j, i, k)];
                }

                Total += Sum;
                PartialCount[i] = Total;
            }

            break;
        }

        case PrimaryColor::Blue:
        default:
        {
            for (int32_t i = vbox.b1; i < vbox.b2 + 1; ++i)
            {
                uint64_t Sum = 0;

                for (int32_t j = vbox.r1; j < vbox.r2 + 1; ++j)
                {
                    for (int32_t k = vbox.g1; k < vbox.g2 + 1; ++k)
                        Sum += histogram[GetColorIndex(j, k, i)];
                }

                Total += Sum;
                PartialCount[i] = Total;
            }
        }
    }

    return { PartialCount, Total };
}

inline bool CompareCount(VBox & a, VBox & b) noexcept
{
    return a.Count() < b.Count();
}

inline bool CompareProduct(VBox & a, VBox & b) noexcept
{
    return ((uint64_t) a.Count() * (uint64_t) a.Volume()) < ((uint64_t) b.Count() * (uint64_t) b.Volume());
}

#pragma endregion
}
