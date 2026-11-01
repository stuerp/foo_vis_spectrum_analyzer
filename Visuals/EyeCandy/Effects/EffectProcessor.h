
/** $VER: EffectProcessor.h (2026.11.01) - Audio processor for Eye Candy effects **/

#pragma once

#include <SDK/audio_chunk.h>

namespace EyeCandy
{
    class processor_t
    {
    public:
        void Process(const audio_chunk & chunk, double deltaTime) noexcept;
        void Decay(double deltaTime) noexcept;

    public:
        double _Loudness = 0.;

        double _Bass     = 0.;
        double _Mid      = 0.;
        double _Treble   = 0.;

        double _Balance  = 0.;
        double _Peak     = 0.;

    private:
        double _LowState  = 0.;
        double _HighState = 0.;
    };
}
