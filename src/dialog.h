/* dialog.h -- what dialog.cpp, dlghost.cpp and the rest of the layer share
 * about dialogs. Private to the layer: the program sees windows.h only.
 *
 *   dialog.cpp   dialog templates, the dialog and control windows, the
 *                standard controls (button, edit, static, list box, combo
 *                box, scroll bar), their messages and their painting.
 *   dlghost.cpp  one SDL window per top-level dialog: a software canvas, the
 *                host's events turned into dialog input, and PPM dumps.
 *   resource.cpp the program image's resources (templates, strings, menus).
 */
#ifndef W32L_DIALOG_H
#define W32L_DIALOG_H

#include "platform.h"

/* ---- dlghost.cpp: the host window and its canvas of one dialog -------------- */

struct DlgHost;
union SDL_Event;

/* A window with a client area of cw x ch at screen (x, y), hidden, whose canvas has
 * `cap` more rows above the client area (a caption, seen in dumps only). `user` is
 * handed back with every event. NULL when the host cannot make one; the
 * dialog then lives without a window (its canvas is still there). */
DlgHost *w32l_dlghost_create(const char *title, int32_t cw, int32_t ch, int32_t cap, int32_t x, int32_t y, void *user);
void     w32l_dlghost_destroy(DlgHost *h);
void     w32l_dlghost_show(DlgHost *h, int32_t visible);
void     w32l_dlghost_title(DlgHost *h, const char *title);
void     w32l_dlghost_place(DlgHost *h, int32_t x, int32_t y, int32_t cw, int32_t ch);
void     w32l_dlghost_raise(DlgHost *h);
void     w32l_dlghost_desktop(int32_t *w, int32_t *h);

/* The canvas: 0xRRGGBB colours, pixels, Latin-1 text in the host's 8x8 font. */
void     w32l_dlghost_fill(DlgHost *h, int32_t x, int32_t y, int32_t w, int32_t ht, uint32_t rgb);
void     w32l_dlghost_text(DlgHost *h, int32_t x, int32_t y, uint32_t rgb, const char *text, int32_t n);
void     w32l_dlghost_clip(DlgHost *h, int32_t x, int32_t y, int32_t w, int32_t ht);
void     w32l_dlghost_unclip(DlgHost *h);
void     w32l_dlghost_present(DlgHost *h);
int32_t  w32l_dlghost_dump(DlgHost *h, const char *path);
/* The host's clipboard, as Latin-1; the result is malloc'd. */
char    *w32l_dlghost_clip_get(void);
void     w32l_dlghost_clip_set(const char *text, int32_t n);

/* One SDL event: 1 when it belonged to a dialog's window and is used up. With
 * `deliver` 0 (a lockstep run, where only the script gives input) it is only
 * used up. */
int32_t  w32l_dlghost_event(const union SDL_Event *ev, int32_t deliver);

/* ---- dialog.cpp: what the host reports ------------------------------------------- */

struct DWin;
enum { W32L_DM_MOVE, W32L_DM_DOWN, W32L_DM_UP, W32L_DM_DBL, W32L_DM_WHEEL };
void w32l_dw_host_mouse(DWin *dlg, int32_t kind, int32_t button, int32_t x, int32_t y, int32_t wheel);
void w32l_dw_host_key(DWin *dlg, int32_t down, uint32_t vk);
void w32l_dw_host_char(DWin *dlg, uint32_t ch);
void w32l_dw_host_close(DWin *dlg);
void w32l_dw_host_focus(DWin *dlg, int32_t gained);
void w32l_dw_host_moved(DWin *dlg, int32_t x, int32_t y);

/* ---- dialog.cpp: what user32.cpp delegates -------------------------------------------- */

int32_t w32l_dw_valid(HWND hwnd);
LRESULT w32l_dw_send(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LONG    w32l_dw_getlong(HWND hwnd, int32_t index);
LONG    w32l_dw_setlong(HWND hwnd, int32_t index, LONG value);
BOOL    w32l_dw_show(HWND hwnd, int32_t cmd);
BOOL    w32l_dw_destroy(HWND hwnd);
BOOL    w32l_dw_rect(HWND hwnd, LPRECT rc, int32_t client);
BOOL    w32l_dw_move(HWND hwnd, int32_t x, int32_t y, int32_t w, int32_t h, int32_t flags);
BOOL    w32l_dw_set_text(HWND hwnd, LPCSTR text);
BOOL    w32l_dw_to_screen(HWND hwnd, LPPOINT pt);
HWND    w32l_dw_focus(void);
HWND    w32l_dw_set_focus(HWND hwnd);
void    w32l_dw_raise(HWND hwnd);
HWND    w32l_dw_at_point(POINT pt);
void    w32l_dw_invalidate(HWND hwnd);

/* A registered window class's procedure by name, user32.cpp's table. */
WNDPROC w32l_class_proc(const char *name);

/* Once per pump: repaint what changed, present, and write the dumps. */
void    w32l_dlg_frame(void);
/* The last dumps, at exit. */
void    w32l_dlg_shutdown(void);
/* A replay action on dialogs; 0 when the verb is not a dialog one. */
int32_t w32l_dlg_replay(const char *verb, int32_t a, int32_t b, const char *text);
/* The host's focus moved between the game's window and a dialog's. */
int32_t w32l_dlg_has_host_focus(void);

#endif
