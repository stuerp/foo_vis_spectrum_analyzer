
/** $VER: RotatingTunnel.h (2026.11.04) */

#pragma once

#include "FieldEffect.h"

#include <cstdint>
#include <string>
#include <vector>

namespace RotatingTunnel
{
    using namespace EyeCandy;

    class effect_t final : public Field::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override final = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Reset() noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        std::wstring GetStatus() noexcept override final;

    private:
        enum class TexturePattern : uint8_t
        {
            Xor,
            Checkerboard,
            DiagonalWeave,
            SquareTiles,
            Chevrons,
            CircuitGrid,
            SteppedDiamonds,

            Count
        };

        static constexpr uint32_t TextureWidth  = 256;
        static constexpr uint32_t TextureHeight = 256;
        static constexpr uint32_t TextureMaskX  = TextureWidth  - 1;
        static constexpr uint32_t TextureMaskY  = TextureHeight - 1;

        static void GenerateTexture(TexturePattern pattern, std::vector<uint8_t> & texture) noexcept;
        static const wchar_t * GetTexturePatternName(TexturePattern pattern) noexcept;
        void GenerateTables() noexcept;
        void DrawTunnel() noexcept;

    private:
        std::vector<uint8_t> _Texture;
        std::vector<uint8_t> _NextTexture;
        std::vector<uint16_t> _DistanceTable;   // Precalculated tunnel coordinates
        std::vector<uint16_t> _AngleTable;

        uint32_t _TableWidth  = 0;
        uint32_t _TableHeight = 0;

        double _Rotation  = 0.;
        double _Travel    = 0.;
        double _LookPhase = 0.;

        TexturePattern _TexturePattern = TexturePattern::Xor;
        TexturePattern _NextTexturePattern = TexturePattern::Xor;
        double _TextureTransition = 1.;
    };
}
