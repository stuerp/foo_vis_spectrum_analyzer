
/** $VER: CColorButton.cpp (2026.02.22) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "pch.h"

#include "CColorButton.h"
#include "CColorDialogEx.h"

#include "Theme.h"
#include "Color.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void CColorButton::Initialize(HWND hWnd) noexcept
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
void CColorButton::Terminate() noexcept
{
    if (!IsWindow() || !_IsSubclassed)
        return;

    DeleteDeviceSpecificResources();
    DeleteDeviceIndependentResources();

    UnsubclassWindow(TRUE);
    _IsSubclassed = false;
}

/// <summary>
/// Sets the color scheme.
/// </summary>
void CColorButton::SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops) noexcept
{
    _GradientStops = gradientStops;

    _Brush.Release();

    EnableWindow(gradientStops.size() > 0);

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Sets the color.
/// </summary>
void CColorButton::SetColor(COLORREF color) noexcept
{
    SetColor(color_t::ToD2D1_COLOR_F(color));
}

/// <summary>
/// Sets the color.
/// </summary>
void CColorButton::SetColor(const D2D1_COLOR_F & color) noexcept
{
    _Color = color;
    _GradientStops.clear();

    _Brush.Release();

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Gets the color.
/// </summary>
void CColorButton::GetColor(D2D1_COLOR_F & color) const noexcept
{
    color = _Color;
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void CColorButton::OnPaint(HDC) noexcept
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
            _RenderTarget->FillRectangle(Rect, _PatternBrush);

        if (_Brush)
            _RenderTarget->FillRectangle(Rect, _Brush);
    }
    else
        _RenderTarget->FillRectangle(Rect, _BackgroundBrush);

    hr = _RenderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
        DeleteDeviceSpecificResources();

    ValidateRect(NULL);
}

/// <summary>
/// Handles the WM_LBUTTONDBLCLK message.
/// </summary>
LRESULT CColorButton::OnLButtonDown(UINT, CPoint) noexcept
{
    if (!_GradientStops.empty())
        return 1;

    CColorDialogEx cd;

    if (cd.SelectColor(m_hWnd, _Color))
    {
        SetColor(_Color);
        SendChangedNotification();
    }

    return 0;
}

/// <summary>
/// Sends a notification to the parent that the content has changed.
/// </summary>
void CColorButton::SendChangedNotification() const noexcept
{
    NMHDR nmhdr = { m_hWnd, (UINT_PTR) GetDlgCtrlID(), (UINT) NM_RETURN };

    ::SendMessageW(GetParent(), WM_NOTIFY, nmhdr.idFrom, (LPARAM) &nmhdr);
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT CColorButton::CreateDeviceSpecificResources() noexcept
{
    HRESULT hr = __super::CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && (_BackgroundBrush == nullptr))
    {
        COLORREF Color = _Theme.GetSysColor(COLOR_BTNFACE);

        hr = _RenderTarget->CreateSolidColorBrush(D2D1::ColorF(Color), &_BackgroundBrush);
    }

    if (SUCCEEDED(hr) && (_Brush == nullptr))
    {
        if (_GradientStops.empty())
        {
            CComPtr<ID2D1SolidColorBrush> SolidBrush;

            hr = _RenderTarget->CreateSolidColorBrush(_Color, &SolidBrush);

            if (SUCCEEDED(hr))
                _Brush = SolidBrush;
        }
        else
        {
            CComPtr<ID2D1GradientStopCollection> Collection;

            hr = _RenderTarget->CreateGradientStopCollection(_GradientStops.data(), (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

            if (SUCCEEDED(hr))
            {
                CComPtr<ID2D1LinearGradientBrush> GradientBrush;
                D2D1_SIZE_F Size = _RenderTarget->GetSize();

                hr = _RenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &GradientBrush);

                if (SUCCEEDED(hr))
                    _Brush = GradientBrush;
            }
        }
    }

    if (SUCCEEDED(hr) && (_PatternBrush == nullptr))
        hr = CreatePatternBrush(_RenderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void CColorButton::DeleteDeviceSpecificResources() noexcept
{
    _PatternBrush.Release();
    _Brush.Release();
    _BackgroundBrush.Release();

    __super::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates a pattern brush for rendering the background.
/// </summary>
HRESULT CColorButton::CreatePatternBrush(ID2D1RenderTarget * renderTarget) noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(8.f, 8.f), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->Clear();

            rt->FillRectangle(D2D1::RectF(0.f, 0.f, 8.f, 8.f), Brush);

            Brush->SetColor(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.f));

            rt->FillRectangle(D2D1::RectF(0.f, 0.f, 4.f, 4.f), Brush);
            rt->FillRectangle(D2D1::RectF(4.f, 4.f, 8.f, 8.f), Brush);

            rt->EndDraw();
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<ID2D1Bitmap> Bitmap;

            hr = rt->GetBitmap(&Bitmap);

            if (SUCCEEDED(hr))
            {
                auto brushProperties = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

                hr = rt->CreateBitmapBrush(Bitmap, brushProperties, &_PatternBrush);
            }
        }
    }

    return hr;
}

#pragma endregion
