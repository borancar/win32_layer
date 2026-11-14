/* dialog.cpp -- dialogs and the standard controls.
 *
 * A dialog is built from a template (a DLGTEMPLATE or DLGTEMPLATEEX, from the
 * image's resources or from memory) into a tree of small window objects: the
 * dialog, and one child per control. HWNDs of these are small integers
 * (0x10000 and up) that name a slot, so a stale handle finds nothing instead
 * of a freed object. The program's own window, user32.cpp's, is not one of
 * them; user32.cpp asks w32l_dw_valid first and hands a dialog's or control's
 * call over.
 *
 * Messages are real: a control is sent WM_SETTEXT, LB_ADDSTRING, BM_SETCHECK
 * and the rest, answers as Windows' control would, and tells its parent about
 * what the user did with WM_COMMAND and a notification code. Mouse and key
 * events from the host (dlghost.cpp) are posted to the program's queue
 * addressed to the control they belong to, and so reach it through the
 * program's own message loop, DispatchMessage. The dialog procedure's answers
 * follow DLGPROC's rules: TRUE means handled, and the value to return from
 * the message itself is DWL_MSGRESULT.
 *
 * Drawing is classic Windows 9x chrome in the host's 8x8 debug font, at 8
 * pixels per 4 dialog units across and 12 per 8 down (the dialog font's real
 * average character is 6x13, but the 8x8 font is wider). Dialogs are repainted
 * once a pump, when something changed; W32L_DIALOGDUMP writes the picture.
 */
#include "dialog.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* ---- constants private to the controls ---------------------------------------- */

#define BS_TYPEMASK 0x0F
#define BS_PUSH 0
#define BS_DEFPUSH 1
#define BS_CHECK 2
#define BS_AUTOCHECK 3
#define BS_RADIO 4
#define BS_3STATE 5
#define BS_AUTO3STATE 6
#define BS_GROUPBOX 7
#define BS_AUTORADIO 9
#define BS_OWNERDRAW 11
#define BM_GETSTATE 0x00F2
#define BM_SETSTATE 0x00F3
#define BM_SETSTYLE 0x00F4

#define ES_CENTER 0x0001
#define ES_RIGHT 0x0002
#define ES_MULTILINE 0x0004
#define ES_UPPERCASE 0x0008
#define ES_LOWERCASE 0x0010
#define ES_PASSWORD 0x0020
#define ES_AUTOVSCROLL 0x0040
#define ES_READONLY 0x0800
#define ES_NUMBER 0x2000
#define EM_GETSEL 0x00B0
#define EM_SETSEL 0x00B1
#define EM_SCROLL 0x00B5
#define EM_LINESCROLL 0x00B6
#define EM_SCROLLCARET 0x00B7
#define EM_GETMODIFY 0x00B8
#define EM_SETMODIFY 0x00B9
#define EM_GETLINECOUNT 0x00BA
#define EM_LINEINDEX 0x00BB
#define EM_LINELENGTH 0x00C1
#define EM_REPLACESEL 0x00C2
#define EM_GETLINE 0x00C4
#define EM_LIMITTEXT 0x00C5
#define EM_CANUNDO 0x00C6
#define EM_LINEFROMCHAR 0x00C9
#define EM_SETPASSWORDCHAR 0x00CC
#define EM_EMPTYUNDOBUFFER 0x00CD
#define EM_SETREADONLY 0x00CF
#define EM_GETFIRSTVISIBLELINE 0x00CE

#define SS_TYPEMASK 0x1F
#define SS_CENTER 1
#define SS_RIGHT 2
#define SS_ICON 3
#define SS_BLACKRECT 4
#define SS_GRAYRECT 5
#define SS_WHITERECT 6
#define SS_BLACKFRAME 7
#define SS_GRAYFRAME 8
#define SS_WHITEFRAME 9
#define SS_SIMPLE 11
#define SS_ETCHEDHORZ 16
#define SS_ETCHEDVERT 17
#define SS_ETCHEDFRAME 18
#define SS_NOTIFY 0x100
#define SS_NOPREFIX 0x80

#define LBS_NOTIFY 0x0001
#define LBS_SORT 0x0002
#define LBS_MULTIPLESEL 0x0008
#define LBS_EXTENDEDSEL 0x0800
#define LB_ERR (-1)
#define LB_ADDSTRING 0x0180
#define LB_INSERTSTRING 0x0181
#define LB_DELETESTRING 0x0182
#define LB_RESETCONTENT 0x0184
#define LB_SETSEL 0x0185
#define LB_SETCURSEL 0x0186
#define LB_GETSEL 0x0187
#define LB_GETCURSEL 0x0188
#define LB_GETTEXT 0x0189
#define LB_GETTEXTLEN 0x018A
#define LB_GETCOUNT 0x018B
#define LB_SELECTSTRING 0x018C
#define LB_DIR 0x018D
#define LB_GETTOPINDEX 0x018E
#define LB_FINDSTRING 0x018F
#define LB_GETSELCOUNT 0x0190
#define LB_GETSELITEMS 0x0191
#define LB_SETTABSTOPS 0x0192
#define LB_SETHORIZONTALEXTENT 0x0194
#define LB_SETCOLUMNWIDTH 0x0195
#define LB_SETTOPINDEX 0x0197
#define LB_GETITEMRECT 0x0198
#define LB_GETITEMDATA 0x0199
#define LB_SETITEMDATA 0x019A
#define LB_SELITEMRANGE 0x019B
#define LB_SETCARETINDEX 0x019E
#define LB_GETCARETINDEX 0x019F
#define LB_SETITEMHEIGHT 0x01A0
#define LB_GETITEMHEIGHT 0x01A1
#define LB_FINDSTRINGEXACT 0x01A2
#define LB_INITSTORAGE 0x01A8
#define LB_ITEMFROMPOINT 0x01A9

#define CBS_SIMPLE 1
#define CBS_DROPDOWN 2
#define CBS_DROPDOWNLIST 3
#define CBS_SORT 0x0100
#define CB_ERR (-1)
#define CB_GETEDITSEL 0x0140
#define CB_LIMITTEXT 0x0141
#define CB_SETEDITSEL 0x0142
#define CB_ADDSTRING 0x0143
#define CB_DELETESTRING 0x0144
#define CB_DIR 0x0145
#define CB_GETCOUNT 0x0146
#define CB_GETCURSEL 0x0147
#define CB_GETLBTEXT 0x0148
#define CB_GETLBTEXTLEN 0x0149
#define CB_INSERTSTRING 0x014A
#define CB_RESETCONTENT 0x014B
#define CB_FINDSTRING 0x014C
#define CB_SELECTSTRING 0x014D
#define CB_SETCURSEL 0x014E
#define CB_SHOWDROPDOWN 0x014F
#define CB_GETITEMDATA 0x0150
#define CB_SETITEMDATA 0x0151
#define CB_SETITEMHEIGHT 0x0153
#define CB_GETITEMHEIGHT 0x0154
#define CB_GETDROPPEDSTATE 0x0157
#define CB_FINDSTRINGEXACT 0x0158
#define CB_GETTOPINDEX 0x015B
#define CB_SETTOPINDEX 0x015C
#define CB_SETDROPPEDWIDTH 0x0160
#define CB_INITSTORAGE 0x0161

#define SBS_VERT 1
#define SBM_SETPOS 0x00E0
#define SBM_GETPOS 0x00E1
#define SBM_SETRANGE 0x00E2
#define SBM_GETRANGE 0x00E3
#define SBM_ENABLE_ARROWS 0x00E4
#define SBM_SETRANGEREDRAW 0x00E6
#define SBM_SETSCROLLINFO 0x00E9
#define SBM_GETSCROLLINFO 0x00EA
#define SIF_RANGE 1
#define SIF_PAGE 2
#define SIF_POS 4
#define SIF_TRACKPOS 16

/* ---- metrics and colours ---------------------------------------------------------- */

#define CW 8            /* a character cell */
#define CH 8
#define ROWH 12         /* a list row */
#define LINEH 10        /* an edit line */
#define COMBOH 20       /* a combo box's closed height */
#define SBW 16          /* a scroll bar's width */
#define DECOR_SIDE 3    /* the frame Windows puts round a dialog's client area */
#define DECOR_CAPTION 19

#define C_FACE 0xC0C0C0u
#define C_LIGHT 0xDFDFDFu
#define C_SHADOW 0x808080u
#define C_DARK 0x000000u
#define C_HILITE 0xFFFFFFu
#define C_WINDOW 0xFFFFFFu
#define C_TEXT 0x000000u
#define C_SELBG 0x000080u
#define C_SELTEXT 0xFFFFFFu
#define C_GRAYTEXT 0x808080u

#define DUX(v) ((int32_t)(v) * 9 / 4)
#define DUY(v) ((int32_t)(v) * 12 / 8)

enum { K_DIALOG, K_BUTTON, K_EDIT, K_STATIC, K_LIST, K_SCROLL, K_COMBO, K_CUSTOM };

struct Item { char *text; uint32_t data; uint8_t sel; };

struct DWin {
    HWND      hwnd;
    int32_t   kind;
    uint32_t  serial;
    char      cls[32];
    DWin     *parent, **kids;
    int32_t   nkids;
    int32_t   id;
    uint32_t  style, exstyle;
    char     *text;
    int32_t   tlen;
    int32_t   x, y, w, h;       /* a control: in its parent's client area; a dialog: client origin on screen */
    int32_t   visible, enabled, dead, dirty;
    /* a dialog */
    DLGPROC   proc;
    WNDPROC   cproc;            /* a control of a registered class, or a subclassed one */
    LONG      userdata, msgresult;
    uint8_t   extra[64];
    DlgHost  *host;
    DWin     *focus, *mousecap, *dropped;
    int32_t   modal, ended, decor, defid, hasfocus;
    INT_PTR   result;
    char      resname[64];
    int32_t   needdump, dumpseq;
    uint32_t  lastdump;
    /* a button */
    int32_t   check, pushed, btype;
    /* an edit, and the edit of a combo box */
    int32_t   caret, anchor, scx, scy, limit, modified, readonly;
    char      pwchar;
    /* a list, a combo box */
    Item     *items;
    int32_t   nitems, citems, cursel, top, caretitem, hover, sbdrag, sbgrab, dropw;
    int32_t   drop;
    /* a scroll bar */
    int32_t   smin, smax, spos, spage, sbpart;
};

#define W32L_DW_MAX 16384
#define W32L_DW_BASE 0x10000u

static DWin    *w32l_win[W32L_DW_MAX];
static uint32_t w32l_win_next, w32l_win_serial, w32l_win_dialogs;
static DWin   **w32l_zombies;
static int32_t  w32l_nzombies;
static int32_t  w32l_mods;      /* 1 shift, 2 control, from the host's key events */
static int32_t  w32l_send_depth; /* messages being delivered: a destroyed window is freed only when none is */

static DWin *dw_get(HWND h)
{
    uintptr_t v = (uintptr_t)h;

    if (v < W32L_DW_BASE || v >= W32L_DW_BASE + W32L_DW_MAX * 4u || (v & 3))
        return NULL;
    return w32l_win[(v - W32L_DW_BASE) / 4];
}

int32_t w32l_dw_valid(HWND h) { return dw_get(h) != NULL; }

static DWin *dw_new(int32_t kind, DWin *parent)
{
    uint32_t slot, tries;
    DWin    *d;

    for (tries = 0; tries < W32L_DW_MAX; tries++) {
        slot = w32l_win_next++ % W32L_DW_MAX;
        if (!w32l_win[slot])
            break;
    }
    if (tries == W32L_DW_MAX)
        return NULL;
    d = (DWin *)calloc(1, sizeof *d);
    d->hwnd = (HWND)(uintptr_t)(W32L_DW_BASE + slot * 4u);
    d->kind = kind;
    d->serial = ++w32l_win_serial;
    d->visible = 1;
    d->enabled = 1;
    d->dirty = 1;
    d->cursel = -1;
    d->caretitem = -1;
    d->hover = -1;
    d->parent = parent;
    d->text = strdup("");
    if (parent) {
        parent->kids = (DWin **)realloc(parent->kids, (size_t)(parent->nkids + 1) * sizeof *parent->kids);
        parent->kids[parent->nkids++] = d;
    }
    w32l_win[slot] = d;
    return d;
}

static void dw_free(DWin *d)
{
    int32_t i;

    for (i = 0; i < d->nitems; i++)
        free(d->items[i].text);
    free(d->items);
    free(d->kids);
    free(d->text);
    free(d);
}

static DWin *dw_top(DWin *d)
{
    while (d->parent)
        d = d->parent;
    return d;
}

static void dw_invalidate(DWin *d)
{
    DWin *t = dw_top(d);
    t->dirty = 1;
    t->needdump = 1;
}

void w32l_dw_invalidate(HWND h)
{
    DWin *d = dw_get(h);
    if (d)
        dw_invalidate(d);
}

static void dw_settext(DWin *d, const char *s)
{
    free(d->text);
    d->text = strdup(s ? s : "");
    d->tlen = (int32_t)strlen(d->text);
}

static LRESULT dw_send(DWin *d, UINT m, WPARAM w, LPARAM l);

static void dw_notify(DWin *c, uint32_t code)
{
    if (c->parent && !c->parent->dead)
        dw_send(c->parent, WM_COMMAND, (WPARAM)(((code & 0xFFFF) << 16) | ((uint32_t)c->id & 0xFFFF)), (LPARAM)(uintptr_t)c->hwnd);
}

static void dw_abs(DWin *d, int32_t *x, int32_t *y)
{
    *x = *y = 0;
    for (; d->parent; d = d->parent) {
        *x += d->x;
        *y += d->y;
    }
}

static DWin *dw_find_id(DWin *dlg, int32_t id)
{
    int32_t i;

    for (i = 0; i < dlg->nkids; i++)
        if (dlg->kids[i]->id == id)
            return dlg->kids[i];
    return NULL;
}

/* ---- an edit's text ------------------------------------------------------------------- */

static int32_t ed_multi(DWin *d) { return d->kind == K_EDIT && (d->style & ES_MULTILINE); }

static int32_t ed_cols(DWin *d)
{
    int32_t c = (d->w - 6 - ((d->style & WS_VSCROLL) ? SBW : 0)) / CW;
    return c > 1 ? c : 1;
}

/* Rows of the text: where each starts and how long it is (a wrapped or
 * newline-ended row; '\r' counts as part of its row but takes no cell). */
struct Rows { int32_t *start, *len; int32_t n; };

static void ed_rows(DWin *d, Rows *r)
{
    int32_t i, start = 0, col = 0, cols = ed_multi(d) ? ed_cols(d) : INT_MAX, cap = 16;

    r->n = 0;
    r->start = (int32_t *)malloc((size_t)cap * sizeof(int32_t));
    r->len = (int32_t *)malloc((size_t)cap * sizeof(int32_t));
#define PUSHROW(s, e) do { if (r->n == cap) { cap *= 2; \
        r->start = (int32_t *)realloc(r->start, (size_t)cap * sizeof(int32_t)); \
        r->len = (int32_t *)realloc(r->len, (size_t)cap * sizeof(int32_t)); } \
        r->start[r->n] = (s); r->len[r->n++] = (e) - (s); } while (0)
    for (i = 0; i < d->tlen; i++) {
        char c = d->text[i];
        if (c == '\n' && ed_multi(d)) {
            PUSHROW(start, i);
            start = i + 1;
            col = 0;
        } else if (c != '\r') {
            if (col >= cols) {
                PUSHROW(start, i);
                start = i;
                col = 0;
            }
            col++;
        }
    }
    PUSHROW(start, d->tlen);
#undef PUSHROW
}

static void ed_free_rows(Rows *r) { free(r->start); free(r->len); }

/* Cells of text before `pos` in its row. */
static int32_t ed_cell(DWin *d, int32_t from, int32_t pos)
{
    int32_t i, n = 0;

    for (i = from; i < pos && i < d->tlen; i++)
        if (d->text[i] != '\r')
            n++;
    return n;
}

