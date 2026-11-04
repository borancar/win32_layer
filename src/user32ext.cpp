/* user32ext.cpp -- the USER32 entries a window-and-dialog game imports beyond
 * what user32.cpp first needed: extra window calls over the one window,
 * the pointer and key state, wvsprintfA, and dialogs and menus.
 *
 * Dialogs and menus have no implementation behind them. There is no
 * resource interpreter, so a dialog cannot be created (DialogBoxParamA fails,
 * CreateDialogParamA answers NULL) and every control query answers "no such
 * control". A game that only opens a dialog for its setup or options screen
 * then skips it; one that needs the answers cannot run until this grows.
 *
 * Input here reads the host's state (sdl.cpp). The layer delivers pointer and
 * key events to DirectInput only; Windows-message delivery (WM_MOUSEMOVE,
 * WM_KEYDOWN, ...) is a separate piece of work.
 */
#include "platform.h"

#include <stdio.h>
#include <string.h>

#ifndef ERROR_CALL_NOT_IMPLEMENTED
#define ERROR_CALL_NOT_IMPLEMENTED 120
#endif

/* ---- classes ------------------------------------------------------------------ */

ATOM WINAPI RegisterClassExA(const WNDCLASSEXA *wc)
{
    WNDCLASSA c;

    if (!wc || wc->cbSize < sizeof *wc)
        return 0;
    memset(&c, 0, sizeof c);
    c.style = wc->style;
    c.lpfnWndProc = wc->lpfnWndProc;
    c.cbClsExtra = wc->cbClsExtra;
    c.cbWndExtra = wc->cbWndExtra;
    c.hInstance = wc->hInstance;
    c.hIcon = wc->hIcon;
    c.hCursor = wc->hCursor;
    c.hbrBackground = wc->hbrBackground;
    c.lpszMenuName = wc->lpszMenuName;
    c.lpszClassName = wc->lpszClassName;
    return RegisterClassA(&c);
}

BOOL WINAPI UnregisterClassA(LPCSTR cls, HINSTANCE inst)
{
    (void)cls; (void)inst;
    return TRUE;
}

/* ---- the one window ------------------------------------------------------------ */

BOOL WINAPI BringWindowToTop(HWND hwnd)
{
    if (hwnd != w32l_the_window())
        return FALSE;
    w32l_host_window_raise();
    return TRUE;
}

HWND WINAPI SetActiveWindow(HWND hwnd)
{
    HWND old = GetActiveWindow();

    if (hwnd == w32l_the_window() && hwnd)
        w32l_host_window_raise();
    return old;
}

HWND WINAPI GetForegroundWindow(void) { return GetFocus(); }

/* A handle that is not the program's window, for code that compares. */
HWND WINAPI GetDesktopWindow(void) { return (HWND)(uintptr_t)0x1D01u; }

/* Nothing needs painting: the program composes every frame and the host
 * shows what DirectDraw presents (see GetUpdateRect). */
BOOL WINAPI InvalidateRect(HWND hwnd, const RECT *rc, BOOL erase)
{
    (void)hwnd; (void)rc; (void)erase;
    return TRUE;
}

BOOL WINAPI MoveWindow(HWND hwnd, int32_t x, int32_t y, int32_t w, int32_t h, BOOL repaint)
{
    (void)repaint;
    return SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER);
}

BOOL WINAPI SetWindowTextA(HWND hwnd, LPCSTR text)
{
    if (hwnd != w32l_the_window() || !text)
        return FALSE;
    w32l_host_window_set_title(text);
    return TRUE;
}

HWND WINAPI WindowFromPoint(POINT pt)
{
    int32_t x, y, w, h;

    if (!w32l_the_window())
        return NULL;
    w32l_host_window_position(&x, &y, &w, &h);
    if (pt.x < x || pt.y < y || pt.x >= x + w || pt.y >= y + h)
        return NULL;
    return w32l_the_window();
}

static HWND w32l_captured;

HWND WINAPI SetCapture(HWND hwnd)
{
    HWND old = w32l_captured;

    w32l_captured = hwnd;
    return old;
}

BOOL WINAPI ReleaseCapture(void)
{
    w32l_captured = NULL;
    return TRUE;
}

