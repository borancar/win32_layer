/* dialogs.cpp -- a standalone test of the layer's dialogs, headless:
 *
 *     SDL_VIDEODRIVER=dummy build/dialogs_test [PE-IMAGE]
 *
 * It builds a dialog from a template in memory (CreateDialogIndirectParamA),
 * drives its controls with messages, with the replay verbs of dialog.cpp
 * (click, type, key) and through the program's own message loop, checks what
 * the dialog procedure was told, and writes PPM dumps to tests/out/ (or
 * $W32L_TEST_OUT). With a PE image as argument it also maps the image,
 * opens every RT_DIALOG it names on the command line after it (default
 * the first few) and dumps those. Exit status 0 when every check held.
 */
#include "../src/dialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- a template in memory ------------------------------------------------------ */

static uint8_t tpl[4096];
static size_t  tpl_n;

static void put(const void *p, size_t n) { memcpy(tpl + tpl_n, p, n); tpl_n += n; }
static void put16(uint16_t v) { put(&v, 2); }
static void put32(uint32_t v) { put(&v, 4); }
static void putw(const char *s) { do put16((uint8_t)*s); while (*s++); }
static void align4(void) { while (tpl_n & 3) tpl[tpl_n++] = 0; }

static void item(uint32_t style, int x, int y, int cx, int cy, int id, uint16_t cls, const char *text)
{
    align4();
    put32(style | WS_CHILD | WS_VISIBLE);
    put32(0);
    put16((uint16_t)x); put16((uint16_t)y); put16((uint16_t)cx); put16((uint16_t)cy);
    put16((uint16_t)id);
    put16(0xFFFF); put16(cls);
    putw(text);
    put16(0);
}

#define BTN 0x80
#define EDT 0x81
#define STA 0x82
#define LST 0x83
#define SCR 0x84
#define CMB 0x85

static void build_template(void)
{
    tpl_n = 0;
    put32(WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE);
    put32(0);
    put16(12);                       /* items */
    put16(10); put16(10); put16(200); put16(120);
    put16(0); put16(0); putw("Test dialog");
    item(0, 6, 6, 40, 10, 10, STA, "&Name:");
    item(WS_TABSTOP | WS_BORDER, 50, 4, 80, 12, 11, EDT, "abc");
    item(WS_TABSTOP | 1, 6, 100, 40, 14, IDOK, BTN, "OK");
    item(WS_TABSTOP, 52, 100, 40, 14, IDCANCEL, BTN, "Cancel");
    item(WS_TABSTOP | 3, 6, 22, 60, 10, 12, BTN, "Check");
    item(WS_GROUP | WS_TABSTOP | 9, 6, 36, 40, 10, 13, BTN, "One");
    item(9, 6, 48, 40, 10, 14, BTN, "Two");
    item(WS_TABSTOP | WS_BORDER | WS_VSCROLL | 1, 100, 22, 60, 50, 15, LST, "");
    item(WS_TABSTOP | WS_VSCROLL | 3, 100, 76, 60, 60, 16, CMB, "");
    item(0, 6, 70, 80, 8, 17, SCR, "");
    item(WS_TABSTOP | 0x4 | WS_BORDER | WS_VSCROLL, 6, 84, 80, 14, 18, EDT, "");   /* multiline */
    item(WS_TABSTOP | 0x2000, 140, 4, 40, 12, 19, EDT, "5");   /* numbers only */
}

/* ---- the dialog procedure ------------------------------------------------------------ */

static int     inits, oks, cancels, edit_changes, lb_sel, cb_sel, closes, destroys, scrolls;
static LPARAM  init_param;
static HWND    last_init_hwnd;

static INT_PTR CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_INITDIALOG:
        inits++;
        init_param = l;
        last_init_hwnd = h;
        SetWindowLongA(h, GWL_USERDATA, 0x5151);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK: oks++; return TRUE;
        case IDCANCEL: cancels++; return TRUE;
        case 11: if (HIWORD(w) == EN_CHANGE) edit_changes++; return TRUE;
        case 15: if (HIWORD(w) == LBN_SELCHANGE) lb_sel++; return TRUE;
        case 16: if (HIWORD(w) == CBN_SELCHANGE) cb_sel++; return TRUE;
        }
        return FALSE;
    case WM_VSCROLL: case WM_HSCROLL: scrolls++; return TRUE;
    case WM_CLOSE: closes++; return FALSE;
    case WM_DESTROY: destroys++; return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK modal_proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    (void)w;
    if (m == WM_INITDIALOG) {
        EndDialog(h, (INT_PTR)l);
        return TRUE;
    }
    return FALSE;
}

/* The program's message loop, for as long as there is something queued. */
static void pump(void)
{
    MSG m;
    int i;

    for (i = 0; i < 4; i++)
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
}