static void ed_rc(DWin *d, int32_t pos, int32_t *row, int32_t *col)
{
    Rows    r;
    int32_t i;

    ed_rows(d, &r);
    for (i = r.n - 1; i > 0 && r.start[i] > pos; i--)
        ;
    /* a wrapped row end is also the next row's start: the caret belongs to the next */
    *row = i;
    *col = ed_cell(d, r.start[i], pos);
    ed_free_rows(&r);
}

static int32_t ed_pos(DWin *d, int32_t row, int32_t col)
{
    Rows    r;
    int32_t p, k = 0;

    ed_rows(d, &r);
    if (row < 0)
        row = 0;
    if (row >= r.n)
        row = r.n - 1;
    for (p = r.start[row]; p < r.start[row] + r.len[row] && k < col; p++)
        if (d->text[p] != '\r')
            k++;
    if (p == r.start[row] + r.len[row] && p > r.start[row] && d->text[p - 1] == '\r')
        p--;
    ed_free_rows(&r);
    return p;
}

static void ed_visible(DWin *d)
{
    int32_t row, col, vis;

    ed_rc(d, d->caret, &row, &col);
    if (!ed_multi(d)) {
        int32_t cols = (d->w - 6) / CW;
        if (col < d->scx)
            d->scx = col;
        if (col >= d->scx + cols - 1)
            d->scx = col - cols + 2;
        if (d->scx < 0)
            d->scx = 0;
        return;
    }
    vis = (d->h - 6) / LINEH;
    if (vis < 1)
        vis = 1;
    if (row < d->scy)
        d->scy = row;
    if (row >= d->scy + vis)
        d->scy = row - vis + 1;
}

static void ed_changed(DWin *d, int32_t notify)
{
    d->modified = 1;
    ed_visible(d);
    dw_invalidate(d);
    if (notify) {
        if (d->kind == K_COMBO) {
            dw_notify(d, CBN_EDITCHANGE);
        } else {
            dw_notify(d, EN_UPDATE);
            dw_notify(d, EN_CHANGE);
        }
    }
}

/* Replace [a, b) with n bytes of s; returns how many went in. */
static int32_t ed_replace(DWin *d, int32_t a, int32_t b, const char *s, int32_t n, int32_t notify)
{
    char   *t;
    int32_t room;

    if (a < 0) a = 0;
    if (b > d->tlen) b = d->tlen;
    if (a > b) a = b;
    room = (d->limit ? d->limit : 0x7FFFFFFE) - (d->tlen - (b - a));
    if (n > room) n = room > 0 ? room : 0;
    t = (char *)malloc((size_t)(d->tlen - (b - a) + n + 1));
    memcpy(t, d->text, (size_t)a);
    memcpy(t + a, s, (size_t)n);
    memcpy(t + a + n, d->text + b, (size_t)(d->tlen - b));
    t[d->tlen - (b - a) + n] = 0;
    free(d->text);
    d->text = t;
    d->tlen = d->tlen - (b - a) + n;
    d->caret = d->anchor = a + n;
    ed_changed(d, notify);
    return n;
}

static void ed_selrange(DWin *d, int32_t *a, int32_t *b)
{
    *a = d->caret < d->anchor ? d->caret : d->anchor;
    *b = d->caret < d->anchor ? d->anchor : d->caret;
}

static void ed_move(DWin *d, int32_t pos, int32_t extend)
{
    if (pos < 0) pos = 0;
    if (pos > d->tlen) pos = d->tlen;
    d->caret = pos;
    if (!extend)
        d->anchor = pos;
    ed_visible(d);
    dw_invalidate(d);
}

static void ed_typed(DWin *d, uint32_t ch)
{
    int32_t a, b;
    char    c = (char)ch;

    ed_selrange(d, &a, &b);
    if (ch == 8) {                              /* backspace */
        if (d->readonly)
            return;
        if (a == b && a > 0)
            a--;
        if (a != b)
            ed_replace(d, a, b, "", 0, 1);
    } else if (ch == 1) {                       /* ^A */
        d->anchor = 0;
        ed_move(d, d->tlen, 1);
    } else if (ch == 3 || ch == 24) {           /* ^C, ^X */
        if (a != b)
            w32l_dlghost_clip_set(d->text + a, b - a);
        if (ch == 24 && !d->readonly && a != b)
            ed_replace(d, a, b, "", 0, 1);
    } else if (ch == 22) {                      /* ^V */
        char *clip = d->readonly ? NULL : w32l_dlghost_clip_get();
        if (clip) {
            if (!ed_multi(d)) {
                char *nl = strpbrk(clip, "\r\n");
                if (nl)
                    *nl = 0;
            }
            ed_replace(d, a, b, clip, (int32_t)strlen(clip), 1);
            free(clip);
        }
    } else if (ch == 13 || ch == 10) {
        if (ed_multi(d) && !d->readonly)
            ed_replace(d, a, b, "\r\n", 2, 1);
    } else if (ch >= 32 && !d->readonly) {
        if ((d->style & ES_NUMBER) && !(ch >= '0' && ch <= '9'))
            return;
        if (d->style & ES_UPPERCASE)
            c = (char)toupper((int)ch);
        else if (d->style & ES_LOWERCASE)
            c = (char)tolower((int)ch);
        ed_replace(d, a, b, &c, 1, 1);
    }
}

static void ed_key(DWin *d, uint32_t vk)
{
    int32_t shift = w32l_mods & 1, row, col, a, b;

    switch (vk) {
    case 0x25: ed_move(d, d->caret - 1, shift); break;
    case 0x27: ed_move(d, d->caret + 1, shift); break;
    case 0x24: case 0x23: {
        Rows r;
        ed_rc(d, d->caret, &row, &col);
        ed_rows(d, &r);
        ed_move(d, vk == 0x24 ? r.start[row] : r.start[row] + r.len[row] - (r.len[row] && d->text[r.start[row] + r.len[row] - 1] == '\r'), shift);
        ed_free_rows(&r);
        break;
    }
    case 0x26: case 0x28: case 0x21: case 0x22:
        if (ed_multi(d)) {
            int32_t step = (vk == 0x21 || vk == 0x22) ? (d->h - 6) / LINEH : 1;
            ed_rc(d, d->caret, &row, &col);
            ed_move(d, ed_pos(d, row + ((vk == 0x26 || vk == 0x21) ? -step : step), col), shift);
        } else
            ed_move(d, (vk == 0x26 || vk == 0x21) ? 0 : d->tlen, shift);
        break;
    case 0x2E:
        if (d->readonly)
            break;
        ed_selrange(d, &a, &b);
        if (a == b && b < d->tlen)
            b++;
        if (a != b)
            ed_replace(d, a, b, "", 0, 1);
        break;
    }
}

/* ---- list items ---------------------------------------------------------------------------- */

static int32_t it_sorted(DWin *d) { return (d->kind == K_LIST && (d->style & LBS_SORT)) || (d->kind == K_COMBO && (d->style & CBS_SORT)); }

static int32_t it_insert(DWin *d, int32_t at, const char *s)
{
    int32_t i;

    if (it_sorted(d) && at < 0) {
        for (at = 0; at < d->nitems && strcasecmp(d->items[at].text, s) <= 0; at++)
            ;
    } else if (at < 0 || at > d->nitems)
        at = d->nitems;
    if (d->nitems == d->citems) {
        d->citems = d->citems ? d->citems * 2 : 16;
        d->items = (Item *)realloc(d->items, (size_t)d->citems * sizeof(Item));
    }
    for (i = d->nitems; i > at; i--)
        d->items[i] = d->items[i - 1];
    d->items[at].text = strdup(s ? s : "");
    d->items[at].data = 0;
    d->items[at].sel = 0;
    d->nitems++;
    if (d->cursel >= at)
        d->cursel++;
    dw_invalidate(d);
    return at;
}

static void it_delete(DWin *d, int32_t at)
{
    int32_t i;

    free(d->items[at].text);
    for (i = at; i + 1 < d->nitems; i++)
        d->items[i] = d->items[i + 1];
    d->nitems--;
    if (d->cursel == at)
        d->cursel = -1;
    else if (d->cursel > at)
        d->cursel--;
    if (d->top > 0 && d->top >= d->nitems)
        d->top = d->nitems > 0 ? d->nitems - 1 : 0;
    dw_invalidate(d);
}

static void it_clear(DWin *d)
{
    int32_t i;

    for (i = 0; i < d->nitems; i++)
        free(d->items[i].text);
    d->nitems = 0;
    d->cursel = -1;
    d->top = 0;
    dw_invalidate(d);
}

static int32_t it_find(DWin *d, int32_t from, const char *s, int32_t exact)
{
    int32_t i, n = d->nitems, len = (int32_t)strlen(s);

    for (i = 0; i < n; i++) {
        int32_t k = (from + 1 + i) % n;
        if (exact ? !strcasecmp(d->items[k].text, s) : !strncasecmp(d->items[k].text, s, (size_t)len))
            return k;
    }
    return -1;
}

static int32_t it_copy(DWin *d, int32_t at, char *out)
{
    if (at < 0 || at >= d->nitems)
        return -1;
    if (out)
        strcpy(out, d->items[at].text);
    return (int32_t)strlen(d->items[at].text);
}

/* The list's visible rows: a combo's drop-down shows its own. */
static int32_t lst_rows(DWin *d)
{
    int32_t h = d->kind == K_COMBO ? (d->drop ? d->dropw : 0) : d->h - 4;
    return h / ROWH > 0 ? h / ROWH : 1;
}

static int32_t cb_drop_height(DWin *d)
{
    int32_t want = d->h - COMBOH, rows;

    if (want < 4 * ROWH + 4)
        want = 4 * ROWH + 4;
    if (want > 8 * ROWH + 4)
        want = 8 * ROWH + 4;
    rows = (want - 4) / ROWH;
    if (d->nitems < rows)
        rows = d->nitems > 0 ? d->nitems : 1;
    return rows * ROWH + 4;
}

static void lst_scroll_to(DWin *d, int32_t at)
{
    int32_t rows = d->kind == K_COMBO ? (cb_drop_height(d) - 4) / ROWH : lst_rows(d);

    if (at < 0)
        return;
    if (at < d->top)
        d->top = at;
    if (at >= d->top + rows)
        d->top = at - rows + 1;
    if (d->top < 0)
        d->top = 0;
    dw_invalidate(d);
}

static void lst_clamp_top(DWin *d)
{
    int32_t rows = d->kind == K_COMBO ? (cb_drop_height(d) - 4) / ROWH : lst_rows(d), mx = d->nitems - rows;

    if (d->top > mx)
        d->top = mx;
    if (d->top < 0)
        d->top = 0;
}

static int32_t lst_multi(DWin *d) { return d->kind == K_LIST && (d->style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)); }

/* ---- focus ------------------------------------------------------------------------------------ */

static void cb_close(DWin *d, int32_t notify);

static void dw_set_focus(DWin *c)
{
    DWin *dlg = dw_top(c), *old = dlg->focus;

    if (old == c || c->dead)
        return;
    dlg->focus = c;
    if (old && !old->dead) {
        dw_send(old, WM_KILLFOCUS, (WPARAM)(uintptr_t)c->hwnd, 0);
        if (old->kind == K_EDIT) dw_notify(old, EN_KILLFOCUS);
        else if (old->kind == K_COMBO) { cb_close(old, 0); dw_notify(old, 4); }
        else if (old->kind == K_LIST) dw_notify(old, 5);
        dw_invalidate(old);
    }
    if (dlg->focus != c)
        return;
    dw_send(c, WM_SETFOCUS, old ? (WPARAM)(uintptr_t)old->hwnd : 0, 0);
    if (c->kind == K_EDIT) dw_notify(c, EN_SETFOCUS);
    else if (c->kind == K_COMBO) dw_notify(c, 3);
    else if (c->kind == K_LIST) dw_notify(c, 4);
    dw_invalidate(c);
}

static int32_t dw_tabstop(DWin *c)
{
    return (c->style & WS_TABSTOP) && c->visible && c->enabled && c->kind != K_STATIC;
}

static DWin *dw_next_tab(DWin *dlg, DWin *from, int32_t back)
{
    int32_t i, n = dlg->nkids, at = -1;

    for (i = 0; i < n; i++)
        if (dlg->kids[i] == from)
            at = i;
    for (i = 1; i <= n; i++) {
        DWin *c = dlg->kids[((at < 0 ? (back ? n : -1) : at) + (back ? -i : i) + n * 2) % n];
        if (dw_tabstop(c))
            return c;
    }
    return NULL;
}

/* ---- buttons --------------------------------------------------------------------------------------- */

static void btn_group_clear(DWin *b)
{
    DWin   *dlg = b->parent;
    int32_t i, at = 0, lo, hi;

    if (!dlg)
        return;
    for (i = 0; i < dlg->nkids; i++)
        if (dlg->kids[i] == b)
            at = i;
    for (lo = at; lo > 0 && !(dlg->kids[lo]->style & WS_GROUP); lo--)
        ;
    for (hi = at + 1; hi < dlg->nkids && !(dlg->kids[hi]->style & WS_GROUP); hi++)
        ;
    for (i = lo; i < hi; i++) {
        DWin *c = dlg->kids[i];
        if (c != b && c->kind == K_BUTTON && (c->btype == BS_RADIO || c->btype == BS_AUTORADIO))
            c->check = 0;
    }
}

static void btn_click(DWin *b)
{
    switch (b->btype) {
    case BS_AUTOCHECK: b->check = !b->check; break;
    case BS_AUTO3STATE: b->check = (b->check + 1) % 3; break;
    case BS_AUTORADIO: btn_group_clear(b); b->check = 1; break;
    case BS_GROUPBOX: return;
    }
    dw_invalidate(b);
    dw_notify(b, BN_CLICKED);
}

static int32_t in_rect(DWin *d, int32_t x, int32_t y) { return x >= 0 && y >= 0 && x < d->w && y < d->h; }

static LRESULT btn_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case BM_GETCHECK: return d->check;
    case BM_SETCHECK: d->check = (int32_t)w; dw_invalidate(d); return 0;
    case BM_GETSTATE: return d->check | (d->pushed ? 4 : 0) | (dw_top(d)->focus == d ? 8 : 0);
    case BM_SETSTATE: d->pushed = w != 0; dw_invalidate(d); return 0;
    case BM_SETSTYLE: d->btype = (int32_t)(w & BS_TYPEMASK); d->style = (d->style & ~(uint32_t)BS_TYPEMASK) | (w & BS_TYPEMASK); dw_invalidate(d); return 0;
    case BM_CLICK: if (d->enabled) btn_click(d); return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (d->enabled && d->btype != BS_GROUPBOX) { d->pushed = 1; dw_invalidate(d); }
        return 0;
    case WM_MOUSEMOVE:
        if (d->pushed || (w & MK_LBUTTON)) {
            int32_t in = in_rect(d, (int16_t)LOWORD(l), (int16_t)HIWORD(l));
            if ((w & MK_LBUTTON) && d->pushed != in) { d->pushed = in; dw_invalidate(d); }
        }
        return 0;
    case WM_LBUTTONUP:
        if (d->pushed) {
            d->pushed = 0;
            dw_invalidate(d);
            if (in_rect(d, (int16_t)LOWORD(l), (int16_t)HIWORD(l)))
                btn_click(d);
        }
        return 0;
    case WM_KEYDOWN:
        if (w == 0x20 && d->enabled && d->btype != BS_GROUPBOX)
            btn_click(d);
        return 0;
    }
    return 0;
}

/* ---- static text -------------------------------------------------------------------------------------- */

static LRESULT st_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    (void)w;
    if (m == WM_LBUTTONDOWN && (d->style & SS_NOTIFY)) {
        dw_notify(d, 0);                 /* STN_CLICKED */
    } else if (m == WM_LBUTTONDBLCLK && (d->style & SS_NOTIFY)) {
        dw_notify(d, 1);
    }
    (void)l;
    return 0;
}

/* ---- edit ------------------------------------------------------------------------------------------------ */

