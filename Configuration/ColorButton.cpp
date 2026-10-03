
/** $VER: ColorButton.cpp (2026.10.03) P. Stuer - Implements a color control that can display gradients. **/

#include "pch.h"

#include "ColorButton.h"
#include "ColorDialog.h"

#include "Theme.h"
#include "Color.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void color_button_t::Initialize(HWND hWnd) noexcept
{
    if (_IsSubclassed)
        return;

    ATLASSERT(::IsWindow(hWnd));

    __super::_hWnd = hWnd;

    _IsSubclassed = SubclassWindow(hWnd);

    if (!_IsSubclassed)
        return;

    CreateDeviceIndependentResources();

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Terminates the control.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void color_button_t::Terminate() noexcept
{
    if (!IsWindow() || !_IsSubclassed)
        return;

    _Capture.IsActive = false;

    DeleteDeviceSpecificResources();
    DeleteDeviceIndependentResources();

    UnsubclassWindow(TRUE);
    _IsSubclassed = false;
}

/// <summary>
/// Sets the color scheme.
/// </summary>
void color_button_t::SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops) noexcept
{
    _GradientStops = gradientStops;

    _Brush.Reset();

    EnableWindow(gradientStops.size() > 0);

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Sets the color.
/// </summary>
void color_button_t::SetColor(COLORREF color) noexcept
{
    SetColor(color_t::ToD2D1_COLOR_F(color));
}

/// <summary>
/// Sets the color.
/// </summary>
void color_button_t::SetColor(const D2D1_COLOR_F & color) noexcept
{
    _Color = color;
    _GradientStops.clear();

    _Brush.Reset();

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Gets the color.
/// </summary>
void color_button_t::GetColor(D2D1_COLOR_F & color) const noexcept
{
    color = _Color;
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void color_button_t::OnPaint(HDC) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources();

    if (FAILED(hr))
        return;

    CRect cr;

    GetClientRect(&cr);

    D2D1_RECT_F Rect = D2D1::RectF(0.f, 0.f, (FLOAT) cr.Width(), (FLOAT) cr.Height());

    _RenderTarget->BeginDraw();

    if (IsWindowEnabled())
    {
        if (_PatternBrush)
            _RenderTarget->FillRectangle(Rect, _PatternBrush.Get());

        if (_Brush)
            _RenderTarget->FillRectangle(Rect, _Brush.Get());
    }
    else
        _RenderTarget->FillRectangle(Rect, _BackgroundBrush.Get());

    hr = _RenderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
        DeleteDeviceSpecificResources();

    ValidateRect(NULL);
}

/// <summary>
/// Handles the WM_LBUTTONDOWN message.
/// </summary>
LRESULT color_button_t::OnLButtonDown(UINT, CPoint point) noexcept
{
    if (_GradientStops.empty())
    {
        color_dialog_t cd;

        if (cd.SelectColor(m_hWnd, _Color))
        {
            SetColor(_Color);

            NotifyParent((UINT) NM_RETURN);
        }
    }
    else
    {
        ::SetCapture(m_hWnd);

        _Capture = { true, point, -1.f };
    }

    return 0;
}

/// <summary>
/// Handles the WM_LBUTTONUP message.
/// </summary>
LRESULT color_button_t::OnLButtonUp(UINT, CPoint point) noexcept
{
    if (_GradientStops.empty())
        return 1;

    _Capture.IsActive = false;

    if (::GetCapture() == m_hWnd)
        ::ReleaseCapture();

    {
        CRect cr;

        GetClientRect(cr);

        const float Position = std::clamp((float) point.y / (float) cr.bottom, 0.f, 1.f);

        NotifyParent(CBN_POSITION_CHANGED, Position);
    }

    return 0;
}

/// <summary>
/// Handles the WM_MOUSEMOVE message.
/// </summary>
void color_button_t::OnMouseMove(UINT flags, CPoint point) noexcept
{
    if (!_Capture.IsActive)
        return;

    {
        CRect cr;

        GetClientRect(cr);

        const float Position = std::clamp((float) point.y / (float) cr.bottom, 0.f, 1.f);

        if (Position == _Capture.Position)
            return;

        NotifyParent(CBN_POSITION_CHANGING, Position);

        _Capture.Position = Position;
    }
}

/// <summary>
/// Handles the WM_CAPTURECHANGED message.
/// </summary>
void color_button_t::OnCaptureChanged(CWindow) noexcept
{
    if (!_Capture.IsActive)
        return;

    _Capture.IsActive = false;
}

/// <summary>
/// Handles the WM_CANCELMODE message.
/// </summary>
void color_button_t::OnCancelMode() noexcept
{
    if (!_Capture.IsActive)
        return;

    _Capture.IsActive = false;
}

/// <summary>
/// Sends a notification to the parent that the content has changed.
/// </summary>
void color_button_t::NotifyParent(UINT code) const noexcept
{
    NMHDR nmh = { m_hWnd, (UINT_PTR) GetDlgCtrlID(), code };

    ::SendMessageW(GetParent(), WM_NOTIFY, nmh.idFrom, (LPARAM) &nmh);
}

/// <summary>
/// Notifies the parent about a position change.
/// </summary>
LRESULT color_button_t::NotifyParent(UINT code, float position) const noexcept
{
    NMCOLORBUTTON nmcb
    {
        .Header
        {
            .hwndFrom = m_hWnd,
            .idFrom   = (UINT_PTR) GetDlgCtrlID(),
            .code     = code,
        },
        .Position = position
    };

    return ::SendMessage(GetParent(), WM_NOTIFY, nmcb.Header.idFrom, (LPARAM) &nmcb);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT color_button_t::CreateDeviceSpecificResources() noexcept
{
    HRESULT hr = __super::CreateDeviceSpecificResources();

    if (FAILED(hr))
        return hr;

    if (_BackgroundBrush == nullptr)
    {
        const COLORREF Color = _Theme.GetSysColor(COLOR_BTNFACE);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(Color), _BackgroundBrush.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    if (_Brush == nullptr)
    {
        if (_GradientStops.empty())
        {
            ComPtr<ID2D1SolidColorBrush> SolidBrush;

            hr = _RenderTarget->CreateSolidColorBrush(_Color, SolidBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;

            _Brush = SolidBrush;
        }
        else
        {
            ComPtr<ID2D1GradientStopCollection> Collection;

            hr = _RenderTarget->CreateGradientStopCollection(_GradientStops.data(), (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, Collection.GetAddressOf());

            if (FAILED(hr))
                return hr;

            ComPtr<ID2D1LinearGradientBrush> GradientBrush;

            const D2D1_SIZE_F Size = _RenderTarget->GetSize();

            hr = _RenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection.Get(), GradientBrush.GetAddressOf());

            if (FAILED(hr))
                return hr;

            _Brush = GradientBrush;
        }
    }

    if (_PatternBrush == nullptr)
        hr = CreatePatternBrush(_RenderTarget.Get());

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void color_button_t::DeleteDeviceSpecificResources() noexcept
{
    _PatternBrush.Reset();
    _Brush.Reset();
    _BackgroundBrush.Reset();

    __super::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates a pattern brush for rendering the background.
/// </summary>
HRESULT color_button_t::CreatePatternBrush(ID2D1RenderTarget * renderTarget) noexcept
{
    ComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(8.f, 8.f), rt.GetAddressOf());

    if (FAILED(hr))
        return hr;

    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    ComPtr<ID2D1SolidColorBrush> Brush;

    hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), Brush.GetAddressOf());

    if (FAILED(hr))
        return hr;

    rt->BeginDraw();

    rt->Clear(); // Transparent

    rt->FillRectangle(D2D1::RectF(0.f, 0.f, 8.f, 8.f), Brush.Get());

    Brush->SetColor(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.f));

    rt->FillRectangle(D2D1::RectF(0.f, 0.f, 4.f, 4.f), Brush.Get());
    rt->FillRectangle(D2D1::RectF(4.f, 4.f, 8.f, 8.f), Brush.Get());

    hr = rt->EndDraw();

    if (FAILED(hr))
        return hr;

    ComPtr<ID2D1Bitmap> Bitmap;

    hr = rt->GetBitmap(Bitmap.GetAddressOf());

    if (FAILED(hr))
        return hr;

    const auto bbp = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

    hr = rt->CreateBitmapBrush(Bitmap.Get(), bbp, _PatternBrush.GetAddressOf());

    return hr;
}
