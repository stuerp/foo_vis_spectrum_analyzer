
// "Point sprite impostor" shader for rendering the stereometer dots as soft glowing circles.

cbuffer Constants : register(b0)
{
    float2 InverseViewport;

    float Decay;
    float Correlation;
    float2 PeakLevels;
    float2 PeakHolds;
    float2 ClipFlags;
    float2 ThresholdData;
    float2 OverThreshold;
    float2 Padding;

    float2 ScopeScale;
    float2 ScopeOffset;
}

struct Input
{
    float2 Center : CENTER;
    float2 Corner : CORNER;
    float3 Color  : COLOR;
    float Energy  : ENERGY;
};

struct Output
{
    float4 Position : SV_POSITION;
    float2 Corner   : TEXCOORD0;
    float3 Color    : COLOR0;
    float Energy    : TEXCOORD1;
};

// Expands each audio point into a small screen-space quad.
Output MainVS(Input i)
{
    Output o;

    // Map audio coordinates into screen coordinates. Scale and position the center.
    float2 Positioned = (i.Center * ScopeScale) + ScopeOffset;

    // Expand the point into a ~6 pixel square quad.
    o.Position = float4(Positioned + (i.Corner * InverseViewport * 6.0), 0, 1);
    o.Corner   = i.Corner;
    o.Color    = i.Color;
    o.Energy   = i.Energy;

    return o;
}

// Turns the quad into a circular glow.
float4 MainPS(Output i) : SV_TARGET
{
    float d = dot(i.Corner, i.Corner);

    // Clip pixels outside the circle.
    clip(1 - d);

    // Calculate the glow factor using a Gaussian-like falloff. Use the energy value to boost the brightness.
    float Glow = exp(-4 * d) * i.Energy;

    return float4(i.Color * Glow, Glow); // Returns a color. Also use the glow value for the alpha channel.
}