static LRESULT ed_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    int32_t a, b;

    switch (m) {
    case WM_SETTEXT:
        dw_settext(d, (const char *)(uintptr_t)l);
        d->caret = d->anchor = 0;
        d->scx = d->scy = 0;
        ed_changed(d, 1);
        d->modified = 0;
        return TRUE;
    case WM_GETTEXTLENGTH: return d->tlen;
    case WM_GETTEXT: {
        int32_t n = (int32_t)w - 1 < d->tlen ? (int32_t)w - 1 : d->tlen;
        char   *out = (char *)(uintptr_t)l;
        if (!out || (int32_t)w <= 0)
            return 0;
        memcpy(out, d->text, (size_t)n);
        out[n] = 0;
        return n;
    }
    case EM_GETSEL:
        ed_selrange(d, &a, &b);
        if (w) *(uint32_t *)(uintptr_t)w = (uint32_t)a;
        if (l) *(uint32_t *)(uintptr_t)l = (uint32_t)b;
        return MAKELONG(a, b);
    case EM_SETSEL:
        a = (int32_t)w;
        b = l;
        if (a == -1) { d->anchor = d->caret; return 0; }
        if (b < 0 || b > d->tlen) b = d->tlen;
        if (a > d->tlen) a = d->tlen;
        d->anchor = a;
        d->caret = b;
        ed_visible(d);
        dw_invalidate(d);
        return 0;
    case EM_REPLACESEL: {
        const char *s = (const char *)(uintptr_t)l;
        ed_selrange(d, &a, &b);
        ed_replace(d, a, b, s, (int32_t)strlen(s), 1);
        return 0;
    }
    case EM_LIMITTEXT: d->limit = (int32_t)w; return 0;
    case EM_SETREADONLY: d->readonly = w != 0; dw_invalidate(d); return TRUE;
    case EM_GETMODIFY: return d->modified;
    case EM_SETMODIFY: d->modified = w != 0; return 0;
    case EM_CANUNDO: return 0;
    case EM_EMPTYUNDOBUFFER: return 0;
    case EM_SCROLLCARET: ed_visible(d); return 0;
    case EM_SETPASSWORDCHAR: d->pwchar = (char)w; dw_invalidate(d); return 0;
    case EM_GETLINECOUNT: {
        Rows r;
        ed_rows(d, &r);
        a = r.n;
        ed_free_rows(&r);
        return a;
    }
    case EM_GETFIRSTVISIBLELINE: return d->scy;
    case EM_LINESCROLL: d->scy += l; if (d->scy < 0) d->scy = 0; dw_invalidate(d); return TRUE;
    case EM_SCROLL: d->scy += (w == SB_LINEDOWN ? 1 : w == SB_LINEUP ? -1 : 0); if (d->scy < 0) d->scy = 0; dw_invalidate(d); return 0;
    case EM_LINEFROMCHAR: {
        int32_t row, col;
        ed_rc(d, (int32_t)w == -1 ? d->caret : (int32_t)w, &row, &col);
        return row;
    }
    case EM_LINEINDEX: {
        Rows r;
        int32_t row = (int32_t)w;
        if (row == -1) { int32_t c; ed_rc(d, d->caret, &row, &c); }
        ed_rows(d, &r);
        a = row >= 0 && row < r.n ? r.start[row] : -1;
        ed_free_rows(&r);
        return a;
    }
    case EM_LINELENGTH: {
        Rows r;
        int32_t row, c;
        ed_rc(d, (int32_t)w == -1 ? d->caret : (int32_t)w, &row, &c);
        ed_rows(d, &r);
        a = r.len[row] - (r.len[row] && d->text[r.start[row] + r.len[row] - 1] == '\r');
        ed_free_rows(&r);
        return a;
    }
    case EM_GETLINE: {
        Rows r;
        int32_t row = (int32_t)w, n = 0;
        char   *out = (char *)(uintptr_t)l;
        ed_rows(d, &r);
        if (row >= 0 && row < r.n && out) {
            int32_t cap = *(uint16_t *)out, len = r.len[row] - (r.len[row] && d->text[r.start[row] + r.len[row] - 1] == '\r');
            n = len < cap ? len : cap;
            memcpy(out, d->text + r.start[row], (size_t)n);
            if (n < cap)
                out[n] = 0;
        }
        ed_free_rows(&r);
        return n;
    }
    case WM_CHAR:
        if (d->enabled)
            ed_typed(d, (uint32_t)w);
        return 0;
    case WM_KEYDOWN:
        if (d->enabled)
            ed_key(d, (uint32_t)w);
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_MOUSEMOVE: {
        int32_t x = (int16_t)LOWORD(l), y = (int16_t)HIWORD(l), row, col, pos;
        if (m == WM_MOUSEMOVE && !(w & MK_LBUTTON))
            return 0;
        row = ed_multi(d) ? d->scy + (y - 3) / LINEH : 0;
        col = (x - 3 + CW / 2) / CW + (ed_multi(d) ? 0 : d->scx);
        if (col < 0) col = 0;
        pos = ed_pos(d, row, col);
        if (m == WM_LBUTTONDBLCLK) {              /* a word */
            int32_t s = pos, e = pos;
            while (s > 0 && isalnum((uint8_t)d->text[s - 1])) s--;
            while (e < d->tlen && isalnum((uint8_t)d->text[e])) e++;
            d->anchor = s;
            d->caret = e;
            dw_invalidate(d);
        } else
            ed_move(d, pos, m == WM_MOUSEMOVE || (w32l_mods & 1));
        return 0;
    }
    case WM_MOUSEWHEEL:
        if (ed_multi(d)) {
            d->scy -= (int16_t)HIWORD(w) / 120 * 3;
            if (d->scy < 0) d->scy = 0;
            dw_invalidate(d);
        }
        return 0;
    }
    return 0;
}

/* ---- list box ------------------------------------------------------------------------------------------------- */

static void lst_select(DWin *d, int32_t at, int32_t extend)
{
    int32_t i;

    if (at < 0 || at >= d->nitems)
        return;
    if (lst_multi(d)) {
        if (d->style & LBS_MULTIPLESEL)
            d->items[at].sel = !d->items[at].sel;
        else if (extend)
            d->items[at].sel = !d->items[at].sel;
        else {
            for (i = 0; i < d->nitems; i++)
                d->items[i].sel = 0;
            d->items[at].sel = 1;
        }
        d->caretitem = at;
    }
    d->cursel = at;
    lst_scroll_to(d, at);
    dw_invalidate(d);
}

/* The list's own scroll bar: where the thumb is, in pixels from the top of the track. */
static void lst_thumb(DWin *d, int32_t trackh, int32_t rows, int32_t *pos, int32_t *len)
{
    int32_t mx = d->nitems - rows;

    *len = trackh * rows / (d->nitems ? d->nitems : 1);
    if (*len < 8) *len = 8;
    *pos = mx > 0 ? (trackh - *len) * d->top / mx : 0;
}

static LRESULT lb_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    int32_t i, rows = lst_rows(d);

    switch (m) {
    case LB_ADDSTRING: return it_insert(d, -1, (const char *)(uintptr_t)l);
    case LB_INSERTSTRING: return it_insert(d, (int32_t)w, (const char *)(uintptr_t)l);
    case LB_DELETESTRING: if ((int32_t)w < 0 || (int32_t)w >= d->nitems) return LB_ERR; it_delete(d, (int32_t)w); return d->nitems;
    case LB_RESETCONTENT: it_clear(d); return 0;
    case LB_GETCOUNT: return d->nitems;
    case LB_GETCURSEL: return lst_multi(d) ? d->caretitem : d->cursel;
    case LB_SETCURSEL:
        if (lst_multi(d)) return LB_ERR;
        d->cursel = (int32_t)w >= 0 && (int32_t)w < d->nitems ? (int32_t)w : -1;
        if (d->cursel >= 0) lst_scroll_to(d, d->cursel);
        dw_invalidate(d);
        return d->cursel < 0 ? LB_ERR : d->cursel;
    case LB_GETTEXT: return it_copy(d, (int32_t)w, (char *)(uintptr_t)l) < 0 ? LB_ERR : (int32_t)strlen(d->items[(int32_t)w].text);
    case LB_GETTEXTLEN: return it_copy(d, (int32_t)w, NULL) < 0 ? LB_ERR : (int32_t)strlen(d->items[(int32_t)w].text);
    case LB_GETITEMDATA: return (int32_t)w >= 0 && (int32_t)w < d->nitems ? (LRESULT)d->items[w].data : LB_ERR;
    case LB_SETITEMDATA: if ((int32_t)w < 0 || (int32_t)w >= d->nitems) return LB_ERR; d->items[w].data = (uint32_t)l; return 0;
    case LB_FINDSTRING: case LB_FINDSTRINGEXACT: return it_find(d, (int32_t)w, (const char *)(uintptr_t)l, m == LB_FINDSTRINGEXACT);
    case LB_SELECTSTRING:
        i = it_find(d, (int32_t)w, (const char *)(uintptr_t)l, 0);
        if (i >= 0) { d->cursel = i; lst_scroll_to(d, i); }
        return i;
    case LB_GETTOPINDEX: return d->top;
    case LB_SETTOPINDEX: d->top = (int32_t)w; lst_clamp_top(d); dw_invalidate(d); return 0;
    case LB_SETSEL:
        if (!lst_multi(d)) return LB_ERR;
        if ((int32_t)l == -1)
            for (i = 0; i < d->nitems; i++) d->items[i].sel = w != 0;
        else if (l >= 0 && l < d->nitems) d->items[l].sel = w != 0;
        dw_invalidate(d);
        return 0;
    case LB_GETSEL:
        if ((int32_t)w < 0 || (int32_t)w >= d->nitems) return LB_ERR;
        return lst_multi(d) ? d->items[w].sel : (d->cursel == (int32_t)w);
    case LB_SELITEMRANGE:
        for (i = LOWORD(l); i <= HIWORD(l) && i < d->nitems; i++) d->items[i].sel = w != 0;
        dw_invalidate(d);
        return 0;
    case LB_GETSELCOUNT: {
        int32_t n = 0;
        if (!lst_multi(d)) return LB_ERR;
        for (i = 0; i < d->nitems; i++) n += d->items[i].sel;
        return n;
    }
    case LB_GETSELITEMS: {
        int32_t n = 0;
        int32_t *out = (int32_t *)(uintptr_t)l;
        for (i = 0; i < d->nitems && n < (int32_t)w; i++)
            if (d->items[i].sel) out[n++] = i;
        return n;
    }
    case LB_GETCARETINDEX: return d->caretitem < 0 ? d->cursel : d->caretitem;
    case LB_SETCARETINDEX: d->caretitem = (int32_t)w; return 0;
    case LB_GETITEMHEIGHT: return ROWH;
    case LB_GETITEMRECT: {
        RECT *rc = (RECT *)(uintptr_t)l;
        if ((int32_t)w < d->top || (int32_t)w >= d->top + rows || (int32_t)w >= d->nitems || !rc) return LB_ERR;
        rc->left = 2; rc->right = d->w - 2;
        rc->top = 2 + ((int32_t)w - d->top) * ROWH; rc->bottom = rc->top + ROWH;
        return 0;
    }
    case LB_ITEMFROMPOINT: {
        int32_t y = (int16_t)HIWORD(l) - 2, at = d->top + y / ROWH;
        return at >= 0 && at < d->nitems && y >= 0 ? at : MAKELONG(d->nitems > 0 ? d->nitems - 1 : 0, 1);
    }
    case LB_SETTABSTOPS: case LB_SETHORIZONTALEXTENT: case LB_SETCOLUMNWIDTH: case LB_INITSTORAGE: case LB_SETITEMHEIGHT: return 0;
    case LB_DIR: return LB_ERR;
    case WM_MOUSEWHEEL:
        d->top -= (int16_t)HIWORD(w) / 120 * 3;
        lst_clamp_top(d);
        dw_invalidate(d);
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_MOUSEMOVE: {
        int32_t x = (int16_t)LOWORD(l), y = (int16_t)HIWORD(l), sbx = d->w - 2 - SBW;
        int32_t hasbar = d->nitems > rows;
        if (m == WM_MOUSEMOVE && !(w & MK_LBUTTON))
            return 0;
        if (m != WM_MOUSEMOVE && !d->enabled)
            return 0;
        if (hasbar && (x >= sbx || d->sbdrag)) {            /* the scroll bar */
            int32_t th = d->h - 4 - 2 * SBW, tp, tl, ty = y - 2 - SBW;
            lst_thumb(d, th, rows, &tp, &tl);
            if (m == WM_MOUSEMOVE) {
                if (d->sbdrag && th > tl) {
                    d->top = (ty - d->sbgrab) * (d->nitems - rows) / (th - tl);
                    lst_clamp_top(d);
                    dw_invalidate(d);
                }
                return 0;
            }
            if (y < 2 + SBW)               d->top--;
            else if (y >= d->h - 2 - SBW)  d->top++;
            else if (ty < tp)              d->top -= rows;
            else if (ty >= tp + tl)        d->top += rows;
            else { d->sbdrag = 1; d->sbgrab = ty - tp; }
            lst_clamp_top(d);
            dw_invalidate(d);
            return 0;
        }
        if (m == WM_MOUSEMOVE && !d->sbdrag) {
            int32_t at = d->top + (y - 2) / ROWH;
            if (at >= 0 && at < d->nitems && at != d->cursel && !lst_multi(d)) {
                d->cursel = at;                              /* dragging selects */
                dw_invalidate(d);
                if (d->style & LBS_NOTIFY) dw_notify(d, LBN_SELCHANGE);
            }
            return 0;
        }
        if (m == WM_LBUTTONDBLCLK) {
            if (d->style & LBS_NOTIFY) dw_notify(d, LBN_DBLCLK);
            return 0;
        }
        i = d->top + (y - 2) / ROWH;
        if (i >= 0 && i < d->nitems && y >= 2) {
            lst_select(d, i, w32l_mods & 2);
            if (d->style & LBS_NOTIFY) dw_notify(d, LBN_SELCHANGE);
        }
        return 0;
    }
    case WM_LBUTTONUP: d->sbdrag = 0; return 0;
    case WM_KEYDOWN: {
        int32_t at = d->cursel < 0 ? 0 : d->cursel, to = at;
        if (!d->nitems) return 0;
        switch (w) {
        case 0x26: to = d->cursel < 0 ? 0 : at - 1; break;
        case 0x28: to = d->cursel < 0 ? 0 : at + 1; break;
        case 0x21: to = at - rows; break;
        case 0x22: to = at + rows; break;
        case 0x24: to = 0; break;
        case 0x23: to = d->nitems - 1; break;
        default: return 0;
        }
        if (to < 0) to = 0;
        if (to >= d->nitems) to = d->nitems - 1;
        if (!lst_multi(d) || to != at) {
            if (lst_multi(d)) { for (i = 0; i < d->nitems; i++) d->items[i].sel = 0; d->items[to].sel = 1; d->caretitem = to; d->cursel = to; lst_scroll_to(d, to); }
            else lst_select(d, to, 0);
            if (d->style & LBS_NOTIFY) dw_notify(d, LBN_SELCHANGE);
        }
        return 0;
    }
    case WM_CHAR:
        if (w >= 32 && d->nitems) {
            char s[2] = { (char)w, 0 };
            i = it_find(d, d->cursel, s, 0);
            if (i >= 0) {
                lst_select(d, i, 0);
                if (d->style & LBS_NOTIFY) dw_notify(d, LBN_SELCHANGE);
            }
        }
        return 0;
    }
    return 0;
}

/* ---- combo box -------------------------------------------------------------------------------------------------------- */

static int32_t cb_has_edit(DWin *d) { return (d->style & 3) != CBS_DROPDOWNLIST; }

static void cb_close(DWin *d, int32_t notify)
{
    if (!d->drop)
        return;
    d->drop = 0;
    if (dw_top(d)->dropped == d)
        dw_top(d)->dropped = NULL;
    dw_invalidate(d);
    if (notify)
        dw_notify(d, CBN_CLOSEUP);
}

static void cb_open(DWin *d)
{
    DWin *t = dw_top(d);

    if (d->drop)
        return;
    if (t->dropped)
        cb_close(t->dropped, 1);
    d->drop = 1;
    d->dropw = cb_drop_height(d);
    t->dropped = d;
    lst_scroll_to(d, d->cursel);
    dw_invalidate(d);
    dw_notify(d, CBN_DROPDOWN);
}

