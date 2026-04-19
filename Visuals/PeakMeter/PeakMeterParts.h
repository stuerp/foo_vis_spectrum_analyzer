
/** $VER: PeakMeterParts.h (2026.04.19) P. Stuer - Defines the various parts of a peak meter. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

#include "Analysis.h"
#include "Style.h"

/// <summary>
/// Implements a base class for the peak meter parts.
/// </summary>
class part_t
{
public:
    part_t(const state_t * state, const graph_description_t * settings) noexcept : _Rect()
    {
        _State    = state;
        _GraphDescription = settings;
    }

    virtual ~part_t() = default;

    virtual void SetRect(const D2D1_RECT_F & rect) noexcept;

    virtual void Bind(ID2D1DeviceContext * deviceContext, style_t * backgroundStyle, style_t * peakStyle, style_t * peak0dBStyle, style_t * maxPeakStyle, style_t * peakTextStyle, style_t * rmsStyle, style_t * rms0dBStyle, style_t * rmsTextStyle, style_t * nameStyle, style_t * scaleTextStyle, style_t * scaleLineStyle, ID2D1SolidColorBrush * debugBrush, ID2D1Bitmap * opacityMask) noexcept;
    virtual void Unbind() noexcept;

    virtual void Render() const noexcept = 0;

    virtual FLOAT Width() const noexcept { return 0.f; }
    virtual FLOAT Height() const noexcept { return 0.f; }

private:
    void CreateAxis() noexcept;

protected:
    const state_t * _State;
    const graph_description_t * _GraphDescription;

    D2D1_RECT_F _Rect;
    D2D1_SIZE_F _Size;

    D2D1_RECT_F _TopNameRect;
    D2D1_RECT_F _BottomNameRect;
    D2D1_RECT_F _PeakRect;
    D2D1_RECT_F _RMSRect;

    struct label_t
    {
        std::wstring Text;
        double Amplitude;
        bool IsHidden;

        D2D1_POINT_2F P1; // Start coord. of left tick
        D2D1_POINT_2F P2; // End coord. of left tick
        D2D1_RECT_F Rect;
    };

    std::vector<label_t> _Labels;

    DWRITE_TEXT_ALIGNMENT _TextAlignment;
    DWRITE_PARAGRAPH_ALIGNMENT _ParagraphAlignment;

    CComPtr<ID2D1DeviceContext> _DeviceContext;

    const FLOAT _TickSize = 4.f;

    style_t * _BackgroundStyle;

    style_t * _PeakStyle;
    style_t * _Peak0dBStyle;
    style_t * _MaxPeakStyle;
    style_t * _PeakTextStyle;

    style_t * _RMSStyle;
    style_t * _RMS0dBStyle;
    style_t * _RMSTextStyle;

    style_t * _NameStyle;

    style_t * _ScaleTextStyle;
    style_t * _ScaleLineStyle;

    CComPtr<IDWriteTextLayout> _NameTextLayout;

    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
    CComPtr<ID2D1Bitmap> _OpacityMask;
};

/// <summary>
/// Represents a meter bar of a peak meter.
/// </summary>
class bar_t : public part_t
{
public:
    bar_t(const state_t * state, const graph_description_t * settings, const peak_measurement_t * measurement) noexcept : part_t(state, settings)
    {
        _Measurement = measurement;

        _dBFSZeroNormalized = msc::Map(0., _GraphDescription->_AmplitudeLo, _GraphDescription->_AmplitudeHi, 0., 1.);
    }

    bar_t(const bar_t &) = delete;
    bar_t & operator=(const bar_t &) = delete;
    bar_t(bar_t &&) = delete;
    bar_t & operator=(bar_t &&) = delete;

    void Unbind() noexcept override final;

    void SetRect(const D2D1_RECT_F & rect) noexcept override final;
    void Render() const noexcept override final;

private:
    HRESULT CreateScaleLinesCommandList() noexcept;

    void DrawHorizontalRectangle(D2D1_RECT_F & rect, const style_t * style) const noexcept;
    void DrawVerticalRectangle(D2D1_RECT_F & rect, const style_t * style) const noexcept;

private:
    const peak_measurement_t * _Measurement;
    double _dBFSZeroNormalized;

    D2D1_MATRIX_3X2_F _Transform;

    FLOAT _LEDSize;

    CComPtr<ID2D1CommandList> _ScaleLinesCommandList;
};

/// <summary>
/// Represents a meter scale of a peak meter.
/// </summary>
class scale_t : public part_t
{
public:
    scale_t(const state_t * state, const graph_description_t * settings, DWRITE_TEXT_ALIGNMENT textAlignment, DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment) noexcept : part_t(state, settings)
    {
        _TextAlignment      = textAlignment;
        _ParagraphAlignment = paragraphAlignment;
    }

    scale_t(const scale_t &) = delete;
    scale_t & operator=(const scale_t &) = delete;
    scale_t(scale_t &&) = delete;
    scale_t & operator=(scale_t &&) = delete;

    virtual void Unbind() noexcept override final;

    void SetRect(const D2D1_RECT_F & rect) noexcept override final;
    void Render() const noexcept override final;

    virtual FLOAT Width() const noexcept { return _Size.width; }
    virtual FLOAT Height() const noexcept { return _Size.height; }

    bool IsCenter() const noexcept { return (_TextAlignment == DWRITE_TEXT_ALIGNMENT_CENTER) && (_ParagraphAlignment == DWRITE_PARAGRAPH_ALIGNMENT_CENTER); } // True if this scale is drawn between the bars.

private:
    HRESULT CreateAxisCommandList() noexcept;

private:
    CComPtr<ID2D1CommandList> _AxisCommandList;
};
