
/** $VER: LorenzAttractor.cpp (2026.10.10) P. Stuer - Implements an effect for the Eye Candy visualizer. **/

#include <pch.h>

#include "LorenzAttractor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <directxmath.h>

/// <summary>
/// Initializes this instance.
/// </summary>
void lorenz_attractor_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    _SmoothedLeftLevel  = 0.;
    _SmoothedRightLevel = 0.;

    _LeftRotationAngle  = 0.;
    _RightRotationAngle = 0.;

    _LorenzX = 0.1;
    _LorenzY = 0.0;
    _LorenzZ = 0.0;

    _IsRenderTimeInitialized = false;
    _IsWarmedUp = false;
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void lorenz_attractor_t::Move(const D2D1_RECT_F & rect) noexcept
{
    __super::Move(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void lorenz_attractor_t::Reset() noexcept
{
    DeleteDeviceSpecificResources();
}

namespace
{
    // Audio reactivity
    double AudioReactivity = 1.0; // [0, 2] Controls rho, point density and opacity.

    // Minimum and maximum rotation speed
    double MinRotationSpeed = DirectX::XMConvertToRadians( 5.f); // deg/s, [0,   45] deg/s, Default: 0.10, Idle rotation speed.
    double MaxRotationSpeed = DirectX::XMConvertToRadians(45.f); // deg/s, [45, 180] deg/s, Default: 0.80

    double Attack  =  40.; // [10, 500] ms
    double Release = 200.; // [50, 2000] ms

    // Audio-level smoothing rates in inverse seconds. These preserve approximately
    // the original 0.32 attack and 0.08 release response at 60 frames per second.
    const auto AttackSmoothingRate  = 1. / std::max(Attack, 0.001);
    const auto ReleaseSmoothingRate = 1. / std::max(Release, 0.001);

    int PointDensity = 8192; // [512, 16384]
    double AudioDensityBoost = 0.75; // [0, 2]




    // The Lorenz system evolves one point through three coupled differential equations:
    //
    //     dx/dt = sigma * (y - x)
    //     dy/dt = x * (rho - z) - y
    //     dz/dt = x * y - beta * z
    //
    // Repeated integration produces a deterministic but chaotic trajectory around two nonzero equilibrium points when rho > 1.
    // Projecting that trajectory onto the render target creates the characteristic two-lobed, butterfly-shaped Lorenz attractor.

    // Ref. http://www.me.rochester.edu/courses/ME406/webexamp5/loreq.pdf

    // h is the integration step. Smaller values improve numerical accuracy and produce more closely spaced points; larger values advance the trajectory faster but can make it coarse or unstable.
    constexpr double h      = 0.008;

    // Sigma and beta retain their classic values, while rho is varied by the audio level to change the shape of the attractor.
    constexpr double Sigma  = 10.;
    constexpr double Beta   = 8. / 3.;
    constexpr double MinRho = 20.;
    constexpr double MaxRho = 52.;

    // Smooth changes in angular velocity independently of the audio-level smoothing.
    // The exponential form produces nearly identical motion at different rendering frame rates.
    constexpr double RotationSmoothingSpeed = 3.;

    // Prevent one unusually long audio buffer from producing excessive work.
    constexpr size_t MaxPointsPerFrame = 8192;

    // Number of initial integration steps used to move the simulation away from its artificial starting point.
    constexpr size_t WarmupIterationCount = 256;

    static constexpr std::array<D2D1_COLOR_F, 10> Colors =
    {
        D2D1_COLOR_F { 0xAA / 255.f, 0x33 / 255.f, 0x55 / 255.f, 1.f },
        D2D1_COLOR_F { 0xCC / 255.f, 0x66 / 255.f, 0x66 / 255.f, 1.f },
        D2D1_COLOR_F { 0xEE / 255.f, 0x99 / 255.f, 0x44 / 255.f, 1.f },
        D2D1_COLOR_F { 0xEE / 255.f, 0xDD / 255.f, 0x00 / 255.f, 1.f },
        D2D1_COLOR_F { 0x99 / 255.f, 0xDD / 255.f, 0x55 / 255.f, 1.f },
        D2D1_COLOR_F { 0x44 / 255.f, 0xDD / 255.f, 0x88 / 255.f, 1.f },
        D2D1_COLOR_F { 0x22 / 255.f, 0xCC / 255.f, 0xBB / 255.f, 1.f },
        D2D1_COLOR_F { 0x00 / 255.f, 0xBB / 255.f, 0xCC / 255.f, 1.f },
        D2D1_COLOR_F { 0x00 / 255.f, 0x99 / 255.f, 0xCC / 255.f, 1.f },
        D2D1_COLOR_F { 0x33 / 255.f, 0x66 / 255.f, 0xBB / 255.f, 1.f },
    };

    constexpr double ColorRepetitions = 3.; // [0.25, 8]

    constexpr double TwoPi = 2. * std::numbers::pi;
}

/// <summary>
/// Renders this instance.
/// </summary>
void lorenz_attractor_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    const audio_sample * Frames = _Analysis->_Chunk.get_data();

    const size_t FrameCount   = _Analysis->_Chunk.get_sample_count();     // get_sample_count() actually returns the number of frames.
    const size_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    if ((FrameCount == 0) || (ChannelCount == 0))
        return;

    const auto HalfHeight = _Size.height / 2;
    const auto HalfWidth  = _Size.width  / 2;

    // Calculate the RMS amplitude of the left and right channels.
    // Squaring the samples gives larger magnitudes proportionally more influence than a mean-absolute measurement.
    // Samples are converted to double before multiplication so the energy accumulation does not overflow an integer sample type.
    double LeftEnergy  = 0.;
    double RightEnergy = 0.;

    const auto FrameEnd = Frames + (FrameCount * ChannelCount);

    for (const audio_sample * Frame = Frames; Frame < FrameEnd; Frame += ChannelCount)
    {
        const auto LeftSample  = (double) Frame[0]; // [-1, 1]
        const auto RightSample = (double) Frame[1]; // [-1, 1]

        LeftEnergy  += LeftSample  * LeftSample;
        RightEnergy += RightSample * RightSample;
    }

    const auto InverseFrameCount = 1. / (double) FrameCount;

    const auto LeftLevel  = std::clamp(std::sqrt(LeftEnergy  * InverseFrameCount), 0., 1.);
    const auto RightLevel = std::clamp(std::sqrt(RightEnergy * InverseFrameCount), 0., 1.);

    // Advance the animation by one interval at the framework's target refresh rate.
    // This keeps audio smoothing and rotation speed synchronized with the configured rendering cadence instead of depending on the wall-clock duration of this call.
    constexpr double DefaultRefreshRate = 60.;

    const auto RefreshRate = (_State->_TargetRefreshRate > 0) ? (double) _State->_TargetRefreshRate : DefaultRefreshRate;
    const auto FrameTime   = 1. / RefreshRate;

    // Convert per-second attack and release rates to dimensionless interpolation factors
    // for the current frame. The exponential response is stable across varying frame rates.
    const auto LeftSmoothingRate  = (LeftLevel  > _SmoothedLeftLevel)  ? AttackSmoothingRate : ReleaseSmoothingRate;
    const auto RightSmoothingRate = (RightLevel > _SmoothedRightLevel) ? AttackSmoothingRate : ReleaseSmoothingRate;

    const auto LeftSmoothing  = 1. - std::exp(-LeftSmoothingRate  * FrameTime);
    const auto RightSmoothing = 1. - std::exp(-RightSmoothingRate * FrameTime);

    _SmoothedLeftLevel  += (LeftLevel  - _SmoothedLeftLevel)  * LeftSmoothing;
    _SmoothedRightLevel += (RightLevel - _SmoothedRightLevel) * RightSmoothing;

    const auto CombinedLevel = std::clamp((_SmoothedLeftLevel + _SmoothedRightLevel) * 0.5, 0., 1.);

    // Apply a mild response curve, then map the combined audio level to rho.
    // In the Lorenz equations, rho changes the system's dynamical regime and determines the positions of the two nonzero equilibria when rho is greater than one.
    // Varying rho in this range changes the size and behavior of the two-lobed trajectory; it does not guarantee chaotic behavior at every instantaneous rho value.
    // For rho greater than one, the two nonzero equilibria are located at (+Equilibrium, +Equilibrium, ZCenter) and (-Equilibrium, -Equilibrium, ZCenter).
    const auto ReactiveLevel = std::sqrt(std::clamp(CombinedLevel * AudioReactivity, 0., 1.));
    const auto Rho           = MinRho + ((MaxRho - MinRho) * ReactiveLevel);
    const auto ZCenter       = Rho - 1.;
    const auto Equilibrium   = std::sqrt(std::max(0., Beta * ZCenter));

    const auto RotationSpeedRange = MaxRotationSpeed - MinRotationSpeed;

    const auto LeftRotationSpeed  = MinRotationSpeed + (_SmoothedLeftLevel  * RotationSpeedRange);
    const auto RightRotationSpeed = MinRotationSpeed + (_SmoothedRightLevel * RotationSpeedRange);

    const auto RotationSmoothing = 1. - std::exp(-RotationSmoothingSpeed * FrameTime);

    _SmoothedLeftRotationSpeed  += (LeftRotationSpeed  - _SmoothedLeftRotationSpeed)  * RotationSmoothing;
    _SmoothedRightRotationSpeed += (RightRotationSpeed - _SmoothedRightRotationSpeed) * RotationSmoothing;

    _LeftRotationAngle  = std::fmod(_LeftRotationAngle  + (_SmoothedLeftRotationSpeed  * FrameTime), TwoPi);
    _RightRotationAngle = std::fmod(_RightRotationAngle + (_SmoothedRightRotationSpeed * FrameTime), TwoPi);

    const auto CosX = std::cos(_LeftRotationAngle);
    const auto SinX = std::sin(_LeftRotationAngle);

    const auto CosY = std::cos(_RightRotationAngle);
    const auto SinY = std::sin(_RightRotationAngle);

    const auto SinXSinY = SinX * SinY;
    const auto CosYSinX = CosY * SinX;

    // Estimate the unrotated attractor dimensions from rho and the equilibrium positions.
    // These empirical radii provide a practical display scale but are not strict bounds, especially after rotating the three-dimensional trajectory.
    const auto EstimatedHorizontalRadius = std::max(1., Equilibrium * 2.35);
    const auto EstimatedVerticalRadius   = std::max(1., Rho   * 1.35);

    const auto HorizontalScale = (double) HalfWidth  / EstimatedHorizontalRadius;
    const auto VerticalScale   = (double) HalfHeight / EstimatedVerticalRadius;

    // Convert Lorenz-space units to DIPs. Choosing the smaller estimated axis scale and applying an 8% margin reduces clipping.
    const auto ScalingMultiplier = 0.92 * std::min(HorizontalScale, VerticalScale);

    // Scale distance from the nearest equilibrium so a distance equal to ColorDistanceScale spans three complete cycles through the color table.
    const auto ColorDistanceScale   = std::max(1., EstimatedHorizontalRadius + EstimatedVerticalRadius);
    const auto ColorIndexMultiplier = (ColorRepetitions * (double) Colors.size()) / ColorDistanceScale;

    // Advance the persistent Lorenz state by one explicit Euler step. Each point depends on the preceding state, producing one continuous numerical trajectory.
    const auto Euler = [Rho](double & x, double & y, double & z) noexcept
    {
        const auto DeltaX = Sigma    * (y - x);
        const auto DeltaY = x * (Rho - z) - y;
        const auto DeltaZ = x * y - Beta * z;

        x += h * DeltaX;
        y += h * DeltaY;
        z += h * DeltaZ;
    };

    // Start slightly away from the origin, which is unstable for the configured rho range.
    // The warm-up advances the solution by 256 steps, or 2.048 Lorenz time units, to reduce the visible influence of the arbitrary initial state before rendering begins.
    if (!_IsWarmedUp)
    {
        _LorenzX = 0.1;
        _LorenzY = 0.0;
        _LorenzZ = 0.0;

        for (size_t i = 0; i < WarmupIterationCount; ++i)
            Euler(_LorenzX, _LorenzY, _LorenzZ);

        _IsWarmedUp = true;
    }

    // Draw enough successive trajectory samples to form a dense trace, but cap the work for unusually large audio buffers.
    // Loud audio increases the number of rendered trajectory samples without changing h.
    // Keeping h fixed makes the numerical behavior consistent; the selected small step reduces, but does not eliminate, the stability and accuracy limitations.
    const auto BasePointCount       = std::min(FrameCount, MaxPointsPerFrame);
    const auto AdditionalPointCount = (size_t) ((double) BasePointCount * ReactiveLevel * AudioDensityBoost);
    const auto PointCount           = std::min(BasePointCount + AdditionalPointCount, MaxPointsPerFrame);

    const auto OldAntialiasMode = deviceContext->GetAntialiasMode();

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    size_t OldColorIndex = Colors.size();

    constexpr FLOAT MinLorenzOpacity = 0.50f;
    constexpr FLOAT MaxLorenzOpacity = 1.00f;

    const auto BrushOpacity = std::lerp(MinLorenzOpacity, MaxLorenzOpacity, (FLOAT) ReactiveLevel);

    const auto OldBrushOpacity = _Brush->GetOpacity();

    _Brush->SetOpacity(BrushOpacity);

    for (size_t i = 0; i < PointCount; ++i)
    {
        Euler(_LorenzX, _LorenzY, _LorenzZ);

        if (!std::isfinite(_LorenzX) || !std::isfinite(_LorenzY) || !std::isfinite(_LorenzZ))
        {
            _LorenzX = 0.1;
            _LorenzY = 0.0;
            _LorenzZ = 0.0;
            continue;
        }

        const auto CenteredZ = _LorenzZ - ZCenter;

        // Translate z so the plane containing the two nonzero equilibria becomes z = 0, then apply the view rotations before projecting to two dimensions.
        // Rotation changes only the view; the persistent Lorenz state remains in simulation space.
        const auto RotatedX = (_LorenzX * CosY) + (CenteredZ * SinY);
        const auto RotatedY = (_LorenzX * SinXSinY) + (_LorenzY * CosX) - (CenteredZ * CosYSinX);

        const auto ScreenX = (FLOAT) std::lround((double) HalfWidth  + (RotatedX * ScalingMultiplier));
        const auto ScreenY = (FLOAT) std::lround((double) HalfHeight + (RotatedY * ScalingMultiplier));

        if ((ScreenX < 0) || (ScreenX >= _Size.width) || (ScreenY < 0) || (ScreenY >= _Size.height))
            continue;

        // The two nonzero equilibria are located at (+Equilibrium, +Equilibrium, ZCenter) and (-Equilibrium, -Equilibrium, ZCenter).
        // Select a color from the point's distance to the nearer equilibrium, producing a repeating radial color pattern around each lobe.
        {
            const auto PositiveDeltaX = _LorenzX - Equilibrium;
            const auto PositiveDeltaY = _LorenzY - Equilibrium;

            const auto NegativeDeltaX = _LorenzX + Equilibrium;
            const auto NegativeDeltaY = _LorenzY + Equilibrium;

            const auto DistanceSquaredP1 = (PositiveDeltaX * PositiveDeltaX) + (PositiveDeltaY * PositiveDeltaY) + (CenteredZ * CenteredZ);
            const auto DistanceSquaredP2 = (NegativeDeltaX * NegativeDeltaX) + (NegativeDeltaY * NegativeDeltaY) + (CenteredZ * CenteredZ);

            const auto MinimumDistanceSquared = std::min(DistanceSquaredP1, DistanceSquaredP2);
            const auto ColorDistance          = std::sqrt(std::max(0., MinimumDistanceSquared));
            const auto UnwrappedColorIndex    = (size_t) (ColorDistance * ColorIndexMultiplier);
            const auto ColorIndex             = UnwrappedColorIndex % Colors.size();

            if (ColorIndex != OldColorIndex)
            {
                _Brush->SetColor(Colors[ColorIndex]);

                OldColorIndex = ColorIndex;
            }
        }

        deviceContext->FillRectangle(D2D1::RectF(ScreenX, ScreenY, ScreenX + 1.f, ScreenY + 1.f), _Brush.Get());
    }

    _Brush->SetOpacity(OldBrushOpacity);

    deviceContext->SetAntialiasMode(OldAntialiasMode);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT lorenz_attractor_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
#ifdef _DEBUG
    if (_Brush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _Brush.GetAddressOf());
#endif

    return S_OK;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void lorenz_attractor_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _Brush.Reset();
#endif
}