/* ---- the pointer ------------------------------------------------------------------ */

/* Screen coordinates, as Windows answers: the host reports the logical
 * position inside the window, and the window sits at the position the host
 * says. */
BOOL WINAPI GetCursorPos(LPPOINT pt)
{
    int32_t  x, y, wx, wy, ww, wh;
    uint32_t buttons;

    if (!pt)
        return FALSE;
    w32l_host_mouse_state(&x, &y, &buttons);
    w32l_host_window_position(&wx, &wy, &ww, &wh);
    pt->x = x + wx;
    pt->y = y + wy;
    return TRUE;
}

BOOL WINAPI SetCursorPos(int32_t x, int32_t y)
{
    int32_t wx, wy, ww, wh;

    w32l_host_window_position(&wx, &wy, &ww, &wh);
    w32l_host_mouse_warp(x - wx, y - wy);
    return TRUE;
}

/* ---- keys --------------------------------------------------------------------------- */

/* Virtual key -> DirectInput scancode, for what the host's key table (indexed
 * by scancode, 0x80 when down) can answer. 0 when there is no mapping. */
static uint8_t w32l_dik_of_vk(uint32_t vk)
{
    static const uint8_t letters[26] = {
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
    };

    if (vk >= 'A' && vk <= 'Z')
        return letters[vk - 'A'];
    if (vk >= '1' && vk <= '9')
        return (uint8_t)(0x02 + (vk - '1'));
    if (vk == '0')
        return 0x0B;
    if (vk >= 0x70 && vk <= 0x79)      /* F1..F10 */
        return (uint8_t)(0x3B + (vk - 0x70));
    switch (vk) {
    case 0x1B: return 0x01;   /* VK_ESCAPE */
    case 0x08: return 0x0E;   /* VK_BACK */
    case 0x09: return 0x0F;   /* VK_TAB */
    case 0x0D: return 0x1C;   /* VK_RETURN */
    case 0x20: return 0x39;   /* VK_SPACE */
    case 0x25: return 0xCB;   /* VK_LEFT */
    case 0x26: return 0xC8;   /* VK_UP */
    case 0x27: return 0xCD;   /* VK_RIGHT */
    case 0x28: return 0xD0;   /* VK_DOWN */
    case 0x21: return 0xC9;   /* VK_PRIOR */
    case 0x22: return 0xD1;   /* VK_NEXT */
    case 0x23: return 0xCF;   /* VK_END */
    case 0x24: return 0xC7;   /* VK_HOME */
    case 0x2D: return 0xD2;   /* VK_INSERT */
    case 0x2E: return 0xD3;   /* VK_DELETE */
    case 0x7A: return 0x57;   /* VK_F11 */
    case 0x7B: return 0x58;   /* VK_F12 */
    }
    return 0;
}

/* High bit set when the key is down. Toggle state is not tracked. */
int16_t WINAPI GetKeyState(int32_t vk)
{
    uint8_t  keys[256];
    uint32_t buttons;
    int32_t  x, y;
    int      down = 0;

    w32l_host_keyboard_state(keys);
    w32l_host_mouse_state(&x, &y, &buttons);
    switch (vk) {
    case VK_LBUTTON: down = (buttons & 1) != 0; break;
    case VK_RBUTTON: down = (buttons & 2) != 0; break;
    case VK_MBUTTON: down = (buttons & 4) != 0; break;
    case VK_SHIFT:   down = (keys[0x2A] | keys[0x36]) & 0x80; break;
    case VK_CONTROL: down = (keys[0x1D] | keys[0x9D]) & 0x80; break;
    case VK_MENU:    down = (keys[0x38] | keys[0xB8]) & 0x80; break;
    default: {
        uint8_t d = w32l_dik_of_vk((uint32_t)vk);
        down = d && (keys[d] & 0x80);
    }
    }
    return down ? (int16_t)0x8000 : 0;
}

/* Types: 0 virtual key -> scancode, 1 scancode -> virtual key, 2 virtual
 * key -> character. The extended-key marker (0x80) of a DirectInput scancode
 * is not part of a Windows scancode. */