static void cb_pick(DWin *d, int32_t at, int32_t notify)
{
    if (at < 0 || at >= d->nitems)
        return;
    d->cursel = at;
    if (cb_has_edit(d)) {
        dw_settext(d, d->items[at].text);
        d->caret = d->anchor = d->tlen;
    }
    dw_invalidate(d);
    if (notify) {
        dw_notify(d, CBN_SELCHANGE);
        dw_notify(d, 9);                   /* CBN_SELENDOK */
    }
}

static LRESULT cb_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    int32_t i;

    switch (m) {
    case CB_ADDSTRING: return it_insert(d, -1, (const char *)(uintptr_t)l);
    case CB_INSERTSTRING: return it_insert(d, (int32_t)w, (const char *)(uintptr_t)l);
    case CB_DELETESTRING: if ((int32_t)w < 0 || (int32_t)w >= d->nitems) return CB_ERR; it_delete(d, (int32_t)w); return d->nitems;
    case CB_RESETCONTENT: it_clear(d); if (cb_has_edit(d)) { dw_settext(d, ""); d->caret = d->anchor = 0; } return 0;
    case CB_GETCOUNT: return d->nitems;
    case CB_GETCURSEL: return d->cursel;
    case CB_SETCURSEL:
        if ((int32_t)w < 0 || (int32_t)w >= d->nitems) {
            d->cursel = -1;
            if (cb_has_edit(d)) { dw_settext(d, ""); d->caret = d->anchor = 0; }
            dw_invalidate(d);
            return CB_ERR;
        }
        cb_pick(d, (int32_t)w, 0);
        return (int32_t)w;
    case CB_GETLBTEXT: return it_copy(d, (int32_t)w, (char *)(uintptr_t)l) < 0 ? CB_ERR : (int32_t)strlen(d->items[(int32_t)w].text);
    case CB_GETLBTEXTLEN: return it_copy(d, (int32_t)w, NULL) < 0 ? CB_ERR : (int32_t)strlen(d->items[(int32_t)w].text);
    case CB_GETITEMDATA: return (int32_t)w >= 0 && (int32_t)w < d->nitems ? (LRESULT)d->items[w].data : CB_ERR;
    case CB_SETITEMDATA: if ((int32_t)w < 0 || (int32_t)w >= d->nitems) return CB_ERR; d->items[w].data = (uint32_t)l; return 0;
    case CB_FINDSTRING: case CB_FINDSTRINGEXACT: return it_find(d, (int32_t)w, (const char *)(uintptr_t)l, m == CB_FINDSTRINGEXACT);
    case CB_SELECTSTRING:
        i = it_find(d, (int32_t)w, (const char *)(uintptr_t)l, 0);
        if (i >= 0) cb_pick(d, i, 0);
        return i;
    case CB_SHOWDROPDOWN: if (w) cb_open(d); else cb_close(d, 1); return TRUE;
    case CB_GETDROPPEDSTATE: return d->drop;
    case CB_GETTOPINDEX: return d->top;
    case CB_SETTOPINDEX: d->top = (int32_t)w; lst_clamp_top(d); return 0;
    case CB_LIMITTEXT: d->limit = (int32_t)w; return TRUE;
    case CB_GETEDITSEL: return ed_proc(d, EM_GETSEL, w, l);
    case CB_SETEDITSEL: ed_proc(d, EM_SETSEL, LOWORD(l), (int16_t)HIWORD(l)); return TRUE;
    case CB_GETITEMHEIGHT: case CB_SETITEMHEIGHT: return ROWH;
    case CB_SETDROPPEDWIDTH: case CB_INITSTORAGE: case CB_DIR: return 0;
    case WM_SETTEXT: case WM_GETTEXT: case WM_GETTEXTLENGTH:
        if (cb_has_edit(d))
            return ed_proc(d, m, w, l);
        if (m == WM_GETTEXTLENGTH) return d->cursel >= 0 ? (int32_t)strlen(d->items[d->cursel].text) : 0;
        if (m == WM_GETTEXT) {
            const char *s = d->cursel >= 0 ? d->items[d->cursel].text : "";
            int32_t n = (int32_t)strlen(s) < (int32_t)w - 1 ? (int32_t)strlen(s) : (int32_t)w - 1;
            if (n < 0) return 0;
            memcpy((char *)(uintptr_t)l, s, (size_t)n);
            ((char *)(uintptr_t)l)[n] = 0;
            return n;
        }
        i = it_find(d, -1, (const char *)(uintptr_t)l, 1);
        if (i >= 0) cb_pick(d, i, 0);
        return i >= 0;
    case WM_CHAR:
        if (cb_has_edit(d)) {
            ed_typed(d, (uint32_t)w);
            return 0;
        }
        if (w >= 32 && d->nitems) {
            char s[2] = { (char)w, 0 };
            i = it_find(d, d->cursel, s, 0);
            if (i >= 0) cb_pick(d, i, 1);
        }
        return 0;
    case WM_KEYDOWN:
        if (w == 0x73) { if (d->drop) cb_close(d, 1); else cb_open(d); return 0; }          /* F4 */
        if ((w == 0x26 || w == 0x28) && d->nitems) {
            int32_t to = d->cursel < 0 ? 0 : d->cursel + (w == 0x28 ? 1 : -1);
            if (to >= 0 && to < d->nitems) { cb_pick(d, to, 1); lst_scroll_to(d, to); }
            return 0;
        }
        if (w == 0x0D && d->drop) { cb_close(d, 1); return 0; }
        if (w == 0x1B && d->drop) { cb_close(d, 1); dw_notify(d, 10); return 0; }
        if (cb_has_edit(d)) ed_key(d, (uint32_t)w);
        return 0;
    case WM_MOUSEWHEEL:
        if (d->drop) {
            d->top -= (int16_t)HIWORD(w) / 120 * 3;
            lst_clamp_top(d);
            dw_invalidate(d);
        }
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_MOUSEMOVE: case WM_LBUTTONUP: {
        int32_t x = (int16_t)LOWORD(l), y = (int16_t)HIWORD(l), dh = d->drop ? d->dropw : 0;
        int32_t rows = (dh - 4) / ROWH, hasbar = d->nitems > rows;
        if (m == WM_LBUTTONUP) { d->sbdrag = 0; return 0; }
        if (m == WM_MOUSEMOVE) {
            if (d->drop && y >= COMBOH) {
                if (d->sbdrag && hasbar) {
                    int32_t th = dh - 4 - 2 * SBW, tp, tl;
                    lst_thumb(d, th, rows, &tp, &tl);
                    if (th > tl) { d->top = (y - COMBOH - 2 - SBW - d->sbgrab) * (d->nitems - rows) / (th - tl); lst_clamp_top(d); dw_invalidate(d); }
                } else {
                    int32_t at = d->top + (y - COMBOH - 2) / ROWH;
                    if (x < d->w - (hasbar ? SBW : 0) && at != d->hover && at < d->nitems) { d->hover = at; dw_invalidate(d); }
                }
            }
            return 0;
        }
        if (!d->enabled)
            return 0;
        if (y < COMBOH) {
            if (!cb_has_edit(d) || x >= d->w - SBW) {
                if (d->drop) cb_close(d, 1); else cb_open(d);
            } else if (d->drop) {
                cb_close(d, 1);
            } else {
                ed_proc(d, WM_LBUTTONDOWN, w, l);
            }
            return 0;
        }
        if (!d->drop)
            return 0;
        if (hasbar && x >= d->w - SBW) {
            int32_t th = dh - 4 - 2 * SBW, tp, tl, ty = y - COMBOH - 2 - SBW;
            lst_thumb(d, th, rows, &tp, &tl);
            if (y < COMBOH + 2 + SBW)                d->top--;
            else if (y >= COMBOH + dh - 2 - SBW)     d->top++;
            else if (ty < tp)                        d->top -= rows;
            else if (ty >= tp + tl)                  d->top += rows;
            else { d->sbdrag = 1; d->sbgrab = ty - tp; }
            lst_clamp_top(d);
            dw_invalidate(d);
            return 0;
        }
        i = d->top + (y - COMBOH - 2) / ROWH;
        if (i >= 0 && i < d->nitems && y - COMBOH >= 2 && y - COMBOH < dh) {
            cb_pick(d, i, 1);
            cb_close(d, 1);
        } else {
            cb_close(d, 1);
            dw_notify(d, 10);                  /* CBN_SELENDCANCEL */
        }
        return 0;
    }
    }
    return 0;
}

/* ---- scroll bar ------------------------------------------------------------------------------------------------------------- */

static int32_t sb_vert(DWin *d) { return (d->style & SBS_VERT) != 0; }

static int32_t sb_maxpos(DWin *d)
{
    int32_t mx = d->smax - (d->spage > 0 ? d->spage - 1 : 0);
    return mx < d->smin ? d->smin : mx;
}

static void sb_set(DWin *d, int32_t pos)
{
    if (pos < d->smin) pos = d->smin;
    if (pos > sb_maxpos(d)) pos = sb_maxpos(d);
    d->spos = pos;
    dw_invalidate(d);
}

static void sb_report(DWin *d, uint32_t code, int32_t pos)
{
    if (d->parent)
        dw_send(d->parent, sb_vert(d) ? WM_VSCROLL : WM_HSCROLL,
                (WPARAM)((code & 0xFFFF) | ((uint32_t)(pos & 0xFFFF) << 16)), (LPARAM)(uintptr_t)d->hwnd);
}

static LRESULT sb_proc(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case SBM_SETPOS: { int32_t old = d->spos; sb_set(d, (int32_t)w); return old; }
    case SBM_GETPOS: return d->spos;
    case SBM_SETRANGE: case SBM_SETRANGEREDRAW: d->smin = (int32_t)w; d->smax = l; sb_set(d, d->spos); return 0;
    case SBM_GETRANGE: if (w) *(int32_t *)(uintptr_t)w = d->smin; if (l) *(int32_t *)(uintptr_t)l = d->smax; return 0;
    case SBM_ENABLE_ARROWS: return TRUE;
    case SBM_SETSCROLLINFO: case SBM_GETSCROLLINFO: {
        uint32_t *si = (uint32_t *)(uintptr_t)l;      /* cbSize, fMask, nMin, nMax, nPage, nPos, nTrackPos */
        if (!si) return 0;
        if (m == SBM_SETSCROLLINFO) {
            if (si[1] & SIF_RANGE) { d->smin = (int32_t)si[2]; d->smax = (int32_t)si[3]; }
            if (si[1] & SIF_PAGE) d->spage = (int32_t)si[4];
            if (si[1] & SIF_POS) d->spos = (int32_t)si[5];
            sb_set(d, d->spos);
            return d->spos;
        }
        si[2] = (uint32_t)d->smin; si[3] = (uint32_t)d->smax; si[4] = (uint32_t)d->spage; si[5] = (uint32_t)d->spos; si[6] = (uint32_t)d->spos;
        return TRUE;
    }
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_MOUSEMOVE: case WM_LBUTTONUP: {
        int32_t x = (int16_t)LOWORD(l), y = (int16_t)HIWORD(l), pos = sb_vert(d) ? y : x, len = sb_vert(d) ? d->h : d->w;
        int32_t th = len - 2 * SBW, range = sb_maxpos(d) - d->smin, tl, tp;
        if (!d->enabled) return 0;
        tl = d->spage > 0 && d->smax > d->smin ? th * d->spage / (d->smax - d->smin + 1) : SBW;
        if (tl < 8) tl = 8;
        if (tl > th) tl = th;
        tp = range > 0 ? (th - tl) * (d->spos - d->smin) / range : 0;
        if (m == WM_LBUTTONUP) {
            if (d->sbpart == 5) sb_report(d, SB_THUMBPOSITION, d->spos);
            if (d->sbpart) sb_report(d, SB_ENDSCROLL, d->spos);
            d->sbpart = 0;
            return 0;
        }
        if (m == WM_MOUSEMOVE) {
            if (d->sbpart == 5 && th > tl) {
                sb_set(d, d->smin + (pos - SBW - d->sbgrab) * range / (th - tl));
                sb_report(d, SB_THUMBTRACK, d->spos);
            }
            return 0;
        }
        if (pos < SBW)                { d->sbpart = 1; sb_report(d, SB_LINEUP, d->spos); }
        else if (pos >= len - SBW)    { d->sbpart = 1; sb_report(d, SB_LINEDOWN, d->spos); }
        else if (pos - SBW < tp)      { d->sbpart = 1; sb_report(d, SB_PAGEUP, d->spos); }
        else if (pos - SBW >= tp + tl) { d->sbpart = 1; sb_report(d, SB_PAGEDOWN, d->spos); }
        else { d->sbpart = 5; d->sbgrab = pos - SBW - tp; }
        return 0;
    }
    }
    return 0;
}

/* ---- painting ------------------------------------------------------------------------------------------------------------------ */

static DlgHost *P_h;
static int32_t  P_ox, P_oy;

static void p_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { w32l_dlghost_fill(P_h, P_ox + x, P_oy + y, w, h, c); }

static void p_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c)
{
    p_fill(x, y, w, 1, c);
    p_fill(x, y + h - 1, w, 1, c);
    p_fill(x, y, 1, h, c);
    p_fill(x + w - 1, y, 1, h, c);
}

/* Two-tone frames: top-left colour, bottom-right colour. */
static void p_tone(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t tl, uint32_t br)
{
    p_fill(x, y, w - 1, 1, tl);
    p_fill(x, y, 1, h - 1, tl);
    p_fill(x + w - 1, y, 1, h, br);
    p_fill(x, y + h - 1, w, 1, br);
}

static void p_raised(int32_t x, int32_t y, int32_t w, int32_t h)
{
    p_tone(x, y, w, h, C_HILITE, C_DARK);
    p_tone(x + 1, y + 1, w - 2, h - 2, C_LIGHT, C_SHADOW);
}

static void p_sunken(int32_t x, int32_t y, int32_t w, int32_t h)
{
    p_tone(x, y, w, h, C_SHADOW, C_HILITE);
    p_tone(x + 1, y + 1, w - 2, h - 2, C_DARK, C_LIGHT);
}

static void p_text(int32_t x, int32_t y, const char *s, int32_t n, uint32_t c)
{
    w32l_dlghost_text(P_h, P_ox + x, P_oy + y, c, s, n);
}

static void p_clip(int32_t x, int32_t y, int32_t w, int32_t h) { w32l_dlghost_clip(P_h, P_ox + x, P_oy + y, w, h); }
static void p_unclip(void) { w32l_dlghost_unclip(P_h); }

/* Text without its '&' mnemonic markers. */
static int32_t strip_amp(const char *s, char *out, int32_t cap)
{
    int32_t n = 0;

    for (; *s && n + 1 < cap; s++) {
        if (*s == '&' && s[1])
            s++;
        if (*s == '\r')
            continue;
        out[n++] = *s;
    }
    out[n] = 0;
    return n;
}

static void p_arrow(int32_t cx, int32_t cy, int32_t dir, uint32_t c)
{
    int32_t i;

    for (i = 0; i < 4; i++) {
        switch (dir) {
        case 0: p_fill(cx - i, cy + i, 2 * i + 1, 1, c); break;      /* down */
        case 1: p_fill(cx - i, cy - i, 2 * i + 1, 1, c); break;      /* up */
        case 2: p_fill(cx - i, cy - i, 1, 2 * i + 1, c); break;      /* left */
        case 3: p_fill(cx + i, cy - i, 1, 2 * i + 1, c); break;      /* right */
        }
    }
}

