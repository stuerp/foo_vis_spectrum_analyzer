
/** $VER: Style.cpp (2024.05.03) P. Stuer **/

#include "framework.h"
#include "Style.h"

#include "Direct2D.h"
#include "Gradients.h"
#include "Support.h"

#include "Log.h"

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

    _TextFormat = other._TextFormat;
    _Width = other._Width;
    _Height = other._Height;

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

    _Width  = 0.f;
    _Height = 0.f;
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
            if (userInterfaceColors.size() > 0)
                _CurrentColor = userInterfaceColors[std::clamp((size_t) _ColorIndex, (size_t) 0, userInterfaceColors.size() - 1)];
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

    return D2D1::ColorF(::GetSysColor(ColorIndex[std::clamp(index, 0U, (uint32_t) _countof(ColorIndex) - 1)]));
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Style::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const std::wstring & text, const D2D1_SIZE_F & size) noexcept
{
    HRESULT hr = S_OK;

    if (_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(_CurrentColor, (ID2D1SolidColorBrush **) &_Brush);
    else
    {
        if (IsSet(_Flags, (uint64_t) Style::AmplitudeBasedColor))
        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0), (ID2D1SolidColorBrush **) &_Brush); // The color of the brush will be set during rendering.

            if (SUCCEEDED(hr))
                hr = CreateAmplitudeMap(_ColorScheme, _CurrentGradientStops, _AmplitudeMap);
        }
        else
            hr = _Direct2D.CreateGradientBrush(renderTarget, _CurrentGradientStops, size, _Flags & Style::HorizontalGradient, (ID2D1LinearGradientBrush **) &_Brush);
    }

    if (_Brush)
        _Brush->SetOpacity(_Opacity);

    if ((_Flags & SupportsFont) && (_TextFormat == nullptr) && !_FontName.empty())
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

        hr = _DirectWrite.CreateTextFormat(_FontName, FontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, _TextFormat);

        if (SUCCEEDED(hr))
            MeasureText(text);
    }

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Style::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset) noexcept
{
    HRESULT hr = S_OK;

    if (_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(_CurrentColor, (ID2D1SolidColorBrush **) &_Brush);
    else
    {
        if (IsSet(_Flags, (uint64_t) Style::AmplitudeBasedColor))
        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0), (ID2D1SolidColorBrush **) &_Brush); // The color of the brush will be set during rendering.

            if (SUCCEEDED(hr))
                hr = CreateAmplitudeMap(_ColorScheme, _CurrentGradientStops, _AmplitudeMap);
        }
        else
            hr = _Direct2D.CreateRadialGradientBrush(renderTarget, _CurrentGradientStops, center, offset, rx, ry, rOffset, (ID2D1RadialGradientBrush **) &_Brush);
    }

    if (_Brush)
        _Brush->SetOpacity(_Opacity);

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
/// Selects the color of a solid color brush from the amplitude map colors based on a value between 0. and 1..
/// </summary>
HRESULT Style::SetBrushColor(double value) noexcept
{
    if (_AmplitudeMap.size() == 0)
        return E_FAIL;

    ID2D1SolidColorBrush * ColorBrush = nullptr;

    HRESULT hr = _Brush->QueryInterface(IID_PPV_ARGS(&ColorBrush));

    if (!SUCCEEDED(hr))
        return hr;

    size_t Index = Map(value, 0., 1., (size_t) 0, _AmplitudeMap.size() - 1);

    D2D1_COLOR_F Color = _AmplitudeMap[Index];

    ColorBrush->SetColor(Color);

    ColorBrush->Release();

    return hr;
}

/// <summary>
/// Creates a color table to map the amplitudes to.
/// </summary>
/// <remarks>Assumes a sane gradient collection with position running from 0.f to 1.f in ascending order.
HRESULT Style::CreateAmplitudeMap(ColorScheme colorScheme, const GradientStops & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept
{
    if (gradientStops.size() == 0)
        return E_FAIL;

    const size_t Steps = 100; // Results in a 101 entry table to be mapped to amplitudes between 0. and 1.

    colors.clear();
    colors.reserve(((gradientStops.size() - 1) * Steps) + 1);

    if (colorScheme != ColorScheme::SoX)
    {
        // Linear interpolation of the colors.

        D2D1_COLOR_F Color1 = gradientStops[0].color;
        FLOAT Position1 = gradientStops[0].position;

        // Add the run-in colors.
        uint32_t n = (uint32_t) (Position1 * (FLOAT) Steps);

        for (uint32_t j = 0; j <= n; ++j)
            colors.push_back(Color1);

        // Add the gradient colors.
        for (size_t i = 1; i < gradientStops.size(); ++i)
        {
            D2D1_COLOR_F Color2 = gradientStops[i].color;
            FLOAT Position2 = gradientStops[i].position;

            const D2D1_COLOR_F Delta = { Color2.r - Color1.r, Color2.g - Color1.g, Color2.b - Color1.b, Color2.a - Color1.a };

            n = (uint32_t) (Position2 * (FLOAT) Steps) - (uint32_t) (Position1 * (FLOAT) Steps);

            for (uint32_t j = 1; j < n; ++j)
            {
                const FLOAT Factor = (FLOAT) j / (FLOAT) n;
                const D2D1_COLOR_F Color =
                {
                    Color1.r + (Delta.r * Factor),
                    Color1.g + (Delta.g * Factor),
                    Color1.b + (Delta.b * Factor),
                    Color1.a + (Delta.a * Factor)
                };

                colors.push_back(Color);
            }

            Color1 = Color2;
            Position1 = Position2;
        }

        // Add the run-out colors.
        for (uint32_t j = (uint32_t) (Position1 * (FLOAT) Steps); j <= Steps; ++j)
            colors.push_back(Color1);

        // The color in the lowest position should map to the highest amplitude values.
        std::reverse(colors.begin(), colors.end());
    }
    else
    {
        double r = 0.;
        double g = 0.;
        double b = 0.;

        for (double amplitude = 0.; amplitude <= 1.; amplitude += 1. / Steps)
        {
            if (amplitude >= 0.13 && amplitude < 0.73)
                r = ::sin((amplitude - 0.13) / 0.60 * M_PI_2);
            else
            if (amplitude >= 0.73)
                r = 1.0;

            if (amplitude >= 0.6 && amplitude < 0.91)
                g = ::sin((amplitude - 0.6) / 0.31 * M_PI_2);
            else
            if (amplitude >= 0.91)
                g = 1.0;

            if (amplitude < 0.60)
                b = 0.5 * ::sin(amplitude / 0.6 * M_PI);
            else
            if (amplitude >= 0.78)
                b = (amplitude - 0.78) / 0.22;

            colors.push_back(D2D1::ColorF((FLOAT) r, (FLOAT) g, (FLOAT) b, 1.f));
        }
    }

    return S_OK;
}

/// <summary>
/// Updates the text width and height.
/// </summary>
HRESULT Style::MeasureText(const std::wstring & text) noexcept
{
    if (_TextFormat == nullptr)
        return E_FAIL;

    return _DirectWrite.GetTextMetrics(_TextFormat, text.c_str(), _Width, _Height);
}
