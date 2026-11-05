/* kernel32ext.cpp -- the KERNEL32 entries a CRT-based game imports beyond what
 * kernel32.cpp and kernel32crt.cpp first needed: thread and process ids, TLS,
 * the lstr* string helpers, locale answers (always en-US), console output,
 * version information, a file copy, and interlocked counters.
 *
 * Locale: every answer is United States English, code page 437 for the OEM
 * side and 1252 for ANSI. A game that formats a date or a number for display
 * gets what an English Windows would show. */
#include "platform.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#ifndef ERROR_FILE_EXISTS
#define ERROR_FILE_EXISTS 80
#endif
#ifndef ERROR_INVALID_FLAGS
#define ERROR_INVALID_FLAGS 1004
#endif

/* ---- ids --------------------------------------------------------------------- */

DWORD WINAPI GetCurrentProcessId(void) { return (DWORD)getpid(); }
DWORD WINAPI GetCurrentThreadId(void) { return (DWORD)syscall(SYS_gettid); }
/* The pseudo-handle Windows hands out for "this thread". */
HANDLE WINAPI GetCurrentThread(void) { return (HANDLE)(LONG_PTR)-2; }

/* ---- interlocked --------------------------------------------------------------- */

LONG WINAPI w32l_InterlockedIncrement(volatile LONG *p) { return __sync_add_and_fetch(p, 1); }
LONG WINAPI w32l_InterlockedDecrement(volatile LONG *p) { return __sync_sub_and_fetch(p, 1); }

/* ---- debugging and process end ------------------------------------------------- */

BOOL WINAPI IsDebuggerPresent(void) { return FALSE; }

void WINAPI DebugBreak(void) { w32l_plat_log("DebugBreak ignored"); }

void WINAPI OutputDebugStringA(LPCSTR s)
{
    if (s)
        w32l_plat_debug("OutputDebugString: %s", s);
}

void WINAPI FatalAppExitA(UINT action, LPCSTR message)
{
    (void)action;
    fprintf(stderr, "FatalAppExit: %s\n", message ? message : "");
    w32l_host_exit(1);
}

/* A C++ throw in the CRT, or the program's own. There is no structured
 * exception unwinder behind this: the program is told what happened and
 * stopped, which is better than carrying on past a raise it expects to
 * transfer control. */
void WINAPI RaiseException(DWORD code, DWORD flags, DWORD nargs, const ULONG_PTR *args)
{
    (void)flags; (void)nargs; (void)args;
    fprintf(stderr, "RaiseException: code 0x%08x, which this layer cannot dispatch\n", (unsigned)code);
    w32l_host_exit(3);
}

/* ---- console ------------------------------------------------------------------- */

UINT WINAPI GetConsoleCP(void) { return 437; }
UINT WINAPI GetConsoleOutputCP(void) { return 437; }

/* No handle is a console: the CRT then writes with WriteFile. */
BOOL WINAPI GetConsoleMode(HANDLE h, LPDWORD mode)
{
    (void)h; (void)mode;
    return FALSE;
}

BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE handler, BOOL add)
{
    (void)handler; (void)add;
    return TRUE;
}

BOOL WINAPI WriteConsoleA(HANDLE h, LPCVOID buf, DWORD n, LPDWORD written, LPVOID reserved)
{
    (void)reserved;
    return WriteFile(h, buf, n, written, NULL);
}

BOOL WINAPI WriteConsoleW(HANDLE h, LPCVOID buf, DWORD n, LPDWORD written, LPVOID reserved)
{
    const WCHAR *w = (const WCHAR *)buf;
    char        *a;
    DWORD        i, put = 0;
    BOOL         ok;

    (void)reserved;
    if (!w || !(a = (char *)malloc(n ? n : 1)))
        return FALSE;
    for (i = 0; i < n; i++)
        a[i] = w[i] < 0x100 ? (char)w[i] : '?';
    ok = WriteFile(h, a, n, &put, NULL);
    free(a);
    if (written)
        *written = put;
    return ok;
}

