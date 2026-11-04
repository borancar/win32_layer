/* ddraw.cpp -- DirectDraw over plain buffers.
 *
 * A surface is a buffer in memory, 8 bits a pixel and palettised (the
 * default, ArmyMen2) or 16 bits a pixel in RGB565 when the loader sets
 * w32l_display_bpp to 16 (Commandos). The primary is the one the
 * screen shows: whenever something lands on it -- a Flip from the back
 * buffer, a Blt or BltFast into it, an Unlock or ReleaseDC on it, or a
 * change to the palette it has attached -- its pixels go through that
 * palette to sdl.cpp as one frame. Nothing else in the layer knows how a
 * frame reaches the screen.
 *
 * Cooperative levels and display modes are accepted and not enforced: the
 * game asks for an exclusive fullscreen 8-bit mode and gets a window whose
 * logical size is the mode it asked for. Surfaces are never lost.
 */
#include "platform.h"
#include <ddraw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

W32L_DEFINE_GUID_VALUE(IID_IDirectDraw,        0x6C14DB80, 0xA733, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
W32L_DEFINE_GUID_VALUE(IID_IDirectDraw2,       0xB3A6F3E0, 0x2B43, 0x11CF, 0xA2, 0xDE, 0x00, 0xAA, 0x00, 0xB9, 0x33, 0x56);
W32L_DEFINE_GUID_VALUE(IID_IDirectDrawSurface, 0x6C14DB81, 0xA733, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
W32L_DEFINE_GUID_VALUE(IID_IDirectDrawPalette, 0x6C14DB84, 0xA733, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);

/* ---- objects ------------------------------------------------------------------ */

typedef struct W32L_DDPalette {
    const IDirectDrawPaletteVtbl *lpVtbl;
    int32_t      refs;
    PALETTEENTRY entries[256];
} W32L_DDPalette;

typedef struct W32L_DDSurface {
    const IDirectDrawSurfaceVtbl *lpVtbl;
    int32_t        refs;
    int32_t        width, height, pitch;
    int32_t        bytes;           /* bytes a pixel: 1 palettised, 2 for RGB565 */
    uint8_t       *pixels;
    int32_t        isPrimary;
    struct W32L_DDSurface *back;    /* the flip chain's back buffer */
    W32L_DDPalette *palette;
    DDCOLORKEY     srcKey;
    int32_t        hasSrcKey;
    DWORD          caps;
} W32L_DDSurface;

typedef struct W32L_DDraw {
    const IDirectDrawVtbl  *lpVtbl;
    const IDirectDraw2Vtbl *lpVtbl2;    /* the IDirectDraw2 view is this+4 */
    int32_t  refs;
    int32_t  modeW, modeH, modeBpp;
} W32L_DDraw;

/* The colour depth of the desktop the program sees; the format of the primary
 * and of every surface that does not ask for another. The loader sets it. */
int32_t w32l_display_bpp = 8;

static W32L_DDSurface *w32l_primary;
/* Set by SetCooperativeLevel: an exclusive fullscreen application owns the
 * display and its primary's palette IS the screen's; a windowed one shares
 * the desktop and shows through the system palette GDI realised, whatever
 * palette it attaches. The game relies on the second: in windowed mode it
 * attaches a DirectDraw palette built from a snapshot that is still zero
 * and expects the RealizePalette it did first to be what shows. */
static int32_t w32l_exclusive;
extern const IDirectDrawSurfaceVtbl w32l_surface_vtbl;
extern const IDirectDrawPaletteVtbl w32l_palette_vtbl;
extern const IDirectDrawVtbl        w32l_ddraw_vtbl;
extern const IDirectDraw2Vtbl       w32l_ddraw2_vtbl;

/* ---- the frame ---------------------------------------------------------------------- */

static const PALETTEENTRY *w32l_primary_palette(void)
{
    return w32l_exclusive && w32l_primary && w32l_primary->palette
               ? w32l_primary->palette->entries : w32l_screen_palette;
}

static void w32l_present(void)
{
    static int32_t presented;

    if (!w32l_primary)
        return;
    if (w32l_primary->bytes != 1) {
        w32l_host_present16(w32l_primary->pixels, w32l_primary->pitch, w32l_primary->width,
                            w32l_primary->height);
        return;
    }
    if (presented < 8) {
        const PALETTEENTRY *pal = w32l_primary_palette();
        int32_t i, nonzero = 0, lit = 0;
        for (i = 0; i < 256; i++)
            if (pal[i].peRed | pal[i].peGreen | pal[i].peBlue)
                nonzero++;
        for (i = 0; i < w32l_primary->height; i++) {
            const uint8_t *row = w32l_primary->pixels + i * w32l_primary->pitch;
            int32_t x;
            for (x = 0; x < w32l_primary->width; x++)
                lit += row[x] != 0;
        }
        w32l_plat_debug("present #%d: %s palette, %d non-black entries, %d non-zero pixels",
                       presented, w32l_primary->palette ? "attached" : "screen", nonzero, lit);
        presented++;
    }
    w32l_host_present(w32l_primary->pixels, w32l_primary->pitch, w32l_primary->width,
                     w32l_primary->height, w32l_primary_palette());
}

int32_t w32l_ddraw_primary_target(W32L_DCTarget *out)
{
    if (!w32l_primary)
        return 0;
    /* GDI draws palette indices; a 16-bit primary has none, so its DC draws
     * nowhere (a NULL target is a no-op in gdi32.cpp). */
    out->pixels = w32l_primary->bytes == 1 ? w32l_primary->pixels : NULL;
    out->pitch = w32l_primary->pitch;
    out->width = w32l_primary->width;
    out->height = w32l_primary->height;
    out->palette = w32l_primary_palette();
    out->changed = w32l_present;
    return 1;
}

/* ---- surfaces ----------------------------------------------------------------------- */

static W32L_DDSurface *w32l_surface_new(int32_t w, int32_t h, DWORD caps, int32_t bytes)
{
    W32L_DDSurface *s = (W32L_DDSurface *)calloc(1, sizeof *s);

    if (!s)
        return NULL;
    s->lpVtbl = &w32l_surface_vtbl;
    s->refs = 1;
    s->width = w;
    s->height = h;
    s->bytes = bytes;
    s->pitch = ((w + 15) & ~15) * bytes;
    s->pixels = (uint8_t *)calloc((size_t)s->pitch * (size_t)h + 16, 1);
    s->caps = caps;
    if (!s->pixels) {
        free(s);
        return NULL;
    }
    return s;
}

static void w32l_surface_free(W32L_DDSurface *s)
{
    if (s->back)
        IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)s->back);
    if (s->palette)
        IDirectDrawPalette_Release((LPDIRECTDRAWPALETTE)s->palette);
    if (s == w32l_primary)
        w32l_primary = NULL;
    free(s->pixels);
    free(s);
}

