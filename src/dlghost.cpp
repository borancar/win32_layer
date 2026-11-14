/* dlghost.cpp -- the host side of dialogs: one SDL window for every top-level
 * dialog, a software canvas it is drawn on, the window's events handed to
 * dialog.cpp, and PPM dumps of the canvas.
 *
 * The canvas is an SDL surface with a software renderer over it, drawn with
 * SDL's own 8x8 debug font; the window shows a copy of it. A host that cannot
 * make a window (no video) leaves the canvas alone, so a dialog still
 * paints, and can still be dumped, without one.
 */
#include "dialog.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct DlgHost {
    SDL_Window   *window;
    SDL_Surface  *canvas;
    SDL_Renderer *renderer;
    void         *user;
    int32_t       w, h, cap;     /* the canvas is h + cap rows: a caption above the client area */
};

#define W32L_DLG_HOSTS 64
static DlgHost *w32l_hosts[W32L_DLG_HOSTS];

static void w32l_dlghost_canvas(DlgHost *h)
{
    if (h->renderer)
        SDL_DestroyRenderer(h->renderer);
    if (h->canvas)
        SDL_DestroySurface(h->canvas);
    h->canvas = SDL_CreateSurface(h->w, h->h + h->cap, SDL_PIXELFORMAT_XRGB8888);
    h->renderer = h->canvas ? SDL_CreateSoftwareRenderer(h->canvas) : NULL;
}

DlgHost *w32l_dlghost_create(const char *title, int32_t cw, int32_t ch, int32_t cap, int32_t x, int32_t y, void *user)
{
    DlgHost *h = (DlgHost *)calloc(1, sizeof *h);
    int32_t  i;

    h->w = cw > 0 ? cw : 1;
    h->h = ch > 0 ? ch : 1;
    h->user = user;
    h->cap = cap;
    w32l_dlghost_canvas(h);
    h->window = SDL_CreateWindow(title ? title : "", h->w, h->h, SDL_WINDOW_HIDDEN);
    if (h->window) {
        SDL_SetWindowPosition(h->window, x, y);
        SDL_StartTextInput(h->window);
    } else
        w32l_plat_debug("dialog window: %s", SDL_GetError());
    for (i = 0; i < W32L_DLG_HOSTS; i++)
        if (!w32l_hosts[i]) {
            w32l_hosts[i] = h;
            break;
        }
    return h;
}

void w32l_dlghost_destroy(DlgHost *h)
{
    int32_t i;

    if (!h)
        return;
    for (i = 0; i < W32L_DLG_HOSTS; i++)
        if (w32l_hosts[i] == h)
            w32l_hosts[i] = NULL;
    if (h->renderer)
        SDL_DestroyRenderer(h->renderer);
    if (h->canvas)
        SDL_DestroySurface(h->canvas);
    if (h->window)
        SDL_DestroyWindow(h->window);
    free(h);
}

void w32l_dlghost_show(DlgHost *h, int32_t visible)
{
    if (!h || !h->window)
        return;
    if (visible)
        SDL_ShowWindow(h->window);
    else
        SDL_HideWindow(h->window);
}

void w32l_dlghost_title(DlgHost *h, const char *title)
{
    if (h && h->window)
        SDL_SetWindowTitle(h->window, title ? title : "");
}

void w32l_dlghost_place(DlgHost *h, int32_t x, int32_t y, int32_t cw, int32_t ch)
{
    if (!h)
        return;
    if (cw > 0 && ch > 0 && (cw != h->w || ch != h->h)) {
        h->w = cw;
        h->h = ch;
        w32l_dlghost_canvas(h);
        if (h->window)
            SDL_SetWindowSize(h->window, cw, ch);
    }
    if (h->window)
        SDL_SetWindowPosition(h->window, x, y);
}

void w32l_dlghost_raise(DlgHost *h)
{
    if (h && h->window)
        SDL_RaiseWindow(h->window);
}

void w32l_dlghost_desktop(int32_t *w, int32_t *h)
{
    SDL_DisplayID id = SDL_GetPrimaryDisplay();
    SDL_Rect      r;

    *w = 1280;
    *h = 1024;
    if (id && SDL_GetDisplayBounds(id, &r) && r.w > 0 && r.h > 0) {
        *w = r.w;
        *h = r.h;
    }
}

/* ---- the canvas ----------------------------------------------------------------- */

