
/** $VER: LorenzSystem.cpp (2026.10.10) P. Stuer - - Implements a visualizer using a Lorenz System. **/

#include <pch.h>

#include "LorenzSystem.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <numbers>
#include <numeric>
#include <string>
#include <thread>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
lorenz_system_t::lorenz_system_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
lorenz_system_t::~lorenz_system_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void lorenz_system_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    CreateDeviceIndependentResources();

    _SmoothedLeftLevel  = 0.;
    _SmoothedRightLevel = 0.;

    _LeftRotationAngle  = 0.;
    _RightRotationAngle = 0.;

    _LorenzX = 0.1;
    _LorenzY = 0.0;
    _LorenzZ = 0.0;

    _IsInitialized = false;
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void lorenz_system_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void lorenz_system_t::Reset() noexcept
{
    if (!_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    _ForceElementToResize = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void lorenz_system_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void lorenz_system_t::Resize() noexcept
{
    if (!_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    _ForceElementToResize = false;
}

namespace
{
    // The Lorenz system evolves one point through three coupled differential equations:
    //
    //     dx/dt = sigma * (y - x)
    //     dy/dt = x * (rho - z) - y
    //     dz/dt = x * y - beta * z
    //
    // Repeated integration produces a deterministic but chaotic trajectory around two nonzero equilibrium points.
    // Projecting that trajectory onto the render target creates the characteristic two-lobed, butterfly-shaped Lorenz attractor.

    // h is the integration step. Smaller values improve numerical accuracy and produce more closely spaced points; larger values advance the trajectory faster but can make it coarse or unstable.
    constexpr double h      = 0.008;

    // Sigma and beta retain their classic values, while rho is varied by the audio level to change the shape of the attractor.
    constexpr double Sigma  = 10.;
    constexpr double Beta   = 8. / 3.;
    constexpr double MinRho = 20.;
    constexpr double MaxRho = 52.;

    constexpr double MinRotationSpeed = 0.04; // rad / s
    constexpr double MaxRotationSpeed = 0.65; // rad / s

    // A higher value reacts faster but produces more visual jitter.
    constexpr double AttackSmoothing  = 0.32;
    constexpr double ReleaseSmoothing = 0.08;

    // Prevent one unusually long audio buffer from producing excessive work.
    constexpr size_t MaxPointsPerFrame = 8192;

    // Number of initial integration steps used to move the simulation away from its artificial starting point.
    constexpr size_t WarmupIterationCount = 256;

    // Smooth changes in angular velocity independently of the audio-level smoothing.
    // The exponential form produces nearly identical motion at different rendering frame rates.
    constexpr double RotationSmoothingSpeed = 3.; // 1 / s
    constexpr double TwoPi                  = 2. * std::numbers::pi;

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
}

/// <summary>
/// Renders this instance.
/// </summary>
void lorenz_system_t::Render(ID2D1DeviceContext * deviceContext) noexcept
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

    // Measure the RMS amplitude of the left and right channels. RMS gives transients
    // and beats more influence than a mean-absolute measurement while remaining stable
    // for sustained audio. Samples are converted before multiplication so integer
    // overflow cannot occur.
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

    auto LeftLevel  = std::sqrt(LeftEnergy  * InverseFrameCount);
    auto RightLevel = std::sqrt(RightEnergy * InverseFrameCount);

    LeftLevel  = std::clamp(LeftLevel,  0., 1.);
    RightLevel = std::clamp(RightLevel, 0., 1.);

    // Smooth rising and falling audio levels at different rates. The faster attack
    // keeps beats responsive, while the slower release prevents the attractor parameters
    // and view rotation from jittering during quieter passages.
    const auto LeftSmoothing  = (LeftLevel  > _SmoothedLeftLevel)  ? AttackSmoothing : ReleaseSmoothing;
    const auto RightSmoothing = (RightLevel > _SmoothedRightLevel) ? AttackSmoothing : ReleaseSmoothing;

    _SmoothedLeftLevel  += (LeftLevel  - _SmoothedLeftLevel)  * LeftSmoothing;
    _SmoothedRightLevel += (RightLevel - _SmoothedRightLevel) * RightSmoothing;

    const auto CombinedLevel = std::clamp((_SmoothedLeftLevel + _SmoothedRightLevel) * 0.5, 0., 1.);

    // Apply a mild response curve, then map the combined audio level to rho. In the
    // Lorenz equations, rho controls the system's regime and the location of the two
    // nonzero equilibria. Keeping rho in this bounded range preserves the recognizable
    // two-lobed attractor while allowing louder audio to expand and reshape it.
    const auto ReactiveLevel = std::sqrt(CombinedLevel);
    const auto LorenzRho     = MinRho + ((MaxRho - MinRho) * ReactiveLevel);
    const auto ZCenter       = LorenzRho - 1.;
    const auto Equilibrium   = std::sqrt(std::max(0., Beta * ZCenter));

    // Measure elapsed wall-clock time so angular velocity is expressed in radians per second and remains independent of the rendering frame rate.
    double FrameTime = 1. / 60.;

    {
        const auto CurrentRenderTime = std::chrono::steady_clock::now();

        if (_RenderTimeInitialized)
        {
            FrameTime = std::chrono::duration<double>(CurrentRenderTime - _PreviousRenderTime).count();

            // Clamp long pauses so resuming rendering cannot produce a sudden rotation jump.
            FrameTime = std::clamp(FrameTime, 0., 1. / 15.);
        }
        else
        {
            _RenderTimeInitialized = true;
        }

        _PreviousRenderTime = CurrentRenderTime;
    }

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

    // Estimate the attractor's projected bounds from rho and the equilibrium positions.
    // The resulting scale fits both dimensions inside the render area.
    const auto EstimatedHorizontalRadius = std::max(1., Equilibrium * 2.35);
    const auto EstimatedVerticalRadius   = std::max(1., LorenzRho   * 1.35);

    const auto HorizontalScale = (double) HalfWidth  / EstimatedHorizontalRadius;
    const auto VerticalScale   = (double) HalfHeight / EstimatedVerticalRadius;

    // Convert Lorenz-space units to DIPs. Choosing the smaller axis scale keeps both
    // dimensions visible; the 8% margin prevents the outer trajectory from touching the edges.
    const auto ScalingMultiplier = 0.92 * std::min(HorizontalScale, VerticalScale);

    // Map distance from the nearest lobe center to three cycles through the color table.
    const auto ColorDistanceScale      = std::max(1., EstimatedHorizontalRadius + EstimatedVerticalRadius);
    const auto ColorIndexMultiplier    = (3. * (double) Colors.size()) / ColorDistanceScale;
    const auto MaximumColorTableIndex  = Colors.size() - 1;

    // Start slightly away from the unstable equilibrium at the origin. The warm-up
    // iterations discard the artificial initial transient and move the persistent state
    // onto the main body of the attractor.
    if (!_IsInitialized)
    {
        _LorenzX = 0.1;
        _LorenzY = 0.0;
        _LorenzZ = 0.0;

        for (size_t i = 0; i < WarmupIterationCount; ++i)
        {
            const auto DeltaX = Sigma * (_LorenzY - _LorenzX);
            const auto DeltaY = _LorenzX    * (LorenzRho - _LorenzZ) - _LorenzY;
            const auto DeltaZ = _LorenzX    * _LorenzY - Beta * _LorenzZ;

            _LorenzX += h * DeltaX;
            _LorenzY += h * DeltaY;
            _LorenzZ += h * DeltaZ;
        }

        _IsInitialized = true;
    }

    // Draw enough successive trajectory samples to form a dense trace, but cap the work for unusually large audio buffers.
    // Loud audio increases point density rather than changing h, because a fixed integration step preserves numerical stability.
    const auto BasePointCount       = std::min(FrameCount, MaxPointsPerFrame);
    const auto AdditionalPointCount = (size_t) ((double) BasePointCount * ReactiveLevel * 0.75);
    const auto PointCount           = std::min(BasePointCount + AdditionalPointCount, MaxPointsPerFrame);

    const auto OldAntialiasMode = deviceContext->GetAntialiasMode();

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    size_t OldColorIndex = Colors.size();

    for (size_t i = 0; i < PointCount; ++i)
    {
        // Advance the persistent Lorenz state by one explicit Euler step. Each new point
        // depends on the previous point, so the rendered trace follows one continuous
        // chaotic trajectory rather than restarting for every frame.
        const auto DeltaX = Sigma * (_LorenzY - _LorenzX);
        const auto DeltaY = _LorenzX    * (LorenzRho - _LorenzZ) - _LorenzY;
        const auto DeltaZ = _LorenzX    * _LorenzY - Beta * _LorenzZ;

        _LorenzX += h * DeltaX;
        _LorenzY += h * DeltaY;
        _LorenzZ += h * DeltaZ;

        if (!std::isfinite(_LorenzX) || !std::isfinite(_LorenzY) || !std::isfinite(_LorenzZ))
        {
            _LorenzX = 0.1;
            _LorenzY = 0.0;
            _LorenzZ = 0.0;
            continue;
        }

        const auto CenteredZ = _LorenzZ - ZCenter;

        // Translate z so the attractor's lobe center lies at the projection origin, then rotate the projected coordinates around the X and Y axes.
        // Rotation changes only the view; the persistent Lorenz state remains in simulation space.
        const auto RotatedX = (_LorenzX * CosY) + (CenteredZ * SinY);
        const auto RotatedY = (_LorenzX * SinXSinY) + (_LorenzY * CosX) - (CenteredZ * CosYSinX);

        const auto ScreenX = (FLOAT) std::lround((double) HalfWidth  + (RotatedX * ScalingMultiplier));
        const auto ScreenY = (FLOAT) std::lround((double) HalfHeight + (RotatedY * ScalingMultiplier));

        if ((ScreenX < 0) || (ScreenX >= _Size.width) || (ScreenY < 0) || (ScreenY >= _Size.height))
            continue;

        // The two nonzero equilibria are located at (+/-Equilibrium, +/-Equilibrium, ZCenter).
        // Select a color according to the point's distance from the nearer equilibrium, which gives each lobe a consistent radial gradient.
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
            const auto ColorIndex             = std::min(UnwrappedColorIndex % Colors.size(), MaximumColorTableIndex);

            if (ColorIndex != OldColorIndex)
            {
                _Brush->SetColor(Colors[ColorIndex]);

                OldColorIndex = ColorIndex;
            }
        }

        deviceContext->FillRectangle(D2D1::RectF(ScreenX, ScreenY, ScreenX + 1.f, ScreenY + 1.f), _Brush.Get());
    }

    deviceContext->SetAntialiasMode(OldAntialiasMode);
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT lorenz_system_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void lorenz_system_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT lorenz_system_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    if ((_Size.width <= 0.f) || _Size.height <= 0.f)
        return E_INVALIDARG;

    Resize();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_Brush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _Brush.GetAddressOf());
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void lorenz_system_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _Brush.Reset();
#endif
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void lorenz_system_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        return;

    _State->_ResizeResources = true;
}