static HRESULT STDMETHODCALLTYPE Surface_QueryInterface(IDirectDrawSurface *p, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, IID_IDirectDrawSurface)) {
        *out = p;
        p->lpVtbl->AddRef(p);
        return DD_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Surface_AddRef(IDirectDrawSurface *p)
{
    return (ULONG)++((W32L_DDSurface *)p)->refs;
}

static ULONG STDMETHODCALLTYPE Surface_Release(IDirectDrawSurface *p)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;
    int32_t        n = --s->refs;

    if (n <= 0) {
        w32l_surface_free(s);
        return 0;
    }
    return (ULONG)n;
}

/* The slots the game never calls, each with its own signature: a stdcall
 * callee pops its own arguments, so a stub reached through the wrong
 * prototype would unbalance the stack. */
#define SURFACE_UNSUPPORTED(name, ...) \
    static HRESULT STDMETHODCALLTYPE Surface_##name(IDirectDrawSurface *, ##__VA_ARGS__) \
    { w32l_plat_log("unsupported surface call: " #name); return DDERR_UNSUPPORTED; }
SURFACE_UNSUPPORTED(AddAttachedSurface, LPDIRECTDRAWSURFACE)
SURFACE_UNSUPPORTED(AddOverlayDirtyRect, LPRECT)
SURFACE_UNSUPPORTED(BltBatch, LPVOID, DWORD, DWORD)
SURFACE_UNSUPPORTED(DeleteAttachedSurface, DWORD, LPDIRECTDRAWSURFACE)
SURFACE_UNSUPPORTED(EnumAttachedSurfaces, LPVOID, LPDDENUMSURFACESCALLBACK)
SURFACE_UNSUPPORTED(EnumOverlayZOrders, DWORD, LPVOID, LPDDENUMSURFACESCALLBACK)
SURFACE_UNSUPPORTED(GetBltStatus, DWORD)
SURFACE_UNSUPPORTED(GetCaps, LPDDSCAPS)
SURFACE_UNSUPPORTED(GetClipper, LPDIRECTDRAWCLIPPER *)
SURFACE_UNSUPPORTED(GetFlipStatus, DWORD)
SURFACE_UNSUPPORTED(GetOverlayPosition, LPLONG, LPLONG)
SURFACE_UNSUPPORTED(Initialize, LPDIRECTDRAW, LPDDSURFACEDESC)
SURFACE_UNSUPPORTED(SetOverlayPosition, LONG, LONG)
SURFACE_UNSUPPORTED(UpdateOverlay, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPVOID)
SURFACE_UNSUPPORTED(UpdateOverlayDisplay, DWORD)
SURFACE_UNSUPPORTED(UpdateOverlayZOrder, DWORD, LPDIRECTDRAWSURFACE)

static int32_t w32l_rect_of(const W32L_DDSurface *s, const RECT *rc, RECT *out)
{
    if (rc) {
        *out = *rc;
    } else {
        out->left = out->top = 0;
        out->right = s->width;
        out->bottom = s->height;
    }
    return out->left < out->right && out->top < out->bottom;
}

static void w32l_fill(W32L_DDSurface *s, const RECT *rc, uint32_t colour)
{
    RECT    r;
    int32_t y;

    if (!w32l_rect_of(s, rc, &r))
        return;
    if (r.left < 0) r.left = 0;
    if (r.top < 0) r.top = 0;
    if (r.right > s->width) r.right = s->width;
    if (r.bottom > s->height) r.bottom = s->height;
    for (y = r.top; y < r.bottom; y++) {
        uint8_t *row = s->pixels + y * s->pitch + r.left * s->bytes;
        if (s->bytes == 1) {
            memset(row, (uint8_t)colour, (size_t)(r.right - r.left));
        } else {
            int32_t x;
            for (x = 0; x < r.right - r.left; x++)
                ((uint16_t *)row)[x] = (uint16_t)colour;
        }
    }
}

/* Copy src's rectangle to (dx, dy) on dst, clipped to both, optionally
 * skipping pixels within src's colour key. Handles overlap by copying
 * through a row buffer when the two surfaces are one. */
static void w32l_copy(W32L_DDSurface *dst, int32_t dx, int32_t dy,
                     const W32L_DDSurface *src, const RECT *srcRect, int32_t keyed)
{
    RECT    r;
    int32_t w, h, y, x;
    uint8_t row[8192];
    int32_t bpp = src->bytes;

    if (dst->bytes != src->bytes)
        return;     /* no conversion between formats */
    if (!w32l_rect_of(src, srcRect, &r))
        return;
    if (r.left < 0) { dx -= r.left; r.left = 0; }
    if (r.top < 0) { dy -= r.top; r.top = 0; }
    if (r.right > src->width) r.right = src->width;
    if (r.bottom > src->height) r.bottom = src->height;
    if (dx < 0) { r.left -= dx; dx = 0; }
    if (dy < 0) { r.top -= dy; dy = 0; }
    w = r.right - r.left;
    h = r.bottom - r.top;
    if (dx + w > dst->width) w = dst->width - dx;
    if (dy + h > dst->height) h = dst->height - dy;
    if (w <= 0 || h <= 0 || w * bpp > (int32_t)sizeof row)
        return;
    /* A surface scrolled onto itself: when the destination lies below the
     * source the rows must go bottom-up, or each row reads the one the
     * previous step just overwrote -- and with a one-pixel scroll every row
     * becomes a copy of the first, which is what the vertical stripes over
     * the map were. memmove settles the horizontal case within a row. */
    for (int32_t i = 0; i < h; i++) {
        const uint8_t *s;
        uint8_t       *d;
        y = (dst == src && dy > r.top) ? h - 1 - i : i;
        s = src->pixels + (r.top + y) * src->pitch + r.left * bpp;
        d = dst->pixels + (dy + y) * dst->pitch + dx * bpp;
        if (!keyed) {
            memmove(d, s, (size_t)(w * bpp));
            continue;
        }
        memcpy(row, s, (size_t)(w * bpp));
        for (x = 0; x < w; x++) {
            uint32_t v = bpp == 1 ? row[x] : ((const uint16_t *)row)[x];
            if (v >= src->srcKey.dwColorSpaceLowValue &&
                v <= src->srcKey.dwColorSpaceHighValue)
                continue;
            if (bpp == 1)
                d[x] = row[x];
            else
                ((uint16_t *)d)[x] = (uint16_t)v;
        }
    }
}

/* Stretch src's rectangle onto dst's, nearest neighbour, sampling each
 * destination pixel's CENTRE: a 16.16 increment (src << 16) / dst, started at
 * half a step and stepped once per destination pixel.
 *
 * HISTORY. This used to reproduce wined3d's arithmetic instead -- the same
 * increment accumulated from ZERO and truncated -- calibrated against a Wine
 * A/B (118 pixels of a Boot Camp dialog frame), on the reasoning that the
 * residue the pointer's save/restore slot leaves is only the original's if
 * the rounding is. That got the provenance backwards: the truncation is
 * Wine's DirectDraw, not the game's, and it is exactly what made the slot
 * round trip lossy (see the note at the loop below). With centre sampling
 * the round trip is exact and the hybrid and the port agree; a Wine frame
 * that differs behind the pointer now differs because Wine smears. */
static void w32l_stretch(W32L_DDSurface *dst, const RECT *dstRect,
                        const W32L_DDSurface *src, const RECT *srcRect, int32_t keyed)
{
    RECT     d, s;
    int32_t  dw, dh, sw, sh, x, y;
    uint32_t xinc, yinc, sx16, sy16;

    if (dst->bytes != src->bytes)
        return;     /* no conversion between formats */
    if (!w32l_rect_of(dst, dstRect, &d) || !w32l_rect_of(src, srcRect, &s))
        return;
    dw = d.right - d.left;
    dh = d.bottom - d.top;
    sw = s.right - s.left;
    sh = s.bottom - s.top;
    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
        return;
    /* CENTRE SAMPLING, NOT wined3d's TRUNCATED STEP. The increment used to be
     * accumulated from 0 and truncated, which is what wined3d does and what
     * a Wine A/B was calibrated against. But the game round-trips the pointer's
     * 12x16 background through a 32x32 save slot every tick (DrawMenuCursor:
     * stretch up on save, stretch down on restore), and under truncation
     * 12 -> 32 -> 12 lands every column past the first ONE PIXEL LEFT of where
     * it came from -- so the restore leaves a shifted residue, and on a screen
     * that does not repaint each tick (the briefing, the pause menu) it
     * accumulates into a smear along the pointer's path. That is a DirectDraw
     * IMPLEMENTATION artifact copied from Wine, not the game's behaviour, and
     * it showed as a 40-pixel divergence at pump 104 of the bootcamp side-by-
     * side. Sampling each destination pixel's CENTRE ((x + 0.5) * src/dst)
     * makes the 12<->32 and 16<->32 round trips exact (simulated: both return
     * the identity), so the slot gives back precisely what was saved. Both the
     * hybrid and the port share this, so the oracle and the reconstruction
     * agree, and the reconstruction can make the original's whole-slot calls
     * rather than work around the platform with a 1:1 sub-rect. */
    xinc = ((uint32_t)sw << 16) / (uint32_t)dw;
    yinc = ((uint32_t)sh << 16) / (uint32_t)dh;
    for (y = 0, sy16 = yinc >> 1; y < dh; y++, sy16 += yinc) {
        int32_t sy = s.top + (int32_t)(sy16 >> 16);
        int32_t dy = d.top + y;
        if (dy < 0 || dy >= dst->height || sy < 0 || sy >= src->height)
            continue;
        for (x = 0, sx16 = xinc >> 1; x < dw; x++, sx16 += xinc) {
            int32_t sx = s.left + (int32_t)(sx16 >> 16);
            int32_t dx = d.left + x;
            uint32_t v;
            if (dx < 0 || dx >= dst->width || sx < 0 || sx >= src->width)
                continue;
            v = src->bytes == 1 ? src->pixels[sy * src->pitch + sx]
                                : ((const uint16_t *)(src->pixels + sy * src->pitch))[sx];
            if (keyed && v >= src->srcKey.dwColorSpaceLowValue &&
                v <= src->srcKey.dwColorSpaceHighValue)
                continue;
            if (dst->bytes == 1)
                dst->pixels[dy * dst->pitch + dx] = (uint8_t)v;
            else
                ((uint16_t *)(dst->pixels + dy * dst->pitch))[dx] = (uint16_t)v;
        }
    }
}

/* W32L_BLT_TRACE=1 logs every Blt and BltFast with its rectangles, flags and
 * source key: how the pointer's save-and-restore slot was found. */
static int32_t w32l_blt_trace(void)
{
    static int32_t on = -1;
    if (on < 0) {
        const char *t = getenv("W32L_BLT_TRACE");
        on = t && *t && *t != '0';
    }
    return on;
}

static HRESULT STDMETHODCALLTYPE Surface_Blt(IDirectDrawSurface *p, LPRECT dstRect,
                                             LPDIRECTDRAWSURFACE srcp, LPRECT srcRect,
                                             DWORD flags, LPDDBLTFX fx)
{
    W32L_DDSurface *dst = (W32L_DDSurface *)p;
    W32L_DDSurface *src = (W32L_DDSurface *)srcp;

    if (dst->isPrimary) {
        static int32_t logged;
        if (logged < 3) {
            logged++;
            w32l_plat_debug("Blt to primary rect %d,%d-%d,%d flags 0x%x src %p",
                           dstRect ? (int)dstRect->left : -1, dstRect ? (int)dstRect->top : -1,
                           dstRect ? (int)dstRect->right : -1, dstRect ? (int)dstRect->bottom : -1,
                           (unsigned)flags, (void *)src);
        }
    }
    if (w32l_blt_trace())
        w32l_plat_log("Blt dst=%p %d,%d-%d,%d src=%p %d,%d-%d,%d flags=0x%x key=%u..%u/%d",
                     (void *)dst, dstRect ? (int)dstRect->left : -1, dstRect ? (int)dstRect->top : -1,
                     dstRect ? (int)dstRect->right : -1, dstRect ? (int)dstRect->bottom : -1,
                     (void *)src, srcRect ? (int)srcRect->left : -1, srcRect ? (int)srcRect->top : -1,
                     srcRect ? (int)srcRect->right : -1, srcRect ? (int)srcRect->bottom : -1,
                     (unsigned)flags, src ? (unsigned)src->srcKey.dwColorSpaceLowValue : 0,
                     src ? (unsigned)src->srcKey.dwColorSpaceHighValue : 0, src ? (int)src->hasSrcKey : 0);
    if (flags & DDBLT_COLORFILL) {
        if (!fx)
            return DDERR_INVALIDPARAMS;
        w32l_fill(dst, dstRect, (uint32_t)fx->dwFillColor);
    } else if (src) {
        RECT    d, s;
        int32_t keyed = (flags & DDBLT_KEYSRC) && src->hasSrcKey;
        w32l_rect_of(dst, dstRect, &d);
        w32l_rect_of(src, srcRect, &s);
        if (d.right - d.left == s.right - s.left && d.bottom - d.top == s.bottom - s.top)
            w32l_copy(dst, d.left, d.top, src, &s, keyed);
        else
            w32l_stretch(dst, &d, src, &s, keyed);
    }
    if (dst->isPrimary)
        w32l_present();
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_BltFast(IDirectDrawSurface *p, DWORD x, DWORD y,
                                                 LPDIRECTDRAWSURFACE srcp, LPRECT srcRect,
                                                 DWORD flags)
{
    W32L_DDSurface *dst = (W32L_DDSurface *)p;
    W32L_DDSurface *src = (W32L_DDSurface *)srcp;

    if (!src)
        return DDERR_INVALIDPARAMS;
    if (w32l_blt_trace())
        w32l_plat_log("BltFast dst=%p at %d,%d src=%p %d,%d-%d,%d flags=0x%x key=%u..%u/%d",
                     (void *)dst, (int)x, (int)y, (void *)src,
                     srcRect ? (int)srcRect->left : -1, srcRect ? (int)srcRect->top : -1,
                     srcRect ? (int)srcRect->right : -1, srcRect ? (int)srcRect->bottom : -1,
                     (unsigned)flags, (unsigned)src->srcKey.dwColorSpaceLowValue,
                     (unsigned)src->srcKey.dwColorSpaceHighValue, (int)src->hasSrcKey);
    if (dst->isPrimary) {
        static int32_t logged;
        if (logged < 3) {
            logged++;
            w32l_plat_debug("BltFast to primary at %d,%d from %dx%d rect %d,%d-%d,%d",
                           (int)x, (int)y, src->width, src->height,
                           srcRect ? (int)srcRect->left : -1, srcRect ? (int)srcRect->top : -1,
                           srcRect ? (int)srcRect->right : -1, srcRect ? (int)srcRect->bottom : -1);
        }
    }
    w32l_copy(dst, (int32_t)x, (int32_t)y, src, srcRect,
             (flags & DDBLTFAST_SRCCOLORKEY) && src->hasSrcKey);
    if (dst->isPrimary)
        w32l_present();
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_Flip(IDirectDrawSurface *p, LPDIRECTDRAWSURFACE to, DWORD flags)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;
    uint8_t       *t;

    (void)to; (void)flags;
    if (!s->isPrimary || !s->back)
        return DDERR_UNSUPPORTED;
    /* A real flip exchanges which memory the display scans out, so the
     * buffers swap places; what was the front is now the back. */
    t = s->pixels;
    s->pixels = s->back->pixels;
    s->back->pixels = t;
    w32l_present();
    w32l_host_wait_vblank();
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_GetAttachedSurface(IDirectDrawSurface *p, LPDDSCAPS caps,
                                                            LPDIRECTDRAWSURFACE *out)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    if (caps && (caps->dwCaps & DDSCAPS_BACKBUFFER) && s->back) {
        s->back->refs++;
        *out = (LPDIRECTDRAWSURFACE)s->back;
        return DD_OK;
    }
    *out = NULL;
    return DDERR_NOTFOUND;
}

static HRESULT STDMETHODCALLTYPE Surface_GetColorKey(IDirectDrawSurface *p, DWORD flags, LPDDCOLORKEY out)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    if ((flags & DDCKEY_SRCBLT) && s->hasSrcKey) {
        *out = s->srcKey;
        return DD_OK;
    }
    return DDERR_NOTFOUND;
}

static void w32l_surface_changed_primary(void)
{
    w32l_present();
}

static HRESULT STDMETHODCALLTYPE Surface_GetDC(IDirectDrawSurface *p, HDC *out)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;
    W32L_DCTarget   t;

    t.pixels = s->bytes == 1 ? s->pixels : NULL;
    t.pitch = s->pitch;
    t.width = s->width;
    t.height = s->height;
    t.palette = s->palette ? s->palette->entries : w32l_primary_palette();
    t.changed = s->isPrimary ? w32l_surface_changed_primary : NULL;
    *out = w32l_dc_create(&t);
    return *out ? DD_OK : DDERR_NODC;
}

static HRESULT STDMETHODCALLTYPE Surface_ReleaseDC(IDirectDrawSurface *p, HDC dc)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    w32l_dc_destroy(dc);
    if (s->isPrimary)
        w32l_present();
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_GetPalette(IDirectDrawSurface *p, LPDIRECTDRAWPALETTE *out)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    if (!s->palette) {
        *out = NULL;
        return DDERR_NOPALETTEATTACHED;
    }
    s->palette->refs++;
    *out = (LPDIRECTDRAWPALETTE)s->palette;
    return DD_OK;
}