static const char *outdir;

static void dump(const char *prefix)
{
    char p[512];

    snprintf(p, sizeof p, "%s/%s", outdir, prefix);
    w32l_dlg_frame();
    w32l_dlg_replay("dlgdump", 0, 0, p);
}

static void test_template(void)
{
    char    buf[128];
    HWND    dlg, ed, lb, cb;
    BOOL    ok;
    RECT    rc;
    int32_t i;

    build_template();
    dlg = CreateDialogIndirectParamA(NULL, (LPCDLGTEMPLATE)tpl, NULL, proc, 0x1234);
    CHECK(dlg != NULL);
    CHECK(inits == 1 && init_param == 0x1234 && last_init_hwnd == dlg);
    CHECK(GetWindowLongA(dlg, GWL_USERDATA) == 0x5151);
    CHECK(IsWindowVisible(dlg));
    ed = GetDlgItem(dlg, 11);
    lb = GetDlgItem(dlg, 15);
    cb = GetDlgItem(dlg, 16);
    CHECK(ed && lb && cb && !GetDlgItem(dlg, 999));
    CHECK(GetDlgCtrlID(ed) == 11);
    CHECK(GetParent(ed) == dlg);

    /* text */
    GetDlgItemTextA(dlg, 11, buf, sizeof buf);
    CHECK(!strcmp(buf, "abc"));
    SetDlgItemTextA(dlg, 11, "hello");
    GetDlgItemTextA(dlg, 11, buf, sizeof buf);
    CHECK(!strcmp(buf, "hello"));
    CHECK(edit_changes == 1);                         /* WM_SETTEXT notifies EN_CHANGE */
    SetDlgItemInt(dlg, 19, 42, FALSE);
    CHECK(GetDlgItemInt(dlg, 19, &ok, FALSE) == 42 && ok);
    SetDlgItemTextA(dlg, 19, "x");
    GetDlgItemInt(dlg, 19, &ok, FALSE);
    CHECK(!ok);
    CHECK(SendMessageA(ed, WM_GETTEXTLENGTH, 0, 0) == 5);
    SendMessageA(ed, 0xB1 /* EM_SETSEL */, 1, 3);
    SendMessageA(ed, 0xC2 /* EM_REPLACESEL */, 0, (LPARAM)"EY");
    GetDlgItemTextA(dlg, 11, buf, sizeof buf);
    CHECK(!strcmp(buf, "hEYlo"));
    SetWindowTextA(dlg, "Renamed");
    GetWindowTextA(dlg, buf, sizeof buf);
    CHECK(!strcmp(buf, "Renamed"));

    /* buttons */
    CheckDlgButton(dlg, 12, BST_CHECKED);
    CHECK(IsDlgButtonChecked(dlg, 12) == BST_CHECKED);
    CheckRadioButton(dlg, 13, 14, 14);
    CHECK(IsDlgButtonChecked(dlg, 13) == 0 && IsDlgButtonChecked(dlg, 14) == 1);

    /* list box and combo box */
    CHECK(SendDlgItemMessageA(dlg, 15, 0x180 /* LB_ADDSTRING */, 0, (LPARAM)"alpha") == 0);
    SendDlgItemMessageA(dlg, 15, 0x180, 0, (LPARAM)"beta");
    SendDlgItemMessageA(dlg, 15, 0x180, 0, (LPARAM)"gamma");
    CHECK(SendMessageA(lb, 0x18B /* LB_GETCOUNT */, 0, 0) == 3);
    SendMessageA(lb, 0x199 + 1 /* LB_SETITEMDATA */, 1, 77);
    CHECK(SendMessageA(lb, 0x199 /* LB_GETITEMDATA */, 1, 0) == 77);
    SendMessageA(lb, 0x186 /* LB_SETCURSEL */, 2, 0);
    CHECK(SendMessageA(lb, 0x188 /* LB_GETCURSEL */, 0, 0) == 2);
    CHECK(SendMessageA(lb, 0x189 /* LB_GETTEXT */, 2, (LPARAM)buf) == 5 && !strcmp(buf, "gamma"));
    CHECK(SendMessageA(lb, 0x18F /* LB_FINDSTRING */, (WPARAM)-1, (LPARAM)"be") == 1);
    for (i = 0; i < 20; i++) {
        snprintf(buf, sizeof buf, "row %d", i);
        SendMessageA(lb, 0x180, 0, (LPARAM)buf);
    }
    SendMessageA(lb, 0x182 /* LB_DELETESTRING */, 0, 0);
    CHECK(SendMessageA(lb, 0x18B, 0, 0) == 22);
    SendMessageA(lb, 0x184 /* LB_RESETCONTENT */, 0, 0);
    CHECK(SendMessageA(lb, 0x18B, 0, 0) == 0);
    for (i = 0; i < 6; i++) {
        snprintf(buf, sizeof buf, "item %d", i);
        SendMessageA(lb, 0x180, 0, (LPARAM)buf);
        SendMessageA(cb, 0x143 /* CB_ADDSTRING */, 0, (LPARAM)buf);
    }
    SendMessageA(cb, 0x14E /* CB_SETCURSEL */, 3, 0);
    CHECK(SendMessageA(cb, 0x147 /* CB_GETCURSEL */, 0, 0) == 3);
    GetWindowTextA(cb, buf, sizeof buf);
    CHECK(!strcmp(buf, "item 3"));

    /* the mouse, through the host's input path and the program's own loop */
    w32l_dlg_replay("dlgclick", 12, 0, NULL);        /* the auto check box */
    pump();
    CHECK(IsDlgButtonChecked(dlg, 12) == BST_UNCHECKED);
    w32l_dlg_replay("dlgclick", 13, 0, NULL);        /* an auto radio button */
    pump();
    CHECK(IsDlgButtonChecked(dlg, 13) == 1 && IsDlgButtonChecked(dlg, 14) == 0);
    w32l_dlg_replay("dlgclick", IDOK, 0, NULL);
    pump();
    CHECK(oks == 1);
    w32l_dlg_replay("dlgsel", 15, 4, NULL);
    pump();
    CHECK(lb_sel == 1 && SendMessageA(lb, 0x188, 0, 0) == 4);
    w32l_dlg_replay("dlgclick", 15, 0, NULL);        /* the middle row */
    pump();
    CHECK(lb_sel == 2);

    /* a combo box dropped, picked from with the mouse */
    w32l_dlg_replay("dlgclick", 16, 0, NULL);
    pump();
    CHECK(SendMessageA(cb, 0x157 /* CB_GETDROPPEDSTATE */, 0, 0) == 1);
    dump("combo-open");
    w32l_dlg_replay("dlgkey", 0, 0, "ESC");
    pump();
    CHECK(SendMessageA(cb, 0x157, 0, 0) == 0);

    /* typing: a click gives the edit the focus, keys go to it */
    w32l_dlg_replay("dlgtype", 11, 0, "_ab");
    pump();
    GetDlgItemTextA(dlg, 11, buf, sizeof buf);
    CHECK(!strcmp(buf, "hEYlo ab"));
    CHECK(GetFocus() == ed);
    w32l_dlg_replay("dlgkey", 0, 0, "BACK");
    w32l_dlg_replay("dlgkey", 0, 0, "LEFT");
    w32l_dlg_replay("dlgkey", 0, 0, "DELETE");
    pump();
    GetDlgItemTextA(dlg, 11, buf, sizeof buf);
    CHECK(!strcmp(buf, "hEYlo "));
    w32l_dlg_replay("dlgtype", 19, 0, "7q9");        /* ES_NUMBER refuses letters */
    pump();
    GetDlgItemTextA(dlg, 19, buf, sizeof buf);
    CHECK(!strcmp(buf, "x79"));

    /* a multi-line edit takes Enter */
    w32l_dlg_replay("dlgtype", 18, 0, "ab");
    w32l_dlg_replay("dlgkey", 0, 0, "ENTER");
    w32l_dlg_replay("dlgtype", 18, 0, "cd");
    pump();
    CHECK(SendMessageA(GetDlgItem(dlg, 18), WM_GETTEXTLENGTH, 0, 0) >= 5);
    CHECK(SendMessageA(GetDlgItem(dlg, 18), 0xBA /* EM_GETLINECOUNT */, 0, 0) == 2);

    /* Tab moves the focus, Enter presses the default button, Esc cancels */
    w32l_dlg_replay("dlgkey", 0, 0, "TAB");
    pump();
    CHECK(GetFocus() != ed);
    w32l_dlg_replay("dlgclick", 17, 0, NULL);        /* the scroll bar */
    pump();
    CHECK(scrolls >= 1);
    w32l_dlg_replay("dlgkey", 0, 0, "ESC");
    pump();
    CHECK(cancels >= 1);

    /* geometry */
    GetWindowRect(dlg, &rc);
    CHECK(rc.right - rc.left > 200 * 9 / 4 && rc.bottom > rc.top);
    GetClientRect(dlg, &rc);
    CHECK(rc.right == 200 * 9 / 4 && rc.bottom == 120 * 12 / 8);
    MoveWindow(dlg, 30, 40, 500, 300, TRUE);
    GetClientRect(dlg, &rc);
    CHECK(rc.right == 500 - 6 && rc.bottom == 300 - 25);
    EnableWindow(GetDlgItem(dlg, IDCANCEL), FALSE);
    CHECK(!IsWindowEnabled(GetDlgItem(dlg, IDCANCEL)));
    dump("full");

    /* the host closes it: WM_CLOSE reaches the procedure, which declines, so
     * DefDlgProc sends IDCANCEL */
    cancels = 0;
    w32l_dlg_replay("dlgclose", 0, 0, NULL);
    pump();
    CHECK(closes == 1 && cancels == 1);

    CHECK(DestroyWindow(dlg));
    CHECK(destroys == 1);
    CHECK(!IsWindow(dlg) && GetDlgItem(dlg, 11) == NULL);
    CHECK(SendMessageA(ed, WM_GETTEXTLENGTH, 0, 0) == 0);

    /* modal: EndDialog from WM_INITDIALOG ends the loop with its value */
    build_template();
    CHECK(DialogBoxIndirectParamA(NULL, (LPCDLGTEMPLATE)tpl, NULL, modal_proc, 7) == 7);
}

