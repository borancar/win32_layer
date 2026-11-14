/* resource.cpp -- the resources of the program's own image: FindResource and
 * its family, string tables, and menus.
 *
 * The program's PE image is mapped by its loader, which tells the layer where
 * (w32l_pe_image); the resource directory is read straight from that memory.
 * A program with no image set (a build that carries no .rsrc) finds nothing,
 * and its dialogs and strings fail as they did before.
 *
 * Names are ASCII or MAKEINTRESOURCE ordinals; the resource's language is not
 * asked for: the first entry of a name answers.
 */
#include "platform.h"
#include "dialog.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

const uint8_t *w32l_pe_image;

#pragma pack(push, 1)
struct W32L_ResDir { uint32_t chars, stamp; uint16_t major, minor, nNamed, nId; };
struct W32L_ResEntry { uint32_t name, off; };
struct W32L_ResData { uint32_t rva, size, codepage, reserved; };
#pragma pack(pop)

static const uint8_t *w32l_rsrc_root(uint32_t *size)
{
    const uint8_t *img = w32l_pe_image;
    uint32_t       pe, rva, sz;

    if (!img || img[0] != 'M' || img[1] != 'Z')
        return NULL;
    memcpy(&pe, img + 0x3c, 4);
    if (memcmp(img + pe, "PE\0\0", 4))
        return NULL;
    memcpy(&rva, img + pe + 24 + 96 + 16, 4);   /* data directory 2: resources */
    memcpy(&sz, img + pe + 24 + 96 + 20, 4);
    if (!rva)
        return NULL;
    *size = sz;
    return img + rva;
}

/* The entry of `dir` called `name` (an ordinal when below 0x10000), or NULL. */
static const W32L_ResEntry *w32l_res_find(const uint8_t *root, const W32L_ResDir *dir, const char *name)
{
    const W32L_ResEntry *e = (const W32L_ResEntry *)(dir + 1);
    uint32_t             n = (uint32_t)dir->nNamed + dir->nId, i;

    for (i = 0; i < n; i++) {
        if ((uintptr_t)name < 0x10000) {
            if (!(e[i].name & 0x80000000u) && e[i].name == (uint32_t)(uintptr_t)name)
                return &e[i];
        } else if (e[i].name & 0x80000000u) {
            const uint16_t *w = (const uint16_t *)(root + (e[i].name & 0x7FFFFFFFu));
            uint32_t        len = w[0], k;
            if (len != strlen(name))
                continue;
            for (k = 0; k < len && (w[1 + k] < 0x80) && tolower(w[1 + k]) == tolower((uint8_t)name[k]); k++)
                ;
            if (k == len)
                return &e[i];
        }
    }
    return NULL;
}

static const W32L_ResDir *w32l_res_sub(const uint8_t *root, const W32L_ResEntry *e)
{
    return e && (e->off & 0x80000000u) ? (const W32L_ResDir *)(root + (e->off & 0x7FFFFFFFu)) : NULL;
}

static int32_t w32l_res_mine(HMODULE mod)
{
    return w32l_pe_image && (!mod || (const uint8_t *)mod == w32l_pe_image);
}

HRSRC WINAPI FindResourceExA(HMODULE mod, LPCSTR type, LPCSTR name, WORD lang)
{
    uint32_t              size;
    const uint8_t        *root;
    const W32L_ResDir    *d;
    const W32L_ResEntry  *e;

    (void)lang;
    if (!w32l_res_mine(mod) || !(root = w32l_rsrc_root(&size)))
        return NULL;
    if (!(d = w32l_res_sub(root, w32l_res_find(root, (const W32L_ResDir *)root, type))))
        return NULL;
    if (!(d = w32l_res_sub(root, w32l_res_find(root, d, name))))
        return NULL;
    e = (const W32L_ResEntry *)(d + 1);
    if (!d->nNamed && !d->nId)
        return NULL;
    if (e->off & 0x80000000u)
        return NULL;
    return (HRSRC)(root + e->off);
}

HRSRC WINAPI FindResourceA(HMODULE mod, LPCSTR name, LPCSTR type)
{
    return FindResourceExA(mod, type, name, 0);
}

