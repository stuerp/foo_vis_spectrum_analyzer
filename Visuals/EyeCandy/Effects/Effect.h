
/** $VER: Effect.h (2026.11.06) P. Stuer - Base class for the Eye Candy effects. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 4746 5039 5045 5246 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include <Windows.h>

#include <d2d1.h>
#include <wrl/client.h>

#include <array>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include <SDK/audio_chunk.h>

#include "Effects.h"
#include "EffectProcessor.h"

namespace EyeCandy
{
    using Microsoft::WRL::ComPtr;

    /// <summary>
    /// Base class for all effects.
    /// </summary>
    class effect_t : public effect_processor_t
    {
    public:
        effect_t() = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        virtual ~effect_t() = default;

        void Resize(float width, float height) noexcept
        {
            _Width  = width;
            _Height = height;

            _CenterX = width  * 0.5f;
            _CenterY = height * 0.5f;

            _ViewPort = D2D1::RectF(0.f, 0.f, _Width, _Height);

            Reset();
        }

        virtual void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept = 0;

        virtual void Reset() noexcept { };

        virtual void OnLButtonDown(HWND hWnd, int x, int y) noexcept
        {
            _DragHelper.Start(hWnd, x, y, _Interaction); 
        };

        virtual void OnLButtonUp(int x, int y) noexcept
        {
            _DragHelper.Stop(x, y, _Interaction);
        };

        virtual void OnMouseMove(int x, int y) noexcept
        {
            _DragHelper.Move(x, y, _Interaction);
        };

        virtual void OnMouseWheel(short wheelDelta) noexcept
        {
        };

    public:
        double GetTime() const noexcept
        {
            return _AnimationTime;
        }

        virtual std::wstring GetStatus() noexcept
        {
            return { };
        }

        void SetQuality(Quality value) noexcept
        {
            if (_Quality == value)
                return;

            _Quality = value;

            Reset();
        }

        void SetSpeed(Speed value) noexcept
        {
            if (_Speed == value)
                return;

            _Speed = value;

            Reset();
        }

        void SetBrightness(Brightness value) noexcept
        {
            if (_Brightness == value)
                return;

            _Brightness = value;

            Reset();
        }

        void SetDensity(Density value)
        {
            if (_Density == value)
                return;

            _Density = value;

            Reset();
        }

        // Factory method
        static std::unique_ptr<effect_t> Create(Effect effect) noexcept;

    protected:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget);
        void DeleteDeviceDependentResources() noexcept;

        virtual void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept;
        virtual void RenderScanlines(ID2D1RenderTarget * renderTarget) noexcept;
        virtual void RenderFrame(ID2D1RenderTarget * renderTarget, const D2D1_COLOR_F & color) noexcept;

    protected:
        ColorTheme GetColorTheme() const noexcept
        {
            return _ColorTheme;
        }

        void UpdateThemeTransition(double deltaTime) noexcept;
        void BeginThemeTransition(ColorTheme theme) noexcept;

        void SetColorTheme(ColorTheme theme) noexcept;
        void NextColorTheme() noexcept;
        void PreviousColorTheme() noexcept;

        static const color_theme_t & GetTheme(ColorTheme theme) noexcept;
        static const std::wstring GetThemeName(ColorTheme theme) noexcept;

        D2D1_COLOR_F GetThemeColor(ThemeRole role, float hueOffset, float saturation, float value, float alpha) const noexcept;

    protected:
        float _Width  = 0.f;
        float _Height = 0.f;

        float _CenterX = 0.f;
        float _CenterY = 0.f;

        D2D1_RECT_F _ViewPort {};

    protected:
        UINT32 _RasterWidth  = 0;
        UINT32 _RasterHeight = 0;
        UINT32 _Stride       = 0;

        std::vector<uint32_t> _Pixels;

        double _AnimationTime = 0.;

        interaction_t _Interaction {};

    protected:
        double _TravelDistance       = 0.;

        double _CameraShake          = 0.;
        double _CameraRoll           = 0.;

    protected:
        Quality _Quality       = Quality::Medium;
        Speed _Speed           = Speed::Normal;
        Brightness _Brightness = Brightness::Medium;
        Density _Density       = Density::Medium;

        static constexpr std::array<const wchar_t *, 3> Qualities =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

        static constexpr std::array<const wchar_t *, 3> Speeds =
        {{
            L"Slow",
            L"Normal",
            L"Fast"
        }};

        static constexpr std::array<const wchar_t *, 3> Brightnesses =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

        static constexpr std::array<const wchar_t *, 3> DensityNames =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

    protected:
        ColorTheme _ColorTheme = ColorTheme::Synthwave;
        ColorTheme _ThemeFrom  = ColorTheme::Synthwave;
        ColorTheme _ThemeTo    = ColorTheme::Synthwave;

        static constexpr double ThemeTransitionInterval     = 3.0;          // seconds
        static constexpr double ThemeTransitionDuration     = 1.2;
        static constexpr double ThemeResourceUpdateInterval = 1.0 / 30.0;

        double _ThemeElapsed           = 0.;
        double _ThemeTransitionElapsed = ThemeTransitionDuration;
        double _ThemeResourceElapsed   = 0.;
        double _ThemeBlend             = 1.;

        bool _IsThemeTransitionActive = false;
        bool _IsThemeResourceDirty    = true;

        double _ThemeTime  = 0.;
        size_t _ThemeIndex = 0;

        double _NextThemeTransitionTime = ThemeTransitionInterval;
        size_t _NextThemeIndex = 0;

        double _TransitionTime = 1.;

    protected:
        std::vector<std::thread> _Workers;
        std::mutex _WorkerMutex;
        std::condition_variable _WorkerCondition;
        std::condition_variable _CompletionCondition;
        size_t _PendingWorkers = 0;
        size_t _Generation = 0;
        bool _IsStopping = false;

        size_t _MaxWorkers = 8;

    protected:
        ID2D1RenderTarget * _RenderTarget = nullptr;

        ComPtr<ID2D1Factory> _D2DFactory;
        ComPtr<ID2D1LinearGradientBrush> _BackgroundBrush;
        ComPtr<ID2D1SolidColorBrush> _FrameBrush;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;

    private:
        drag_helper_t _DragHelper;

    private:
        ComPtr<ID2D1SolidColorBrush> _ScanlineBrush;
        ComPtr<ID2D1GradientStopCollection> _BackgroundStops;
    };
}
