/* gdi32ext.cpp -- the GDI calls a text-and-pixel game imports beyond the
 * first set: colours, single pixels, positioned text and stock objects. */
#include "platform.h"

static COLORREF w32l_bk_colour = 0x00FFFFFFu;

COLORREF WINAPI SetBkColor(HDC dc, COLORREF colour)
{
    COLORREF old = w32l_bk_colour;

    (void)dc;
    w32l_bk_colour = colour;
    return old;
}

/* A DC here has no bitmap of its own to write a single pixel into; the
 * program draws through DirectDraw. The colour is returned as success. */
COLORREF WINAPI SetPixel(HDC dc, int32_t x, int32_t y, COLORREF colour)
{
    (void)dc; (void)x; (void)y;
    return colour;
}

BOOL WINAPI ExtTextOutA(HDC dc, int32_t x, int32_t y, UINT options, const RECT *rc,
                        LPCSTR text, UINT n, const int32_t *dx)
{
    (void)options; (void)rc; (void)dx;
    if (!text)
        return TRUE;
    return TextOutA(dc, x, y, text, (int32_t)n);
}

BOOL WINAPI GetTextExtentPointA(HDC dc, LPCSTR text, int32_t n, LPSIZE out)
{
    return GetTextExtentPoint32A(dc, text, n, out);
}

/* A distinct non-NULL handle per stock object, so SelectObject round-trips. */
HGDIOBJ WINAPI GetStockObject(int32_t index)
{
    return (HGDIOBJ)(uintptr_t)(0x57000000u + (uint32_t)index);
}
