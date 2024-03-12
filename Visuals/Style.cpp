
/** $VER: Style.cpp (2024.03.12) P. Stuer **/

#include "Style.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "Gradients.h"

#include "Support.h"
#include "Log.h"

#include <cassert>
#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes an instance.
/// </summary>
Style::Style(const Style & other)
{
    operator=(other);
}

/// <summary>
/// Implements the = operator.
/// </summary>
Style & Style::operator=(const Style & other)
{
    _Flags = other._Flags;

    _ColorSource = other._ColorSource;
    _ColorIndex = other._ColorIndex;
    _ColorScheme = other._ColorScheme;

    _CustomColor = other._CustomColor;
    _CustomGradientStops = other._CustomGradientStops;

    _Opacity = other._Opacity;
    _Thickness = other._Thickness;

    _FontName = other._FontName;
    _FontSize = other._FontSize;

    _CurrentColor = other._CurrentColor;
    _CurrentGradientStops = other._CurrentGradientStops;

    return *this;
}

/// <summary>
/// Initializes an instance.
/// </summary>
Style::Style(uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize)
{
    _Flags = flags;

    _ColorSource = colorSource;
    _ColorIndex = colorIndex;
    _ColorScheme = colorScheme;

    _CustomColor = customColor;
    _CustomGradientStops = customGradientStops;

    _Opacity = opacity;
    _Thickness = thickness;

    _FontName = fontName;
    _FontSize = fontSize;

    _CurrentColor         = customColor;
    _CurrentGradientStops = (_ColorScheme == ColorScheme::Custom) ? _CustomGradientStops : GetGradientStops(_ColorScheme);
}

/// <summary>
/// Updates the current color based on the color source.
/// </summary>
void Style::UpdateCurrentColor(const D2D1_COLOR_F & dominantColor, const std::vector<D2D1_COLOR_F> & userInterfaceColors)
{
    switch (_ColorSource)
    {
        case ColorSource::None:
        {
            _CurrentColor = D2D1::ColorF(0, 0.f);
            break;
        }

        case ColorSource::Solid:
        {
            _CurrentColor = _CustomColor;
            break;
        }

        case ColorSource::DominantColor:
        {
            _CurrentColor = dominantColor;
            break;
        }

        case ColorSource::Gradient:
        {
            _CurrentColor = D2D1::ColorF(0, 0.f);
            break;
        }

        case ColorSource::Windows:
        {
            _CurrentColor = GetWindowsColor(_ColorIndex);
            break;
        }

        case ColorSource::UserInterface:
        {
            _CurrentColor = userInterfaceColors[Clamp((size_t) _ColorIndex, (size_t) 0, userInterfaceColors.size())];
            break;
        }
    }
}

/// <summary>
/// Gets the selected Windows color.
/// </summary>
D2D1_COLOR_F Style::GetWindowsColor(uint32_t index) noexcept
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

    return D2D1::ColorF(::GetSysColor(ColorIndex[Clamp(index, 0U, (uint32_t) _countof(ColorIndex) - 1)]));
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Style::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size) noexcept
{
    HRESULT hr = S_OK;

    if (_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(_CurrentColor, (ID2D1SolidColorBrush **) &_Brush);
    else
    {
        if ((_Flags & (Style::HorizontalGradient | Style::AmplitudeBasedColor)) == (Style::HorizontalGradient | Style::AmplitudeBasedColor))
        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0), (ID2D1SolidColorBrush **) &_Brush); // The color of the brush will be set during rendering.

            if (SUCCEEDED(hr))
                hr = CreateAmplitudeMap(_CurrentGradientStops, _AmplitudeMap);
        }
        else
            hr = _Direct2D.CreateGradientBrush(renderTarget, _CurrentGradientStops, size, _Flags & Style::HorizontalGradient, (ID2D1LinearGradientBrush **) &_Brush);
    }

    if (_Brush)
        _Brush->SetOpacity(_Opacity);

    if ((_Flags & SupportsFont) && (_TextFormat == nullptr))
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

        hr = _DirectWrite.Factory->CreateTextFormat(_FontName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Style::ReleaseDeviceSpecificResources()
{
    _TextFormat.Release();
    _Brush.Release();
}

/// <summary>
/// Sets the color of a solid color brush from the gradient colors based on a value between 0. and 1..
/// </summary>
HRESULT Style::SetBrushColor(double value) noexcept
{
    if (_AmplitudeMap.size() == 0)
        return;

    ID2D1SolidColorBrush * ColorBrush = nullptr;

    HRESULT hr = _Brush->QueryInterface(IID_PPV_ARGS(&ColorBrush));

    if (!SUCCEEDED(hr))
        return;

    size_t Index = Map(value, 0., 1., (size_t) 0, _AmplitudeMap.size() - 1);

    D2D1_COLOR_F Color = _AmplitudeMap[Index];

    ColorBrush->SetColor(Color);

    ColorBrush->Release();

    return hr;
}

/// <summary>
/// Creates a color table to map the amplitudes to.
/// </summary>
HRESULT Style::CreateAmplitudeMap(const GradientStops & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept
{
    if (gradientStops.size() == 0)
        return E_FAIL;

    // Create a list of colors based on the gradient colors to map the amplitude to.
    colors.clear();

    D2D1_COLOR_F Color1 = gradientStops[0].color;
    FLOAT Position1 = 0.f;

    for (size_t i = 0; i < gradientStops.size(); ++i)
    {
        D2D1_COLOR_F Color2 = gradientStops[i].color;
        FLOAT Position2 = gradientStops[i].position;

        FLOAT n = (FLOAT) ((uint32_t) (Position2 * 100.f) - (uint32_t) (Position1 * 100.f));

        for (FLOAT j = 0; j < n; ++j)
        {
            D2D1_COLOR_F Color = Color1;

            const FLOAT Factor = j / n;

            Color.r += (Color2.r - Color1.r) * Factor;
            Color.g += (Color2.g - Color1.g) * Factor;
            Color.b += (Color2.b - Color1.b) * Factor;
            Color.a += (Color2.a - Color1.a) * Factor;

            colors.push_back(Color);
        }

        Color1 = Color2;
        Position1 = Position2;
    }

    // The color in the lowest position should map to the highest amplitude values.
    std::reverse(colors.begin(), colors.end());

    return S_OK;
}