static void p_vbar(int32_t x, int32_t y, int32_t w, int32_t h, int32_t top, int32_t rows, int32_t total)
{
    int32_t th = h - 2 * SBW, tp, tl;

    p_fill(x, y, w, h, C_LIGHT);
    p_fill(x, y, w, SBW, C_FACE);
    p_raised(x, y, w, SBW);
    p_arrow(x + w / 2, y + SBW / 2 - 1, 1, C_TEXT);
    p_fill(x, y + h - SBW, w, SBW, C_FACE);
    p_raised(x, y + h - SBW, w, SBW);
    p_arrow(x + w / 2, y + h - SBW / 2 - 2, 0, C_TEXT);
    if (th > 0) {
        int32_t mx = total - rows;
        tl = th * rows / (total ? total : 1);
        if (tl < 8) tl = 8;
        if (tl > th) tl = th;
        tp = mx > 0 ? (th - tl) * top / mx : 0;
        p_fill(x, y + SBW + tp, w, tl, C_FACE);
        p_raised(x, y + SBW + tp, w, tl);
    }
}

static void paint_button(DWin *d)
{
    char    t[256];
    int32_t n = strip_amp(d->text, t, sizeof t), tc = d->enabled ? C_TEXT : C_GRAYTEXT;
    int32_t foc = dw_top(d)->focus == d;

    switch (d->btype) {
    case BS_GROUPBOX:
        p_tone(0, 4, d->w, d->h - 4, C_SHADOW, C_HILITE);
        p_tone(1, 5, d->w - 2, d->h - 6, C_HILITE, C_SHADOW);
        if (n) {
            p_fill(6, 0, n * CW + 4, 9, C_FACE);
            p_text(8, 0, t, n, tc);
        }
        break;
    case BS_PUSH: case BS_DEFPUSH: case BS_OWNERDRAW: {
        int32_t def = d->btype == BS_DEFPUSH || (dw_top(d)->defid == d->id && d->id);
        int32_t off = d->pushed ? 1 : 0, tw = n * CW;
        p_fill(0, 0, d->w, d->h, C_FACE);
        if (def) p_frame(0, 0, d->w, d->h, C_DARK);
        if (d->pushed) { p_tone(def, def, d->w - 2 * def, d->h - 2 * def, C_SHADOW, C_SHADOW); }
        else if (def) { p_raised(1, 1, d->w - 2, d->h - 2); }
        else p_raised(0, 0, d->w, d->h);
        if (!d->enabled) p_text((d->w - tw) / 2 + 1, (d->h - CH) / 2 + 1, t, n, C_HILITE);
        p_text((d->w - tw) / 2 + off, (d->h - CH) / 2 + off, t, n, tc);
        if (foc) p_frame(4 + off, 4 + off, d->w - 8, d->h - 8, C_SHADOW);
        break;
    }
    default: {                                  /* check boxes and radio buttons */
        int32_t radio = d->btype == BS_RADIO || d->btype == BS_AUTORADIO, by = (d->h - 13) / 2;
        p_fill(0, 0, d->w, d->h, C_FACE);
        if (radio) {
            static const int8_t rw[13] = { 5, 9, 11, 13, 13, 13, 13, 13, 13, 13, 11, 9, 5 };
            int32_t i;
            for (i = 0; i < 13; i++)
                p_fill((13 - rw[i]) / 2, by + i, rw[i], 1, d->enabled ? C_WINDOW : C_FACE);
            p_tone(2, by + 2, 9, 9, C_SHADOW, C_HILITE);
            if (d->check) p_fill(5, by + 5, 3, 3, d->enabled ? C_DARK : C_SHADOW);
        } else {
            p_fill(0, by, 13, 13, d->enabled ? C_WINDOW : C_FACE);
            p_sunken(0, by, 13, 13);
            if (d->check == 1) {
                int32_t i;
                for (i = 0; i < 3; i++) { p_fill(3 + i, by + 5 + i, 1, 3, d->enabled ? C_DARK : C_SHADOW); }
                for (i = 0; i < 4; i++) { p_fill(6 + i, by + 7 - i, 1, 3, d->enabled ? C_DARK : C_SHADOW); }
            } else if (d->check == 2)
                p_fill(3, by + 3, 7, 7, C_SHADOW);
        }
        if (!d->enabled) p_text(19, (d->h - CH) / 2 + 1, t, n, C_HILITE);
        p_text(18, (d->h - CH) / 2, t, n, tc);
        if (foc) p_frame(16, (d->h - 12) / 2, n * CW + 4 > d->w - 16 ? d->w - 16 : n * CW + 4, 12, C_SHADOW);
    }
    }
}

static void paint_static(DWin *d)
{
    char    t[1024];
    int32_t type = d->style & SS_TYPEMASK, tc = d->enabled ? C_TEXT : C_GRAYTEXT;

    switch (type) {
    case SS_BLACKRECT: p_fill(0, 0, d->w, d->h, C_DARK); return;
    case SS_GRAYRECT: p_fill(0, 0, d->w, d->h, C_SHADOW); return;
    case SS_WHITERECT: p_fill(0, 0, d->w, d->h, C_WINDOW); return;
    case SS_BLACKFRAME: p_frame(0, 0, d->w, d->h, C_DARK); return;
    case SS_GRAYFRAME: p_frame(0, 0, d->w, d->h, C_SHADOW); return;
    case SS_WHITEFRAME: p_frame(0, 0, d->w, d->h, C_WINDOW); return;
    case SS_ETCHEDHORZ: p_fill(0, d->h / 2 - 1, d->w, 1, C_SHADOW); p_fill(0, d->h / 2, d->w, 1, C_HILITE); return;
    case SS_ETCHEDVERT: p_fill(d->w / 2 - 1, 0, 1, d->h, C_SHADOW); p_fill(d->w / 2, 0, 1, d->h, C_HILITE); return;
    case SS_ETCHEDFRAME: p_tone(0, 0, d->w, d->h, C_SHADOW, C_HILITE); p_tone(1, 1, d->w - 2, d->h - 2, C_HILITE, C_SHADOW); return;
    case SS_ICON: p_frame(0, 0, d->w > 1 ? d->w : 1, d->h > 1 ? d->h : 1, C_SHADOW); return;
    }
    {
        int32_t n = (d->style & SS_NOPREFIX) ? (int32_t)snprintf(t, sizeof t, "%s", d->text) : strip_amp(d->text, t, sizeof t);
        int32_t cols = d->w / CW > 0 ? d->w / CW : 1, y = 0, i = 0;
        while (i < n && y + CH <= d->h + 2) {              /* word-wrapped rows */
            int32_t end = i, brk = -1, k;
            while (end < n && end - i < cols && t[end] != '\n') {
                if (t[end] == ' ') brk = end;
                end++;
            }
            if (end < n && t[end] != '\n' && brk > i)
                end = brk;
            for (k = end; k > i && (t[k - 1] == ' ' || t[k - 1] == '\r'); k--)
                ;
            {
                int32_t tw = (k - i) * CW, x = type == SS_CENTER ? (d->w - tw) / 2 : type == SS_RIGHT ? d->w - tw : 0;
                p_text(x, y, t + i, k - i, tc);
            }
            i = end;
            while (i < n && (t[i] == ' ' || t[i] == '\r' || t[i] == '\n')) {
                if (t[i] == '\n') { i++; break; }
                i++;
            }
            y += CH + 2;
        }
    }
}

static void paint_edit_text(DWin *d, int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t focused, uint32_t bg)
{
    Rows    r;
    int32_t row, vis = ed_multi(d) ? (h / LINEH > 0 ? h / LINEH : 1) : 1, a, b, crow, ccol;
    uint32_t tc = d->enabled ? C_TEXT : C_GRAYTEXT;

    ed_rows(d, &r);
    ed_selrange(d, &a, &b);
    ed_rc(d, d->caret, &crow, &ccol);
    p_clip(x0, y0, w, h);
    for (row = d->scy; row < r.n && row < d->scy + vis; row++) {
        int32_t i, col = 0, y = y0 + (row - d->scy) * LINEH + (ed_multi(d) ? 0 : (h - CH) / 2);
        for (i = r.start[row]; i < r.start[row] + r.len[row]; i++) {
            char c = d->text[i];
            int32_t sel;
            if (c == '\r')
                continue;
            sel = i >= a && i < b && focused;
            if (!ed_multi(d) && col < d->scx) { col++; continue; }
            {
                int32_t x = x0 + (col - (ed_multi(d) ? 0 : d->scx)) * CW;
                if (x >= x0 + w) break;
                if (sel) p_fill(x, y - 1, CW, CH + 2, C_SELBG);
                if (d->pwchar || (d->style & ES_PASSWORD)) c = d->pwchar ? d->pwchar : '*';
                p_text(x, y, &c, 1, sel ? C_SELTEXT : tc);
            }
            col++;
        }
        if (focused && row == crow && !d->readonly) {
            int32_t cx = x0 + (ccol - (ed_multi(d) ? 0 : d->scx)) * CW;
            p_fill(cx, y - 1, 1, CH + 2, C_TEXT);
        }
    }
    p_unclip();
    (void)bg;
    ed_free_rows(&r);
}

static void paint_edit(DWin *d)
{
    int32_t foc = dw_top(d)->focus == d, hasbar = ed_multi(d) && (d->style & WS_VSCROLL);

    p_fill(0, 0, d->w, d->h, d->readonly || !d->enabled ? C_FACE : C_WINDOW);
    p_sunken(0, 0, d->w, d->h);
    paint_edit_text(d, 3, 3, d->w - 6 - (hasbar ? SBW : 0), d->h - 6, foc, C_WINDOW);
    if (hasbar) {
        Rows r;
        ed_rows(d, &r);
        p_vbar(d->w - 2 - SBW, 2, SBW, d->h - 4, d->scy, (d->h - 6) / LINEH, r.n);
        ed_free_rows(&r);
    }
}

static void paint_rows(DWin *d, int32_t x, int32_t y, int32_t w, int32_t h, int32_t sel_follow_hover)
{
    int32_t rows = h / ROWH > 0 ? h / ROWH : 1, i, bar = d->nitems > rows;
    int32_t iw = w - (bar ? SBW : 0);

    p_clip(x, y, w, h);
    for (i = d->top; i < d->nitems && i < d->top + rows; i++) {
        int32_t ry = y + (i - d->top) * ROWH;
        int32_t sel = lst_multi(d) ? d->items[i].sel : sel_follow_hover ? (d->hover >= 0 ? d->hover == i : d->cursel == i) : d->cursel == i;
        if (sel) p_fill(x, ry, iw, ROWH, C_SELBG);
        p_text(x + 2, ry + (ROWH - CH) / 2, d->items[i].text, (int32_t)strlen(d->items[i].text), sel ? C_SELTEXT : d->enabled ? C_TEXT : C_GRAYTEXT);
    }
    p_unclip();
    if (bar)
        p_vbar(x + w - SBW, y, SBW, h, d->top, rows, d->nitems);
}

static void paint_list(DWin *d)
{
    p_fill(0, 0, d->w, d->h, C_WINDOW);
    p_sunken(0, 0, d->w, d->h);
    paint_rows(d, 2, 2, d->w - 4, d->h - 4, 0);
}

static void paint_combo(DWin *d)
{
    int32_t foc = dw_top(d)->focus == d, tw = d->w - SBW - 4;

    p_fill(0, 0, d->w, COMBOH, d->enabled ? C_WINDOW : C_FACE);
    p_sunken(0, 0, d->w, COMBOH);
    if (cb_has_edit(d)) {
        paint_edit_text(d, 3, 2, tw, COMBOH - 4, foc, C_WINDOW);
    } else {
        const char *s = d->cursel >= 0 && d->cursel < d->nitems ? d->items[d->cursel].text : "";
        int32_t n = (int32_t)strlen(s);
        p_clip(2, 2, tw, COMBOH - 4);
        if (foc) p_fill(2, 2, tw, COMBOH - 4, C_SELBG);
        p_text(4, (COMBOH - CH) / 2, s, n, foc ? C_SELTEXT : d->enabled ? C_TEXT : C_GRAYTEXT);
        p_unclip();
    }
    p_fill(d->w - SBW - 2, 2, SBW, COMBOH - 4, C_FACE);
    if (d->drop) p_tone(d->w - SBW - 2, 2, SBW, COMBOH - 4, C_SHADOW, C_SHADOW);
    else p_raised(d->w - SBW - 2, 2, SBW, COMBOH - 4);
    p_arrow(d->w - SBW / 2 - 2, COMBOH / 2 - 1, 0, d->enabled ? C_TEXT : C_GRAYTEXT);
}

static void paint_combo_drop(DWin *d)
{
    p_fill(0, COMBOH, d->w, d->dropw, C_WINDOW);
    p_frame(0, COMBOH, d->w, d->dropw, C_DARK);
    paint_rows(d, 1, COMBOH + 2, d->w - 2, d->dropw - 4, 1);
}

static void paint_scroll(DWin *d)
{
    int32_t len = sb_vert(d) ? d->h : d->w, th = len - 2 * SBW, range = sb_maxpos(d) - d->smin, tl, tp;

    p_fill(0, 0, d->w, d->h, C_LIGHT);
    tl = d->spage > 0 && d->smax > d->smin ? th * d->spage / (d->smax - d->smin + 1) : SBW;
    if (tl < 8) tl = 8;
    if (tl > th) tl = th > 0 ? th : 0;
    tp = range > 0 ? (th - tl) * (d->spos - d->smin) / range : 0;
    if (sb_vert(d)) {
        p_fill(0, 0, d->w, SBW, C_FACE); p_raised(0, 0, d->w, SBW); p_arrow(d->w / 2, SBW / 2 - 1, 1, C_TEXT);
        p_fill(0, d->h - SBW, d->w, SBW, C_FACE); p_raised(0, d->h - SBW, d->w, SBW); p_arrow(d->w / 2, d->h - SBW / 2 - 2, 0, C_TEXT);
        if (th > 0) { p_fill(0, SBW + tp, d->w, tl, C_FACE); p_raised(0, SBW + tp, d->w, tl); }
    } else {
        p_fill(0, 0, SBW, d->h, C_FACE); p_raised(0, 0, SBW, d->h); p_arrow(SBW / 2 - 1, d->h / 2, 2, C_TEXT);
        p_fill(d->w - SBW, 0, SBW, d->h, C_FACE); p_raised(d->w - SBW, 0, SBW, d->h); p_arrow(d->w - SBW / 2 - 2, d->h / 2, 3, C_TEXT);
        if (th > 0) { p_fill(SBW + tp, 0, tl, d->h, C_FACE); p_raised(SBW + tp, 0, tl, d->h); }
    }
}

static void paint_custom(DWin *d)
{
    p_fill(0, 0, d->w, d->h, C_FACE);
    p_sunken(0, 0, d->w, d->h);
    p_clip(2, 2, d->w - 4, d->h - 4);
    p_text(4, 4, d->cls, (int32_t)strlen(d->cls), C_GRAYTEXT);
    p_unclip();
}

static void paint_control(DWin *d, int32_t ox, int32_t oy)
{
    P_ox = ox + d->x;
    P_oy = oy + d->y;
    p_clip(0, 0, d->kind == K_COMBO && d->drop ? d->w : d->w, d->kind == K_COMBO ? COMBOH : d->h);
    switch (d->kind) {
    case K_BUTTON: paint_button(d); break;
    case K_STATIC: paint_static(d); break;
    case K_EDIT: paint_edit(d); break;
    case K_LIST: paint_list(d); break;
    case K_COMBO: paint_combo(d); break;
    case K_SCROLL: paint_scroll(d); break;
    default: paint_custom(d); break;
    }
    p_unclip();
    if (!d->enabled && d->kind != K_STATIC && d->kind != K_BUTTON && d->kind != K_CUSTOM) { /* the controls dim themselves */ }
}

static void paint_dialog(DWin *dlg)
{
    int32_t i;
    int32_t cap = dlg->decor ? DECOR_CAPTION : 0;

    P_h = dlg->host;
    P_ox = 0;
    P_oy = 0;
    if (cap) {                                   /* the caption bar, above the client area */
        p_fill(0, 0, dlg->w, cap, C_SELBG);
        p_clip(0, 0, dlg->w - 20, cap);
        p_text(4, (cap - CH) / 2, dlg->text, dlg->tlen, C_SELTEXT);
        p_unclip();
        p_fill(dlg->w - 18, 3, 16, cap - 6, C_FACE);
        p_raised(dlg->w - 18, 3, 16, cap - 6);
        p_text(dlg->w - 14, (cap - CH) / 2, "x", 1, C_TEXT);
    }
    P_oy = cap;
    p_fill(0, 0, dlg->w, dlg->h, C_FACE);
    for (i = 0; i < dlg->nkids; i++)
        if (dlg->kids[i]->visible)
            paint_control(dlg->kids[i], 0, cap);
    if (dlg->dropped && dlg->dropped->drop) {
        DWin *c = dlg->dropped;
        P_ox = c->x;
        P_oy = cap + c->y;
        p_clip(0, 0, c->w, COMBOH + c->dropw + 1);
        paint_combo_drop(c);
        p_unclip();
    }
}

