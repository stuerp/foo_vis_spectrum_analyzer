
/** $VER: ThreeD.h (2026.10.22) P. Stuer - 3D support for the Eye Candy effects. **/

#pragma once

#include <d2d1.h>

#include <algorithm>
#include <cmath>

#include "Effect.h"

namespace EyeCandy
{
    class ThreeD
    {
        public:
            struct point3d_t
            {
                double X = 0.;
                double Y = 0.;
                double Z = 0.;
            };

            struct world_t
            {
                point3d_t Position {};
                point3d_t Rotation {};
                point3d_t Scale { 1., 1., 1. };
            };

            struct camera_t
            {
                point3d_t Position {};
                point3d_t Target { 0., 0., 1. };
                point3d_t Up { 0., 1., 0. };
            };

            struct object_t
            {
                point3d_t Position {};
                point3d_t Rotation {};
                point3d_t Scale { 1., 1., 1. };
            };

            struct projection_t
            {
                double FieldOfView = 93.6;
                double AspectRatio = 1.;
                double LensOffsetX = 0.;
                double LensOffsetY = 0.;
                double NearPlane   = 0.035;
                double FarPlane    = 1.;
                double Width       = 0.;
                double Height      = 0.;

                inline bool IsValid() const noexcept
                {
                    return std::isfinite(FieldOfView) &&
                           std::isfinite(AspectRatio) &&
                           std::isfinite(LensOffsetX) &&
                           std::isfinite(LensOffsetY) &&
                           std::isfinite(NearPlane) &&
                           std::isfinite(FarPlane) &&
                           std::isfinite(Width) &&
                           std::isfinite(Height) &&
                           (FieldOfView > MinFieldOfView) && (FieldOfView < MaxFieldOfView) &&
                           (AspectRatio > 0.) &&
                           (NearPlane > 0.) &&
                           (NearPlane < FarPlane) &&
                           (Width > 0.) && 
                           (Height > 0.);
                }
            };

            struct projected_point_t
            {
                D2D1_POINT_2F Position {};
                double LinearDepth       = 0.;
                double ViewDepth   = 0.;
                bool IsProjectable = false; // Viewport clipping should be handled separately.
            };

            struct prepared_projection_t
            {
                point3d_t CameraPosition {};
                point3d_t Right {};
                point3d_t Up {};
                point3d_t Forward {};
                object_t Object {};
                world_t World {};
                double HorizontalScale   = 0.;
                double VerticalScale     = 0.;
                double LensOffsetX       = 0.;
                double LensOffsetY       = 0.;
                double NearPlane         = 0.;
                double FarPlane          = 0.;
                double InverseDepthRange = 0.;
                double HalfWidth         = 0.;
                double HalfHeight        = 0.;
                bool IsValid             = false;
            };

            inline prepared_projection_t Prepare() const noexcept;

            inline static projected_point_t Project(point3d_t point, const prepared_projection_t & prepared) noexcept;

            inline static point3d_t Add(const point3d_t & left, const point3d_t & right) noexcept;
            inline static point3d_t Subtract(const point3d_t & left, const point3d_t & right) noexcept;
            inline static point3d_t Multiply(const point3d_t & point, double value) noexcept;
            inline static double Dot(const point3d_t & left, const point3d_t & right) noexcept;
            inline static point3d_t Cross(const point3d_t & left, const point3d_t & right) noexcept;
            inline static point3d_t Normalize(const point3d_t & value) noexcept;

            inline static point3d_t Rotate(point3d_t point, const point3d_t & rotation) noexcept;
            inline static point3d_t Transform(point3d_t point, const object_t & object, const world_t & world) noexcept;

            inline static bool ClipLineToNearPlane(point3d_t & from, point3d_t & to, const prepared_projection_t & prepared) noexcept;

        public:
            world_t World {};
            camera_t Camera {};
            object_t Object {};
            projection_t Projection {};

            static constexpr double MinFieldOfView =  20.;
            static constexpr double MaxFieldOfView = 100.;

            static constexpr double DirectionEpsilonSquared = 1e-24;
    };