void w32l_dlghost_fill(DlgHost *h, int32_t x, int32_t y, int32_t w, int32_t ht, uint32_t rgb)
{
    SDL_FRect r;

    if (!h || !h->renderer || w <= 0 || ht <= 0)
        return;
    r.x = (float)x; r.y = (float)y; r.w = (float)w; r.h = (float)ht;
    SDL_SetRenderDrawColor(h->renderer, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, 255);
    SDL_RenderFillRect(h->renderer, &r);
}

/* The debug font has ASCII only: a Latin-1 letter is drawn as its base letter. */
static char w32l_dlg_ascii(uint8_t c)
{
    static const char base[] =
        "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";

    if (c < 0x80)
        return (char)c;
    if (c >= 0xC0)
        return base[c - 0xC0];
    if (c == 0xBF) return '?';
    if (c == 0xA1) return '!';
    return '?';
}

void w32l_dlghost_text(DlgHost *h, int32_t x, int32_t y, uint32_t rgb, const char *text, int32_t n)
{
    char    buf[1024];
    int32_t i, o = 0;

    if (!h || !h->renderer)
        return;
    for (i = 0; i < n && text[i] && o + 3 < (int32_t)sizeof buf; i++) {
        buf[o++] = w32l_dlg_ascii((uint8_t)text[i]);
    }
    buf[o] = 0;
    SDL_SetRenderDrawColor(h->renderer, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, 255);
    SDL_RenderDebugText(h->renderer, (float)x, (float)y, buf);
}

void w32l_dlghost_clip(DlgHost *h, int32_t x, int32_t y, int32_t w, int32_t ht)
{
    SDL_Rect r;

    if (!h || !h->renderer)
        return;
    r.x = x; r.y = y; r.w = w > 0 ? w : 0; r.h = ht > 0 ? ht : 0;
    SDL_SetRenderClipRect(h->renderer, &r);
}

void w32l_dlghost_unclip(DlgHost *h)
{
    if (h && h->renderer)
        SDL_SetRenderClipRect(h->renderer, NULL);
}

void w32l_dlghost_present(DlgHost *h)
{
    SDL_Surface *ws;

    if (!h || !h->window || !h->canvas)
        return;
    SDL_FlushRenderer(h->renderer);
    ws = SDL_GetWindowSurface(h->window);
    if (ws) {
        SDL_Rect src;
        src.x = 0; src.y = h->cap; src.w = h->w; src.h = h->h;
        SDL_BlitSurface(h->canvas, &src, ws, NULL);
        SDL_UpdateWindowSurface(h->window);
    }
}