/* Wide names: a string is narrowed to ASCII, an ordinal passes through. */
static const char *w32l_narrow(LPCWSTR w, char *out, size_t cap)
{
    size_t i;

    if ((uintptr_t)w < 0x10000)
        return (const char *)w;
    for (i = 0; w[i] && i + 1 < cap; i++)
        out[i] = w[i] < 0x100 ? (char)w[i] : '?';
    out[i] = 0;
    return out;
}

HRSRC WINAPI FindResourceW(HMODULE mod, LPCWSTR name, LPCWSTR type)
{
    char a[256], b[64];

    return FindResourceExA(mod, w32l_narrow(type, b, sizeof b), w32l_narrow(name, a, sizeof a), 0);
}

HGLOBAL WINAPI LoadResource(HMODULE mod, HRSRC res)
{
    const W32L_ResData *d = (const W32L_ResData *)res;

    if (!res || !w32l_res_mine(mod))
        return NULL;
    return (HGLOBAL)(w32l_pe_image + d->rva);
}

LPVOID WINAPI LockResource(HGLOBAL res) { return res; }

DWORD WINAPI SizeofResource(HMODULE mod, HRSRC res)
{
    return res && w32l_res_mine(mod) ? ((const W32L_ResData *)res)->size : 0;
}

/* ---- strings -------------------------------------------------------------------- */

/* The string table block of `id`: 16 length-prefixed UTF-16 strings. */
static const uint16_t *w32l_string_of(HINSTANCE inst, UINT id, uint32_t *len)
{
    HRSRC           r = FindResourceExA(inst, (LPCSTR)RT_STRING, (LPCSTR)(uintptr_t)((id >> 4) + 1), 0);
    const uint16_t *p;
    uint32_t        i;

    if (!r)
        return NULL;
    p = (const uint16_t *)LoadResource(inst, r);
    for (i = 0; i < (id & 15); i++)
        p += 1 + p[0];
    *len = p[0];
    return p + 1;
}

int32_t WINAPI LoadStringA(HINSTANCE inst, UINT id, LPSTR out, int32_t cch)
{
    uint32_t        len;
    const uint16_t *s = w32l_string_of(inst, id, &len);
    uint32_t        i;

    if (!s || !out || cch <= 0)
        return 0;
    for (i = 0; i < len && (int32_t)i < cch - 1; i++)
        out[i] = s[i] < 0x100 ? (char)s[i] : '?';
    out[i] = 0;
    return (int32_t)i;
}

int32_t WINAPI LoadStringW(HINSTANCE inst, UINT id, LPWSTR out, int32_t cch)
{
    uint32_t        len;
    const uint16_t *s = w32l_string_of(inst, id, &len);
    uint32_t        i;

    if (!s || !out || cch <= 0)
        return 0;
    for (i = 0; i < len && (int32_t)i < cch - 1; i++)
        out[i] = s[i];
    out[i] = 0;
    return (int32_t)i;
}

/* ---- menus ------------------------------------------------------------------------ */

#define W32L_MENU_MAGIC 0x4D454E55u
#define MF_GRAYED 0x0001
#define MF_DISABLED 0x0002
#define MF_CHECKED 0x0008
#define MF_POPUP 0x0010
#define MF_END 0x0080
#define MFT_SEPARATOR 0x0800
#define MIIM_STATE 0x0001
#define MIIM_ID 0x0002
#define MIIM_SUBMENU 0x0004
#define MIIM_TYPE 0x0010
#define MIIM_STRING 0x0040

struct W32L_MenuItem { char *text; uint32_t id, flags; struct W32L_Menu *sub; };
struct W32L_Menu { uint32_t magic; W32L_MenuItem *items; int32_t count; };

static void w32l_menu_free(W32L_Menu *m)
{
    int32_t i;

    for (i = 0; i < m->count; i++) {
        free(m->items[i].text);
        if (m->items[i].sub)
            w32l_menu_free(m->items[i].sub);
    }
    free(m->items);
    m->magic = 0;
    free(m);
}

