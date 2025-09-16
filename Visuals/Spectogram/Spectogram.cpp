
/** $VER: Spectogram.cpp (2024.05.01) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "pch.h"
#include "Spectogram.h"

#include "Support.h"

#include "DirectWrite.h"

#include "Log.h"

#pragma hdrstop

Spectogram::Spectogram()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Spectogram::Initialize(state_t * state, const GraphSettings * settings, const analysis_t * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();

    InitFreqAxis();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Spectogram::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

    _Bitmap.Release();
    _BitmapRenderTarget.Release();
}

/// <summary>
/// Resets this instance.
/// </summary>
void Spectogram::Reset()
{
    _X = 0.f;
    _Y = 0.f;
    _PlaybackTime = -1.;
    _TrackTime = -1.;

    _TimeLabels.clear();

    _Bitmap.Release();
    _BitmapRenderTarget.Release();

    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void Spectogram::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    // Resize the offscreen bitmap. Compensate for the axes.
    {
        _BitmapBounds = _Bounds;

        if (_State->_HorizontalSpectogram)
        {
            if (_GraphSettings->_XAxisTop)
                _BitmapBounds.top += _TimeTextStyle->_Height;

            if (_GraphSettings->_XAxisBottom)
                _BitmapBounds.bottom -= _TimeTextStyle->_Height;

            if (_GraphSettings->_YAxisLeft)
                _BitmapBounds.left += _FreqTextStyle->_Width;

            if (_GraphSettings->_YAxisRight)
                _BitmapBounds.right -= _FreqTextStyle->_Width;
        }
        else
        {
            if (_GraphSettings->_XAxisTop)
                _BitmapBounds.right -= _TimeTextStyle->_Width;

            if (_GraphSettings->_XAxisBottom)
                _BitmapBounds.left += _TimeTextStyle->_Width;

            if (_GraphSettings->_YAxisLeft)
                _BitmapBounds.top += _FreqTextStyle->_Height;

            if (_GraphSettings->_YAxisRight)
                _BitmapBounds.bottom -= _FreqTextStyle->_Height;
        }

        _BitmapSize = { _BitmapBounds.right - _BitmapBounds.left, _BitmapBounds.bottom - _BitmapBounds.top };
    }

    const FLOAT Bandwidth = std::max(::floor(_BitmapSize.width / (FLOAT) _Analysis->_FrequencyBands.size()), 2.f); // In DIP
    const FLOAT SpectrumWidth = Bandwidth * (FLOAT) _Analysis->_FrequencyBands.size();

    // Resize the offscreen bitmap. Compensate for the spectrum bar rounding.
    {
        if (_State->_UseSpectrumBarMetrics && !_State->_HorizontalSpectogram)
        {
            const FLOAT dx = (_BitmapSize.width - SpectrumWidth) / 2.f;

            _BitmapBounds.left  += dx;
            _BitmapBounds.right -= dx;

            _BitmapSize = { _BitmapBounds.right - _BitmapBounds.left, _BitmapBounds.bottom - _BitmapBounds.top };
        }
    }

    // Resize the frequency axis.
    {
        const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
        const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

        rect_t Rect = { };

        if (_State->_HorizontalSpectogram)
        {
            const FLOAT y1 = (_GraphSettings->_XAxisTop ? _TimeTextStyle->_Height : 0.f) - (_FreqTextStyle->_Height / 2.f);

            for (auto & Iter : _FreqLabels)
            {
                const FLOAT y = msc::Map(ScaleF(Iter.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, _BitmapSize.height);

                if (!_GraphSettings->_FlipVertically)
                {
                    Rect.y1 = y1 + _BitmapSize.height - y;
                    Rect.y2 = Rect.y1 + _FreqTextStyle->_Height;
                }
                else
                {
                    Rect.y1 = y1 + y;
                    Rect.y2 = Rect.y1 + _FreqTextStyle->_Height;
                }

                Iter.Rect1 = Rect;

                Iter.Rect1.left  = 0.f;
                Iter.Rect1.right = _FreqTextStyle->_Width - Offset;
  
                Iter.Rect2 = Rect;

                Iter.Rect2.left  = _Size.width - _FreqTextStyle->_Width + Offset;
                Iter.Rect2.right = _Size.width;

                Iter.IsHidden = (Rect.y2 < _BitmapBounds.top) || (Rect.y1 > _BitmapBounds.bottom);
            }

            if (_FreqLabels.size() > 2)
            {
                #define NotesMode (_GraphSettings->_XAxisMode == XAxisMode::Notes)

                const FreqLabel * Anchor = &_FreqLabels[0];

                // Determine which labels should be hidden.
                for (size_t i = 1; i < _FreqLabels.size() - 1; ++i)
                {
                    if (_FreqLabels[i].IsHidden)
                        continue;

                    // Don't render the label if it overlaps with the anchor label.
                    if (IsOverlappingVertically(_FreqLabels[i].Rect1, Anchor->Rect1))
                    {
                        _FreqLabels[i].IsHidden = true;
                        continue;
                    }

                    // Don't render the label if it overlaps with the next label and that label is a major label.
                    if (IsOverlappingVertically(_FreqLabels[i].Rect1, _FreqLabels[i + 1].Rect1) && (NotesMode && !_FreqLabels[i + 1].IsMinor))
                    {
                        _FreqLabels[i].IsHidden = true;
                        continue;
                    }

                    Anchor = &_FreqLabels[i];
                }
            }
        }
        else
        {
            for (auto & Iter : _FreqLabels)
            {
                const FLOAT x = msc::Map(ScaleF(Iter.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, _BitmapSize.width);

                {
                    CComPtr<IDWriteTextLayout> TextLayout;

                    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(Iter.Text.c_str(), (UINT) Iter.Text.size(), _FreqTextStyle->_TextFormat, _Size.width, _Size.height, &TextLayout);

                    if (SUCCEEDED(hr))
                    {
                        DWRITE_TEXT_METRICS TextMetrics = { };

                        TextLayout->GetMetrics(&TextMetrics);

                        if (!_GraphSettings->_FlipHorizontally)
                        {
                            Rect.x1 = _BitmapBounds.left + x - (TextMetrics.width / 2.f);
                            Rect.x2 = Rect.x1 + TextMetrics.width;
                        }
                        else
                        {
                            Rect.x2 = _BitmapBounds.right - x + (TextMetrics.width / 2.f);
                            Rect.x1 = Rect.x2 - TextMetrics.width;
                        }
                    }
                }

                Iter.Rect1 = Rect;

                Iter.Rect1.top    = 0.f;
                Iter.Rect1.bottom = _FreqTextStyle->_Height;

                Iter.Rect2 = Rect;

                Iter.Rect2.bottom = _Size.height;
                Iter.Rect2.top    = Iter.Rect2.bottom - _FreqTextStyle->_Height;

                Iter.IsHidden = (Rect.x2 < _BitmapBounds.left) || (Rect.x1 > _BitmapBounds.right);
            }

            if (_FreqLabels.size() > 2)
            {
                #define NotesMode (_GraphSettings->_XAxisMode == XAxisMode::Notes)

                const FreqLabel * Anchor = &_FreqLabels[0];

                // Determine which labels should be hidden.
                for (size_t i = 1; i < _FreqLabels.size() - 1; ++i)
                {
                    if (_FreqLabels[i].IsHidden)
                        continue;

                    // Don't render the label if it overlaps with the anchor label.
                    if (IsOverlappingHorizontally(_FreqLabels[i].Rect1, Anchor->Rect1))
                    {
                        _FreqLabels[i].IsHidden = true;
                        continue;
                    }

                    // Don't render the label if it overlaps with the next label and that label is a major label.
                    if (IsOverlappingHorizontally(_FreqLabels[i].Rect1, _FreqLabels[i + 1].Rect1) && (NotesMode && !_FreqLabels[i + 1].IsMinor))
                    {
                        _FreqLabels[i].IsHidden = true;
                        continue;
                    }

                    Anchor = &_FreqLabels[i];
                }
            }
        }
    }

    _IsResized = false;
}

/// <summary>
/// Renders the spectrum analysis as a spectogram.
/// </summary>
void Spectogram::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    // Update the offscreen bitmap.
    if (!Update())
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Draw the offscreen bitmap.
    if (_State->_HorizontalSpectogram)
    {
        SetTransform(renderTarget, _BitmapBounds);

        if (_State->_ScrollingSpectogram)
        {
            // Render the new lines.
            D2D1_RECT_F Src = D2D1_RECT_F( _X, 0.f, _BitmapSize.width,      _BitmapSize.height);
            D2D1_RECT_F Dst = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width - _X, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

            Src.right = Src.left;
            Src.left  = 0.f;

            Dst.left  = Dst.right;
            Dst.right = _BitmapSize.width;

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
        }
        else
        {
            D2D1_RECT_F Rect = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Rect, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        }

        ResetTransform(renderTarget);

        // Draw the Time axis.
        {
            if (_GraphSettings->_XAxisTop)
            {
                if (!_TimeLabels.empty())
                    RenderTimeAxis(renderTarget, true);

                renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

                renderTarget->DrawLine({ _BitmapBounds.left,  _BitmapBounds.top }, { _BitmapBounds.left,  _BitmapBounds.bottom }, _FreqLineStyle->_Brush, _FreqLineStyle->_Thickness);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                if (!_TimeLabels.empty())
                    RenderTimeAxis(renderTarget, false);
    
                renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

                renderTarget->DrawLine({ _BitmapBounds.right, _BitmapBounds.top }, { _BitmapBounds.right, _BitmapBounds.bottom }, _FreqLineStyle->_Brush, _FreqLineStyle->_Thickness);
            }
        }

        // Draw the Frequency axis.
        if (!_FreqLabels.empty())
        {
            if (_GraphSettings->_YAxisLeft)
                RenderFreqAxis(renderTarget, true);

            if (_GraphSettings->_YAxisRight)
                RenderFreqAxis(renderTarget, false);
        }
    }
    else
    {
        SetTransform(renderTarget, _BitmapBounds);

        if (_State->_ScrollingSpectogram)
        {
            // Render the new lines.
            D2D1_RECT_F Src = D2D1_RECT_F(0.f,  _Y, _BitmapSize.width, _BitmapSize.height);
            D2D1_RECT_F Dst = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width, _BitmapSize.height - _Y);

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

            // Render the old lines.
            Src.bottom = Src.top;
            Src.top    = 0.f;

            Dst.top    = Dst.bottom;
            Dst.bottom = _BitmapSize.height;

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
        }
        else
        {
            D2D1_RECT_F Rect = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Rect, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        }

        ResetTransform(renderTarget);

        // Draw the Time axis.
        {
            if (_GraphSettings->_XAxisTop)
            {
                if (!_TimeLabels.empty())
                    RenderTimeAxis(renderTarget, true);

                renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

                renderTarget->DrawLine({ _BitmapBounds.left,  _BitmapBounds.top }, { _BitmapBounds.left,  _BitmapBounds.bottom }, _FreqLineStyle->_Brush, _FreqLineStyle->_Thickness);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                if (!_TimeLabels.empty())
                    RenderTimeAxis(renderTarget, false);
    
                renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

                renderTarget->DrawLine({ _BitmapBounds.right, _BitmapBounds.top }, { _BitmapBounds.right, _BitmapBounds.bottom }, _FreqLineStyle->_Brush, _FreqLineStyle->_Thickness);
            }
        }

        // Draw the Frequency axis.
        {
            if (_GraphSettings->_YAxisLeft)
            {
                if (!_FreqLabels.empty())
                    RenderFreqAxis(renderTarget, true);
            }

            if (_GraphSettings->_YAxisRight)
            {
                if (!_FreqLabels.empty())
                    RenderFreqAxis(renderTarget, false);
            }
        }
    }

    if (_State->_PlaybackTime != _PlaybackTime) // Not paused
    {
        if (_State->_HorizontalSpectogram)
        {
            _X++;

            if (_X > _BitmapSize.width)
            {
                _X = 0.f;

                if (!_State->_ScrollingSpectogram)
                    _TimeLabels.clear();
            }
        }
        else
        {
            _Y++;

            if (_Y > _BitmapSize.height)
            {
                _Y = 0.f;

                if (!_State->_ScrollingSpectogram)
                    _TimeLabels.clear();
            }
        }

        _PlaybackTime = _State->_PlaybackTime;
    }
}

/// <summary>
/// Renders an X-axis (Time)
/// </summary>
void Spectogram::RenderTimeAxis(ID2D1RenderTarget * renderTarget, bool first) const noexcept
{
    if (_State->_HorizontalSpectogram)
    {
        const FLOAT y1 = first ? 0.f : _Size.height - _TimeTextStyle->_Height;
        const FLOAT y2 = first ? _TimeTextStyle->_Height : _Size.height;

        rect_t Rect = { 0.f, first ? 0.f : y1, 0.f, first ? y2 : _Size.height };

        renderTarget->PushAxisAlignedClip({ _BitmapBounds.left, y1, _BitmapBounds.right, y2 }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        for (const auto & Label : _TimeLabels)
        {
            const FLOAT x = !_GraphSettings->_FlipHorizontally ? _BitmapBounds.left + Label.X : _BitmapBounds.right - Label.X;

            // Draw the tick.
            renderTarget->DrawLine( { x, y1 }, { x, y2 }, _TimeLineStyle->_Brush, _TimeLineStyle->_Thickness);

            if (!_GraphSettings->_FlipHorizontally)
            {
                Rect.x1 = x + Offset;
                Rect.x2 = Rect.x1 + _TimeTextStyle->_Width;
            }
            else
            {
                Rect.x1 = x - Offset;
                Rect.x2 = Rect.x1 - _TimeTextStyle->_Width;
            }

            // Draw the label.
            _TimeTextStyle->SetHorizontalAlignment(_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _TimeTextStyle->_TextFormat, Rect, _TimeTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        renderTarget->PopAxisAlignedClip();
    }
    else
    {
        const FLOAT x1 = first ? _BitmapBounds.right                          : _BitmapBounds.left - _TimeTextStyle->_Width;
        const FLOAT x2 = first ? _BitmapBounds.right + _TimeTextStyle->_Width : _BitmapBounds.left;

        rect_t Rect = { x1, 0.f, x2, 0.f };

        renderTarget->PushAxisAlignedClip({ x1, _BitmapBounds.top, x2, _BitmapBounds.bottom }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        for (const auto & Label : _TimeLabels)
        {
            const FLOAT y = !_GraphSettings->_FlipVertically ? _BitmapBounds.top - _TimeTextStyle->_Height + Label.Y : _BitmapBounds.top + Label.Y;

            // Draw the tick.
            renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
            renderTarget->DrawLine( { x1, y }, { x2, y }, _TimeLineStyle->_Brush, _TimeLineStyle->_Thickness);

            if (!_GraphSettings->_FlipVertically)
            {
                Rect.y2 = y;
                Rect.y1 = Rect.y2 - _TimeTextStyle->_Height;
            }
            else
            {
                Rect.y2 = y;
                Rect.y1 = Rect.y2 + _TimeTextStyle->_Height;
            }

            // Draw the label.
            _TimeTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

            renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _TimeTextStyle->_TextFormat, Rect, _TimeTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        renderTarget->PopAxisAlignedClip();
    }
}

/// <summary>
/// Renders a Y-axis (Frequency)
/// </summary>
void Spectogram::RenderFreqAxis(ID2D1RenderTarget * renderTarget, bool left) const noexcept
{
    const FLOAT Opacity = _FreqTextStyle->_Brush->GetOpacity();

    if (_State->_HorizontalSpectogram)
        _FreqTextStyle->SetHorizontalAlignment(left? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
    else
        _FreqTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    for (const auto & Iter : _FreqLabels)
    {
        if (Iter.IsHidden)
            continue;

        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _FreqTextStyle->_Brush->SetOpacity(Iter.IsMinor ? Opacity * .5f : Opacity);

        if (left)
            renderTarget->DrawTextW(Iter.Text.c_str(), (UINT32) Iter.Text.size(), _FreqTextStyle->_TextFormat, Iter.Rect1, _FreqTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        else
            renderTarget->DrawTextW(Iter.Text.c_str(), (UINT32) Iter.Text.size(), _FreqTextStyle->_TextFormat, Iter.Rect2, _FreqTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    if (left)
        renderTarget->DrawLine({ _BitmapBounds.left, _BitmapBounds.top },    { _BitmapBounds.right, _BitmapBounds.top },    _TimeLineStyle->_Brush, _TimeLineStyle->_Thickness);
    else
        renderTarget->DrawLine({ _BitmapBounds.left, _BitmapBounds.bottom }, { _BitmapBounds.right, _BitmapBounds.bottom }, _TimeLineStyle->_Brush, _TimeLineStyle->_Thickness);

    _FreqTextStyle->_Brush->SetOpacity(Opacity);
}

/// <summary>
/// Updates this instance.
/// </summary>
bool Spectogram::Update() noexcept
{
    if (_Analysis->_NyquistFrequency == 0.f)
        return false;

    _BitmapRenderTarget->BeginDraw();

    if (_State->_HorizontalSpectogram)
    {
        // Draw the next spectogram line.
        {
            const FLOAT Bandwidth = _BitmapSize.height / (FLOAT) _BandCount;

            FLOAT y1 = 0.f;
            FLOAT y2 = Bandwidth;

            for (const auto & fb : _Analysis->_FrequencyBands)
            {
                if ((fb.Ctr >= _Analysis->_NyquistFrequency) && _State->_SuppressMirrorImage)
                    break;

                assert(msc::InRange(fb.CurValue, 0.0, 1.0));

                _SpectogramStyle->SetBrushColor(fb.CurValue);

                _BitmapRenderTarget->DrawLine({ _X, y1 }, { _X, y2 }, _SpectogramStyle->_Brush);

                y1  = y2;
                y2 += Bandwidth;
            }
        }

        // Draw the Nyquist marker.
        if (_NyquistMarkerStyle->IsEnabled())
            RenderNyquistFrequencyMarker(_BitmapRenderTarget);

        _BitmapRenderTarget->EndDraw();

        // Update the time axis.
        if (_State->_ScrollingSpectogram && (_State->_PlaybackTime != _PlaybackTime))
        {
            for (auto & Label : _TimeLabels)
            {
                if (!_GraphSettings->_FlipHorizontally)
                    Label.X--; // Scroll to the left.
                else
                    Label.X++; // Scroll to the right.
            }
        }

        if (_TrackTime != _State->_TrackTime) // in seconds
        {
            if (_State->_ScrollingSpectogram)
            {
                _TimeLabels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) _State->_TrackTime)), _GraphSettings->_FlipHorizontally ? 0.f : _BitmapSize.width });

                if (_TimeLabels.back().X + _TimeTextStyle->_Width < 0.f)
                    _TimeLabels.pop_back();
            }
            else
                _TimeLabels.push_back({ pfc::wideFromUTF8(pfc::format_time((uint64_t) _State->_TrackTime)), _GraphSettings->_FlipHorizontally ? _BitmapSize.width - _X : _X });

            _TrackTime = _State->_TrackTime;
        }
    }
    else
    {
        // Draw the next spectogram line.
        {
            const FLOAT Bandwidth = _State->_UseSpectrumBarMetrics ? std::max(::floor(_BitmapSize.width / (FLOAT) _Analysis->_FrequencyBands.size()), 2.f) : _BitmapSize.width / (FLOAT) _BandCount;
            const FLOAT SpectrumWidth = Bandwidth * (FLOAT) _Analysis->_FrequencyBands.size();

            FLOAT x1 = _State->_UseSpectrumBarMetrics ? (_BitmapSize.width - SpectrumWidth) / 2.f : 0.f;
            FLOAT x2 = Bandwidth;

            for (const auto & fb : _Analysis->_FrequencyBands)
            {
                if ((fb.Ctr >= _Analysis->_NyquistFrequency) && _State->_SuppressMirrorImage)
                    break;

                assert(msc::InRange(fb.CurValue, 0.0, 1.0));

                _SpectogramStyle->SetBrushColor(fb.CurValue);

                _BitmapRenderTarget->DrawLine({ x1, _Y }, { x2, _Y }, _SpectogramStyle->_Brush);

                x1  = x2;
                x2 += Bandwidth;
            }
        }

        // Draw the Nyquist marker.
        if (_NyquistMarkerStyle->IsEnabled())
            RenderNyquistFrequencyMarker(_BitmapRenderTarget);

        _BitmapRenderTarget->EndDraw();

        // Update the time axis.
        if (_State->_ScrollingSpectogram && (_State->_PlaybackTime != _PlaybackTime))
        {
            for (auto & Label : _TimeLabels)
            {
                if (!_GraphSettings->_FlipVertically)
                    Label.Y++; // Scroll to the bottom.
                else
                    Label.Y--; // Scroll to the top.
            }
        }

        if (_TrackTime != _State->_TrackTime) // in seconds
        {
            if (_State->_ScrollingSpectogram)
            {
                _TimeLabels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) _State->_TrackTime)), 0.f, !_GraphSettings->_FlipVertically ? _BitmapBounds.top : _BitmapSize.height });

                if (_TimeLabels.back().Y > _BitmapSize.height + _TimeTextStyle->_Height)
                    _TimeLabels.pop_back();
            }
            else
                _TimeLabels.push_back({ pfc::wideFromUTF8(pfc::format_time((uint64_t) _State->_TrackTime)), 0.f, !_GraphSettings->_FlipVertically ? _BitmapSize.height - _Y : _Y });

            _TrackTime = _State->_TrackTime;
        }
    }

    return true;
}

/// <summary>
/// Renders a marker for the Nyquist frequency.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectogram::RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget) const noexcept
{
    const double MinScale = ScaleF(_Analysis->_FrequencyBands.front().Ctr, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_Analysis->_FrequencyBands.back() .Ctr, _State->_ScalingFunction, _State->_SkewFactor);

    const double NyquistScale = std::clamp(ScaleF(_Analysis->_NyquistFrequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale);

    if (_State->_HorizontalSpectogram)
    {
        const FLOAT y = msc::Map(NyquistScale, MinScale, MaxScale, 0.f, _BitmapSize.height);

        renderTarget->DrawLine(D2D1_POINT_2F(_X, y), D2D1_POINT_2F(_X, y + 1), _NyquistMarkerStyle->_Brush, _NyquistMarkerStyle->_Thickness, nullptr);
    }
    else
    {
        const FLOAT x = msc::Map(NyquistScale, MinScale, MaxScale, 0.f, _BitmapSize.width);

        renderTarget->DrawLine(D2D1_POINT_2F(x, _Y), D2D1_POINT_2F(x + 1, _Y), _NyquistMarkerStyle->_Brush, _NyquistMarkerStyle->_Thickness, nullptr);
    }
}

/// <summary>
/// Initializes the Y-axis.
/// </summary>
void Spectogram::InitFreqAxis() noexcept
{
    _FreqLabels.clear();

    const FrequencyBands & fb = _Analysis->_FrequencyBands;

    if (fb.size() == 0)
        return;

    _BandCount = fb.size();

    _LoFrequency = fb.front().Ctr;
    _HiFrequency = fb.back().Ctr;

    // Precalculate the labels.
    {
        WCHAR Text[32] = { };

        switch (_GraphSettings->_XAxisMode)
        {
            case XAxisMode::None:
                break;

            default:

            case XAxisMode::Bands:
            {
                for (size_t i = 0; i < fb.size(); i += 10)
                {
                    double Frequency = fb[i].Ctr;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    FreqLabel lb = { Text, Frequency };

                    _FreqLabels.push_back(lb);
                }
                break;
            }

            case XAxisMode::Decades:
            {
                double Frequency = 0.;
                int i = 1;
                int j = 10;

                while (Frequency < fb.back().Lo)
                {
                    Frequency = j * i;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    FreqLabel lb = { Text, Frequency };

                    _FreqLabels.push_back(lb);

                    if (++i == 10)
                    {
                        i = 1;
                        j *= 10;
                    }
                }
                break;
            }

            case XAxisMode::Octaves:
            {
                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                for (int i = 0; Frequency < fb.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    FreqLabel lb = { Text, Frequency };

                    _FreqLabels.push_back(lb);

                    Note += 12.;
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);
                }
                break;
            }

            case XAxisMode::Notes:
            {
                static const char Name[] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
                static const int Step[] = { 2, 2, 1, 2, 2, 2, 1 };

                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                int j = 0;

                while (Frequency < fb.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    FreqLabel lb = { Text, Frequency, j != 0 };

                    _FreqLabels.push_back(lb);

                    Note += Step[j];
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);

                    if (j < 6) j++; else j = 0;
                }
                break;
            }
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT Spectogram::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Spectogram, renderTarget, Size, L"", &_SpectogramStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, renderTarget, Size, L"", &_TimeLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, Size, L"00:00", &_TimeTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, Size, L"", &_FreqLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, Size, L"99.9fk", &_FreqTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::NyquistMarker, renderTarget, Size, L"", &_NyquistMarkerStyle);

    if (SUCCEEDED(hr))
        Resize();

    // Create the offscreen bitmap resources.
    {
        if (SUCCEEDED(hr) && (_BitmapRenderTarget == nullptr))
            hr = renderTarget->CreateCompatibleRenderTarget(_BitmapSize, &_BitmapRenderTarget);

        if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        {
            _BitmapRenderTarget->BeginDraw();
            _BitmapRenderTarget->Clear(); // Make the bitmap completely transparent.
            _BitmapRenderTarget->EndDraw();

//          _BitmapRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE); // Don't use: This causes ghost images in the final output.
            _BitmapRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

            hr = _BitmapRenderTarget->GetBitmap(&_Bitmap);
        }
    }

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.f,1.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectogram::ReleaseDeviceSpecificResources()
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    _Bitmap.Release();
    _BitmapRenderTarget.Release();

    if (_NyquistMarkerStyle)
    {
        _NyquistMarkerStyle->ReleaseDeviceSpecificResources();
        _NyquistMarkerStyle = nullptr;
    }

    if (_FreqTextStyle)
    {
        _FreqTextStyle->ReleaseDeviceSpecificResources();
        _FreqTextStyle = nullptr;
    }

    if (_FreqLineStyle)
    {
        _FreqLineStyle->ReleaseDeviceSpecificResources();
        _FreqLineStyle = nullptr;
    }

    if (_TimeTextStyle)
    {
        _TimeTextStyle->ReleaseDeviceSpecificResources();
        _TimeTextStyle = nullptr;
    }

    if (_TimeLineStyle)
    {
        _TimeLineStyle->ReleaseDeviceSpecificResources();
        _TimeLineStyle = nullptr;
    }

    if (_SpectogramStyle)
    {
        _SpectogramStyle->ReleaseDeviceSpecificResources();
        _SpectogramStyle = nullptr;
    }
}