int32_t w32l_dlghost_dump(DlgHost *h, const char *path)
{
    FILE    *f;
    int32_t  x, y;

    if (!h || !h->canvas || !(f = fopen(path, "wb")))
        return 0;
    SDL_FlushRenderer(h->renderer);
    fprintf(f, "P6\n%d %d\n255\n", h->w, h->h + h->cap);
    for (y = 0; y < h->h + h->cap; y++) {
        const uint8_t *row = (const uint8_t *)h->canvas->pixels + (size_t)y * h->canvas->pitch;
        for (x = 0; x < h->w; x++) {
            uint32_t p;
            uint8_t  rgb[3];
            memcpy(&p, row + x * 4, 4);
            rgb[0] = (uint8_t)(p >> 16); rgb[1] = (uint8_t)(p >> 8); rgb[2] = (uint8_t)p;
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
    return 1;
}

char *w32l_dlghost_clip_get(void)
{
    char *u = SDL_GetClipboardText(), *out;
    size_t i, o = 0;

    if (!u)
        return NULL;
    out = (char *)malloc(strlen(u) + 1);
    for (i = 0; u[i]; i++) {
        uint8_t c = (uint8_t)u[i];
        if (c < 0x80)
            out[o++] = (char)c;
        else if ((c & 0xE0) == 0xC0 && (u[i + 1] & 0xC0) == 0x80) {
            uint32_t cp = ((uint32_t)(c & 0x1F) << 6) | (u[i + 1] & 0x3F);
            out[o++] = cp < 0x100 ? (char)cp : '?';
            i++;
        }
    }
    out[o] = 0;
    SDL_free(u);
    return out;
}

void w32l_dlghost_clip_set(const char *text, int32_t n)
{
    char    *u = (char *)malloc((size_t)n * 2 + 1);
    int32_t  i, o = 0;

    for (i = 0; i < n; i++) {
        uint8_t c = (uint8_t)text[i];
        if (c < 0x80)
            u[o++] = (char)c;
        else {
            u[o++] = (char)(0xC0 | (c >> 6));
            u[o++] = (char)(0x80 | (c & 0x3F));
        }
    }
    u[o] = 0;
    SDL_SetClipboardText(u);
    free(u);
}

/* ---- events ----------------------------------------------------------------------- */

static DlgHost *w32l_dlghost_of(SDL_WindowID id)
{
    int32_t i;

    for (i = 0; i < W32L_DLG_HOSTS; i++)
        if (w32l_hosts[i] && w32l_hosts[i]->window && SDL_GetWindowID(w32l_hosts[i]->window) == id)
            return w32l_hosts[i];
    return NULL;
}

static uint32_t w32l_dlg_vk(SDL_Keycode k)
{
    if (k >= 'a' && k <= 'z')
        return (uint32_t)(k - 'a' + 'A');
    if (k >= '0' && k <= '9')
        return (uint32_t)k;
    if (k >= SDLK_F1 && k <= SDLK_F12)
        return 0x70 + (uint32_t)(k - SDLK_F1);
    switch (k) {
    case SDLK_BACKSPACE: return 0x08;
    case SDLK_TAB:       return 0x09;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
    case SDLK_ESCAPE:    return 0x1B;
    case SDLK_SPACE:     return 0x20;
    case SDLK_PAGEUP:    return 0x21;
    case SDLK_PAGEDOWN:  return 0x22;
    case SDLK_END:       return 0x23;
    case SDLK_HOME:      return 0x24;
    case SDLK_LEFT:      return 0x25;
    case SDLK_UP:        return 0x26;
    case SDLK_RIGHT:     return 0x27;
    case SDLK_DOWN:      return 0x28;
    case SDLK_INSERT:    return 0x2D;
    case SDLK_DELETE:    return 0x2E;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
    case SDLK_LCTRL: case SDLK_RCTRL: return 0x11;
    case SDLK_LALT: case SDLK_RALT: return 0x12;
    }
    return 0;
}

int32_t w32l_dlghost_event(const SDL_Event *ev, int32_t deliver)
{
    SDL_WindowID id;
    DlgHost     *h;

    switch (ev->type) {
    case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: id = ev->key.windowID; break;
    case SDL_EVENT_TEXT_INPUT:       id = ev->text.windowID; break;
    case SDL_EVENT_MOUSE_MOTION:     id = ev->motion.windowID; break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: id = ev->button.windowID; break;
    case SDL_EVENT_MOUSE_WHEEL:      id = ev->wheel.windowID; break;
    default:
        if (ev->type >= SDL_EVENT_WINDOW_FIRST && ev->type <= SDL_EVENT_WINDOW_LAST)
            id = ev->window.windowID;
        else
            return 0;
    }
    h = w32l_dlghost_of(id);
    if (!h)
        return 0;
    if (!deliver)
        return 1;
    DWin *d = (DWin *)h->user;
    switch (ev->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED: w32l_dw_host_close(d); break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:    w32l_dw_host_focus(d, 1); break;
    case SDL_EVENT_WINDOW_MOUSE_ENTER:     SDL_ShowCursor(); break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:      w32l_dw_host_focus(d, 0); break;
    case SDL_EVENT_WINDOW_MOVED:           w32l_dw_host_moved(d, ev->window.data1, ev->window.data2); break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        uint32_t vk = w32l_dlg_vk(ev->key.key);
        if (vk)
            w32l_dw_host_key(d, ev->key.down, vk);
        break;
    }
    case SDL_EVENT_TEXT_INPUT: {
        const char *s;
        for (s = ev->text.text; *s; s++) {
            uint8_t c = (uint8_t)*s;
            if (c < 0x80)
                w32l_dw_host_char(d, c);
            else if ((c & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
                uint32_t cp = ((uint32_t)(c & 0x1F) << 6) | (s[1] & 0x3F);
                if (cp < 0x100)
                    w32l_dw_host_char(d, cp);
                s++;
            }
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        w32l_dw_host_mouse(d, W32L_DM_MOVE, 0, (int32_t)ev->motion.x, (int32_t)ev->motion.y, 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int32_t b = ev->button.button == SDL_BUTTON_LEFT ? 0
                  : ev->button.button == SDL_BUTTON_RIGHT ? 1 : 2;
        w32l_dw_host_mouse(d, !ev->button.down ? W32L_DM_UP : ev->button.clicks >= 2 ? W32L_DM_DBL : W32L_DM_DOWN,
                           b, (int32_t)ev->button.x, (int32_t)ev->button.y, 0);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        w32l_dw_host_mouse(d, W32L_DM_WHEEL, 0, (int32_t)ev->wheel.mouse_x, (int32_t)ev->wheel.mouse_y,
                           (int32_t)(ev->wheel.y * 120));
        break;
    }
    return 1;
}
