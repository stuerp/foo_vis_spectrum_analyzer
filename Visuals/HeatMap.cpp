
/** $VER: HeatMap.cpp (2024.03.15) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "HeatMap.h"

#include "Support.h"

#pragma hdrstop

HeatMap::HeatMap()
{
    _Size = { };
    _X = 0.;
}

/// <summary>
/// Initializes this instance.
/// </summary>
void HeatMap::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void HeatMap::Move(const D2D1_RECT_F & rect)
{
    _Size= rect;
}

/// <summary>
/// Renders the spectrum analysis as a heat map.
/// </summary>
void HeatMap::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        const FLOAT Bandwidth = Max((_Size.height / (FLOAT) frequencyBands.size()), 1.f);

        D2D1_RECT_F Rect = { (FLOAT) _X, 0.f, (FLOAT) _X + 4.f, Bandwidth };

        for (const auto & fb : frequencyBands)
        {
            double Amplitude = _GraphSettings->ScaleA(fb.CurValue);

            if (Amplitude > 0.0)
            {
                _BarArea->SetBrushColor(Amplitude);

                renderTarget->FillRectangle(Rect, _BarArea->_Brush);
            }

            Rect.top    = ::round(Rect.bottom);
            Rect.bottom = Rect.top  + Bandwidth;
        }

        _X += 4.;

        if (_X > (double) _Size.width)
            _X = 0.;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT HeatMap::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_RenderTarget == nullptr))
        hr = _RenderTarget->CreateCompatibleRenderTarget(_Size, &_RenderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void HeatMap::ReleaseDeviceSpecificResources()
{
    _RenderTarget.Release();
}