/* One menu level, from `p`; leaves *pp after its last item. */
static W32L_Menu *w32l_menu_parse(const uint16_t **pp)
{
    W32L_Menu     *m = (W32L_Menu *)calloc(1, sizeof *m);
    const uint16_t *p = *pp;
    uint16_t        flags;

    m->magic = W32L_MENU_MAGIC;
    do {
        W32L_MenuItem *it;
        char           text[256];
        uint32_t       n = 0;

        flags = *p++;
        m->items = (W32L_MenuItem *)realloc(m->items, (size_t)(m->count + 1) * sizeof *m->items);
        it = &m->items[m->count++];
        memset(it, 0, sizeof *it);
        it->flags = flags;
        if (!(flags & MF_POPUP))
            it->id = *p++;
        while (*p) {
            if (n + 1 < sizeof text)
                text[n++] = *p < 0x100 ? (char)*p : '?';
            p++;
        }
        p++;
        text[n] = 0;
        it->text = strdup(text);
        if (flags & MF_POPUP)
            it->sub = w32l_menu_parse(&p);
    } while (!(flags & MF_END));
    *pp = p;
    return m;
}

HMENU WINAPI LoadMenuA(HINSTANCE inst, LPCSTR name)
{
    HRSRC           r = FindResourceExA(inst, (LPCSTR)RT_MENU, name, 0);
    const uint16_t *p;

    if (!r)
        return NULL;
    p = (const uint16_t *)LoadResource(inst, r);
    p += 2 + p[1] / 2;          /* the header: version and its size */
    return (HMENU)w32l_menu_parse(&p);
}

static W32L_Menu *w32l_menu_of(HMENU h)
{
    W32L_Menu *m = (W32L_Menu *)h;
    return m && m->magic == W32L_MENU_MAGIC ? m : NULL;
}

BOOL WINAPI DestroyMenu(HMENU menu)
{
    W32L_Menu *m = w32l_menu_of(menu);

    if (!m)
        return FALSE;
    w32l_menu_free(m);
    return TRUE;
}

int32_t WINAPI GetMenuItemCount(HMENU menu)
{
    W32L_Menu *m = w32l_menu_of(menu);
    return m ? m->count : -1;
}

HMENU WINAPI GetSubMenu(HMENU menu, int32_t pos)
{
    W32L_Menu *m = w32l_menu_of(menu);
    return m && pos >= 0 && pos < m->count ? (HMENU)m->items[pos].sub : NULL;
}

BOOL WINAPI GetMenuItemInfoA(HMENU menu, UINT item, BOOL byPosition, LPMENUITEMINFOA info)
{
    W32L_Menu     *m = w32l_menu_of(menu);
    W32L_MenuItem *it = NULL;
    int32_t        i;

    if (!m || !info)
        return FALSE;
    if (byPosition)
        it = item < (UINT)m->count ? &m->items[item] : NULL;
    else
        for (i = 0; i < m->count && !it; i++)
            if (!(m->items[i].flags & MF_POPUP) && m->items[i].id == item)
                it = &m->items[i];
    if (!it)
        return FALSE;
    if (info->fMask & MIIM_ID)
        info->wID = it->id;
    if (info->fMask & MIIM_SUBMENU)
        info->hSubMenu = (HMENU)it->sub;
    if (info->fMask & MIIM_STATE)
        info->fState = (it->flags & (MF_GRAYED | MF_DISABLED | MF_CHECKED));
    if (info->fMask & MIIM_TYPE)
        info->fType = it->text[0] ? 0 : MFT_SEPARATOR;
    if ((info->fMask & (MIIM_STRING | MIIM_TYPE)) && info->dwTypeData && info->cch) {
        strncpy(info->dwTypeData, it->text, info->cch - 1);
        info->dwTypeData[info->cch - 1] = 0;
        info->cch = (UINT)strlen(info->dwTypeData);
    }
    return TRUE;
}

/* A popup menu has no window here: it answers "nothing chosen". */
BOOL WINAPI TrackPopupMenu(HMENU menu, UINT flags, int32_t x, int32_t y, int32_t reserved,
                           HWND hwnd, const RECT *rc)
{
    (void)menu; (void)flags; (void)x; (void)y; (void)reserved; (void)hwnd; (void)rc;
    return FALSE;
}
