
/** $VER: GaugeScales.cpp (2024.05.03) P. Stuer - Implements the gauge scales of the peak meter. **/

#include "pch.h"

#include "GaugeScales.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void GaugeScales::Initialize(state_t * state, const GraphSettings * settings, const analysis_t * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();

    _Labels.clear();

    if (_GraphSettings->_YAxisMode == YAxisMode::None)
        return;

    // Create the labels.
    {
        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

            Label lb = { Text, Amplitude };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance.
/// </summary>
void GaugeScales::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void GaugeScales::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void GaugeScales::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    if (_State->_HorizontalPeakMeter)
    {
        const FLOAT cx = (_TextStyle->GetWidth() / 2.f);

        // Calculate the position of the labels based on the width.
        D2D1_RECT_F OldRect = {  };

        const FLOAT xMin = !_GraphSettings->_FlipHorizontally ? 0.f : GetWidth();
        const FLOAT xMax = !_GraphSettings->_FlipHorizontally ? GetWidth() : 0.f;

        const FLOAT y1 = _GraphSettings->_YAxisLeft  ? _TextStyle->GetHeight() : 0.f;
        const FLOAT y2 = _GraphSettings->_YAxisRight ? GetHeight() - _TextStyle->GetHeight() : GetHeight();

        for (Label & Iter : _Labels)
        {
            FLOAT x = msc::Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., xMin, xMax);

            // Don't generate any labels outside the bounds.
            if (!msc::InRange(x, 0.f, GetWidth()))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.Point1 = D2D1_POINT_2F(x, y1);
            Iter.Point2 = D2D1_POINT_2F(x, y2);

            x -= cx;

            Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;

            if (!_GraphSettings->_XAxisBottom)
            {
                if (!_GraphSettings->_FlipHorizontally && (x <= 0.f))
                {
                    x = 0.f;
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
                }
                else
                if (_GraphSettings->_FlipHorizontally && ((x + _TextStyle->GetWidth()) > GetWidth()))
                {
                    x = GetWidth() - _TextStyle->GetWidth();
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
            }

            if (!_GraphSettings->_XAxisTop)
            {
                if (_GraphSettings->_FlipHorizontally && (x <= 0.f))
                {
                    x = 0.f;
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
                }
                else
                if (!_GraphSettings->_FlipHorizontally && ((x + _TextStyle->GetWidth()) > GetWidth()))
                {
                    x = GetWidth() - _TextStyle->GetWidth();
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
            }

            Iter.Rect1 = { x, 0.f,                                   x + _TextStyle->GetWidth(), _TextStyle->GetHeight() };
            Iter.Rect2 = { x, GetHeight() - _TextStyle->GetHeight(), x + _TextStyle->GetWidth(), GetHeight() };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingHorizontally(Iter.Rect1, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.Rect1;
        }
    }
    else
    {
        const FLOAT cy = _TextStyle->GetHeight() / 2.f;

        // Calculate the position of the labels based on the height.
        D2D1_RECT_F OldRect = {  };

        const FLOAT x1 = _GraphSettings->_YAxisLeft  ? _TextStyle->GetWidth() : 0.f;
        const FLOAT x2 = _GraphSettings->_YAxisRight ? GetWidth() - _TextStyle->GetWidth() : GetWidth();

        const FLOAT yMin = !_GraphSettings->_FlipVertically ? GetHeight() : 0.f;
        const FLOAT yMax = !_GraphSettings->_FlipVertically ? 0.f : GetHeight();

        for (Label & Iter : _Labels)
        {
            FLOAT y = msc::Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., yMin, yMax);

            // Don't generate any labels outside the bounds.
            if (!msc::InRange(y, 0.f, GetHeight()))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.Point1 = D2D1_POINT_2F(x1, y);
            Iter.Point2 = D2D1_POINT_2F(x2, y);

            y -= cy;

            Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;

            if (!_GraphSettings->_XAxisBottom)
            {
                if (_GraphSettings->_FlipVertically && (y <= 0.f))
                {
                    y = 0.f;
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                }
                else
                if (!_GraphSettings->_FlipVertically && ((y + _TextStyle->GetHeight()) > GetHeight()))
                {
                    y = GetHeight() - _TextStyle->GetHeight();
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
            }

            if (!_GraphSettings->_XAxisTop)
            {
                if (!_GraphSettings->_FlipVertically && (y <= 0.f))
                {
                    y = 0.f;
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                }
                else
                if (_GraphSettings->_FlipVertically && ((y + _TextStyle->GetHeight()) > GetHeight()))
                {
                    y = GetHeight() - _TextStyle->GetHeight();
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
            }

            Iter.Rect1 = { 0.f,                                 y, _TextStyle->GetWidth(), y + _TextStyle->GetHeight() };
            Iter.Rect2 = { GetWidth() - _TextStyle->GetWidth(), y, GetWidth(),             y + _TextStyle->GetHeight() };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingVertically(Iter.Rect1, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.Rect1;
        }
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void GaugeScales::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if ((_GraphSettings->_YAxisMode == YAxisMode::None) || (!_GraphSettings->_YAxisLeft && !_GraphSettings->_YAxisRight))
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    if (_State->_HorizontalPeakMeter)
    {
        for (const Label & Iter : _Labels)
        {
            // Draw the vertical grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                _TextStyle->SetHorizontalAlignment(Iter._HAlignment);

                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
    else
    {
        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                _TextStyle->SetVerticalAlignment(Iter._VAlignment);

                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
}

HRESULT GaugeScales::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, Size, L"+999", &_TextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, Size, L"", &_LineStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeScales::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_LineStyle)
    {
        _LineStyle->ReleaseDeviceSpecificResources();
        _LineStyle = nullptr;
    }

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}
