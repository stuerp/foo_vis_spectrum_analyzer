
/** $VER: Goniometer.h (2026.09.23) P. Stuer - Implements a goniometer. **/

#pragma once

#include <pch.h>

#include "Visualization.h"
#include "AudioProcessor.h"

class goniometer_t : public visualization_t
{
public:
    goniometer_t();

    goniometer_t(const goniometer_t &) = delete;
    goniometer_t & operator=(const goniometer_t &) = delete;
    goniometer_t(goniometer_t &&) = delete;
    goniometer_t & operator=(goniometer_t &&) = delete;

    virtual ~goniometer_t();

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept override final;
    void Reset() noexcept override final;
    void Release() noexcept override final;

    void OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept override final;

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device> d3dDevice, CComPtr<ID3D11DeviceContext> d3dDeviceContext) noexcept;
    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteSizeDependentResources() noexcept;

    HRESULT CreateSprites() noexcept;
    HRESULT ClearBitmaps() noexcept;
    HRESULT CreatePointSprite(ComPtr<ID2D1Bitmap1> & bitmap) noexcept;
    HRESULT CreateStaticContent() noexcept;

    double _LowBand  = 0.; // Hz
    double _HighBand = 0.; // Hz

    static constexpr FLOAT Radius = .98f;

    static constexpr FLOAT SpriteRadius = 2.5f;
    static constexpr D2D1_RECT_U SpriteRectangle = { 0u, 0u, (UINT32) (SpriteRadius * 2.f), (UINT32) (SpriteRadius * 2.f) };

    size_t _PrevBitmapIndex = 1; // Start drawing in bitmap 0.

    FLOAT _Side  = 0.f;
    D2D1_RECT_F _DestinationRectangle = { };
    D2D1::Matrix3x2F _TranslationMatrix;

    style_t _SignalStyle;
    style_t _StaticTextStyle;
    style_t _StaticLinesStyle;

    // Device independent resources
    ComPtr<ID2D1StrokeStyle1> _StaticStrokeStyle;

    // Device dependent resources
#ifdef _DEBUG
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    ComPtr<ID2D1DeviceContext3> _DeviceContext; // Device context used to render to the back buffers.

    ComPtr<ID2D1Effect> _OpacityEffect;
    ComPtr<ID2D1Effect> _BlurEffect;

    ComPtr<ID2D1Bitmap1> _Sprite;
    ComPtr<ID2D1SpriteBatch> _SpriteBatch;

    std::vector<D2D1_RECT_F> _SpriteDestinations;
    std::vector<D2D1_RECT_U> _SpriteSources;
    std::vector<D2D1_COLOR_F> _SpriteColors;
    std::vector<D2D1_MATRIX_3X2_F> _SpriteTransforms;

    // Device dependent resources (Size dependent)
    ComPtr<ID2D1Bitmap1> _Bitmaps[2];
    ComPtr<ID2D1CommandList> _StaticContent;

    audio_processor_t _AudioProcessor;
};