UINT WINAPI MapVirtualKeyA(UINT code, UINT type)
{
    uint32_t vk;

    if (type == 0)
        return w32l_dik_of_vk(code) & 0x7F;
    if (type == 1)
        for (vk = 1; vk < 0x80; vk++)
            if ((w32l_dik_of_vk(vk) & 0x7F) == code && w32l_dik_of_vk(vk))
                return vk;
    if (type == 2 && ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9') || code == ' '))
        return code;
    return 0;
}

/* ---- text ---------------------------------------------------------------------------- */

int32_t WINAPI wvsprintfA(LPSTR out, LPCSTR fmt, va_list ap)
{
    /* USER32's wvsprintf writes at most 1024 bytes, like wsprintfA. */
    return vsnprintf(out, 1024, fmt, ap);
}

/* ---- dialogs ------------------------------------------------------------------------------ */

INT_PTR WINAPI DialogBoxParamA(HINSTANCE inst, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    (void)inst; (void)templ; (void)parent; (void)proc; (void)param;
    w32l_plat_log("DialogBoxParamA: no dialog resources here; failing");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
}

HWND WINAPI CreateDialogParamA(HINSTANCE inst, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    (void)inst; (void)templ; (void)parent; (void)proc; (void)param;
    w32l_plat_log("CreateDialogParamA: no dialog resources here; failing");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

BOOL WINAPI EndDialog(HWND dlg, INT_PTR result) { (void)dlg; (void)result; return FALSE; }
HWND WINAPI GetDlgItem(HWND dlg, int32_t id) { (void)dlg; (void)id; return NULL; }
int32_t WINAPI GetDlgCtrlID(HWND hwnd) { (void)hwnd; return 0; }

UINT WINAPI GetDlgItemInt(HWND dlg, int32_t id, BOOL *translated, BOOL isSigned)
{
    (void)dlg; (void)id; (void)isSigned;
    if (translated)
        *translated = FALSE;
    return 0;
}

UINT WINAPI GetDlgItemTextA(HWND dlg, int32_t id, LPSTR out, int32_t cch)
{
    (void)dlg; (void)id;
    if (out && cch > 0)
        out[0] = 0;
    return 0;
}

BOOL WINAPI SetDlgItemInt(HWND dlg, int32_t id, UINT value, BOOL isSigned)
{
    (void)dlg; (void)id; (void)value; (void)isSigned;
    return FALSE;
}

BOOL WINAPI SetDlgItemTextA(HWND dlg, int32_t id, LPCSTR text)
{
    (void)dlg; (void)id; (void)text;
    return FALSE;
}

BOOL WINAPI CheckDlgButton(HWND dlg, int32_t id, UINT check)
{
    (void)dlg; (void)id; (void)check;
    return FALSE;
}

UINT WINAPI IsDlgButtonChecked(HWND dlg, int32_t id) { (void)dlg; (void)id; return 0; }

LRESULT WINAPI SendDlgItemMessageA(HWND dlg, int32_t id, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)dlg; (void)id; (void)msg; (void)wp; (void)lp;
    return 0;
}

/* ---- menus ---------------------------------------------------------------------------------- */

HMENU WINAPI LoadMenuA(HINSTANCE inst, LPCSTR name) { (void)inst; (void)name; return NULL; }
BOOL WINAPI DestroyMenu(HMENU menu) { (void)menu; return FALSE; }
int32_t WINAPI GetMenuItemCount(HMENU menu) { (void)menu; return -1; }

BOOL WINAPI GetMenuItemInfoA(HMENU menu, UINT item, BOOL byPosition, LPMENUITEMINFOA info)
{
    (void)menu; (void)item; (void)byPosition; (void)info;
    return FALSE;
}

HMENU WINAPI GetSubMenu(HMENU menu, int32_t pos) { (void)menu; (void)pos; return NULL; }

BOOL WINAPI TrackPopupMenu(HMENU menu, UINT flags, int32_t x, int32_t y, int32_t reserved,
                           HWND hwnd, const RECT *rc)
{
    (void)menu; (void)flags; (void)x; (void)y; (void)reserved; (void)hwnd; (void)rc;
    return FALSE;
}
