
/** $VER: RotatingTunnel.cpp (2026.11.04) Based on https://lodev.org/cgtutor/tunnel.html */

#include <pch.h>

#include "RotatingTunnel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace RotatingTunnel
{
    using namespace Field;

    /// <summary>
    /// Returns a description of the current visualization settings.
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Rotating Tunnel, {}, {}, {} quality, {} speed", GetTexturePatternName(_TexturePattern), _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// Creates a resolution-independent tunnel raster and its doubled lookup tables.
    /// </summary>
    void effect_t::Reset() noexcept
    {
        constexpr uint32_t MaximumLongSide[] = { 400, 640, 960 };
        const uint32_t LongSide = MaximumLongSide[std::min<size_t>((size_t) _Quality, _countof(MaximumLongSide) - 1)];
        const double AspectRatio = (double) std::max(_Width, 1.f) / (double) std::max(_Height, 1.f);

        if (AspectRatio >= 1.)
        {
            _RasterWidth = LongSide;
            _RasterHeight = std::max(64u, (uint32_t) std::lround((double) LongSide / AspectRatio));
        }
        else
        {
            _RasterHeight = LongSide;
            _RasterWidth = std::max(64u, (uint32_t) std::lround((double) LongSide * AspectRatio));
        }

        _Stride = _RasterWidth * BytesPerPixel;

        _Pixels.assign((size_t) _RasterWidth * _RasterHeight, PackRGB(0., 0., 0.));

        _Texture    .resize((size_t) TextureWidth * TextureHeight);
        _NextTexture.resize((size_t) TextureWidth * TextureHeight);

        _TableWidth  = _RasterWidth  * 2;
        _TableHeight = _RasterHeight * 2;

        _DistanceTable.resize((size_t) _TableWidth * _TableHeight);
        _AngleTable   .resize((size_t) _TableWidth * _TableHeight);

        _TexturePattern = (TexturePattern) (_ThemeIndex % (size_t) TexturePattern::Count);
        _NextTexturePattern = _TexturePattern;
        _TextureTransition = 1.;

        GenerateTexture(_TexturePattern, _Texture);
        _NextTexture = _Texture;
        GenerateTables();

        _Rotation  = 0.;
        _Travel    = 0.;
        _LookPhase = 0.;

        _BackBuffer.Reset();
    }

    /// <summary>
    /// Advances and renders the audio-reactive rotating tunnel.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        constexpr double DefaultFrameTime = 1. / 60.;

        if (!std::isfinite(deltaTime) || deltaTime <= 0. || deltaTime > .1)
            deltaTime = DefaultFrameTime;

        // Update the raster.
        {
            Process(chunk, deltaTime);

            const double SpeedFactor = [&]() noexcept
            {
                switch (_Speed)
                {
                    case Speed::Slow:
                        return 0.55;

                    default:
                    case Speed::Normal:
                        return 1.0;

                    case Speed::Fast:
                        return 1.75;
                }
            }();

            const double AudioDrive  = .75 + (.70 * _Loudness) + (.35 * _BassLevel);

            _AnimationTime += deltaTime * SpeedFactor;
            _Travel        += deltaTime * SpeedFactor * AudioDrive * 58.;
            _Rotation      += deltaTime * SpeedFactor * (35.00 + 42.00 * _TrebleLevel);
            _LookPhase     += deltaTime * SpeedFactor * ( 0.65 +  0.35 * _MidLevel);

            {
                if (_AnimationTime > _NextThemeTransitionTime)
                {
                    _NextThemeIndex = (_NextThemeIndex + 1) % Field::theme_t::BuiltInCount;

                    _NextThemeTransitionTime = _AnimationTime + ThemeTransitionInterval;
                }

                if (_ThemeIndex != _NextThemeIndex)
                {
                    _FromTheme = _Theme;
                    _ToTheme   = Field::theme_t::BuiltIn(_NextThemeIndex);

                    _ThemeIndex = _NextThemeIndex;
                    _TransitionTime = 0.;

                    _NextTexturePattern = (TexturePattern) (_ThemeIndex % (size_t) TexturePattern::Count);

                    if (_NextTexturePattern != _TexturePattern)
                    {
                        GenerateTexture(_NextTexturePattern, _NextTexture);
                        _TextureTransition = 0.;
                    }
                }

                _TransitionTime = std::min(_TransitionTime + deltaTime / .85, 1.);
                _TextureTransition = std::min(_TextureTransition + deltaTime / .85, 1.);

                if (_TransitionTime < 1.)
                    _Theme = Field::theme_t::Blend(_FromTheme, _ToTheme, _TransitionTime);

                if (_TextureTransition >= 1. && _TexturePattern != _NextTexturePattern)
                {
                    _Texture.swap(_NextTexture);
                    _TexturePattern = _NextTexturePattern;
                }
            }

            DrawTunnel();
        }

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _Stride);

            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);
        }
    }

    /// <summary>
    /// Generates the requested 256 by 256 XOR texture in memory.
    /// </summary>
    const wchar_t * effect_t::GetTexturePatternName(TexturePattern pattern) noexcept
    {
        switch (pattern)
        {
            default:
            case TexturePattern::Count:
            case TexturePattern::Xor:             return L"XOR";
            case TexturePattern::Checkerboard:    return L"checkerboard";
            case TexturePattern::DiagonalWeave:   return L"diagonal weave";
            case TexturePattern::SquareTiles:     return L"square tiles";
            case TexturePattern::Chevrons:        return L"chevrons";
            case TexturePattern::CircuitGrid:     return L"circuit grid";
            case TexturePattern::SteppedDiamonds: return L"stepped diamonds";
        }
    }

    /// <summary>
    /// Generates a crisp periodic geometric texture. No random or organic components are used.
    /// </summary>
    void effect_t::GenerateTexture(TexturePattern pattern, std::vector<uint8_t> & texture) noexcept
    {
        texture.resize((size_t) TextureWidth * TextureHeight);

        for (uint32_t Y = 0; Y < TextureHeight; ++Y)
        {
            const size_t Row = (size_t) Y * TextureWidth;

            for (uint32_t X = 0; X < TextureWidth; ++X)
            {
                uint32_t Value = 0;

                switch (pattern)
                {
                    default:
                    case TexturePattern::Count:
                    case TexturePattern::Xor:
                    {
                        Value = (X * 256u / TextureWidth) ^ (Y * 256u / TextureHeight);
                        break;
                    }

                    case TexturePattern::Checkerboard:
                    {
                        const uint32_t Large = (((X >> 5) ^ (Y >> 5)) & 1u) * 144u;
                        const uint32_t Small = (((X >> 3) ^ (Y >> 3)) & 1u) * 56u;
                        const uint32_t Grid = ((X & 31u) < 2u || (Y & 31u) < 2u) ? 55u : 0u;
                        Value = Large + Small + Grid;
                        break;
                    }

                    case TexturePattern::DiagonalWeave:
                    {
                        const uint32_t Forward = (X + Y) & 31u;
                        const uint32_t Backward = (X - Y) & 31u;
                        const bool ForwardBand = Forward < 7u;
                        const bool BackwardBand = Backward < 7u;
                        Value = ForwardBand && BackwardBand ? 255u : ForwardBand ? 190u : BackwardBand ? 110u : 24u;
                        break;
                    }

                    case TexturePattern::SquareTiles:
                    {
                        const uint32_t LocalX = X & 31u;
                        const uint32_t LocalY = Y & 31u;
                        const uint32_t Edge = std::min(std::min(LocalX, 31u - LocalX), std::min(LocalY, 31u - LocalY));
                        const uint32_t Tile = (((X >> 5) + 2u * (Y >> 5)) & 3u) * 36u;
                        Value = Edge < 2u ? 245u : std::min(235u, 48u + Tile + Edge * 4u);
                        break;
                    }

                    case TexturePattern::Chevrons:
                    {
                        const uint32_t FoldY = (Y & 63u) < 32u ? (Y & 63u) : 63u - (Y & 63u);
                        const uint32_t Chevron = ((X + FoldY) >> 3) & 7u;
                        const uint32_t Divider = ((X + Y) & 63u) < 3u ? 64u : 0u;
                        Value = 28u + Chevron * 27u + Divider;
                        break;
                    }

                    case TexturePattern::CircuitGrid:
                    {
                        const bool Horizontal = (Y & 31u) < 3u;
                        const bool Vertical = (X & 31u) < 3u;
                        const bool Pad = (X & 31u) >= 12u && (X & 31u) < 20u && (Y & 31u) >= 12u && (Y & 31u) < 20u;
                        const uint32_t Cell = (((X >> 5) * 5u + (Y >> 5) * 3u) & 7u) * 15u;
                        Value = Pad ? 255u : Horizontal || Vertical ? 180u + Cell / 2u : 20u + Cell;
                        break;
                    }

                    case TexturePattern::SteppedDiamonds:
                    {
                        const int32_t DX = (int32_t) (X & 31u) - 16;
                        const int32_t DY = (int32_t) (Y & 31u) - 16;
                        const uint32_t Diamond = (uint32_t) (std::abs(DX) + std::abs(DY));
                        const uint32_t Step = (Diamond >> 2) & 7u;
                        const uint32_t Alternate = (((X >> 5) ^ (Y >> 5)) & 1u) * 48u;
                        Value = std::min(255u, 30u + Step * 25u + Alternate);
                        break;
                    }
                }

                texture[Row + X] = (uint8_t) std::min(Value, 255u);
            }
        }
    }

    /// <summary>
    /// Precalculates polar texture coordinates for a viewport twice the visible size.
    /// </summary>
    void effect_t::GenerateTables() noexcept
    {
        const double CenterX = (double) _RasterWidth;
        const double CenterY = (double) _RasterHeight;

        constexpr double Ratio = 32.;

        for (uint32_t Y = 0; Y < _TableHeight; ++Y)
        {
            const double DY = (double) Y - CenterY;
            const size_t Row = (size_t) Y * _TableWidth;

            for (uint32_t X = 0; X < _TableWidth; ++X)
            {
                const double DX = (double) X - CenterX;
                const double Radius = std::sqrt(DX * DX + DY * DY);

                const size_t I = Row + X;

                _DistanceTable[I] = (uint16_t) ((Radius > .5) ? (uint32_t) (Ratio * TextureHeight / Radius) & TextureMaskY : 0u);
                _AngleTable[I]    = (uint16_t) ((int32_t) std::lround((double) TextureWidth * std::atan2(DY, DX) / (2. * Pi)) & TextureMaskX);
            }
        }
    }

    /// <summary>
    /// Produces the tunnel using only lookup operations in the per-pixel hot path.
    /// </summary>
    void effect_t::DrawTunnel() noexcept
    {
        std::array<uint32_t, 256> Palette{ };

        {
            const double Loudness = Clamp(_Loudness);
            const double Peak     = Clamp(_Peak);
            const double Bass     = Clamp(_BassLevel);
            const double Treble   = Clamp(_TrebleLevel);

            // Determine the global brightness of the frame based on the audio.
            const double Brightness = .48 + (.42 * std::sqrt(Loudness)) + (.10 * Peak);

            const double BaseHue = _Theme.BaseHue + _Theme.BassHue * Bass;
            const double S       = Clamp(_Theme.Saturation + (.14 * Treble));

            constexpr double InversePaletteMaximum = 1. / 255.;

            uint32_t * const PaletteData = Palette.data();

            for (uint32_t Value = 0; Value < Palette.size(); ++Value)
            {
                const double T = (double)Value * InversePaletteMaximum;
                const double Bands = .34 + .66 * T;

                const double H = BaseHue + _Theme.HueSpan * T + .08 * std::sin(T * 12. * Pi + _AnimationTime);
                const double V = Clamp( _Theme.GetValue(Bands * Brightness) + (.06 * Peak));

                double R;
                double G;
                double B;

                HSVToRGB(H, S, V, R, G, B);

                PaletteData[Value] = PackRGB(R, G, B);
            }
        }

        // Calculate the position of the raster-sized window inside the doubled distance and angle tables.
        const double LookAmountX = .43 + .10 * Clamp(_StereoWidth);  // Horizontal movement
        const double LookAmountY = .38 + .08 * Clamp(_MidLevel);    // Vertical movement

            // Calculate the look-up window position. The sin functions control the movement.
        const int32_t RequestedLookX = (int32_t) (_RasterWidth  / 2) + (int32_t) std::lround((double) _RasterWidth  * LookAmountX * std::sin(_LookPhase * 0.50));
        const int32_t RequestedLookY = (int32_t) (_RasterHeight / 2) + (int32_t) std::lround((double) _RasterHeight * LookAmountY * std::sin(_LookPhase * 1.75));

        const uint32_t MaximumTableOffsetX = _TableWidth  - _RasterWidth;
        const uint32_t MaximumTableOffsetY = _TableHeight - _RasterHeight;

        const uint32_t TableOffsetX = (uint32_t) std::clamp(RequestedLookX, 0, (int32_t) MaximumTableOffsetX); // Move forward or backwards in the tunnel.
        const uint32_t TableOffsetY = (uint32_t) std::clamp(RequestedLookY, 0, (int32_t) MaximumTableOffsetY); // Rotate the tunnel texture around the viewer.

        // Calculate the texture offsets.
        const uint32_t ShiftDistance = (uint32_t) std::lround(_Travel)   & TextureMaskX;
        const uint32_t ShiftAngle    = (uint32_t) std::lround(_Rotation) & TextureMaskY;

        const uint32_t RasterWidth  = _RasterWidth;
        const uint32_t RasterHeight = _RasterHeight;
        const uint32_t TableWidth   = _TableWidth;

        const uint8_t  * const __restrict Texture       = _Texture.data();
        const uint8_t  * const __restrict NextTexture   = _NextTexture.data();

        const uint16_t * const __restrict DistanceTable = _DistanceTable.data();
        const uint16_t * const __restrict AngleTable    = _AngleTable.data();
        const uint32_t * const __restrict PaletteData   = Palette.data();

        const uint32_t TextureBlend        = (uint32_t) std::lround(std::clamp(_TextureTransition, 0., 1.) * 256.);
        const uint32_t TextureBlendInverse = 256u - TextureBlend;

        uint32_t * const __restrict Pixels = _Pixels.data();

        size_t PixelIndex = 0;

        for (uint32_t Y = 0; Y < RasterHeight; ++Y)
        {
            const size_t TableRow = ((size_t)(Y + TableOffsetY) * TableWidth) + TableOffsetX;

            const uint16_t * const __restrict DistanceRow = DistanceTable + TableRow;
            const uint16_t * const __restrict AngleRow    = AngleTable    + TableRow;

            uint32_t * const __restrict PixelRow = Pixels + PixelIndex;

            for (uint32_t X = 0; X < RasterWidth; ++X)
            {
                const uint32_t TextureX = ((uint32_t) DistanceRow[X] + ShiftDistance) & TextureMaskX;
                const uint32_t TextureY = ((uint32_t) AngleRow[X]    + ShiftAngle)    & TextureMaskY;

                const size_t TextureIndex = ((size_t) TextureY * TextureWidth) + TextureX;

                const uint32_t Texel = ((uint32_t) Texture[TextureIndex] * TextureBlendInverse + (uint32_t) NextTexture[TextureIndex] * TextureBlend + 128u) >> 8;

                PixelRow[X] = PaletteData[Texel];
            }

            PixelIndex += RasterWidth;
        }
    }
}
