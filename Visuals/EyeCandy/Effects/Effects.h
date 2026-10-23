
/** $VER: Effects.h (2026.10.22) P. Stuer **/

#pragma once

namespace EyeCandy
{
    enum class Category
    {
        DynamicSystems = 0,
        Fields = 1,
        ParticleSystems = 2,
        Retro = 3,
        Trails = 4,

        Min = DynamicSystems,
        Max = Trails,
    };

    enum class Effect
    {
        Waveform = 0,

        // Dynamic Systems
        AizawaAttractor,
        DoublePendulum,
        HalvorsenAttractor,
        LorenzAttractor,
        RosslerAttractor,
        ThomasAttractor,

        CliffordAttractor,
        DeJongAttractor,

        // Trails
        LissajousTrails,
        RibbonFlow,
        Spirograph,

        // Particle Systems
        Fireflies,
        Fireworks,
        ParticleVortex,
        Starfield,
        AudioGalaxy,
        Swarm,

        // Retro
        NeonTunnel,
        VectorGrid,
        Metaballs,

        // Fields
        Plasma,
        FluidInk,
        Aurora,

        Min = Waveform,
        Max = Plasma,
    };
}
