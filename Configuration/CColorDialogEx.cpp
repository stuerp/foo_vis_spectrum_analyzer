
/** $VER: CColorDialogEx.cpp (2023.12.31) P. Stuer - Implements a color dialog with alpha channel support using WTL. **/

#include "framework.h"
#include "CColorDialogEx.h"

#include "Resources.h"
#include "Support.h"
#include "Color.h"

#include <colordlg.h>

#pragma hdrstop

/// <summary>
/// Shows the Color dialog.
/// </summary>
bool CColorDialogEx::SelectColor(HWND hWnd, D2D1_COLOR_F & color)
{
    _Color = color;

    static COLORREF CustomColors[16] =
    {
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
    };

    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };

    cc.hwndOwner = hWnd;

    cc.lpCustColors = (LPDWORD) CustomColors;
    cc.rgbResult = Color::ToCOLORREF(color);
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK | CC_ENABLETEMPLATE | CC_SOLIDCOLOR;
    cc.lpfnHook = (LPCCHOOKPROC) Hook;
    cc.hInstance = (HWND) GetCurrentModule();//::GetModuleHandleW(TEXT(STR_COMPONENT_FILENAME));
    cc.lpTemplateName = MAKEINTRESOURCE(IDD_CHOOSECOLOR);
    cc.lCustData = (LPARAM) this;

    if (!::ChooseColorW(&cc))
        return false;

    color = _Color;

    return true;
}

/// <summary>
/// Hooks the standard Color dialog procedure.
/// </summary>
UINT_PTR CALLBACK CColorDialogEx::Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CColorDialogEx * This;

    if (msg == WM_INITDIALOG)
    {
        CHOOSECOLORW * cc = (CHOOSECOLORW *) lParam;

        This = (CColorDialogEx *) cc->lCustData;

        ::SetWindowLongPtrW(hDlg, GWLP_USERDATA, LONG_PTR(This));
    }

    This = (CColorDialogEx *) ::GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    if (This != nullptr)
        return This->ProcessMessage(hDlg, msg, wParam, lParam);

    return 0;
}

UINT_PTR CColorDialogEx::ProcessMessage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            _Alpha.Initialize(::GetDlgItem(hDlg, IDC_ALPHA_SLIDER));

            UpdateAlphaSlider();

            ::SetRect(&_SliderRect, 296 + 8, 4, 304 + 6, 120);
            ::MapDialogRect(hDlg, &_SliderRect);

            _SliderRect.top += 1;

            ::SetDlgItemInt(hDlg, IDC_ALPHA_VALUE, (UINT) (_Color.a * 100.f), FALSE);
            break;
        }

        case WM_PAINT:
        {
            DrawAlphaSliderCursor(hDlg);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            LONG x = GET_X_LPARAM(lParam);
            LONG y = GET_Y_LPARAM(lParam);

            if (!::PtInRect(&_SliderRect, POINT(x, y)))
                break;

            ::SetCapture(hDlg);

            RECT ClipRect = _SliderRect;

            ::ClientToScreen(hDlg, (POINT *) &ClipRect.left);
            ::ClientToScreen(hDlg, (POINT *) &ClipRect.right);

            ::ClipCursor(&ClipRect);

            _Color.a = Map(y, _SliderRect.top, _SliderRect.bottom - 1, 0.f, 1.f);

            ::SetDlgItemInt(hDlg, IDC_ALPHA_VALUE, (UINT) (_Color.a * 100.f), FALSE);
            ::InvalidateRect(hDlg, &_SliderRect, FALSE);

            return 1;
        }

        case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) == 0)
                break;

            LONG x = GET_X_LPARAM(lParam);
            LONG y = GET_Y_LPARAM(lParam);

            if (!::PtInRect(&_SliderRect, POINT(x, y)))
                break;

            _Color.a = Map(y, _SliderRect.top, _SliderRect.bottom - 1, 0.f, 1.f);

            ::SetDlgItemInt(hDlg, IDC_ALPHA_VALUE, (UINT) (_Color.a * 100.f), FALSE);
            ::InvalidateRect(hDlg, &_SliderRect, FALSE);

            return 1;
        }

        case WM_LBUTTONUP:
        {
            ::ClipCursor(nullptr);
            ::ReleaseCapture();
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) != EN_CHANGE)
                break;

            switch (LOWORD(wParam))
            {
                case COLOR_RED:
                {
                    BOOL Success = FALSE;

                    UINT Value = ::GetDlgItemInt(hDlg, COLOR_RED, &Success, FALSE);

                    if (!Success)
                        break;

                    _Color.r = (FLOAT) Clamp(Value, 0U, 255U) / 255.f;

                    UpdateAlphaSlider();
                    ::InvalidateRect(hDlg, &_SliderRect, FALSE);
                    break;
                }

                case COLOR_GREEN:
                {
                    BOOL Success = FALSE;

                    UINT Value = ::GetDlgItemInt(hDlg, COLOR_GREEN, &Success, FALSE);

                    if (!Success)
                        break;

                    _Color.g = (FLOAT) Clamp(Value, 0U, 255U) / 255.f;

                    UpdateAlphaSlider();
                    ::InvalidateRect(hDlg, &_SliderRect, FALSE);
                    break;
                }

                case COLOR_BLUE:
                {
                    BOOL Success = FALSE;

                    UINT Value = ::GetDlgItemInt(hDlg, COLOR_BLUE, &Success, FALSE);

                    if (!Success)
                        break;

                    _Color.b = (FLOAT) Clamp(Value, 0U, 255U) / 255.f;

                    UpdateAlphaSlider();
                    ::InvalidateRect(hDlg, &_SliderRect, FALSE);
                    break;
                }

                case IDC_ALPHA_VALUE:
                {
                    BOOL Success = FALSE;

                    UINT Value = ::GetDlgItemInt(hDlg, IDC_ALPHA_VALUE, &Success, FALSE);

                    if (!Success)
                        break;

                    _Color.a = (FLOAT) Clamp(Value, 0U, 100U) / 100.f;

                    ::InvalidateRect(hDlg, &_SliderRect, FALSE);

                    return 1;
                }
            }

            break;
        }
    }

    return 0;
}