/* ---- the string helpers --------------------------------------------------------- */

LPSTR WINAPI lstrcatA(LPSTR a, LPCSTR b)
{
    if (!a || !b)
        return NULL;
    return strcat(a, b);
}

int32_t WINAPI lstrcmpA(LPCSTR a, LPCSTR b)
{
    int32_t r = strcmp(a ? a : "", b ? b : "");
    return r < 0 ? -1 : r > 0 ? 1 : 0;
}

int32_t WINAPI lstrcmpiA(LPCSTR a, LPCSTR b)
{
    int32_t r = strcasecmp(a ? a : "", b ? b : "");
    return r < 0 ? -1 : r > 0 ? 1 : 0;
}

LPSTR WINAPI lstrcpyA(LPSTR a, LPCSTR b)
{
    if (!a || !b)
        return NULL;
    return strcpy(a, b);
}

/* At most n-1 characters, always terminated. */
LPSTR WINAPI lstrcpynA(LPSTR a, LPCSTR b, int32_t n)
{
    int32_t i;

    if (!a || !b || n <= 0)
        return NULL;
    for (i = 0; i < n - 1 && b[i]; i++)
        a[i] = b[i];
    a[i] = 0;
    return a;
}

/* ---- TLS: one pthread key a slot ------------------------------------------------ */

#define W32L_TLS_SLOTS 1088

static pthread_key_t   w32l_tls_key[W32L_TLS_SLOTS];
static uint8_t         w32l_tls_used[W32L_TLS_SLOTS];
static pthread_mutex_t w32l_tls_lock = PTHREAD_MUTEX_INITIALIZER;

DWORD WINAPI TlsAlloc(void)
{
    DWORD i, r = TLS_OUT_OF_INDEXES;

    pthread_mutex_lock(&w32l_tls_lock);
    for (i = 0; i < W32L_TLS_SLOTS; i++)
        if (!w32l_tls_used[i]) {
            if (pthread_key_create(&w32l_tls_key[i], NULL) == 0) {
                w32l_tls_used[i] = 1;
                r = i;
            }
            break;
        }
    pthread_mutex_unlock(&w32l_tls_lock);
    if (r == TLS_OUT_OF_INDEXES)
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return r;
}

BOOL WINAPI TlsFree(DWORD index)
{
    BOOL ok = FALSE;

    pthread_mutex_lock(&w32l_tls_lock);
    if (index < W32L_TLS_SLOTS && w32l_tls_used[index]) {
        pthread_key_delete(w32l_tls_key[index]);
        w32l_tls_used[index] = 0;
        ok = TRUE;
    }
    pthread_mutex_unlock(&w32l_tls_lock);
    return ok;
}

