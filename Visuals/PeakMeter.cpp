
/** $VER: PeakMeter.cpp (2024.03.29) P. Stuer - Represents a peak meter. **/

#include "PeakMeter.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

PeakMeter::PeakMeter()
{
    _Bounds = { };
    _Size = { };

    _FontFamilyName = L"Segoe UI";
    _FontSize = 6.f;

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakMeter::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void PeakMeter::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakMeter::Render(ID2D1RenderTarget * renderTarget, const Analysis & analysis)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    SetTransform(renderTarget);

    const FLOAT BarWidth = _Size.width / 2.f;

    D2D1_RECT_F Rect = { _Bounds.left, 0.f, _Bounds.left + BarWidth - 1.f, 0.f };

    for (const auto & mv : analysis._MeterValues)
    {

        {
            double Value = Clamp((double) mv.Peak, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _Bounds.top, _Bounds.bottom);
            renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
        }

        {
            double Value = Clamp((double) mv.RMS, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _Bounds.top, _Bounds.bottom);
            renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
        }

        Rect.left  += BarWidth;
        Rect.right += BarWidth;
    }

    ResetTransform(renderTarget);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

    if (SUCCEEDED(hr) && (_TextFormat == nullptr))
    {
        hr = _DirectWrite.CreateTextFormat(_FontFamilyName, FontSize, _GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, _TextFormat);

        if (SUCCEEDED(hr))
        {
            auto Text = std::wstring(L"WWW");

            hr = _DirectWrite.GetTextMetrics(_TextFormat, Text, _TextWidth, _TextHeight);
        }
    }

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterPeakLevel, renderTarget, _Size, &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterRMSLevel, renderTarget, _Size, &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionText, renderTarget, _Size, &_TextStyle);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakMeter::ReleaseDeviceSpecificResources()
{
    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }

    if (_RMSStyle)
    {
        _RMSStyle->ReleaseDeviceSpecificResources();
        _RMSStyle = nullptr;
    }

    if (_PeakStyle)
    {
        _PeakStyle->ReleaseDeviceSpecificResources();
        _PeakStyle = nullptr;
    }

    _TextFormat.Release();
}
