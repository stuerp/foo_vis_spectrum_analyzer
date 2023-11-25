
/** $VER: CColorButton.cpp (2023.11.25) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "framework.h"

#include "CColorButton.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void CColorButton::Initialize(HWND hWnd)
{
    ATLASSERT(::IsWindow(hWnd));

    SubclassWindow(hWnd);

    CreateDeviceIndependentResources();

    // Create the tooltip.
    {
        _ToolTip.Create(m_hWnd);
        ATLASSERT(_ToolTip.IsWindow());

        int Id = GetDlgCtrlID();

        RECT rect;

        GetClientRect(&rect);

        CToolInfo ti(0, m_hWnd, -Id, &rect, nullptr);

        _ToolTip.AddTool(&ti);

        _ToolTip.UpdateTipText(L"Hello World", m_hWnd, -Id);
        _ToolTip.Activate(TRUE);
    }

    Invalidate();
    UpdateWindow();
}

/// <summary>
/// Sets the color scheme.
/// </summary>
void CColorButton::SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops)
{
    _GradientStops = gradientStops;

    ReleaseDeviceSpecificResources();
    InvalidateRect(NULL);
}

/// <summary>
/// Sets the color.
/// </summary>
void CColorButton::SetColor(COLORREF color)
{
    SetColor(ToD2D1_COLOR_F(color));
}

/// <summary>
/// Sets the color.
/// </summary>
void CColorButton::SetColor(const D2D1_COLOR_F & color)
{
    _Color = color;
    _GradientStops.clear();

    ReleaseDeviceSpecificResources();
    InvalidateRect(NULL);
}

/// <summary>
/// Handles the WM_PAINT message.
/// </summary>
void CColorButton::OnPaint(HDC)
{
    CRect ClientRect; GetClientRect(ClientRect);

    D2D1_SIZE_U Size = D2D1::SizeU((UINT32) ClientRect.Width(), (UINT32) ClientRect.Height());

    {
        HRESULT hr = CreateDeviceSpecificResources(m_hWnd, Size);

        if (FAILED(hr))
            return;

        _RenderTarget->BeginDraw();

        D2D1_RECT_F Rect = D2D1::RectF(0.f, 0.f, (FLOAT) Size.width, (FLOAT) Size.height);

        _RenderTarget->FillRectangle(Rect, _Brush);

        hr = _RenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
            ReleaseDeviceSpecificResources();
    }

    ValidateRect(NULL);
}

/// <summary>
/// Handles the WM_LBUTTONDBLCLK message.
/// </summary>
LRESULT CColorButton::OnLButtonDblClick(UINT, CPoint)
{
    if (_GradientStops.size() == 0)
        return 1;

    if (SelectColor(m_hWnd, _Color))
        SetColor(_Color);

    return 0;
}

/// <summary>
/// Passes mouse messages to the tooltip control for processing.
/// </summary>
LRESULT CColorButton::OnMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam, BOOL & handled)
{
    if (_ToolTip.IsWindow())
    {
        MSG Msg = { m_hWnd, msg, wParam, lParam };

        _ToolTip.RelayEvent(&Msg);
    }

    handled = FALSE;

    return 1;
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT CColorButton::CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size)
{
    HRESULT hr = __super::CreateDeviceSpecificResources(hWnd, size);

    if (SUCCEEDED(hr) && (_Brush == nullptr))
    {
        if (_GradientStops.size() == 0)
        {
            CComPtr<ID2D1SolidColorBrush> SolidBrush;

            hr = _RenderTarget->CreateSolidColorBrush(_Color, &SolidBrush);

            if (SUCCEEDED(hr))
                _Brush = SolidBrush;
        }
        else
        {
            CComPtr<ID2D1GradientStopCollection> Collection;

            hr = _RenderTarget->CreateGradientStopCollection(&_GradientStops[0], (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

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

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void CColorButton::ReleaseDeviceSpecificResources()
{
    _Brush.Release();

    __super::ReleaseDeviceSpecificResources();
}

#pragma endregion