/* ---- messages --------------------------------------------------------------------------------------------------------------- */

static void dw_show(DWin *d, int32_t visible);

static LRESULT dlg_default(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CLOSE:
        dw_send(d, WM_COMMAND, IDCANCEL, 0);
        return 0;
    case WM_SETTEXT:
        dw_settext(d, (const char *)(uintptr_t)l);
        w32l_dlghost_title(d->host, d->text);
        return TRUE;
    case WM_GETTEXTLENGTH: return d->tlen;
    case WM_GETTEXT: {
        int32_t n = (int32_t)w - 1 < d->tlen ? (int32_t)w - 1 : d->tlen;
        if (n < 0 || !l) return 0;
        memcpy((char *)(uintptr_t)l, d->text, (size_t)n);
        ((char *)(uintptr_t)l)[n] = 0;
        return n;
    }
    case DM_GETDEFID: return d->defid ? MAKELONG(d->defid, 0x534B) : 0;
    case DM_SETDEFID: d->defid = (int32_t)w; dw_invalidate(d); return TRUE;
    case WM_NEXTDLGCTL: {
        DWin *to = l ? dw_get((HWND)(uintptr_t)w) : dw_next_tab(d, d->focus, (int32_t)w != 0);
        if (to && to->parent == d) {
            dw_set_focus(to);
            if (to->kind == K_EDIT)
                ed_proc(to, EM_SETSEL, 0, -1);
        }
        return 0;
    }
    }
    return 0;
}

static LRESULT dlg_send(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    if (d->proc) {
        LRESULT r = d->proc(d->hwnd, m, w, l);
        if (r) {
            if (m == WM_INITDIALOG || (m >= 0x132 && m <= 0x138) || m == 0x39 || m == 0x2E || m == 0x2F || m == 0x37)
                return r;
            return d->msgresult;
        }
        if (d->dead)
            return 0;
    }
    return dlg_default(d, m, w, l);
}

static LRESULT ctl_send(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    if (d->cproc)
        return d->cproc(d->hwnd, m, w, l);
    switch (m) {
    case WM_ENABLE: d->enabled = w != 0; dw_invalidate(d); return 0;
    case WM_SETFONT: case WM_GETFONT: case WM_SETREDRAW: case WM_DESTROY: case WM_SETFOCUS: case WM_KILLFOCUS: return 0;
    case WM_SHOWWINDOW: d->visible = w != 0; dw_invalidate(d); return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (d->enabled && (d->kind == K_EDIT || d->kind == K_LIST || d->kind == K_COMBO ||
                           (d->kind == K_BUTTON && d->btype != BS_GROUPBOX)))
            dw_set_focus(d);
        break;
    }
    switch (d->kind) {
    case K_BUTTON: case K_STATIC: case K_CUSTOM:
        if (m == WM_SETTEXT) { dw_settext(d, (const char *)(uintptr_t)l); dw_invalidate(d); return TRUE; }
        if (m == WM_GETTEXTLENGTH) return d->tlen;
        if (m == WM_GETTEXT) {
            int32_t n = (int32_t)w - 1 < d->tlen ? (int32_t)w - 1 : d->tlen;
            if (n < 0 || !l) return 0;
            memcpy((char *)(uintptr_t)l, d->text, (size_t)n);
            ((char *)(uintptr_t)l)[n] = 0;
            return n;
        }
        return d->kind == K_BUTTON ? btn_proc(d, m, w, l) : d->kind == K_STATIC ? st_proc(d, m, w, l) : 0;
    case K_EDIT: return ed_proc(d, m, w, l);
    case K_LIST: return lb_proc(d, m, w, l);
    case K_COMBO: return cb_proc(d, m, w, l);
    case K_SCROLL: return sb_proc(d, m, w, l);
    }
    return 0;
}

static LRESULT dw_send(DWin *d, UINT m, WPARAM w, LPARAM l)
{
    LRESULT r;

    if (d->dead)
        return 0;
    w32l_send_depth++;
    r = d->kind == K_DIALOG ? dlg_send(d, m, w, l) : ctl_send(d, m, w, l);
    w32l_send_depth--;
    return r;
}

LRESULT w32l_dw_send(HWND h, UINT m, WPARAM w, LPARAM l)
{
    DWin *d = dw_get(h);
    return d ? dw_send(d, m, w, l) : 0;
}

/* ---- windows: show, destroy, geometry ---------------------------------------------------------------------------------- */

static void dw_decor(DWin *d, int32_t *l, int32_t *t, int32_t *r, int32_t *b)
{
    int32_t side = d->decor ? DECOR_SIDE : 0, cap = d->decor ? DECOR_SIDE + DECOR_CAPTION : 0;

    *l = *r = *b = side;
    *t = cap;
}

static void dw_show(DWin *d, int32_t visible)
{
    if (d->dead || d->visible == visible)
        return;
    d->visible = visible;
    if (d->kind == K_DIALOG) {
        dw_send(d, WM_SHOWWINDOW, (WPARAM)visible, 0);
        w32l_dlghost_show(d->host, visible);
        d->dirty = d->needdump = 1;
    } else {
        dw_send(d, WM_SHOWWINDOW, (WPARAM)visible, 0);
        dw_invalidate(d);
    }
}

BOOL w32l_dw_show(HWND h, int32_t cmd)
{
    DWin *d = dw_get(h);
    int32_t was;

    if (!d)
        return FALSE;
    was = d->visible;
    dw_show(d, cmd != SW_HIDE && cmd != SW_MINIMIZE);
    return was != 0;
}

static void dw_destroy_tree(DWin *d)
{
    int32_t i;

    if (d->dead)
        return;
    dw_send(d, WM_DESTROY, 0, 0);
    while (d->nkids > 0) {
        DWin *k = d->kids[d->nkids - 1];
        if (k->dead) {
            d->nkids--;
            continue;
        }
        dw_destroy_tree(k);
    }
    if (d->dead)
        return;
    d->dead = 1;
    if (d->parent) {
        DWin *p = d->parent;
        for (i = 0; i < p->nkids; i++)
            if (p->kids[i] == d) {
                memmove(&p->kids[i], &p->kids[i + 1], (size_t)(p->nkids - i - 1) * sizeof *p->kids);
                p->nkids--;
                break;
            }
        if (p->focus == d) p->focus = NULL;
        if (p->mousecap == d) p->mousecap = NULL;
        if (p->dropped == d) p->dropped = NULL;
    }
    if (d->kind == K_DIALOG) {
        w32l_dlghost_destroy(d->host);
        d->host = NULL;
        w32l_win_dialogs--;
    }
    w32l_win[((uintptr_t)d->hwnd - W32L_DW_BASE) / 4] = NULL;
    w32l_zombies = (DWin **)realloc(w32l_zombies, (size_t)(w32l_nzombies + 1) * sizeof *w32l_zombies);
    w32l_zombies[w32l_nzombies++] = d;
}

BOOL w32l_dw_destroy(HWND h)
{
    DWin *d = dw_get(h);

    if (!d)
        return FALSE;
    if (d->kind == K_DIALOG && d->modal) {
        d->ended = 1;
        if (!d->result)
            d->result = 0;
    }
    dw_destroy_tree(d);
    return TRUE;
}

BOOL w32l_dw_rect(HWND h, LPRECT rc, int32_t client)
{
    DWin   *d = dw_get(h);
    int32_t l, t, r, b, x, y;

    if (!d || !rc)
        return FALSE;
    if (client) {
        rc->left = rc->top = 0;
        rc->right = d->w;
        rc->bottom = d->kind == K_COMBO ? d->h : d->h;
        return TRUE;
    }
    if (d->kind == K_DIALOG) {
        dw_decor(d, &l, &t, &r, &b);
        rc->left = d->x - l;
        rc->top = d->y - t;
        rc->right = d->x + d->w + r;
        rc->bottom = d->y + d->h + b;
        return TRUE;
    }
    dw_abs(d->parent, &x, &y);
    x += dw_top(d)->x;
    y += dw_top(d)->y;
    rc->left = x + d->x;
    rc->top = y + d->y;
    rc->right = rc->left + d->w;
    rc->bottom = rc->top + d->h;
    return TRUE;
}

BOOL w32l_dw_move(HWND h, int32_t x, int32_t y, int32_t w, int32_t ht, int32_t flags)
{
    DWin   *d = dw_get(h);
    int32_t l, t, r, b, ow, oh, ox, oy;

    if (!d)
        return FALSE;
    ow = d->w; oh = d->h; ox = d->x; oy = d->y;
    if (d->kind == K_DIALOG) {
        dw_decor(d, &l, &t, &r, &b);
        if (!(flags & SWP_NOMOVE)) { d->x = x + l; d->y = y + t; }
        if (!(flags & SWP_NOSIZE) && w > 0 && ht > 0) { d->w = w - l - r; d->h = ht - t - b; }
        w32l_dlghost_place(d->host, d->x, d->y, d->w, d->h);
        d->dirty = d->needdump = 1;
        if (d->x != ox || d->y != oy)
            dw_send(d, WM_MOVE, 0, MAKELONG(d->x, d->y));
        if (d->w != ow || d->h != oh)
            dw_send(d, WM_SIZE, 0, MAKELONG(d->w, d->h));
        return TRUE;
    }
    if (!(flags & SWP_NOMOVE)) { d->x = x; d->y = y; }
    if (!(flags & SWP_NOSIZE) && w >= 0 && ht >= 0) { d->w = w; d->h = ht; }
    dw_invalidate(d);
    return TRUE;
}

BOOL w32l_dw_set_text(HWND h, LPCSTR text)
{
    DWin *d = dw_get(h);
    if (!d)
        return FALSE;
    dw_send(d, WM_SETTEXT, 0, (LPARAM)(uintptr_t)(text ? text : ""));
    return TRUE;
}

BOOL w32l_dw_to_screen(HWND h, LPPOINT pt)
{
    DWin   *d = dw_get(h);
    int32_t x, y;

    if (!d || !pt)
        return FALSE;
    dw_abs(d, &x, &y);
    pt->x += x + dw_top(d)->x;
    pt->y += y + dw_top(d)->y;
    return TRUE;
}

LONG w32l_dw_getlong(HWND h, int32_t index)
{
    DWin *d = dw_get(h);
    LONG  v = 0;

    if (!d)
        return 0;
    if (index >= 0) {
        if (d->kind == K_DIALOG) {
            if (index == DWL_MSGRESULT) return d->msgresult;
            if (index == DWL_DLGPROC) return (LONG)(uintptr_t)d->proc;
            index -= DWL_USER;
        }
        if (index >= 0 && (size_t)index + sizeof v <= sizeof d->extra)
            memcpy(&v, d->extra + index, sizeof v);
        return v;
    }
    switch (index) {
    case GWL_STYLE: return (LONG)(d->style | (d->visible ? WS_VISIBLE : 0) | (d->enabled ? 0 : WS_DISABLED));
    case GWL_EXSTYLE: return (LONG)d->exstyle;
    case GWL_USERDATA: return d->userdata;
    case GWL_ID: return d->id;
    case GWL_WNDPROC: return (LONG)(uintptr_t)(d->kind == K_DIALOG ? (WNDPROC)d->proc : d->cproc);
    case GWL_HINSTANCE: return 0x400000;
    case -8: return d->parent ? (LONG)(uintptr_t)d->parent->hwnd : 0;      /* GWL_HWNDPARENT */
    }
    return 0;
}

LONG w32l_dw_setlong(HWND h, int32_t index, LONG value)
{
    DWin *d = dw_get(h);
    LONG  old = w32l_dw_getlong(h, index);

    if (!d)
        return 0;
    if (index >= 0) {
        if (d->kind == K_DIALOG) {
            if (index == DWL_MSGRESULT) { d->msgresult = value; return old; }
            if (index == DWL_DLGPROC) { d->proc = (DLGPROC)(uintptr_t)value; return old; }
            index -= DWL_USER;
        }
        if (index >= 0 && (size_t)index + sizeof value <= sizeof d->extra)
            memcpy(d->extra + index, &value, sizeof value);
        return old;
    }
    switch (index) {
    case GWL_STYLE:
        d->style = (uint32_t)value & ~(uint32_t)(WS_VISIBLE | WS_DISABLED);
        if (d->kind == K_BUTTON) d->btype = (int32_t)(value & BS_TYPEMASK);
        dw_invalidate(d);
        break;
    case GWL_EXSTYLE: d->exstyle = (uint32_t)value; break;
    case GWL_USERDATA: d->userdata = value; break;
    case GWL_ID: d->id = value; break;
    case GWL_WNDPROC:
        if (d->kind == K_DIALOG) d->proc = (DLGPROC)(uintptr_t)value;
        else d->cproc = (WNDPROC)(uintptr_t)value;
        break;
    }
    return old;
}

/* ---- creation from a template ---------------------------------------------------------------------------------------------------- */

static uint16_t rd16(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static uint32_t rd32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }

/* A "sz_Or_Ord" at p: an ordinal (0xFFFF, n) or a wide string, narrowed to Latin-1. */
static const uint8_t *tpl_sz(const uint8_t *p, char *out, size_t cap, uint32_t *ord)
{
    size_t n = 0;

    *ord = 0;
    out[0] = 0;
    if (rd16(p) == 0xFFFF) {
        *ord = rd16(p + 2);
        return p + 4;
    }
    for (; rd16(p); p += 2)
        if (n + 1 < cap)
            out[n++] = rd16(p) < 0x100 ? (char)rd16(p) : '?';
    out[n] = 0;
    return p + 2;
}

static int32_t class_kind(const char *name, uint32_t ord)
{
    if (ord == 0x80 || !strcasecmp(name, "BUTTON")) return K_BUTTON;
    if (ord == 0x81 || !strcasecmp(name, "EDIT")) return K_EDIT;
    if (ord == 0x82 || !strcasecmp(name, "STATIC")) return K_STATIC;
    if (ord == 0x83 || !strcasecmp(name, "LISTBOX")) return K_LIST;
    if (ord == 0x84 || !strcasecmp(name, "SCROLLBAR")) return K_SCROLL;
    if (ord == 0x85 || !strcasecmp(name, "COMBOBOX")) return K_COMBO;
    return K_CUSTOM;
}

static void ctl_init(DWin *c, const char *cls, uint32_t ord)
{
    snprintf(c->cls, sizeof c->cls, "%s", cls[0] ? cls : (ord == 0x80 ? "BUTTON" : "?"));
    c->visible = (c->style & WS_VISIBLE) != 0;
    c->enabled = !(c->style & WS_DISABLED);
    switch (c->kind) {
    case K_BUTTON: c->btype = (int32_t)(c->style & BS_TYPEMASK); break;
    case K_EDIT: c->readonly = (c->style & ES_READONLY) != 0; break;
    case K_COMBO: c->dropw = 0; break;
    case K_SCROLL: c->smin = 0; c->smax = 100; break;
    }
}