    /// <summary>
    /// Prepares the values shared by all 3D projections in a frame.
    /// </summary>
    ThreeD::prepared_projection_t ThreeD::Prepare() const noexcept
    {
        prepared_projection_t Result {};

        if (!Projection.IsValid())
            return Result;

        // Calculate the camera's forward direction.
        {
            Result.Forward = Normalize(Subtract(Camera.Target, Camera.Position));

            // Avoid treating an extremely short and numerically unstable direction as valid.
            if (Dot(Result.Forward, Result.Forward) <= DirectionEpsilonSquared)
                return Result;
        }

        // For a right-handed camera basis:
        //
        //   Right = UpHint x Forward
        //
        // With the default camera:
        //
        //   Forward = { 0, 0, 1 }
        //   UpHint  = { 0, 1, 0 }
        //   Right   = { 1, 0, 0 }
        //

        // Calculate the camera's horizontal axis.
        {
            Result.Right = Normalize(Cross(Camera.Up, Result.Forward));

            // The configured up vector may be zero, parallel to Forward, or nearly parallel to Forward. Choose a stable fallback.
            if (Dot(Result.Right, Result.Right) <= 0.000001)
            {
                const point3d_t FallbackUp = (std::abs(Result.Forward.Y) < 0.999) ? point3d_t { 0., 1., 0. } : point3d_t { 0., 0., 1. };

                Result.Right = Normalize(Cross(FallbackUp, Result.Forward));
            }
        }

        // Reconstruct Up so that the complete basis is orthonormal:
        //
        //   Up = Forward x Right
        //

        // Recalculate the real camera Up vector.
        Result.Up = Normalize(Cross(Result.Forward, Result.Right));

        if (!std::isfinite(Result.Right.X)   || !std::isfinite(Result.Right.Y)   || !std::isfinite(Result.Right.Z)   ||
            !std::isfinite(Result.Up.X)      || !std::isfinite(Result.Up.Y)      || !std::isfinite(Result.Up.Z)      ||
            !std::isfinite(Result.Forward.X) || !std::isfinite(Result.Forward.Y) || !std::isfinite(Result.Forward.Z))
            return {};

        const double FieldOfView = std::clamp(Projection.FieldOfView, 20., 100.) * Pi / 180.; // To radians
        const double FocalLength = 1. / std::tan(FieldOfView * 0.5);

        Result.CameraPosition    = Camera.Position;
        Result.Object            = Object;
        Result.World             = World;
        Result.HorizontalScale   = FocalLength / Projection.AspectRatio;
        Result.VerticalScale     = FocalLength;
        Result.LensOffsetX       = Projection.LensOffsetX;
        Result.LensOffsetY       = Projection.LensOffsetY;
        Result.NearPlane         = Projection.NearPlane;
        Result.FarPlane          = Projection.FarPlane;
        Result.InverseDepthRange = 1. / (Projection.FarPlane - Projection.NearPlane);
        Result.HalfWidth         = Projection.Width  * 0.5;
        Result.HalfHeight        = Projection.Height * 0.5;
        Result.IsValid           = true;

        return Result;
    }

    /// <summary>
    /// Projects a 3D object-local coordinate into a 2D screen coordinate using precomputed values.
    /// </summary>
    ThreeD::projected_point_t ThreeD::Project(point3d_t point, const prepared_projection_t & prepared) noexcept
    {
        if (!prepared.IsValid)
            return {};

        // Transform the point into world space.
        const point3d_t WorldPoint = Transform(point, prepared.Object, prepared.World);

        if (!std::isfinite(WorldPoint.X) || !std::isfinite(WorldPoint.Y) || !std::isfinite(WorldPoint.Z))
            return {};

        // Make the point relative to the camera.
        const point3d_t CameraRelativePoint = Subtract(WorldPoint, prepared.CameraPosition);

        // Convert from world orientation to camera space.
        const double CameraX = Dot(CameraRelativePoint, prepared.Right);
        const double CameraY = Dot(CameraRelativePoint, prepared.Up);
        const double CameraZ = Dot(CameraRelativePoint, prepared.Forward);

        // Reject invalid coordinates and perform near-plane and far-plane clipping (Point behind the camera, at the camera position, too close to the camera or on the near plane)
        if (!std::isfinite(CameraX) || !std::isfinite(CameraY) || !std::isfinite(CameraZ) || (CameraZ <= prepared.NearPlane) || (CameraZ >= prepared.FarPlane))
            return {};

        const double InverseZ = 1. / CameraZ;

        // Apply perspective correction.
        const double NormalizedX = CameraX * prepared.HorizontalScale * InverseZ + prepared.LensOffsetX;
        const double NormalizedY = CameraY * prepared.VerticalScale   * InverseZ + prepared.LensOffsetY;

        projected_point_t Result{};

        // Conver to Direct2D pixel coordinates.
        Result.Position =
        {
            (float) ((NormalizedX + 1.) * prepared.HalfWidth),
            (float) ((1. - NormalizedY) * prepared.HalfHeight)
        };

        Result.LinearDepth   = std::clamp((CameraZ - prepared.NearPlane) * prepared.InverseDepthRange, 0., 1.);
        Result.ViewDepth     = CameraZ;
        Result.IsProjectable = std::isfinite(Result.Position.x) && std::isfinite(Result.Position.y);

        return Result;
    }