/* The pixel format of a surface of `bytes` a pixel: palettised 8, or RGB565. */
static void w32l_pixel_format(int32_t bytes, LPDDPIXELFORMAT f)
{
    memset(f, 0, sizeof *f);
    f->dwSize = sizeof *f;
    if (bytes == 1) {
        f->dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        f->dwRGBBitCount = 8;
    } else {
        f->dwFlags = DDPF_RGB;
        f->dwRGBBitCount = 16;
        f->dwRBitMask = 0xF800;
        f->dwGBitMask = 0x07E0;
        f->dwBBitMask = 0x001F;
    }
}

static void w32l_describe(const W32L_DDSurface *s, LPDDSURFACEDESC d, int32_t withPointer,
                         const RECT *rc)
{
    DWORD size = d->dwSize;

    memset(d, 0, size < sizeof *d ? size : sizeof *d);
    d->dwSize = size;
    d->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    d->dwHeight = (DWORD)s->height;
    d->dwWidth = (DWORD)s->width;
    d->lPitch = s->pitch;
    d->dwBackBufferCount = s->back ? 1 : 0;
    w32l_pixel_format(s->bytes, &d->ddpfPixelFormat);
    d->ddsCaps.dwCaps = s->caps;
    if (withPointer) {
        d->dwFlags |= DDSD_LPSURFACE;
        d->lpSurface = s->pixels;
        if (rc)
            d->lpSurface = s->pixels + rc->top * s->pitch + rc->left;
    }
}