static DWin *dlg_build(const uint8_t *tpl, DLGPROC proc, LPARAM param, const char *resname, int32_t modal)
{
    const uint8_t *p = tpl;
    uint32_t       style, exstyle, ord;
    uint32_t       nitems;
    int32_t        x, y, cx, cy, ex = rd16(tpl) == 1 && rd16(tpl + 2) == 0xFFFF, i, dx, dy, dw_, dh;
    char           buf[1024], title[256];
    DWin          *dlg, *first = NULL;
    LRESULT        r;

    if (ex) {
        exstyle = rd32(p + 8);
        style = rd32(p + 12);
        nitems = rd16(p + 16);
        x = (int16_t)rd16(p + 18); y = (int16_t)rd16(p + 20); cx = (int16_t)rd16(p + 22); cy = (int16_t)rd16(p + 24);
        p += 26;
    } else {
        style = rd32(p);
        exstyle = rd32(p + 4);
        nitems = rd16(p + 8);
        x = (int16_t)rd16(p + 10); y = (int16_t)rd16(p + 12); cx = (int16_t)rd16(p + 14); cy = (int16_t)rd16(p + 16);
        p += 18;
    }
    p = tpl_sz(p, buf, sizeof buf, &ord);               /* menu */
    p = tpl_sz(p, buf, sizeof buf, &ord);               /* class */
    p = tpl_sz(p, title, sizeof title, &ord);
    if (style & DS_SETFONT) {
        p += ex ? 6 : 2;
        p = tpl_sz(p, buf, sizeof buf, &ord);
    }
    dlg = dw_new(K_DIALOG, NULL);
    if (!dlg)
        return NULL;
    dlg->proc = proc;
    dlg->style = style & ~(uint32_t)WS_VISIBLE;
    dlg->exstyle = exstyle;
    dlg->decor = (style & WS_CAPTION) == WS_CAPTION || (style & WS_DLGFRAME);
    dlg->visible = 0;
    dlg->modal = modal;
    dw_settext(dlg, title);
    snprintf(dlg->resname, sizeof dlg->resname, "%s", resname ? resname : "");
    dlg->w = DUX(cx);
    dlg->h = DUY(cy);
    dw_decor(dlg, &dx, &dy, &dw_, &dh);
    dlg->x = DUX(x) + dx;
    dlg->y = DUY(y) + dy;
    if (style & DS_CENTER) {
        int32_t sw, sh;
        w32l_dlghost_desktop(&sw, &sh);
        dlg->x = (sw - dlg->w) / 2;
        dlg->y = (sh - dlg->h) / 2;
    }
    w32l_win_dialogs++;
    dlg->host = w32l_dlghost_create(title, dlg->w, dlg->h, dlg->decor ? DECOR_CAPTION : 0, dlg->x, dlg->y, dlg);
    for (i = 0; i < (int32_t)nitems; i++) {
        uint32_t istyle, iex, cord, tord;
        int32_t  ix, iy, icx, icy, id;
        char     cls[64], text[512];
        DWin    *c;

        p = tpl + (((p - tpl) + 3) & ~3);
        if (ex) {
            iex = rd32(p + 4); istyle = rd32(p + 8);
            ix = (int16_t)rd16(p + 12); iy = (int16_t)rd16(p + 14); icx = (int16_t)rd16(p + 16); icy = (int16_t)rd16(p + 18);
            id = (int32_t)rd32(p + 20);
            p += 24;
        } else {
            istyle = rd32(p); iex = rd32(p + 4);
            ix = (int16_t)rd16(p + 8); iy = (int16_t)rd16(p + 10); icx = (int16_t)rd16(p + 12); icy = (int16_t)rd16(p + 14);
            id = (int32_t)(uint16_t)rd16(p + 16);
            if (id == 0xFFFF) id = -1;
            p += 18;
        }
        p = tpl_sz(p, cls, sizeof cls, &cord);
        p = tpl_sz(p, text, sizeof text, &tord);
        p += 2 + rd16(p);
        c = dw_new(class_kind(cls, cord), dlg);
        if (!c)
            break;
        c->id = id;
        c->style = istyle;
        c->exstyle = iex;
        c->x = DUX(ix); c->y = DUY(iy); c->w = DUX(icx); c->h = DUY(icy);
        if (c->kind == K_CUSTOM)
            c->cproc = w32l_class_proc(cls);
        ctl_init(c, cls, cord);
        if (c->kind == K_EDIT || c->kind == K_COMBO || c->kind == K_BUTTON || c->kind == K_STATIC || c->kind == K_CUSTOM) {
            dw_settext(c, tord ? "" : text);
            c->caret = c->anchor = 0;
        }
        if (c->kind == K_BUTTON && c->btype == BS_DEFPUSH && !dlg->defid)
            dlg->defid = c->id;
        if (!first && dw_tabstop(c))
            first = c;
    }
    if (!dlg->defid && dw_find_id(dlg, IDOK))
        dlg->defid = IDOK;
    for (i = 0; i < dlg->nkids; i++) {
        DWin *c = dlg->kids[i];
        if (c->kind == K_COMBO || c->kind == K_LIST) { c->cursel = -1; }
    }
    r = dw_send(dlg, WM_INITDIALOG, first ? (WPARAM)(uintptr_t)first->hwnd : 0, param);
    if (dlg->dead)
        return NULL;
    if (r && first) {
        dw_set_focus(first);
        if (first->kind == K_EDIT)
            ed_proc(first, EM_SETSEL, 0, -1);
    }
    if (style & WS_VISIBLE)
        dw_show(dlg, 1);
    return dlg;
}

/* The dialog template of a resource name, or NULL. */
static const uint8_t *dlg_template(HINSTANCE inst, LPCSTR name)
{
    HRSRC r = FindResourceExA(inst, RT_DIALOG, name, 0);
    return r ? (const uint8_t *)LoadResource(inst, r) : NULL;
}

static HWND dlg_create_name(HINSTANCE inst, LPCSTR name, DLGPROC proc, LPARAM param, int32_t modal, DWin **out)
{
    const uint8_t *t = dlg_template(inst, name);
    DWin          *d;
    char           rn[64];

    if (!t) {
        w32l_plat_log("dialog %s: no such RT_DIALOG resource", (uintptr_t)name < 0x10000 ? "#" : name);
        SetLastError(1814);
        return NULL;
    }
    if ((uintptr_t)name < 0x10000)
        snprintf(rn, sizeof rn, "id%u", (unsigned)(uintptr_t)name);
    else
        snprintf(rn, sizeof rn, "%s", name);
    d = dlg_build(t, proc, param, rn, modal);
    if (out)
        *out = d;
    return d ? d->hwnd : NULL;
}

static INT_PTR dlg_run_modal(DWin *d)
{
    static int32_t autoclose = -2;
    int32_t        pumps = 0;
    HWND           h = d->hwnd;
    INT_PTR        res;

    if (autoclose == -2) {
        const char *e = getenv("W32L_DIALOG_MODAL_PUMPS");
        autoclose = e ? atoi(e) : 0;
    }
    if (!d->visible)
        dw_show(d, 1);
    while (!d->ended && !d->dead) {
        MSG m;
        if (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) {
                PostQuitMessage((int32_t)m.wParam);
                break;
            }
            DispatchMessageA(&m);
        } else
            Sleep(1);
        if (autoclose && ++pumps > autoclose) {
            w32l_plat_log("dialog %s: closed after %d pumps (W32L_DIALOG_MODAL_PUMPS)", d->resname, pumps);
            d->result = IDCANCEL;
            d->ended = 1;
        }
    }
    res = d->result;
    if (w32l_dw_valid(h))
        w32l_dw_destroy(h);
    return res;
}

HWND WINAPI CreateDialogParamA(HINSTANCE inst, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    (void)parent;
    return dlg_create_name(inst, templ, proc, param, 0, NULL);
}

HWND WINAPI CreateDialogIndirectParamA(HINSTANCE inst, LPCDLGTEMPLATE templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DWin *d = templ ? dlg_build((const uint8_t *)templ, proc, param, "indirect", 0) : NULL;

    (void)inst; (void)parent;
    return d ? d->hwnd : NULL;
}

INT_PTR WINAPI DialogBoxParamA(HINSTANCE inst, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DWin *d = NULL;

    (void)parent;
    if (!dlg_create_name(inst, templ, proc, param, 1, &d))
        return -1;
    return dlg_run_modal(d);
}

INT_PTR WINAPI DialogBoxIndirectParamA(HINSTANCE inst, LPCDLGTEMPLATE templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DWin *d = templ ? dlg_build((const uint8_t *)templ, proc, param, "indirect", 1) : NULL;

    (void)inst; (void)parent;
    return d ? dlg_run_modal(d) : -1;
}

static const char *wide_name(LPCWSTR w, char *out, size_t cap)
{
    size_t i;

    if ((uintptr_t)w < 0x10000)
        return (const char *)w;
    for (i = 0; w[i] && i + 1 < cap; i++)
        out[i] = w[i] < 0x100 ? (char)w[i] : '?';
    out[i] = 0;
    return out;
}

HWND WINAPI CreateDialogParamW(HINSTANCE inst, LPCWSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    char n[128];
    return CreateDialogParamA(inst, wide_name(templ, n, sizeof n), parent, proc, param);
}

INT_PTR WINAPI DialogBoxParamW(HINSTANCE inst, LPCWSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    char n[128];
    return DialogBoxParamA(inst, wide_name(templ, n, sizeof n), parent, proc, param);
}

BOOL WINAPI EndDialog(HWND h, INT_PTR result)
{
    DWin *d = dw_get(h);

    if (!d || d->kind != K_DIALOG)
        return FALSE;
    if (d->modal) {
        d->result = result;
        d->ended = 1;
    }
    dw_show(d, 0);
    return TRUE;
}

/* ---- the Win32 calls over dialog items ------------------------------------------------------------------------------------------------ */

HWND WINAPI GetDlgItem(HWND h, int32_t id)
{
    DWin *d = dw_get(h), *c;

    if (!d)
        return NULL;
    c = dw_find_id(d, id);
    return c ? c->hwnd : NULL;
}

int32_t WINAPI GetDlgCtrlID(HWND h)
{
    DWin *d = dw_get(h);
    return d ? d->id : 0;
}

LRESULT WINAPI SendDlgItemMessageA(HWND h, int32_t id, UINT m, WPARAM w, LPARAM l)
{
    HWND c = GetDlgItem(h, id);
    return c ? SendMessageA(c, m, w, l) : 0;
}

UINT WINAPI GetDlgItemTextA(HWND h, int32_t id, LPSTR out, int32_t cch)
{
    HWND c = GetDlgItem(h, id);

    if (out && cch > 0)
        out[0] = 0;
    return c ? (UINT)SendMessageA(c, WM_GETTEXT, (WPARAM)cch, (LPARAM)(uintptr_t)out) : 0;
}

BOOL WINAPI SetDlgItemTextA(HWND h, int32_t id, LPCSTR text)
{
    HWND c = GetDlgItem(h, id);
    return c ? (SendMessageA(c, WM_SETTEXT, 0, (LPARAM)(uintptr_t)(text ? text : "")), TRUE) : FALSE;
}

UINT WINAPI GetDlgItemInt(HWND h, int32_t id, BOOL *translated, BOOL isSigned)
{
    char    t[64], *end;
    int64_t v;
    UINT    n = GetDlgItemTextA(h, id, t, sizeof t);

    if (translated)
        *translated = FALSE;
    if (!n)
        return 0;
    v = strtoll(t, &end, 10);
    if (*end || end == t || (!isSigned && t[0] == '-') || (isSigned ? (v < INT32_MIN || v > INT32_MAX) : (v > UINT32_MAX)))
        return 0;
    if (translated)
        *translated = TRUE;
    return (UINT)v;
}

BOOL WINAPI SetDlgItemInt(HWND h, int32_t id, UINT value, BOOL isSigned)
{
    char t[32];

    if (isSigned)
        snprintf(t, sizeof t, "%d", (int)value);
    else
        snprintf(t, sizeof t, "%u", (unsigned)value);
    return SetDlgItemTextA(h, id, t);
}

BOOL WINAPI CheckDlgButton(HWND h, int32_t id, UINT check)
{
    HWND c = GetDlgItem(h, id);
    return c ? (SendMessageA(c, BM_SETCHECK, check, 0), TRUE) : FALSE;
}

UINT WINAPI IsDlgButtonChecked(HWND h, int32_t id)
{
    HWND c = GetDlgItem(h, id);
    return c ? (UINT)SendMessageA(c, BM_GETCHECK, 0, 0) : 0;
}

BOOL WINAPI CheckRadioButton(HWND h, int32_t first, int32_t last, int32_t check)
{
    int32_t id;

    for (id = first; id <= last; id++)
        CheckDlgButton(h, id, id == check);
    return TRUE;
}

LRESULT WINAPI DefDlgProcA(HWND h, UINT m, WPARAM w, LPARAM l)
{
    DWin *d = dw_get(h);
    return d && d->kind == K_DIALOG ? dlg_default(d, m, w, l) : 0;
}

BOOL WINAPI MapDialogRect(HWND h, LPRECT rc)
{
    if (!dw_get(h) || !rc)
        return FALSE;
    rc->left = DUX(rc->left); rc->right = DUX(rc->right);
    rc->top = DUY(rc->top); rc->bottom = DUY(rc->bottom);
    return TRUE;
}

HWND WINAPI GetNextDlgTabItem(HWND h, HWND ctl, BOOL previous)
{
    DWin *d = dw_get(h), *n;

    if (!d || d->kind != K_DIALOG)
        return NULL;
    n = dw_next_tab(d, dw_get(ctl), previous);
    return n ? n->hwnd : NULL;
}

BOOL WINAPI EnableWindow(HWND h, BOOL enable)
{
    DWin *d = dw_get(h);
    BOOL  was;

    if (!d)
        return FALSE;
    was = !d->enabled;
    if ((enable != 0) != d->enabled) {
        d->enabled = enable != 0;
        dw_send(d, WM_ENABLE, (WPARAM)(enable != 0), 0);
        dw_invalidate(d);
    }
    return was;
}

BOOL WINAPI IsWindow(HWND h) { return dw_get(h) != NULL || (h && h == w32l_the_window()); }

BOOL WINAPI IsWindowVisible(HWND h)
{
    DWin *d = dw_get(h);
    return d ? d->visible && (!d->parent || d->parent->visible) : (h && h == w32l_the_window());
}

BOOL WINAPI IsWindowEnabled(HWND h)
{
    DWin *d = dw_get(h);
    return d ? d->enabled : (h && h == w32l_the_window());
}

HWND WINAPI GetParent(HWND h)
{
    DWin *d = dw_get(h);
    return d && d->parent ? d->parent->hwnd : NULL;
}

int32_t WINAPI GetWindowTextA(HWND h, LPSTR out, int32_t cch)
{
    if (out && cch > 0)
        out[0] = 0;
    return dw_get(h) ? (int32_t)SendMessageA(h, WM_GETTEXT, (WPARAM)cch, (LPARAM)(uintptr_t)out) : 0;
}

int32_t WINAPI GetWindowTextLengthA(HWND h)
{
    return dw_get(h) ? (int32_t)SendMessageA(h, WM_GETTEXTLENGTH, 0, 0) : 0;
}

BOOL WINAPI SetForegroundWindow(HWND h)
{
    if (dw_get(h)) {
        w32l_dw_raise(h);
        return TRUE;
    }
    return h == w32l_the_window();
}

LRESULT WINAPI CallWindowProcA(WNDPROC proc, HWND h, UINT m, WPARAM w, LPARAM l)
{
    return proc ? proc(h, m, w, l) : 0;
}

/* ---- focus, the point, raising --------------------------------------------------------------------------------------------------------- */

static DWin *dw_with_host_focus(void)
{
    uint32_t i;

    for (i = 0; i < W32L_DW_MAX; i++)
        if (w32l_win[i] && w32l_win[i]->kind == K_DIALOG && w32l_win[i]->hasfocus)
            return w32l_win[i];
    return NULL;
}

int32_t w32l_dlg_has_host_focus(void) { return dw_with_host_focus() != NULL; }

HWND w32l_dw_focus(void)
{
    DWin *d = dw_with_host_focus();
    return d ? (d->focus ? d->focus->hwnd : d->hwnd) : NULL;
}

HWND w32l_dw_set_focus(HWND h)
{
    DWin *d = dw_get(h), *old;

    if (!d)
        return NULL;
    if (d->kind == K_DIALOG) {
        w32l_dw_raise(h);
        return NULL;
    }
    old = dw_top(d)->focus;
    dw_set_focus(d);
    return old ? old->hwnd : NULL;
}

