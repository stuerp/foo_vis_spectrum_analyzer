
/** $VER: Style.cpp (2026.09.30) P. Stuer - Represents the style of a visual element. **/

#include "pch.h"
#include "Style.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "Gradient.h"
#include "Support.h"
#include "State.h"

#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes an instance.
/// </summary>
style_t::style_t(const style_t & other) noexcept
{
    operator=(other);
}

/// <summary>
/// Implements the = operator.
/// </summary>
style_t & style_t::operator=(const style_t & other) noexcept
{
    _Name                 = other._Name;
    _UsedBy               = other._UsedBy;

    _Flags                = other._Flags;

    _ColorSource          = other._ColorSource;
    _ColorIndex           = other._ColorIndex;
    _ColorScheme          = other._ColorScheme;

    _CustomColor          = other._CustomColor;
    _CustomGradient       = other._CustomGradient;

    _Opacity              = other._Opacity;
    _Thickness            = other._Thickness;

    _FontName             = other._FontName;
    _FontSize             = other._FontSize;

    // Non-serialized
    _CurrentColor         = other._CurrentColor;
    _CurrentGradientStops = other._CurrentGradientStops;

    _Width                = other._Width;
    _Height               = other._Height;

    DeleteDeviceSpecificResources();

    return *this;
}

/// <summary>
/// Initializes an instance.
/// </summary>
style_t::style_t(const std::wstring & name, VisualizationTypes usedBy, style_t::Features flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, std::vector<D2D1_GRADIENT_STOP> customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize) noexcept
{
    _Name                 = name;
    _UsedBy               = usedBy;

    _Flags                = flags;

    _ColorSource          = colorSource;
    _ColorIndex           = colorIndex;
    _ColorScheme          = colorScheme;

    _CustomColor          = customColor;
    _CustomGradient       = gradient_t::ConvertFormat(customGradientStops);

    _Opacity              = opacity;
    _Thickness            = thickness;

    _FontName             = fontName;
    _FontSize             = fontSize;

    // Non-serialized
    _CurrentColor         = customColor;
    _CurrentGradientStops = customGradientStops;

    _Width                = 0.f;
    _Height               = 0.f;
}

/// <summary>
/// Sets the current color based on the color source.
/// </summary>
void style_t::SetColor(const state_t * state) noexcept
{
    switch (_ColorSource)
    {
        case ColorSource::None:
        {
            _CurrentColor = D2D1::ColorF(0, 0.f); // Transparent

            return;
        }

        case ColorSource::Solid:
        {
            _CurrentColor = _CustomColor;

            return;
        }

        case ColorSource::DominantColor:
        {
            _CurrentColor = state->_ArtworkDominantColor;

            return;
        }

        case ColorSource::Gradient:
        {
            _CurrentColor = D2D1::ColorF(0, 0.f); // Transparent

            if (_ColorScheme == ColorScheme::Artwork)
            {
                _CurrentGradientStops = state->_ArtworkGradientStops;

                return;
            }

            if (_ColorScheme == ColorScheme::Custom)
            {
                _CurrentGradientStops = gradient_t::ConvertFormat(_CustomGradient);

                return;
            }

            _CurrentGradientStops = gradient_t::GetBuiltIn(_ColorScheme);

            return;
        }

        case ColorSource::Windows:
        {
            _CurrentColor = GetWindowsColor(_ColorIndex);

            return;
        }

        case ColorSource::UserInterface:
        {
            if (state->_UserInterfaceColors.empty())
            {
                _CurrentColor = D2D1_COLOR_F(D2D1::ColorF::Red);

                return;
            }

            const auto Index = std::clamp((size_t) _ColorIndex, (size_t) 0, state->_UserInterfaceColors.size() - 1);

            _CurrentColor = state->_UserInterfaceColors[Index];

            return;
        }
    }
}