LPVOID WINAPI TlsGetValue(DWORD index)
{
    if (index >= W32L_TLS_SLOTS || !w32l_tls_used[index]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    SetLastError(0);
    return pthread_getspecific(w32l_tls_key[index]);
}

BOOL WINAPI TlsSetValue(DWORD index, LPVOID value)
{
    if (index >= W32L_TLS_SLOTS || !w32l_tls_used[index]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return pthread_setspecific(w32l_tls_key[index], value) == 0;
}

/* ---- the process heap -------------------------------------------------------------- */

/* One heap, as HeapCreate's comment in kernel32crt.cpp says; this is the
 * handle it would have made first. */
HANDLE WINAPI GetProcessHeap(void) { return (HANDLE)(uintptr_t)0x48450000u; }

/* ---- time ----------------------------------------------------------------------------- */

void WINAPI GetSystemTimeAsFileTime(LPFILETIME out)
{
    SYSTEMTIME st;
    struct tm  t;
    uint64_t   ticks;

    GetSystemTime(&st);
    memset(&t, 0, sizeof t);
    t.tm_year = st.wYear - 1900;
    t.tm_mon = st.wMonth - 1;
    t.tm_mday = st.wDay;
    t.tm_hour = st.wHour;
    t.tm_min = st.wMinute;
    t.tm_sec = st.wSecond;
    /* 100 ns ticks since 1601-01-01. */
    ticks = ((uint64_t)timegm(&t) + 11644473600ULL) * 10000000ULL +
            (uint64_t)st.wMilliseconds * 10000ULL;
    out->dwLowDateTime = (DWORD)ticks;
    out->dwHighDateTime = (DWORD)(ticks >> 32);
}

/* ---- version -------------------------------------------------------------------------- */

/* Windows 98 SE, the same answer GetVersion gives (kernel32crt.cpp), unless the
 * host sets w32l_os_version (a 2005-era tool wants NT 5.1, i.e. XP). */
w32l_OsVersion w32l_os_version = { 4, 10, 0x040A08AEu, VER_PLATFORM_WIN32_WINDOWS };

BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA v)
{
    if (!v || v->dwOSVersionInfoSize < sizeof *v)
        return FALSE;
    v->dwMajorVersion = w32l_os_version.major;
    v->dwMinorVersion = w32l_os_version.minor;
    v->dwBuildNumber = w32l_os_version.build;
    v->dwPlatformId = w32l_os_version.platform;
    v->szCSDVersion[0] = 0;
    return TRUE;
}

/* ---- locale: United States English --------------------------------------------------- */

WORD WINAPI GetSystemDefaultLangID(void) { return 0x0409; }
LCID WINAPI GetUserDefaultLCID(void) { return 0x0409; }

BOOL WINAPI IsValidLocale(LCID locale, DWORD flags)
{
    (void)flags;
    return locale == 0x0409;
}

BOOL WINAPI IsValidCodePage(UINT cp)
{
    return cp == 437 || cp == 850 || cp == 1252 || cp == 65001 || cp == 20127;
}

/* The one locale there is, offered to the program's callback. */
BOOL WINAPI EnumSystemLocalesA(LOCALE_ENUMPROCA proc, DWORD flags)
{
    char name[] = "00000409";

    (void)flags;
    if (!proc)
        return FALSE;
    proc(name);
    return TRUE;
}

struct LocaleString { DWORD type; const char *value; };
static const LocaleString w32l_locale_strings[] = {
    { 0x0001, "0409" },                     /* LOCALE_ILANGUAGE */
    { 0x0002, "English (United States)" },  /* LOCALE_SLANGUAGE */
    { 0x0003, "ENU" },                      /* LOCALE_SABBREVLANGNAME */
    { 0x0005, "1" },                        /* LOCALE_ICOUNTRY */
    { 0x0006, "United States" },            /* LOCALE_SCOUNTRY */
    { 0x0007, "USA" },                      /* LOCALE_SABBREVCTRYNAME */
    { 0x000B, "437" },                      /* LOCALE_IDEFAULTCODEPAGE */
    { 0x000C, "," },                        /* LOCALE_SLIST */
    { 0x000D, "1" },                        /* LOCALE_IMEASURE */
    { 0x000E, "." },                        /* LOCALE_SDECIMAL */
    { 0x000F, "," },                        /* LOCALE_STHOUSAND */
    { 0x0014, "$" },                        /* LOCALE_SCURRENCY */
    { 0x001D, "/" },                        /* LOCALE_SDATE */
    { 0x001E, ":" },                        /* LOCALE_STIME */
    { 0x001F, "M/d/yyyy" },                 /* LOCALE_SSHORTDATE */
    { 0x0020, "dddd, MMMM dd, yyyy" },      /* LOCALE_SLONGDATE */
    { 0x0021, "0" },                        /* LOCALE_IDATE */
    { 0x0022, "0" },                        /* LOCALE_ILDATE */
    { 0x0023, "0" },                        /* LOCALE_ITIME (12 hour) */
    { 0x0028, "AM" },                       /* LOCALE_S1159 */
    { 0x0029, "PM" },                       /* LOCALE_S2359 */
    { 0x1001, "English" },                  /* LOCALE_SENGLANGUAGE */
    { 0x1002, "United States" },            /* LOCALE_SENGCOUNTRY */
    { 0x1003, "h:mm:ss tt" },               /* LOCALE_STIMEFORMAT */
    { 0x1004, "1252" },                     /* LOCALE_IDEFAULTANSICODEPAGE */
};

static const char *w32l_locale_value(DWORD type)
{
    size_t i;

    type &= 0xFFFF;    /* LOCALE_NOUSEROVERRIDE and LOCALE_USE_CP_ACP live above */
    for (i = 0; i < sizeof w32l_locale_strings / sizeof w32l_locale_strings[0]; i++)
        if (w32l_locale_strings[i].type == type)
            return w32l_locale_strings[i].value;
    return NULL;
}

int32_t WINAPI GetLocaleInfoA(LCID locale, DWORD type, LPSTR out, int32_t cch)
{
    const char *v = w32l_locale_value(type);
    int32_t     n;

    (void)locale;
    if (!v) {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }
    n = (int32_t)strlen(v) + 1;
    if (cch == 0)
        return n;
    if (!out || cch < n) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    memcpy(out, v, (size_t)n);
    return n;
}

int32_t WINAPI GetLocaleInfoW(LCID locale, DWORD type, LPWSTR out, int32_t cch)
{
    const char *v = w32l_locale_value(type);
    int32_t     n, i;

    (void)locale;
    if (!v) {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }
    n = (int32_t)strlen(v) + 1;
    if (cch == 0)
        return n;
    if (!out || cch < n) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    for (i = 0; i < n; i++)
        out[i] = (WCHAR)(uint8_t)v[i];
    return n;
}

/* ---- date and time text ------------------------------------------------------------------- */

static const char *const w32l_day_names[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char *const w32l_month_names[12] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};

/* Format `t` by a Windows picture: d dd ddd dddd, M MM MMM MMMM, yy yyyy,
 * h hh H HH, m mm, s ss, t tt, and 'quoted text'. Returns the length
 * including the terminator; writes only if it fits. */
static int32_t w32l_format_picture(const SYSTEMTIME *t, const char *fmt,
                                   char *out, int32_t cch)
{
    char    buf[256];
    size_t  n = 0;
    const char *p = fmt;

    while (*p && n < sizeof buf - 40) {
        char    c = *p;
        int32_t run = 1;
        int32_t h12 = t->wHour % 12 ? t->wHour % 12 : 12;

        if (c == '\'') {
            p++;
            while (*p && *p != '\'' && n < sizeof buf - 2)
                buf[n++] = *p++;
            if (*p)
                p++;
            continue;
        }
        while (p[run] == c)
            run++;
        switch (c) {
        case 'd':
            if (run >= 4)      n += (size_t)snprintf(buf + n, sizeof buf - n, "%s", w32l_day_names[t->wDayOfWeek % 7]);
            else if (run == 3) n += (size_t)snprintf(buf + n, sizeof buf - n, "%.3s", w32l_day_names[t->wDayOfWeek % 7]);
            else if (run == 2) n += (size_t)snprintf(buf + n, sizeof buf - n, "%02d", t->wDay);
            else               n += (size_t)snprintf(buf + n, sizeof buf - n, "%d", t->wDay);
            break;
        case 'M':
            if (run >= 4)      n += (size_t)snprintf(buf + n, sizeof buf - n, "%s", w32l_month_names[(t->wMonth + 11) % 12]);
            else if (run == 3) n += (size_t)snprintf(buf + n, sizeof buf - n, "%.3s", w32l_month_names[(t->wMonth + 11) % 12]);
            else if (run == 2) n += (size_t)snprintf(buf + n, sizeof buf - n, "%02d", t->wMonth);
            else               n += (size_t)snprintf(buf + n, sizeof buf - n, "%d", t->wMonth);
            break;
        case 'y':
            if (run >= 3)      n += (size_t)snprintf(buf + n, sizeof buf - n, "%04d", t->wYear);
            else               n += (size_t)snprintf(buf + n, sizeof buf - n, "%02d", t->wYear % 100);
            break;
        case 'h':
            n += (size_t)snprintf(buf + n, sizeof buf - n, run >= 2 ? "%02d" : "%d", h12);
            break;
        case 'H':
            n += (size_t)snprintf(buf + n, sizeof buf - n, run >= 2 ? "%02d" : "%d", t->wHour);
            break;
        case 'm':
            n += (size_t)snprintf(buf + n, sizeof buf - n, run >= 2 ? "%02d" : "%d", t->wMinute);
            break;
        case 's':
            n += (size_t)snprintf(buf + n, sizeof buf - n, run >= 2 ? "%02d" : "%d", t->wSecond);
            break;
        case 't':
            n += (size_t)snprintf(buf + n, sizeof buf - n, run >= 2 ? "%s" : "%.1s",
                                  t->wHour < 12 ? "AM" : "PM");
            break;
        default:
            buf[n++] = c;
            run = 1;
            break;
        }
        p += run;
    }
    buf[n] = 0;
    if (cch == 0)
        return (int32_t)n + 1;
    if (!out || cch < (int32_t)n + 1) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    memcpy(out, buf, n + 1);
    return (int32_t)n + 1;
}

static int32_t w32l_format_time_text(const SYSTEMTIME *t, const char *fmt,
                                     const char *dflt, char *out, int32_t cch)
{
    SYSTEMTIME now;

    if (!t) {
        GetLocalTime(&now);
        t = &now;
    }
    return w32l_format_picture(t, fmt ? fmt : dflt, out, cch);
}

int32_t WINAPI GetDateFormatA(LCID locale, DWORD flags, const SYSTEMTIME *t,
                              LPCSTR fmt, LPSTR out, int32_t cch)
{
    (void)locale;
    /* DATE_LONGDATE is 2; the default is the short date. */
    return w32l_format_time_text(t, fmt, (flags & 2) ? "dddd, MMMM dd, yyyy" : "M/d/yyyy", out, cch);
}

int32_t WINAPI GetTimeFormatA(LCID locale, DWORD flags, const SYSTEMTIME *t,
                              LPCSTR fmt, LPSTR out, int32_t cch)
{
    (void)locale;
    /* TIME_NOSECONDS is 2. */
    return w32l_format_time_text(t, fmt, (flags & 2) ? "h:mm tt" : "h:mm:ss tt", out, cch);
}

/* ---- files -------------------------------------------------------------------------------------- */

HANDLE WINAPI CreateFileW(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                          DWORD create, DWORD attrs, HANDLE templ)
{
    char    narrow[1024];
    size_t  i;

    if (!path) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    for (i = 0; path[i] && i < sizeof narrow - 1; i++)
        narrow[i] = path[i] < 0x100 ? (char)path[i] : '?';
    narrow[i] = 0;
    return CreateFileA(narrow, access, share, sa, create, attrs, templ);
}

/* Through this layer's own CreateFileA, ReadFile and WriteFile, so the
 * Windows paths are translated the same way as everywhere else. */
BOOL WINAPI CopyFileA(LPCSTR from, LPCSTR to, BOOL failIfExists)
{
    HANDLE  in, out;
    uint8_t buf[16384];
    DWORD   got, put;
    BOOL    ok = TRUE;

    in = CreateFileA(from, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (in == INVALID_HANDLE_VALUE)
        return FALSE;
    out = CreateFileA(to, GENERIC_WRITE, 0, NULL, failIfExists ? CREATE_NEW : CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (out == INVALID_HANDLE_VALUE) {
        if (failIfExists)
            SetLastError(ERROR_FILE_EXISTS);
        CloseHandle(in);
        return FALSE;
    }
    while (ReadFile(in, buf, sizeof buf, &got, NULL) && got) {
        if (!WriteFile(out, buf, got, &put, NULL) || put != got) {
            ok = FALSE;
            break;
        }
    }
    CloseHandle(in);
    CloseHandle(out);
    return ok;
}