void w32l_dw_raise(HWND h)
{
    DWin *d = dw_get(h);
    if (d)
        w32l_dlghost_raise(dw_top(d)->host);
}

static DWin *dw_hit(DWin *dlg, int32_t x, int32_t y)
{
    int32_t i;

    for (i = dlg->nkids - 1; i >= 0; i--) {
        DWin *c = dlg->kids[i];
        if (!c->visible || x < c->x || y < c->y || x >= c->x + c->w || y >= c->y + c->h)
            continue;
        if (c->kind == K_BUTTON && c->btype == BS_GROUPBOX)
            continue;
        if (c->kind == K_STATIC && !(c->style & SS_NOTIFY))
            continue;
        return c;
    }
    return NULL;
}

HWND w32l_dw_at_point(POINT pt)
{
    uint32_t i;
    DWin    *best = NULL;

    for (i = 0; i < W32L_DW_MAX; i++) {
        DWin *d = w32l_win[i];
        if (d && d->kind == K_DIALOG && d->visible && pt.x >= d->x && pt.y >= d->y && pt.x < d->x + d->w && pt.y < d->y + d->h)
            if (!best || d->serial > best->serial)
                best = d;
    }
    if (best) {
        DWin *c = dw_hit(best, pt.x - best->x, pt.y - best->y);
        return (c ? c : best)->hwnd;
    }
    return NULL;
}

/* ---- input from the host --------------------------------------------------------------------------------------------------------------- */

static void dw_post(DWin *to, UINT m, WPARAM w, LPARAM l)
{
    w32l_plat_debug("dialog post %p (id %d) msg 0x%x wp=0x%x lp=0x%x", (void *)to->hwnd, to->id, (unsigned)m, (unsigned)w, (unsigned)l);
    w32l_queue_post(to->hwnd, m, w, l);
}

void w32l_dw_host_mouse(DWin *dlg, int32_t kind, int32_t button, int32_t x, int32_t y, int32_t wheel)
{
    static const UINT downs[3] = { WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN };
    static int32_t    held;
    DWin   *t = dlg->mousecap;
    UINT    msg;
    int32_t ax, ay;
    WPARAM  wp;

    if (dlg->dead)
        return;
    if (!t) {
        DWin *c = dlg->dropped;
        if (c && c->drop && x >= c->x && x < c->x + c->w && y >= c->y && y < c->y + COMBOH + c->dropw)
            t = c;
        else if (kind == W32L_DM_WHEEL && dlg->focus)
            t = dlg->focus;
        else
            t = dw_hit(dlg, x, y);
    }
    if (!t)
        t = dlg;
    switch (kind) {
    case W32L_DM_MOVE: msg = WM_MOUSEMOVE; break;
    case W32L_DM_WHEEL: msg = WM_MOUSEWHEEL; break;
    case W32L_DM_UP: msg = downs[button] + 1; break;
    case W32L_DM_DBL: msg = button == 0 ? WM_LBUTTONDBLCLK : downs[button] + 2; break;
    default: msg = downs[button]; break;
    }
    if (kind == W32L_DM_DOWN || kind == W32L_DM_DBL) {
        held |= 1 << button;
        if (dlg->dropped && dlg->dropped != t && dlg->dropped->drop)
            dw_post(dlg->dropped, CB_SHOWDROPDOWN, 0, 0);
        if (t != dlg && button == 0)
            dlg->mousecap = t;
        /* The click gives focus now, not when the message is dispatched: the
         * keys the user types next are addressed to it. */
        if (t != dlg && t->enabled && button == 0 &&
            (t->kind == K_EDIT || t->kind == K_LIST || t->kind == K_COMBO || (t->kind == K_BUTTON && t->btype != BS_GROUPBOX)))
            dw_set_focus(t);
    }
    if (kind == W32L_DM_UP)
        held &= ~(1 << button);
    wp = ((held & 1) ? MK_LBUTTON : 0) | ((held & 2) ? MK_RBUTTON : 0) | ((held & 4) ? MK_MBUTTON : 0);
    if (kind == W32L_DM_WHEEL)
        wp |= (uint32_t)(wheel & 0xFFFF) << 16;
    dw_abs(t, &ax, &ay);
    dw_post(t, msg, wp, MAKELONG(x - ax, y - ay));
    if (kind == W32L_DM_UP && button == 0)
        dlg->mousecap = NULL;
}

static void dw_default_click(DWin *dlg)
{
    DWin *b = dlg->defid ? dw_find_id(dlg, dlg->defid) : NULL;

    if (b && b->kind == K_BUTTON && b->enabled && b->visible)
        dw_post(b, BM_CLICK, 0, 0);
    else
        dw_post(dlg, WM_COMMAND, (WPARAM)(dlg->defid ? dlg->defid : IDOK), 0);
}

void w32l_dw_host_key(DWin *dlg, int32_t down, uint32_t vk)
{
    DWin *f = dlg->focus;

    if (dlg->dead)
        return;
    if (vk == 0x10 || vk == 0x11) {
        int32_t bit = vk == 0x10 ? 1 : 2;
        w32l_mods = down ? (w32l_mods | bit) : (w32l_mods & ~bit);
        return;
    }
    if (!down) {
        dw_post(f ? f : dlg, WM_KEYUP, vk, 0xC0000001);
        return;
    }
    if (vk == 0x09 && !(f && f->kind == K_EDIT && ed_multi(f) && (w32l_mods & 2))) {
        dw_send(dlg, WM_NEXTDLGCTL, (w32l_mods & 1) ? 1 : 0, 0);
        return;
    }
    if (vk == 0x1B) {
        if (dlg->dropped && dlg->dropped->drop)
            dw_post(dlg->dropped, WM_KEYDOWN, vk, 1);
        else
            dw_post(dlg, WM_COMMAND, IDCANCEL, 0);
        return;
    }
    if (vk == 0x0D) {
        if (f && f->kind == K_EDIT && ed_multi(f))
            dw_post(f, WM_CHAR, 13, 1);
        else if (f && f->kind == K_COMBO && f->drop)
            dw_post(f, WM_KEYDOWN, vk, 1);
        else if (f && f->kind == K_BUTTON && f->btype != BS_GROUPBOX && f->enabled)
            dw_post(f, BM_CLICK, 0, 0);
        else
            dw_default_click(dlg);
        return;
    }
    if (vk == 0x08) {
        if (f)
            dw_post(f, WM_CHAR, 8, 1);
        return;
    }
    if ((w32l_mods & 2) && vk >= 'A' && vk <= 'Z') {
        if (f)
            dw_post(f, WM_CHAR, vk - 'A' + 1, 1);
        return;
    }
    dw_post(f ? f : dlg, WM_KEYDOWN, vk, 1);
}

void w32l_dw_host_char(DWin *dlg, uint32_t ch)
{
    if (!dlg->dead && dlg->focus && !(w32l_mods & 2))
        dw_post(dlg->focus, WM_CHAR, ch, 1);
}

void w32l_dw_host_close(DWin *dlg)
{
    if (!dlg->dead)
        dw_post(dlg, WM_CLOSE, 0, 0);
}

void w32l_dw_host_focus(DWin *dlg, int32_t gained)
{
    dlg->hasfocus = gained;
    if (!gained)
        w32l_mods = 0;
}

void w32l_dw_host_moved(DWin *dlg, int32_t x, int32_t y)
{
    if (dlg->dead || (dlg->x == x && dlg->y == y))
        return;
    dlg->x = x;
    dlg->y = y;
    dw_post(dlg, WM_MOVE, 0, MAKELONG(x, y));
}

BOOL WINAPI IsDialogMessageA(HWND h, LPMSG msg)
{
    DWin *dlg = dw_get(h), *t;

    if (!dlg || dlg->kind != K_DIALOG || !msg)
        return FALSE;
    t = dw_get(msg->hwnd);
    if (!t || dw_top(t) != dlg)
        return FALSE;
    if (msg->message == WM_KEYDOWN && (msg->wParam == 0x09 || msg->wParam == 0x1B || msg->wParam == 0x0D)) {
        DWin *f = dlg->focus;
        if (msg->wParam == 0x09) {
            dw_send(dlg, WM_NEXTDLGCTL, (w32l_mods & 1) ? 1 : 0, 0);
        } else if (msg->wParam == 0x1B) {
            dw_send(dlg, WM_COMMAND, IDCANCEL, 0);
        } else if (f && f->kind == K_EDIT && ed_multi(f)) {
            return FALSE;
        } else if (f && f->kind == K_BUTTON && f->btype != BS_GROUPBOX) {
            dw_send(f, BM_CLICK, 0, 0);
        } else
            dw_default_click(dlg);
        return TRUE;
    }
    return FALSE;
}

/* ---- the pump: repaint, dump ------------------------------------------------------------------------------------------------------------------ */

static void dump_name(DWin *d, char *out, size_t cap, const char *prefix)
{
    char     nm[64];
    size_t   i, n = 0;
    const char *src = d->resname[0] ? d->resname : d->text;

    for (i = 0; src[i] && n + 1 < sizeof nm; i++)
        nm[n++] = isalnum((uint8_t)src[i]) ? src[i] : '_';
    nm[n] = 0;
    snprintf(out, cap, "%sdlg%02d-%s.ppm", prefix, d->dumpseq, nm);
}

static int32_t w32l_dump_counter;

static void dump_dialog(DWin *d, const char *prefix)
{
    char path[1024];

    if (!d->dumpseq)
        d->dumpseq = ++w32l_dump_counter;
    paint_dialog(d);
    dump_name(d, path, sizeof path, prefix);
    if (w32l_dlghost_dump(d->host, path))
        w32l_plat_log("dialog dump %s", path);
    d->needdump = 0;
}

static const char *dump_dir(char *out, size_t cap)
{
    const char *e = getenv("W32L_DIALOGDUMP");

    if (!e || !*e)
        return NULL;
    snprintf(out, cap, "%s/", e);
    return out;
}

void w32l_dlg_frame(void)
{
    static uint32_t frame;
    static int32_t  every = -1;
    uint32_t        i;
    char            dir[512];

    while (w32l_nzombies && !w32l_send_depth)
        dw_free(w32l_zombies[--w32l_nzombies]);
    if (!w32l_win_dialogs)
        return;
    if (every < 0) {
        const char *e = getenv("W32L_DIALOGDUMP_EVERY");
        every = e ? atoi(e) : 60;
    }
    frame++;
    for (i = 0; i < W32L_DW_MAX; i++) {
        DWin *d = w32l_win[i];
        if (!d || d->kind != K_DIALOG || !d->visible)
            continue;
        if (d->dirty) {
            paint_dialog(d);
            w32l_dlghost_present(d->host);
            d->dirty = 0;
        }
        if (d->needdump && dump_dir(dir, sizeof dir) && (!d->lastdump || frame - d->lastdump >= (uint32_t)every)) {
            dump_dialog(d, dir);
            d->lastdump = frame;
        }
    }
}

void w32l_dlg_shutdown(void)
{
    uint32_t i;
    char     dir[512];

    if (!dump_dir(dir, sizeof dir))
        return;
    for (i = 0; i < W32L_DW_MAX; i++) {
        DWin *d = w32l_win[i];
        if (d && d->kind == K_DIALOG && d->visible && d->needdump)
            dump_dialog(d, dir);
    }
}

/* ---- replay verbs ------------------------------------------------------------------------------------------------------------------------------ */

static DWin *newest_dialog(void)
{
    uint32_t i;
    DWin    *best = NULL;

    for (i = 0; i < W32L_DW_MAX; i++) {
        DWin *d = w32l_win[i];
        if (d && d->kind == K_DIALOG && d->visible && (!best || d->serial > best->serial))
            best = d;
    }
    return best;
}

static DWin *find_ctl_anywhere(int32_t id)
{
    uint32_t i;
    DWin    *best = NULL, *c;

    for (i = 0; i < W32L_DW_MAX; i++) {
        DWin *d = w32l_win[i];
        if (d && d->kind == K_DIALOG && d->visible && (c = dw_find_id(d, id)) && (!best || d->serial > dw_top(best)->serial))
            best = c;
    }
    return best;
}

static const struct { const char *name; uint32_t vk; } w32l_dlg_keys[] = {
    { "ENTER", 0x0D }, { "RETURN", 0x0D }, { "TAB", 0x09 }, { "ESCAPE", 0x1B }, { "ESC", 0x1B }, { "BACK", 0x08 },
    { "DELETE", 0x2E }, { "LEFT", 0x25 }, { "UP", 0x26 }, { "RIGHT", 0x27 }, { "DOWN", 0x28 }, { "HOME", 0x24 },
    { "END", 0x23 }, { "PAGEUP", 0x21 }, { "PAGEDOWN", 0x22 }, { "SPACE", 0x20 }, { "F4", 0x73 },
};

/* dlgclick ID | dlgat X Y | dlgtype ID TEXT | dlgkey NAME | dlgsel ID INDEX | dlgclose | dlgdump PREFIX */
int32_t w32l_dlg_replay(const char *verb, int32_t a, int32_t b, const char *text)
{
    DWin *c, *dlg;

    if (strncmp(verb, "dlg", 3))
        return 0;
    w32l_plat_debug("dialog replay %s %d %d %s", verb, a, b, text ? text : "");
    if (!strcmp(verb, "dlgdump")) {
        uint32_t i;
        for (i = 0; i < W32L_DW_MAX; i++)
            if (w32l_win[i] && w32l_win[i]->kind == K_DIALOG && w32l_win[i]->visible)
                dump_dialog(w32l_win[i], text);
        return 1;
    }
    if (!strcmp(verb, "dlgat")) {
        dlg = newest_dialog();
        if (dlg) {
            w32l_dw_host_mouse(dlg, W32L_DM_MOVE, 0, a, b, 0);
            w32l_dw_host_mouse(dlg, W32L_DM_DOWN, 0, a, b, 0);
            w32l_dw_host_mouse(dlg, W32L_DM_UP, 0, a, b, 0);
        }
        return 1;
    }
    if (!strcmp(verb, "dlgclick") || !strcmp(verb, "dlgtype")) {
        int32_t ax, ay;
        c = find_ctl_anywhere(a);
        if (!c) {
            w32l_plat_log("replay: %s: no control %d", verb, a);
            return 1;
        }
        dlg = dw_top(c);
        dw_abs(c, &ax, &ay);
        ax += c->w / 2;
        ay += c->kind == K_COMBO ? COMBOH / 2 : c->h / 2;
        w32l_dw_host_mouse(dlg, W32L_DM_DOWN, 0, ax, ay, 0);
        w32l_dw_host_mouse(dlg, W32L_DM_UP, 0, ax, ay, 0);
        if (!strcmp(verb, "dlgtype"))
            for (; text && *text; text++)
                w32l_dw_host_char(dlg, *text == '_' ? ' ' : (uint8_t)*text);
        return 1;
    }
    if (!strcmp(verb, "dlgkey")) {
        size_t i;
        dlg = newest_dialog();
        for (i = 0; dlg && i < sizeof w32l_dlg_keys / sizeof w32l_dlg_keys[0]; i++)
            if (!strcasecmp(w32l_dlg_keys[i].name, text)) {
                w32l_dw_host_key(dlg, 1, w32l_dlg_keys[i].vk);
                w32l_dw_host_key(dlg, 0, w32l_dlg_keys[i].vk);
            }
        return 1;
    }
    if (!strcmp(verb, "dlgsel")) {
        c = find_ctl_anywhere(a);
        if (c && c->kind == K_LIST) {
            lb_proc(c, LB_SETCURSEL, (WPARAM)b, 0);
            if (c->style & LBS_NOTIFY) dw_post(dw_top(c), WM_COMMAND, (WPARAM)((LBN_SELCHANGE << 16) | (c->id & 0xFFFF)), (LPARAM)(uintptr_t)c->hwnd);
        } else if (c && c->kind == K_COMBO) {
            cb_pick(c, b, 1);
        }
        return 1;
    }
    if (!strcmp(verb, "dlgclose")) {
        dlg = newest_dialog();
        if (dlg)
            w32l_dw_host_close(dlg);
        return 1;
    }
    return 0;
}