    /// <summary>
    /// Clips a line segment against the camera near plane using precomputed values.
    /// </summary>
    bool ThreeD::ClipLineToNearPlane(point3d_t & from, point3d_t & to, const prepared_projection_t & prepared) noexcept
    {
        if (!prepared.IsValid)
            return false;

        const point3d_t FromWorld = Transform(from, prepared.Object, prepared.World);
        const point3d_t ToWorld   = Transform(to, prepared.Object, prepared.World);

        const double FromDepth = Dot(Subtract(FromWorld, prepared.CameraPosition), prepared.Forward);
        const double ToDepth   = Dot(Subtract(ToWorld, prepared.CameraPosition), prepared.Forward);

        if (!std::isfinite(FromDepth) || !std::isfinite(ToDepth))
            return false;

        const double ClippingDepth = std::nextafter(prepared.NearPlane, prepared.FarPlane);

        const bool IsFromVisible = FromDepth >= ClippingDepth;
        const bool IsToVisible   = ToDepth   >= ClippingDepth;

        if (IsFromVisible == IsToVisible)
            return IsFromVisible;

        const double Amount = (ClippingDepth - FromDepth) / (ToDepth - FromDepth);
        const point3d_t Intersection = Add(from, Multiply(Subtract(to, from), Amount));

        if (IsFromVisible)
            to = Intersection;
        else
            from = Intersection;

        return true;
    }

    /// <summary>
    /// Converts a point from object-local space into world space.
    /// </summary>
    ThreeD::point3d_t ThreeD::Transform(point3d_t p, const object_t & object, const world_t & world) noexcept
    {
        // Object scaling
        p =
        {
            p.X * object.Scale.X,
            p.Y * object.Scale.Y,
            p.Z * object.Scale.Z
        };

        // Object rotation and translation
        p = Add(Rotate(p, object.Rotation), object.Position);

        // World scaling
        p =
        {
            p.X * world.Scale.X,
            p.Y * world.Scale.Y,
            p.Z * world.Scale.Z
        };

        // World rotation and translation
        return Add(Rotate(p, world.Rotation), world.Position);
    }

    /// <summary>
    /// Rotates a 3D point around the origin using three Euler-angle rotations,
    /// </summary>
    ThreeD::point3d_t ThreeD::Rotate(point3d_t p, const point3d_t & r) noexcept
    {
        const double CX = std::cos(r.X);
        const double SX = std::sin(r.X);

        const double CY = std::cos(r.Y);
        const double SY = std::sin(r.Y);

        const double CZ = std::cos(r.Z);
        const double SZ = std::sin(r.Z);

        // Rotate around the X axis.
        p =
        {
            p.X,
            p.Y * CX - p.Z * SX,
            p.Y * SX + p.Z * CX
        };

        // Rotate around the Y axis.
        p =
        {
            p.X * CY + p.Z * SY,
            p.Y,
           -p.X * SY + p.Z * CY
        };

        // Rotate around the Z axis.
        return
        {
            p.X * CZ - p.Y * SZ,
            p.X * SZ + p.Y * CZ,
            p.Z
        };
    }

    /// <summary>
    ///
    /// </summary>
    ThreeD::point3d_t ThreeD::Add(const point3d_t & p1, const point3d_t & p2) noexcept
    {
        return { p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z };
    }

    /// <summary>
    ///
    /// </summary>
    ThreeD::point3d_t ThreeD::Subtract(const point3d_t & p1, const point3d_t & p2) noexcept
    {
        return { p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
    }

    /// <summary>
    ///
    /// </summary>
    ThreeD::point3d_t ThreeD::Multiply(const point3d_t & p, double v) noexcept
    {
        return { p.X * v, p.Y * v, p.Z * v };
    }

    /// <summary>
    /// Calculates the dot product.
    /// </summary>
    double ThreeD::Dot(const point3d_t & p1, const point3d_t & p2) noexcept
    {
        return p1.X * p2.X + p1.Y * p2.Y + p1.Z * p2.Z;
    }

    /// <summary>
    /// Calculates the cross product.
    /// </summary>
    ThreeD::point3d_t ThreeD::Cross(const point3d_t & p1, const point3d_t & p2) noexcept
    {
        return
        {
            p1.Y * p2.Z - p1.Z * p2.Y,
            p1.Z * p2.X - p1.X * p2.Z,
            p1.X * p2.Y - p1.Y * p2.X
        };
    }

    /// <summary>
    ///
    /// </summary>
    ThreeD::point3d_t ThreeD::Normalize(const point3d_t & pt) noexcept
    {
        const double LengthSquared = Dot(pt, pt);

        return (LengthSquared > 0.000001) ? Multiply(pt, 1. / std::sqrt(LengthSquared)) : point3d_t {};
    }
}
