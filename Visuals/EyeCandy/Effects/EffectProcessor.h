
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

        double _BassEnergy     = 0.;
        double _MidEnergy      = 0.;
        double _TrebleEnergy   = 0.;

        double _Balance  = 0.;
        double _Peak     = 0.;

    private:
        double _BassState  = 0.;
        double _MidState = 0.;
    };
}