/// <summary>
/// Draws the alpha slider cursor.
/// </summary>
void CColorDialogEx::DrawAlphaSliderCursor(HWND hDlg)
{
    // Calculate the background rectangle.
    RECT BackRect = _SliderRect;

    BackRect.top    -= ArrowSize;
    BackRect.bottom += ArrowSize;

    // Calculate the polygon coordinates.
//  static const POINT Points[4] = { POINT(ArrowSize, 0), POINT(ArrowSize, ArrowSize * 2), POINT(0, ArrowSize), POINT(ArrowSize, 0) };

    LONG Offset = MapEx(_Color.a, 0.f, 1.f, _SliderRect.top, _SliderRect.bottom - 1);

    const POINT Points[4] =
    {
        POINT(_SliderRect.left + 1 + ArrowSize, Offset - ArrowSize),
        POINT(_SliderRect.left + 1 + ArrowSize, Offset + ArrowSize),
        POINT(_SliderRect.left + 1,             Offset),
        POINT(_SliderRect.left + 1 + ArrowSize, Offset - ArrowSize)
    };

    // Render the cursor.
    {
        HDC hDC = ::GetDC(hDlg);

        ::FillRect(hDC, &BackRect, (HBRUSH) ::GetSysColorBrush(COLOR_3DFACE));

        HGDIOBJ OldBrush = ::SelectObject(hDC, (HBRUSH) ::GetStockObject(BLACK_BRUSH));

        ::Polygon(hDC, Points, _countof(Points));

        ::SelectObject(hDC, OldBrush);

        ::ReleaseDC(hDlg, hDC);
    }
}

/// <summary>
/// Updates the color gradient of the alpha slider.
/// </summary>
void CColorDialogEx::UpdateAlphaSlider()
{
    std::vector<D2D1_GRADIENT_STOP> GradientStops =
    {
        { 0.f / 1.f, D2D1::ColorF(_Color.r, _Color.g, _Color.b, 0.f) },
        { 1.f / 1.f, D2D1::ColorF(_Color.r, _Color.g, _Color.b, 1.f) },
    };

    _Alpha.SetGradientStops(GradientStops);
}