static HRESULT STDMETHODCALLTYPE Surface_GetSurfaceDesc(IDirectDrawSurface *p, LPDDSURFACEDESC d)
{
    if (!d || d->dwSize < sizeof *d)
        return DDERR_INVALIDPARAMS;
    w32l_describe((W32L_DDSurface *)p, d, 0, NULL);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_GetPixelFormat(IDirectDrawSurface *p, LPDDPIXELFORMAT f)
{
    if (!f)
        return DDERR_INVALIDPARAMS;
    w32l_pixel_format(((W32L_DDSurface *)p)->bytes, f);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_IsLost(IDirectDrawSurface *p) { (void)p; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surface_Restore(IDirectDrawSurface *p) { (void)p; return DD_OK; }

static HRESULT STDMETHODCALLTYPE Surface_Lock(IDirectDrawSurface *p, LPRECT rc, LPDDSURFACEDESC d,
                                              DWORD flags, HANDLE ev)
{
    (void)flags; (void)ev;
    if (!d || d->dwSize < sizeof *d)
        return DDERR_INVALIDPARAMS;
    w32l_describe((W32L_DDSurface *)p, d, 1, rc);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_Unlock(IDirectDrawSurface *p, LPVOID ptr)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    (void)ptr;
    if (s->isPrimary)
        w32l_present();
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_SetClipper(IDirectDrawSurface *p, LPDIRECTDRAWCLIPPER c)
{
    (void)p; (void)c;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_SetColorKey(IDirectDrawSurface *p, DWORD flags, LPDDCOLORKEY key)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;

    if (!(flags & DDCKEY_SRCBLT))
        return DDERR_UNSUPPORTED;
    if (key) {
        s->srcKey = *key;
        s->hasSrcKey = 1;
    } else {
        s->hasSrcKey = 0;
    }
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surface_SetPalette(IDirectDrawSurface *p, LPDIRECTDRAWPALETTE palp)
{
    W32L_DDSurface *s = (W32L_DDSurface *)p;
    W32L_DDPalette *pal = (W32L_DDPalette *)palp;

    if (pal)
        pal->refs++;
    if (s->palette)
        IDirectDrawPalette_Release((LPDIRECTDRAWPALETTE)s->palette);
    s->palette = pal;
    w32l_plat_debug("SetPalette on %s surface", s->isPrimary ? "the primary" : "an offscreen");
    if (s->isPrimary) {
        if (pal && w32l_exclusive)
            memcpy(w32l_screen_palette, pal->entries, sizeof w32l_screen_palette);
        w32l_present();
    }
    return DD_OK;
}

const IDirectDrawSurfaceVtbl w32l_surface_vtbl = {
    Surface_QueryInterface, Surface_AddRef, Surface_Release,
    Surface_AddAttachedSurface, Surface_AddOverlayDirtyRect, Surface_Blt,
    Surface_BltBatch, Surface_BltFast, Surface_DeleteAttachedSurface,
    Surface_EnumAttachedSurfaces, Surface_EnumOverlayZOrders, Surface_Flip,
    Surface_GetAttachedSurface, Surface_GetBltStatus, Surface_GetCaps,
    Surface_GetClipper, Surface_GetColorKey, Surface_GetDC, Surface_GetFlipStatus,
    Surface_GetOverlayPosition, Surface_GetPalette, Surface_GetPixelFormat,
    Surface_GetSurfaceDesc, Surface_Initialize, Surface_IsLost, Surface_Lock,
    Surface_ReleaseDC, Surface_Restore, Surface_SetClipper, Surface_SetColorKey,
    Surface_SetOverlayPosition, Surface_SetPalette, Surface_Unlock,
    Surface_UpdateOverlay, Surface_UpdateOverlayDisplay, Surface_UpdateOverlayZOrder,
};

/* ---- palettes ----------------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE Palette_QueryInterface(IDirectDrawPalette *p, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, IID_IDirectDrawPalette)) {
        *out = p;
        p->lpVtbl->AddRef(p);
        return DD_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Palette_AddRef(IDirectDrawPalette *p)
{
    return (ULONG)++((W32L_DDPalette *)p)->refs;
}

static ULONG STDMETHODCALLTYPE Palette_Release(IDirectDrawPalette *p)
{
    W32L_DDPalette *pal = (W32L_DDPalette *)p;
    int32_t        n = --pal->refs;

    if (n <= 0) {
        free(pal);
        return 0;
    }
    return (ULONG)n;
}

static HRESULT STDMETHODCALLTYPE Palette_GetCaps(IDirectDrawPalette *p, LPDWORD out)
{
    (void)p;
    *out = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Palette_GetEntries(IDirectDrawPalette *p, DWORD flags, DWORD start,
                                                    DWORD count, LPPALETTEENTRY out)
{
    W32L_DDPalette *pal = (W32L_DDPalette *)p;

    (void)flags;
    if (start + count > 256)
        return DDERR_INVALIDPARAMS;
    memcpy(out, pal->entries + start, count * sizeof(PALETTEENTRY));
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Palette_Initialize(IDirectDrawPalette *p, LPDIRECTDRAW dd, DWORD flags,
                                                    LPPALETTEENTRY entries)
{
    (void)p; (void)dd; (void)flags; (void)entries;
    return DDERR_UNSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE Palette_SetEntries(IDirectDrawPalette *p, DWORD flags, DWORD start,
                                                    DWORD count, LPPALETTEENTRY entries)
{
    W32L_DDPalette *pal = (W32L_DDPalette *)p;

    (void)flags;
    if (start + count > 256 || !entries)
        return DDERR_INVALIDPARAMS;
    memcpy(pal->entries + start, entries, count * sizeof(PALETTEENTRY));
    /* A palette change on the screen shows at once -- the game fades this
     * way, without flipping between steps. */
    if (w32l_primary && w32l_primary->palette == pal) {
        if (w32l_exclusive)
            memcpy(w32l_screen_palette, pal->entries, sizeof w32l_screen_palette);
        w32l_present();
    }
    return DD_OK;
}

const IDirectDrawPaletteVtbl w32l_palette_vtbl = {
    Palette_QueryInterface, Palette_AddRef, Palette_Release,
    Palette_GetCaps, Palette_GetEntries, Palette_Initialize, Palette_SetEntries,
};

/* ---- the DirectDraw object --------------------------------------------------------- */

static W32L_DDraw *w32l_ddraw_of(void *p, int32_t v2)
{
    return (W32L_DDraw *)((uint8_t *)p - (v2 ? sizeof(void *) : 0));
}

static HRESULT w32l_ddraw_qi(W32L_DDraw *dd, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, IID_IDirectDraw)) {
        *out = &dd->lpVtbl;
    } else if (IsEqualGUID(iid, IID_IDirectDraw2)) {
        *out = &dd->lpVtbl2;
    } else {
        *out = NULL;
        return E_NOINTERFACE;
    }
    dd->refs++;
    return DD_OK;
}

static ULONG w32l_ddraw_release(W32L_DDraw *dd)
{
    int32_t n = --dd->refs;

    if (n <= 0) {
        free(dd);
        return 0;
    }
    return (ULONG)n;
}

static HRESULT w32l_ddraw_create_palette(W32L_DDraw *dd, DWORD flags, LPPALETTEENTRY entries,
                                        LPDIRECTDRAWPALETTE *out)
{
    W32L_DDPalette *pal = (W32L_DDPalette *)calloc(1, sizeof *pal);

    (void)dd; (void)flags;
    if (!pal)
        return DDERR_OUTOFMEMORY;
    pal->lpVtbl = &w32l_palette_vtbl;
    pal->refs = 1;
    if (entries)
        memcpy(pal->entries, entries, sizeof pal->entries);
    *out = (LPDIRECTDRAWPALETTE)pal;
    return DD_OK;
}

static HRESULT w32l_ddraw_create_surface(W32L_DDraw *dd, LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE *out)
{
    W32L_DDSurface *s;
    DWORD          caps;

    *out = NULL;
    if (!d || d->dwSize < sizeof *d || !(d->dwFlags & DDSD_CAPS))
        return DDERR_INVALIDPARAMS;
    caps = d->ddsCaps.dwCaps;
    w32l_plat_debug("CreateSurface caps=0x%x %ux%u", (unsigned)caps,
                   (unsigned)d->dwWidth, (unsigned)d->dwHeight);
    if (caps & DDSCAPS_PRIMARYSURFACE) {
        int32_t w = dd->modeW ? dd->modeW : 640;
        int32_t h = dd->modeH ? dd->modeH : 480;
        if (w32l_primary) {
            w32l_primary->refs++;
            *out = (LPDIRECTDRAWSURFACE)w32l_primary;
            return DD_OK;
        }
        s = w32l_surface_new(w, h, caps | DDSCAPS_VIDEOMEMORY, w32l_display_bpp > 8 ? 2 : 1);
        if (!s)
            return DDERR_OUTOFMEMORY;
        s->isPrimary = 1;
        if ((d->dwFlags & DDSD_BACKBUFFERCOUNT) && d->dwBackBufferCount > 0) {
            s->back = w32l_surface_new(w, h, DDSCAPS_BACKBUFFER | DDSCAPS_FLIP |
                                            DDSCAPS_VIDEOMEMORY, s->bytes);
            if (!s->back) {
                w32l_surface_free(s);
                return DDERR_OUTOFMEMORY;
            }
        }
        w32l_primary = s;
        w32l_host_display_mode(w, h);
        *out = (LPDIRECTDRAWSURFACE)s;
        return DD_OK;
    }
    if (!(d->dwFlags & DDSD_WIDTH) || !(d->dwFlags & DDSD_HEIGHT) ||
        d->dwWidth == 0 || d->dwHeight == 0 || d->dwWidth > 16384 || d->dwHeight > 16384)
        return DDERR_INVALIDPARAMS;
    {
        /* The display's format unless the description asks for another. */
        int32_t bytes = w32l_display_bpp > 8 ? 2 : 1;
        if ((d->dwFlags & DDSD_PIXELFORMAT) && (d->ddpfPixelFormat.dwFlags & DDPF_RGB))
            bytes = d->ddpfPixelFormat.dwRGBBitCount == 8 ? 1 : 2;
        s = w32l_surface_new((int32_t)d->dwWidth, (int32_t)d->dwHeight,
                            caps | ((caps & DDSCAPS_SYSTEMMEMORY) ? 0 : DDSCAPS_VIDEOMEMORY), bytes);
    }
    if (!s)
        return DDERR_OUTOFMEMORY;
    *out = (LPDIRECTDRAWSURFACE)s;
    return DD_OK;
}

static HRESULT w32l_ddraw_set_mode(W32L_DDraw *dd, DWORD w, DWORD h, DWORD bpp)
{
    if (w == 0 || h == 0 || w > 4096 || h > 4096)
        return DDERR_INVALIDPARAMS;
    if (bpp != 0 && bpp != 8 && bpp != 16)
        return DDERR_INVALIDMODE;
    if (bpp)
        w32l_display_bpp = (int32_t)bpp;     /* the screen is now in this depth */
    dd->modeW = (int32_t)w;
    dd->modeH = (int32_t)h;
    dd->modeBpp = w32l_display_bpp;
    w32l_host_display_mode((int32_t)w, (int32_t)h);
    w32l_plat_log("display mode %ux%u at %d bpp", (unsigned)w, (unsigned)h, (int)w32l_display_bpp);
    return DD_OK;
}

/* The two vtables share most of their entries, so each method is written
 * once over the shared state and wrapped for whichever view is calling. */
#define DD_METHOD(name, v2, ...) \
    static HRESULT STDMETHODCALLTYPE name##_##v2(__VA_ARGS__)

/* ---- IDirectDrawClipper: holds the window and list, clips nothing ---------- */

struct W32L_DDClipper { const IDirectDrawClipperVtbl *lpVtbl; ULONG refs; HWND hwnd; };

static HRESULT STDMETHODCALLTYPE Clip_QueryInterface(IDirectDrawClipper *p, REFIID, LPVOID *out)
{ *out = p; return DD_OK; }
static ULONG STDMETHODCALLTYPE Clip_AddRef(IDirectDrawClipper *p)
{ return ++((W32L_DDClipper *)p)->refs; }
static ULONG STDMETHODCALLTYPE Clip_Release(IDirectDrawClipper *p)
{
    W32L_DDClipper *c = (W32L_DDClipper *)p;
    ULONG n = --c->refs;
    if (!n)
        free(c);
    return n;
}
static HRESULT STDMETHODCALLTYPE Clip_GetClipList(IDirectDrawClipper *, LPRECT, LPVOID, LPDWORD)
{ return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Clip_GetHWnd(IDirectDrawClipper *p, HWND *h)
{ *h = ((W32L_DDClipper *)p)->hwnd; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Clip_Initialize(IDirectDrawClipper *, LPDIRECTDRAW, DWORD)
{ return DD_OK; }
static HRESULT STDMETHODCALLTYPE Clip_IsClipListChanged(IDirectDrawClipper *, BOOL *b)
{ *b = 0; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Clip_SetClipList(IDirectDrawClipper *, LPVOID, DWORD)
{ return DD_OK; }
static HRESULT STDMETHODCALLTYPE Clip_SetHWnd(IDirectDrawClipper *p, DWORD, HWND h)
{ ((W32L_DDClipper *)p)->hwnd = h; return DD_OK; }

static const IDirectDrawClipperVtbl w32l_clipper_vtbl = {
    Clip_QueryInterface, Clip_AddRef, Clip_Release, Clip_GetClipList, Clip_GetHWnd,
    Clip_Initialize, Clip_IsClipListChanged, Clip_SetClipList, Clip_SetHWnd,
};

static HRESULT w32l_create_clipper(LPDIRECTDRAWCLIPPER *out)
{
    W32L_DDClipper *c = (W32L_DDClipper *)calloc(1, sizeof *c);
    if (!c)
        return DDERR_OUTOFMEMORY;
    c->lpVtbl = &w32l_clipper_vtbl;
    c->refs = 1;
    *out = (LPDIRECTDRAWCLIPPER)c;
    return DD_OK;
}

/* The modes offered: the usual 4:3 sizes in the program's depth, and 8 bit. */
static HRESULT w32l_enum_modes(LPDDENUMMODESCALLBACK cb, LPVOID ctx)
{
    static const int32_t sizes[][2] = { {320,240}, {640,480}, {800,600}, {1024,768} };
    static const int32_t depths[] = { 8, 16 };
    size_t i, k;

    if (!cb)
        return DDERR_INVALIDPARAMS;
    for (k = 0; k < 2; k++)
        for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
            DDSURFACEDESC d;
            memset(&d, 0, sizeof d);
            d.dwSize = sizeof d;
            d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            d.dwWidth = (DWORD)sizes[i][0];
            d.dwHeight = (DWORD)sizes[i][1];
            w32l_pixel_format(depths[k] == 8 ? 1 : 2, &d.ddpfPixelFormat);
            if (!cb(&d, ctx))
                return DD_OK;
        }
    return DD_OK;
}

#define DD_BOTH(name, body, ...) \
    static HRESULT STDMETHODCALLTYPE DD1_##name(IDirectDraw *p, ##__VA_ARGS__) { W32L_DDraw *dd = w32l_ddraw_of(p, 0); (void)dd; body } \
    static HRESULT STDMETHODCALLTYPE DD2_##name(IDirectDraw2 *p, ##__VA_ARGS__) { W32L_DDraw *dd = w32l_ddraw_of(p, 1); (void)dd; body }

DD_BOTH(QueryInterface, { return w32l_ddraw_qi(dd, iid, out); }, REFIID iid, LPVOID *out)
DD_BOTH(Compact, { return DD_OK; })
DD_BOTH(CreateClipper, { (void)flags; (void)outer; return w32l_create_clipper(out); }, DWORD flags, LPDIRECTDRAWCLIPPER *out, IUnknown *outer)
DD_BOTH(CreatePalette, { (void)outer; return w32l_ddraw_create_palette(dd, flags, entries, out); }, DWORD flags, LPPALETTEENTRY entries, LPDIRECTDRAWPALETTE *out, IUnknown *outer)
DD_BOTH(CreateSurface, { (void)outer; return w32l_ddraw_create_surface(dd, d, out); }, LPDDSURFACEDESC d, LPDIRECTDRAWSURFACE *out, IUnknown *outer)
DD_BOTH(DuplicateSurface, { (void)s; *out = NULL; return DDERR_UNSUPPORTED; }, LPDIRECTDRAWSURFACE s, LPDIRECTDRAWSURFACE *out)
DD_BOTH(EnumDisplayModes, { (void)flags; (void)d; return w32l_enum_modes(cb, ctx); }, DWORD flags, LPDDSURFACEDESC d, LPVOID ctx, LPDDENUMMODESCALLBACK cb)
DD_BOTH(EnumSurfaces, { (void)flags; (void)d; (void)ctx; (void)cb; return DDERR_UNSUPPORTED; }, DWORD flags, LPDDSURFACEDESC d, LPVOID ctx, LPDDENUMSURFACESCALLBACK cb)
DD_BOTH(FlipToGDISurface, { return DD_OK; })
DD_BOTH(GetCaps, { if (drv) { memset(drv, 0, sizeof *drv); drv->dwSize = sizeof *drv; } if (hel) { memset(hel, 0, sizeof *hel); hel->dwSize = sizeof *hel; } return DD_OK; }, LPDDCAPS drv, LPDDCAPS hel)
DD_BOTH(GetDisplayMode, { if (!d || d->dwSize < sizeof *d) return DDERR_INVALIDPARAMS; memset(d, 0, sizeof *d); d->dwSize = sizeof *d; d->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT; d->dwWidth = dd->modeW ? dd->modeW : 640; d->dwHeight = dd->modeH ? dd->modeH : 480; w32l_pixel_format(w32l_display_bpp > 8 ? 2 : 1, &d->ddpfPixelFormat); return DD_OK; }, LPDDSURFACEDESC d)
DD_BOTH(GetFourCCCodes, { *n = 0; (void)codes; return DD_OK; }, LPDWORD n, LPDWORD codes)
DD_BOTH(GetGDISurface, { if (!w32l_primary) { *out = NULL; return DDERR_NOTFOUND; } w32l_primary->refs++; *out = (LPDIRECTDRAWSURFACE)w32l_primary; return DD_OK; }, LPDIRECTDRAWSURFACE *out)
DD_BOTH(GetMonitorFrequency, { *out = 60; return DD_OK; }, LPDWORD out)
DD_BOTH(GetScanLine, { *out = 0; return DD_OK; }, LPDWORD out)
DD_BOTH(GetVerticalBlankStatus, { *out = TRUE; return DD_OK; }, LPBOOL out)
DD_BOTH(Initialize, { (void)g; return DDERR_UNSUPPORTED; }, GUID *g)
DD_BOTH(RestoreDisplayMode, { return DD_OK; })
DD_BOTH(SetCooperativeLevel, { (void)hwnd; w32l_exclusive = (flags & DDSCL_EXCLUSIVE) != 0; w32l_plat_debug("SetCooperativeLevel 0x%x", (unsigned)flags); return DD_OK; }, HWND hwnd, DWORD flags)
DD_BOTH(WaitForVerticalBlank, { (void)flags; (void)ev; return DD_OK; }, DWORD flags, HANDLE ev)

static ULONG STDMETHODCALLTYPE DD1_AddRef(IDirectDraw *p) { return (ULONG)++w32l_ddraw_of(p, 0)->refs; }
static ULONG STDMETHODCALLTYPE DD2_AddRef(IDirectDraw2 *p) { return (ULONG)++w32l_ddraw_of(p, 1)->refs; }
static ULONG STDMETHODCALLTYPE DD1_Release(IDirectDraw *p) { return w32l_ddraw_release(w32l_ddraw_of(p, 0)); }
static ULONG STDMETHODCALLTYPE DD2_Release(IDirectDraw2 *p) { return w32l_ddraw_release(w32l_ddraw_of(p, 1)); }
static HRESULT STDMETHODCALLTYPE DD1_SetDisplayMode(IDirectDraw *p, DWORD w, DWORD h, DWORD bpp)
{
    return w32l_ddraw_set_mode(w32l_ddraw_of(p, 0), w, h, bpp);
}
static HRESULT STDMETHODCALLTYPE DD2_SetDisplayMode(IDirectDraw2 *p, DWORD w, DWORD h, DWORD bpp,
                                                    DWORD refresh, DWORD flags)
{
    (void)refresh; (void)flags;
    return w32l_ddraw_set_mode(w32l_ddraw_of(p, 1), w, h, bpp);
}
static HRESULT STDMETHODCALLTYPE DD2_GetAvailableVidMem(IDirectDraw2 *p, LPDDSCAPS caps,
                                                        LPDWORD total, LPDWORD freeMem)
{
    (void)p; (void)caps;
    if (total)
        *total = 64u << 20;
    if (freeMem)
        *freeMem = 64u << 20;
    return DD_OK;
}

const IDirectDrawVtbl w32l_ddraw_vtbl = {
    DD1_QueryInterface, DD1_AddRef, DD1_Release, DD1_Compact, DD1_CreateClipper,
    DD1_CreatePalette, DD1_CreateSurface, DD1_DuplicateSurface, DD1_EnumDisplayModes,
    DD1_EnumSurfaces, DD1_FlipToGDISurface, DD1_GetCaps, DD1_GetDisplayMode,
    DD1_GetFourCCCodes, DD1_GetGDISurface, DD1_GetMonitorFrequency, DD1_GetScanLine,
    DD1_GetVerticalBlankStatus, DD1_Initialize, DD1_RestoreDisplayMode,
    DD1_SetCooperativeLevel, DD1_SetDisplayMode, DD1_WaitForVerticalBlank,
};

const IDirectDraw2Vtbl w32l_ddraw2_vtbl = {
    DD2_QueryInterface, DD2_AddRef, DD2_Release, DD2_Compact, DD2_CreateClipper,
    DD2_CreatePalette, DD2_CreateSurface, DD2_DuplicateSurface, DD2_EnumDisplayModes,
    DD2_EnumSurfaces, DD2_FlipToGDISurface, DD2_GetCaps, DD2_GetDisplayMode,
    DD2_GetFourCCCodes, DD2_GetGDISurface, DD2_GetMonitorFrequency, DD2_GetScanLine,
    DD2_GetVerticalBlankStatus, DD2_Initialize, DD2_RestoreDisplayMode,
    DD2_SetCooperativeLevel, DD2_SetDisplayMode, DD2_WaitForVerticalBlank,
    DD2_GetAvailableVidMem,
};

HRESULT WINAPI DirectDrawCreate(GUID *guid, LPDIRECTDRAW *out, IUnknown *outer)
{
    W32L_DDraw *dd;

    (void)guid; (void)outer;
    dd = (W32L_DDraw *)calloc(1, sizeof *dd);
    if (!dd)
        return DDERR_OUTOFMEMORY;
    dd->lpVtbl = &w32l_ddraw_vtbl;
    dd->lpVtbl2 = &w32l_ddraw2_vtbl;
    dd->refs = 1;
    *out = (LPDIRECTDRAW)&dd->lpVtbl;
    return DD_OK;
}