/* ---- the program's resources --------------------------------------------------------- */

static int map_image(const char *path)
{
    int          fd = open(path, O_RDONLY);
    struct stat  st;
    uint8_t     *file, *img;
    uint32_t     pe, nsec, optsz, base, size, i;

    if (fd < 0 || fstat(fd, &st))
        return 0;
    file = (uint8_t *)mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    memcpy(&pe, file + 0x3C, 4);
    nsec = *(uint16_t *)(file + pe + 6);
    optsz = *(uint16_t *)(file + pe + 20);
    memcpy(&base, file + pe + 24 + 28, 4);
    memcpy(&size, file + pe + 24 + 56, 4);
    img = (uint8_t *)mmap((void *)(uintptr_t)base, size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (img == MAP_FAILED)
        return 0;
    memcpy(img, file, *(uint32_t *)(file + pe + 24 + 60));       /* the headers */
    for (i = 0; i < nsec; i++) {
        const uint8_t *s = file + pe + 24 + optsz + i * 40;
        uint32_t va, rsz, raw;
        memcpy(&va, s + 12, 4); memcpy(&rsz, s + 16, 4); memcpy(&raw, s + 20, 4);
        if (raw + rsz <= (uint32_t)st.st_size && va + rsz <= size)
            memcpy(img + va, file + raw, rsz);
    }
    w32l_pe_image = img;
    return 1;
}

static void test_resources(int argc, char **argv)
{
    int      i;
    HRSRC    r;
    char     buf[256];

    if (!map_image(argv[1])) {
        fprintf(stderr, "cannot map %s (the image's base must be free)\n", argv[1]);
        failures++;
        return;
    }
    r = FindResourceA(NULL, "D_EDIT_LIST", RT_DIALOG);
    CHECK(r != NULL);
    CHECK(SizeofResource(NULL, r) > 100);
    CHECK(FindResourceA(NULL, "NO_SUCH_DIALOG", RT_DIALOG) == NULL);
    for (i = 2; i < argc; i++) {
        HWND d = CreateDialogParamA(NULL, argv[i], NULL, proc, 0);
        CHECK(d != NULL);
        if (d) {
            ShowWindow(d, SW_SHOW);
            snprintf(buf, sizeof buf, "res-%s-", argv[i]);
            dump(buf);
            DestroyWindow(d);
        }
    }
    if (argc == 2) {
        HWND d = CreateDialogParamA(NULL, "D_EDIT_LIST", NULL, proc, 0);
        CHECK(d != NULL);
        ShowWindow(d, SW_SHOW);
        dump("res-");
        DestroyWindow(d);
    }
    (void)LoadStringA(NULL, 1, buf, sizeof buf);
}

static int g_argc;
static char **g_argv;

int32_t WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int32_t show)
{
    const char *o = getenv("W32L_TEST_OUT");
    char        mk[600];

    (void)inst; (void)prev; (void)cmd; (void)show;
    outdir = o ? o : "tests/out";
    snprintf(mk, sizeof mk, "mkdir -p '%s'", outdir);
    if (system(mk))
        return 2;
    test_template();
    if (g_argc > 1)
        test_resources(g_argc, g_argv);
    fprintf(stderr, "dialogs_test: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}

/* The layer's main() has no argv for WinMain: take them from /proc. */
static void __attribute__((constructor)) grab_args(void)
{
    static char  buf[4096], *av[64];
    FILE        *f = fopen("/proc/self/cmdline", "r");
    size_t       n = f ? fread(buf, 1, sizeof buf - 1, f) : 0, i;

    if (f)
        fclose(f);
    buf[n] = 0;
    for (i = 0; i < n && g_argc < 63; i += strlen(buf + i) + 1)
        av[g_argc++] = buf + i;
    av[g_argc] = NULL;
    g_argv = av;
}