/// <summary>
/// Gets the selected Windows color.
/// </summary>
D2D1_COLOR_F style_t::GetWindowsColor(uint32_t index) noexcept
{
    static const int ColorIndex[] =
    {
        COLOR_WINDOW,           // Window Background
        COLOR_WINDOWTEXT,       // Window Text
        COLOR_BTNFACE,          // Button Background
        COLOR_BTNTEXT,          // Button Text
        COLOR_HIGHLIGHT,        // Highlight Background
        COLOR_HIGHLIGHTTEXT,    // Highlight Text
        COLOR_GRAYTEXT,         // Gray Text
        COLOR_HOTLIGHT,         // Hot Light
    };

    return D2D1::ColorF(::GetSysColor(ColorIndex[std::clamp(index, 0u, (uint32_t) _countof(ColorIndex) - 1)]));
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT style_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const std::wstring & text, FLOAT scaleFactor) noexcept
{
    HRESULT hr = S_OK;

    ComPtr<ID2D1SolidColorBrush> SolidColorBrush;

    // Create the DirectX brush.
    if (_ColorSource != ColorSource::Gradient)
    {
        hr = deviceContext->CreateSolidColorBrush(_CurrentColor, SolidColorBrush.GetAddressOf());

        if (FAILED(hr))
            return hr;

        _Brush = SolidColorBrush;
    }
    else
    {
        if (Has(style_t::Features::HorizontalGradient | style_t::Features::AmplitudeBasedColor))
        {
            hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(0, 0.f), SolidColorBrush.GetAddressOf()); // The color of the brush will be set during rendering.

            if (FAILED(hr))
                return hr;

            _Brush = SolidColorBrush;

            hr = CreateAmplitudeMap(_ColorScheme, _CurrentGradientStops, _AmplitudeMap);

            if (FAILED(hr))
                return hr;
        }
        else
        {
            ComPtr<ID2D1LinearGradientBrush> LinearGradientBrush;

            hr = Direct2D::CreateGradientBrush(deviceContext, _CurrentGradientStops, size, Has(style_t::Features::HorizontalGradient), LinearGradientBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;

            _Brush = LinearGradientBrush;
        }
    }

    if (_Brush != nullptr)
        _Brush->SetOpacity(_Opacity);

    // Create the DirectX text format.
    if (Has(style_t::Features::SupportsFont) && (_TextFormat == nullptr) && !_FontName.empty())
    {
        const FLOAT FontSize = ToDIPs(_FontSize) / scaleFactor; // In DIPs

        hr = DirectWrite::CreateTextFormat(_FontName, FontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, _TextFormat.GetAddressOf());

        if (FAILED(hr))
            return hr;

        MeasureText(text);
    }

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device. Specialized version for radial visualizations.
/// </summary>
HRESULT style_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset) noexcept
{
    HRESULT hr = S_OK;

    ComPtr<ID2D1SolidColorBrush> SolidColorBrush;

    // Create the DirectX brush.
    if (_ColorSource != ColorSource::Gradient)
    {
        hr = deviceContext->CreateSolidColorBrush(_CurrentColor, SolidColorBrush.GetAddressOf());

        if (FAILED(hr))
            return hr;

        _Brush = SolidColorBrush;
    }
    else
    {
        if (Has(style_t::Features::HorizontalGradient | style_t::Features::AmplitudeBasedColor))
        {
            hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(0, 0.f), SolidColorBrush.GetAddressOf()); // The color of the brush will be set during rendering.

            if (FAILED(hr))
                return hr;

            _Brush = SolidColorBrush;

            hr = CreateAmplitudeMap(_ColorScheme, _CurrentGradientStops, _AmplitudeMap);

            if (FAILED(hr))
                return hr;
        }
        else
        {
            ComPtr<ID2D1RadialGradientBrush> RadialGradientBrush;

            hr = Direct2D::CreateRadialGradientBrush(deviceContext, _CurrentGradientStops, center, offset, rx, ry, rOffset, RadialGradientBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;

            _Brush = RadialGradientBrush;
        }
    }

    if (_Brush != nullptr)
        _Brush->SetOpacity(_Opacity);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void style_t::DeleteDeviceSpecificResources() noexcept
{
    _TextFormat.Reset();
    _Brush.Reset();
}

/// <summary>
/// Selects the color of a solid color brush from the amplitude map colors based on a value between 0. and 1..
/// </summary>
HRESULT style_t::SetBrushColor(double value) noexcept
{
    if (_AmplitudeMap.empty())
        return E_FAIL;

    ComPtr<ID2D1SolidColorBrush> SolidColorBrush;

    HRESULT hr = _Brush.As(&SolidColorBrush);

    if (FAILED(hr))
        return hr;

    const size_t Index = msc::Map(value, 0., 1., (size_t) 0, _AmplitudeMap.size() - 1);

    SolidColorBrush->SetColor(_AmplitudeMap[Index]);

    return hr;
}

/// <summary>
/// Creates a color table to map the amplitudes to.
/// </summary>
/// <remarks>Assumes a sane gradient collection with position running from 0 to 1 in ascending order.</remarks>
HRESULT style_t::CreateAmplitudeMap(ColorScheme colorScheme, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept
{
    if (gradientStops.empty())
        return E_INVALIDARG;

    try
    {
        constexpr std::size_t StepCount  = 100;
        constexpr std::size_t ColorCount = StepCount + 1;

        colors.resize(ColorCount);

        if (colorScheme != ColorScheme::SoX)
        {
            // Work on a sorted copy because stops can temporarily be out of order while the user edits the gradient.
            auto GradientStops = gradientStops;

            std::stable_sort(GradientStops.begin(), GradientStops.end(), [](const D2D1_GRADIENT_STOP & lhs, const D2D1_GRADIENT_STOP & rhs) noexcept
            {
                return lhs.position < rhs.position;
            });

            size_t NextStop = 1;

            for (size_t Step = 0; Step <= StepCount; ++Step)
            {
                const FLOAT Position = (FLOAT) Step / (FLOAT) StepCount;

                const size_t Index = StepCount - Step;

                while ((NextStop < GradientStops.size()) && (GradientStops[NextStop].position < Position))
                {
                    ++NextStop;
                }

                if (Position <= GradientStops.front().position)
                {
                    colors[Index] = GradientStops.front().color;
                    continue;
                }

                if (NextStop == GradientStops.size())
                {
                    colors[Index] = GradientStops.back().color;
                    continue;
                }

                const D2D1_GRADIENT_STOP & l = GradientStops[NextStop - 1];
                const D2D1_GRADIENT_STOP & r = GradientStops[NextStop];

                const FLOAT Interval = r.position - l.position;

                if (Interval <= 0.f)
                {
                    colors[Index] = r.color;
                    continue;
                }

                const FLOAT Factor = std::clamp((Position - l.position) / Interval, 0.f, 1.f);

                colors[Index] =
                {
                    std::lerp(l.color.r, r.color.r, Factor),
                    std::lerp(l.color.g, r.color.g, Factor),
                    std::lerp(l.color.b, r.color.b, Factor),
                    std::lerp(l.color.a, r.color.a, Factor)
                };
            }

            return S_OK;
        }
        else
        {
            // Converted from the SoX source code.
            constexpr double pi2 = std::numbers::pi / 2.;

            for (size_t Step = 0; Step <= StepCount; ++Step)
            {
                const auto Amplitude = (double) Step / (double) StepCount;

                FLOAT r;

                if (Amplitude < 0.13)
                    r = 0.f;
                else
                if (Amplitude < 0.73)
                    r = (FLOAT) std::sin((Amplitude - 0.13) / 0.60 * pi2);
                else
                    r = 1.f;

                FLOAT g;

                if (Amplitude < 0.60)
                    g = 0.f;
                else
                if (Amplitude < 0.91)
                    g = (FLOAT) std::sin((Amplitude - 0.60) / 0.31 * pi2);
                else
                    g = 1.f;

                FLOAT b;

                if (Amplitude < 0.60)
                    b = (FLOAT) (0.5 * std::sin(Amplitude / 0.60 * std::numbers::pi));
                else
                if (Amplitude < 0.78)
                    b = 0.f;
                else
                    b = (FLOAT) ((Amplitude - 0.78) / 0.22);

                colors[Step] = D2D1::ColorF(r, g, b, 1.f);
            }
        }

        return S_OK;
    }
    catch (const std::bad_alloc &)
    {
        colors.clear();

        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        colors.clear();

        return E_FAIL;
    }
}

/// <summary>
/// Updates the text width and height to the actual width and height of the text.
/// </summary>
HRESULT style_t::MeasureText(const std::wstring & text) noexcept
{
    if (_TextFormat == nullptr)
        return E_FAIL;

    return DirectWrite::GetTextMetrics(_TextFormat.Get(), text.c_str(), _Width, _Height);
}
