/* kernel32w.cpp -- the KERNEL32 (and a few ADVAPI32/SHELL32/msvcrt) entries a
 * later toolchain imports: Visual C++ 2005's cl, c1xx, c2, link and the real
 * MSVCR80/MSVCP80 they sit on. That generation of program is Unicode-first, so
 * most of this is the wide-character form of a function kernel32crt.cpp
 * already has: the string is converted to UTF-8 and the narrow function does
 * the work (and the Windows-path translation). The rest is what a compiler
 * needs and a game never did: environment variables, file mapping, file
 * times and sizes, system and memory information.
 *
 * Process creation, pipes, the console and the registry answer "not
 * available" honestly; a host that needs one (vc8host runs the compiler
 * passes in-process) provides it. The functions are registered in
 * w32l_exports_w, which kernel32.cpp's lookups consult after their own table.
 *
 * The environment: GetEnvironmentVariable and friends read the HOST's
 * environment, as a tool needs (INCLUDE, LIB, TMP). GetEnvironmentStrings
 * stays the canonical block a game sees unless the host sets
 * w32l_host_environment (kernel32crt.cpp), because a game's heap layout depends
 * on it. */
#include "platform.h"
#include "handle.h"
#include <direct.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define PATH_BUF W32L_NATIVE_PATH_MAX
#define MAX_PATH_W 260

extern char **environ;

/* ---- UTF-16 <-> UTF-8 ------------------------------------------------------------ */

static size_t w2u(const WCHAR *w, char *out, size_t cap)
{
    size_t n = 0;

    if (!w) {
        if (cap)
            out[0] = 0;
        return 0;
    }
    while (*w && n + 4 < cap) {
        uint32_t c = *w++;
        if (c >= 0xD800 && c < 0xDC00 && *w >= 0xDC00 && *w < 0xE000)
            c = 0x10000 + ((c - 0xD800) << 10) + (*w++ - 0xDC00);
        if (c < 0x80) {
            out[n++] = (char)c;
        } else if (c < 0x800) {
            out[n++] = (char)(0xC0 | (c >> 6));
            out[n++] = (char)(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            out[n++] = (char)(0xE0 | (c >> 12));
            out[n++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[n++] = (char)(0x80 | (c & 0x3F));
        } else {
            out[n++] = (char)(0xF0 | (c >> 18));
            out[n++] = (char)(0x80 | ((c >> 12) & 0x3F));
            out[n++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[n++] = (char)(0x80 | (c & 0x3F));
        }
    }
    if (cap)
        out[n] = 0;
    return n;
}

/* Returns the number of UTF-16 units written (without the NUL). */
static size_t u2w(const char *u, WCHAR *out, size_t cap)
{
    size_t n = 0;

    while (u && *u && n + 2 < cap) {
        uint32_t c = (unsigned char)*u++;
        if (c >= 0xF0 && u[0] && u[1] && u[2]) {
            c = ((c & 7) << 18) | ((u[0] & 0x3F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
            u += 3;
        } else if (c >= 0xE0 && u[0] && u[1]) {
            c = ((c & 0xF) << 12) | ((u[0] & 0x3F) << 6) | (u[1] & 0x3F);
            u += 2;
        } else if (c >= 0xC0 && u[0]) {
            c = ((c & 0x1F) << 6) | (u[0] & 0x3F);
            u += 1;
        }
        if (c >= 0x10000) {
            c -= 0x10000;
            out[n++] = (WCHAR)(0xD800 + (c >> 10));
            out[n++] = (WCHAR)(0xDC00 + (c & 0x3FF));
        } else {
            out[n++] = (WCHAR)c;
        }
    }
    if (cap)
        out[n] = 0;
    return n;
}

/* Copy a narrow answer into the caller's wide buffer the Windows way: the
 * length without the NUL when it fits, the size needed with it when not. */
static DWORD put_wide(const char *a, LPWSTR out, DWORD cap)
{
    WCHAR  tmp[PATH_BUF * 2];
    size_t n = u2w(a, tmp, sizeof tmp / sizeof tmp[0]);

    if (n + 1 > cap || !out)
        return (DWORD)n + 1;
    memcpy(out, tmp, (n + 1) * sizeof(WCHAR));
    return (DWORD)n;
}

static DWORD put_narrow(const char *a, LPSTR out, DWORD cap)
{
    size_t n = strlen(a);

    if (n + 1 > cap || !out)
        return (DWORD)n + 1;
    memcpy(out, a, n + 1);
    return (DWORD)n;
}

#define U(var, w) char var[PATH_BUF]; w2u((w), var, sizeof var)

/* ---- the environment ---------------------------------------------------------------- */

#define ERROR_ENVVAR_NOT_FOUND 203

DWORD WINAPI GetEnvironmentVariableA(LPCSTR name, LPSTR out, DWORD cap)
{
    const char *v = name ? getenv(name) : NULL;

    if (!v) {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }
    return put_narrow(v, out, cap);
}

DWORD WINAPI GetEnvironmentVariableW(LPCWSTR name, LPWSTR out, DWORD cap)
{
    U(n, name);
    const char *v = getenv(n);

    if (!v) {
        SetLastError(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }
    return put_wide(v, out, cap);
}

BOOL WINAPI SetEnvironmentVariableW(LPCWSTR name, LPCWSTR value)
{
    U(n, name);
    char v[PATH_BUF * 4];

    if (value) {
        w2u(value, v, sizeof v);
        setenv(n, v, 1);
    } else {
        unsetenv(n);
    }
    return TRUE;
}

/* %NAME% expansion into `out` (narrow); returns the size needed with the NUL. */
static DWORD expand_env(const char *in, char *out, size_t cap)
{
    size_t n = 0;

    while (*in) {
        if (*in == '%') {
            const char *end = strchr(in + 1, '%');
            if (end && end > in + 1) {
                char        name[256];
                const char *v;
                size_t      l = (size_t)(end - in - 1);
                if (l < sizeof name) {
                    memcpy(name, in + 1, l);
                    name[l] = 0;
                    v = getenv(name);
                    if (v) {
                        size_t vl = strlen(v);
                        if (n + vl < cap)
                            memcpy(out + n, v, vl);
                        n += vl;
                        in = end + 1;
                        continue;
                    }
                }
            }
        }
        if (n + 1 < cap)
            out[n] = *in;
        n++;
        in++;
    }
    if (n < cap)
        out[n] = 0;
    return (DWORD)n + 1;
}

DWORD WINAPI ExpandEnvironmentStringsA(LPCSTR in, LPSTR out, DWORD cap)
{
    char  tmp[PATH_BUF * 4];
    DWORD need = expand_env(in, tmp, sizeof tmp);

    if (out && need <= cap)
        memcpy(out, tmp, need);
    return need;
}

DWORD WINAPI ExpandEnvironmentStringsW(LPCWSTR in, LPWSTR out, DWORD cap)
{
    char  a[PATH_BUF * 4], tmp[PATH_BUF * 4];
    WCHAR w[PATH_BUF * 4];
    DWORD need;

    w2u(in, a, sizeof a);
    expand_env(a, tmp, sizeof tmp);
    need = (DWORD)u2w(tmp, w, sizeof w / sizeof w[0]) + 1;
    if (out && need <= cap)
        memcpy(out, w, need * sizeof(WCHAR));
    return need;
}

LPWSTR WINAPI GetCommandLineW(void)
{
    static WCHAR *w;

    if (!w) {
        const char *a = GetCommandLineA();
        w = (WCHAR *)malloc((strlen(a) + 1) * 2 * sizeof(WCHAR));
        u2w(a, w, (strlen(a) + 1) * 2);
    }
    return w;
}

typedef struct { DWORD cb; LPWSTR r, d, t; DWORD x, y, xs, ys, xc, yc, fill, flags;
                 WORD show, res2; BYTE *res2p; HANDLE in, out, err; } STARTUPINFOW_;

void WINAPI GetStartupInfoW(STARTUPINFOW_ *out)
{
    memset(out, 0, sizeof *out);
    out->cb = sizeof *out;
    out->show = SW_SHOWNORMAL;
}

/* ---- paths and files -------------------------------------------------------------------- */

#define ERROR_ALREADY_EXISTS 183
#define ERROR_NO_MORE_FILES 18

BOOL WINAPI CreateDirectoryW(LPCWSTR p, LPSECURITY_ATTRIBUTES sa)
{
    U(a, p);
    return CreateDirectoryA(a, sa);
}
BOOL WINAPI DeleteFileW(LPCWSTR p) { U(a, p); return DeleteFileA(a); }
BOOL WINAPI RemoveDirectoryW(LPCWSTR p) { U(a, p); return RemoveDirectoryA(a); }
BOOL WINAPI SetCurrentDirectoryW(LPCWSTR p) { U(a, p); return SetCurrentDirectoryA(a); }
DWORD WINAPI GetFileAttributesW(LPCWSTR p) { U(a, p); return GetFileAttributesA(a); }
BOOL WINAPI SetFileAttributesW(LPCWSTR p, DWORD attrs) { U(a, p); return SetFileAttributesA(a, attrs); }

DWORD WINAPI GetCurrentDirectoryW(DWORD cap, LPWSTR out)
{
    char a[PATH_BUF];

    if (!GetCurrentDirectoryA(sizeof a, a))
        return 0;
    return put_wide(a, out, cap);
}

DWORD WINAPI GetFullPathNameW(LPCWSTR p, DWORD cap, LPWSTR out, LPWSTR *part)
{
    U(a, p);
    char   full[PATH_BUF];
    DWORD  n = GetFullPathNameA(a, sizeof full, full, NULL);

    if (!n)
        return 0;
    n = put_wide(full, out, cap);
    if (part && out && n < cap) {
        LPWSTR s = out + n;
        while (s > out && s[-1] != '\\' && s[-1] != '/')
            s--;
        *part = s;
    }
    return n;
}

/* A long or short name is the name itself: the host has only one. */
DWORD WINAPI GetLongPathNameW(LPCWSTR p, LPWSTR out, DWORD cap)
{
    size_t n = 0;

    while (p && p[n])
        n++;
    if (n + 1 > cap || !out)
        return (DWORD)n + 1;
    memcpy(out, p, (n + 1) * sizeof(WCHAR));
    return (DWORD)n;
}
DWORD WINAPI GetShortPathNameW(LPCWSTR p, LPWSTR out, DWORD cap) { return GetLongPathNameW(p, out, cap); }

DWORD WINAPI GetSystemDirectoryW(LPWSTR out, DWORD cap) { return put_wide("C:\\WINDOWS\\SYSTEM32", out, cap); }

DWORD WINAPI GetTempPathW(DWORD cap, LPWSTR out)
{
    const char *t = getenv("TMP");
    char        a[PATH_BUF];

    if (!t || !*t)
        t = getenv("TEMP");
    snprintf(a, sizeof a, "%s%s", t && *t ? t : "/tmp", t && *t && t[strlen(t) - 1] == '/' ? "" : "/");
    return put_wide(a, out, cap);
}

UINT WINAPI GetTempFileNameW(LPCWSTR dir, LPCWSTR prefix, UINT unique, LPWSTR out)
{
    char    d[PATH_BUF], pfx[16], a[PATH_BUF];
    size_t  dn;
    UINT    id = unique ? unique : (UINT)(getpid() * 31u + (unsigned)time(NULL)) & 0xFFFF;

    w2u(dir, d, sizeof d);
    w2u(prefix, pfx, sizeof pfx);
    pfx[3] = 0;
    dn = strlen(d);
    for (;;) {
        snprintf(a, sizeof a, "%s%s%s%X.tmp", d, dn && d[dn - 1] != '\\' && d[dn - 1] != '/' ? "/" : "",
                 pfx, id);
        if (unique || GetFileAttributesA(a) == 0xFFFFFFFFu)
            break;
        id = (id + 1) & 0xFFFF;
    }
    if (!unique) {
        HANDLE h = CreateFileA(a, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE)
            return 0;
        CloseHandle(h);
    }
    put_wide(a, out, MAX_PATH_W);
    return id;
}

BOOL WINAPI MoveFileA(LPCSTR from, LPCSTR to)
{
    char a[PATH_BUF], b[PATH_BUF];

    if (!from || !to)
        return FALSE;
    w32l_native_path(from, a);
    w32l_native_path(to, b);
    if (rename(a, b) != 0) {
        SetLastError(errno == ENOENT ? ERROR_FILE_NOT_FOUND : errno == EEXIST ? ERROR_ALREADY_EXISTS : 5);
        return FALSE;
    }
    return TRUE;
}
BOOL WINAPI MoveFileW(LPCWSTR f, LPCWSTR t) { U(a, f); U(b, t); return MoveFileA(a, b); }

BOOL WINAPI CopyFileW(LPCWSTR f, LPCWSTR t, BOOL failIfExists)
{
    U(a, f);
    U(b, t);
    return CopyFileA(a, b, failIfExists);
}

/* File times: Windows counts 100 ns ticks from 1601, the host seconds from 1970. */
static void to_filetime(time_t t, long nsec, FILETIME *ft)
{
    uint64_t v = ((uint64_t)t + 11644473600ull) * 10000000ull + (uint64_t)nsec / 100;

    ft->dwLowDateTime = (DWORD)v;
    ft->dwHighDateTime = (DWORD)(v >> 32);
}

static DWORD attrs_of(const struct stat *st)
{
    DWORD a = S_ISDIR(st->st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;

    if (!(st->st_mode & 0222))
        a |= FILE_ATTRIBUTE_READONLY;
    return a;
}

typedef struct { DWORD attrs; FILETIME created, accessed, written; DWORD sizeHigh, sizeLow; } FILE_ATTR_DATA;

BOOL WINAPI GetFileAttributesExW(LPCWSTR p, int32_t level, LPVOID out)
{
    U(a, p);
    char        n[PATH_BUF];
    struct stat st;
    FILE_ATTR_DATA *d = (FILE_ATTR_DATA *)out;

    (void)level;
    w32l_native_path(a, n);
    if (stat(n, &st) != 0) {
        SetLastError(errno == ENOENT ? ERROR_FILE_NOT_FOUND : ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    d->attrs = attrs_of(&st);
    to_filetime(st.st_ctime, st.st_ctim.tv_nsec, &d->created);
    to_filetime(st.st_atime, st.st_atim.tv_nsec, &d->accessed);
    to_filetime(st.st_mtime, st.st_mtim.tv_nsec, &d->written);
    d->sizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
    d->sizeLow = (DWORD)st.st_size;
    return TRUE;
}

typedef struct { DWORD attrs; FILETIME created, accessed, written; DWORD volume, sizeHigh, sizeLow, links,
                 indexHigh, indexLow; } BY_HANDLE_INFO;

BOOL WINAPI GetFileInformationByHandle(HANDLE h, LPVOID out)
{
    int32_t        fd = w32l_handle_fd(h);
    struct stat    st;
    BY_HANDLE_INFO *d = (BY_HANDLE_INFO *)out;

    if (fd < 0 || fstat(fd, &st) != 0) {
        SetLastError(6);       /* ERROR_INVALID_HANDLE */
        return FALSE;
    }
    memset(d, 0, sizeof *d);
    d->attrs = attrs_of(&st);
    to_filetime(st.st_ctime, st.st_ctim.tv_nsec, &d->created);
    to_filetime(st.st_atime, st.st_atim.tv_nsec, &d->accessed);
    to_filetime(st.st_mtime, st.st_mtim.tv_nsec, &d->written);
    d->volume = (DWORD)st.st_dev;
    d->sizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
    d->sizeLow = (DWORD)st.st_size;
    d->links = (DWORD)st.st_nlink;
    d->indexHigh = (DWORD)((uint64_t)st.st_ino >> 32);
    d->indexLow = (DWORD)st.st_ino;
    return TRUE;
}

DWORD WINAPI GetFileSize(HANDLE h, LPDWORD high)
{
    int32_t     fd = w32l_handle_fd(h);
    struct stat st;

    if (fd < 0 || fstat(fd, &st) != 0) {
        SetLastError(6);
        return 0xFFFFFFFFu;
    }
    if (high)
        *high = (DWORD)((uint64_t)st.st_size >> 32);
    return (DWORD)st.st_size;
}

BOOL WINAPI GetFileTime(HANDLE h, LPFILETIME c, LPFILETIME a, LPFILETIME w)
{
    int32_t     fd = w32l_handle_fd(h);
    struct stat st;

    if (fd < 0 || fstat(fd, &st) != 0) {
        SetLastError(6);
        return FALSE;
    }
    if (c) to_filetime(st.st_ctime, st.st_ctim.tv_nsec, c);
    if (a) to_filetime(st.st_atime, st.st_atim.tv_nsec, a);
    if (w) to_filetime(st.st_mtime, st.st_mtim.tv_nsec, w);
    return TRUE;
}

static void from_filetime(const FILETIME *ft, struct timespec *ts)
{
    uint64_t v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;

    ts->tv_sec = (time_t)(v / 10000000ull) - 11644473600ll;
    ts->tv_nsec = (long)(v % 10000000ull) * 100;
}

BOOL WINAPI SetFileTime(HANDLE h, const FILETIME *c, const FILETIME *a, const FILETIME *w)
{
    int32_t         fd = w32l_handle_fd(h);
    struct timespec ts[2];

    (void)c;
    if (fd < 0) {
        SetLastError(6);
        return FALSE;
    }
    ts[0].tv_sec = 0; ts[0].tv_nsec = UTIME_OMIT;
    ts[1].tv_sec = 0; ts[1].tv_nsec = UTIME_OMIT;
    if (a) from_filetime(a, &ts[0]);
    if (w) from_filetime(w, &ts[1]);
    return futimens(fd, ts) == 0;
}

BOOL WINAPI LocalFileTimeToFileTime(const FILETIME *in, LPFILETIME out) { *out = *in; return TRUE; }  /* UTC */

BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME *st, LPFILETIME out)
{
    struct tm tm;

    memset(&tm, 0, sizeof tm);
    tm.tm_year = st->wYear - 1900;
    tm.tm_mon = st->wMonth - 1;
    tm.tm_mday = st->wDay;
    tm.tm_hour = st->wHour;
    tm.tm_min = st->wMinute;
    tm.tm_sec = st->wSecond;
    to_filetime(timegm(&tm), (long)st->wMilliseconds * 1000000L, out);
    return TRUE;
}

/* FAT date/time: date = (year-1980)<<9 | month<<5 | day; time = hour<<11 | minute<<5 | second/2. */
BOOL WINAPI DosDateTimeToFileTime(WORD date, WORD time, LPFILETIME out)
{
    SYSTEMTIME st;

    memset(&st, 0, sizeof st);
    st.wYear = (WORD)(1980 + (date >> 9));
    st.wMonth = (WORD)((date >> 5) & 15);
    st.wDay = (WORD)(date & 31);
    st.wHour = (WORD)(time >> 11);
    st.wMinute = (WORD)((time >> 5) & 63);
    st.wSecond = (WORD)((time & 31) * 2);
    if (st.wMonth < 1 || st.wMonth > 12 || st.wDay < 1 || st.wHour > 23 || st.wMinute > 59 || st.wSecond > 59) {
        SetLastError(87);
        return FALSE;
    }
    return SystemTimeToFileTime(&st, out);
}

BOOL WINAPI FileTimeToDosDateTime(const FILETIME *in, LPWORD date, LPWORD time)
{
    SYSTEMTIME st;

    if (!FileTimeToSystemTime(in, &st) || st.wYear < 1980 || st.wYear > 2107) {
        SetLastError(87);
        return FALSE;
    }
    *date = (WORD)(((st.wYear - 1980) << 9) | (st.wMonth << 5) | st.wDay);
    *time = (WORD)((st.wHour << 11) | (st.wMinute << 5) | (st.wSecond / 2));
    return TRUE;
}

BOOL WINAPI SetLocalTime(const SYSTEMTIME *st) { (void)st; SetLastError(5); return FALSE; }

BOOL WINAPI LockFile(HANDLE h, DWORD a, DWORD b, DWORD c, DWORD d) { (void)h; (void)a; (void)b; (void)c; (void)d; return TRUE; }
BOOL WINAPI UnlockFile(HANDLE h, DWORD a, DWORD b, DWORD c, DWORD d) { (void)h; (void)a; (void)b; (void)c; (void)d; return TRUE; }
BOOL WINAPI LockFileEx(HANDLE h, DWORD f, DWORD r, DWORD lo, DWORD hi, LPVOID o)
{ (void)h; (void)f; (void)r; (void)lo; (void)hi; (void)o; return TRUE; }
BOOL WINAPI UnlockFileEx(HANDLE h, DWORD r, DWORD lo, DWORD hi, LPVOID o)
{ (void)h; (void)r; (void)lo; (void)hi; (void)o; return TRUE; }

/* FindFirstFileW/NextW: the narrow walk, converted. */
typedef struct {
    DWORD    attrs;
    FILETIME created, accessed, written;
    DWORD    sizeHigh, sizeLow, reserved0, reserved1;
    WCHAR    name[260];
    WCHAR    alt[14];
} FIND_DATAW;

static void find_w(const WIN32_FIND_DATAA *a, FIND_DATAW *w)
{
    memset(w, 0, sizeof *w);
    w->attrs = a->dwFileAttributes;
    w->created = a->ftCreationTime;
    w->accessed = a->ftLastAccessTime;
    w->written = a->ftLastWriteTime;
    w->sizeHigh = a->nFileSizeHigh;
    w->sizeLow = a->nFileSizeLow;
    u2w(a->cFileName, w->name, 260);
}

HANDLE WINAPI FindFirstFileW(LPCWSTR spec, FIND_DATAW *out)
{
    U(s, spec);
    WIN32_FIND_DATAA a;
    HANDLE           h = FindFirstFileA(s, &a);

    if (h != INVALID_HANDLE_VALUE)
        find_w(&a, out);
    return h;
}

BOOL WINAPI FindNextFileW(HANDLE h, FIND_DATAW *out)
{
    WIN32_FIND_DATAA a;

    if (!FindNextFileA(h, &a))
        return FALSE;
    find_w(&a, out);
    return TRUE;
}

/* ---- drives and disks ------------------------------------------------------------------------ */

UINT WINAPI GetDriveTypeW(LPCWSTR root) { (void)root; return 3; }          /* DRIVE_FIXED */
DWORD WINAPI GetLogicalDrives(void) { return 1u << 2; }                    /* C: */
int32_t w32l_getdrives(void) { return 1 << 2; }

BOOL WINAPI GetDiskFreeSpaceA(LPCSTR root, LPDWORD spc, LPDWORD bps, LPDWORD freeC, LPDWORD totalC)
{
    struct statvfs sv;

    (void)root;
    if (statvfs(".", &sv) != 0)
        return FALSE;
    if (spc) *spc = 1;
    if (bps) *bps = (DWORD)sv.f_frsize;
    if (freeC) *freeC = (DWORD)sv.f_bavail;
    if (totalC) *totalC = (DWORD)sv.f_blocks;
    return TRUE;
}

BOOL WINAPI GetDiskFreeSpaceExW(LPCWSTR root, uint64_t *avail, uint64_t *total, uint64_t *freeB)
{
    struct statvfs sv;

    (void)root;
    if (statvfs(".", &sv) != 0)
        return FALSE;
    if (avail) *avail = (uint64_t)sv.f_bavail * sv.f_frsize;
    if (total) *total = (uint64_t)sv.f_blocks * sv.f_frsize;
    if (freeB) *freeB = (uint64_t)sv.f_bfree * sv.f_frsize;
    return TRUE;
}

/* ---- file mapping -------------------------------------------------------------------------------- */

/* A mapping handle: the descriptor to map (a dup of the file's, or an
 * anonymous memfd), its size, and the protection it was made with. */
#define MAPPING_KIND W32L_H_MAPPING

typedef struct { void *addr; size_t len; } View;
static View            views[256];
static pthread_mutex_t views_lock = PTHREAD_MUTEX_INITIALIZER;

static HANDLE make_mapping(HANDLE file, DWORD prot, DWORD hi, DWORD lo)
{
    W32L_Handle *h = w32l_handle_new(MAPPING_KIND);
    uint64_t     size = ((uint64_t)hi << 32) | lo;
    int32_t      fd;

    if (!h)
        return NULL;
    if (file == INVALID_HANDLE_VALUE) {
        fd = (int32_t)syscall(279 /* memfd_create, i386 */, "w32l-mapping", 0);
        if (fd < 0 || ftruncate(fd, (off_t)size) != 0) {
            if (fd >= 0)
                close(fd);
            free(h);
            SetLastError(8);    /* ERROR_NOT_ENOUGH_MEMORY */
            return NULL;
        }
    } else {
        struct stat st;
        int32_t     src = w32l_handle_fd(file);
        if (src < 0 || fstat(src, &st) != 0) {
            free(h);
            SetLastError(6);
            return NULL;
        }
        fd = dup(src);
        if (!size)
            size = (uint64_t)st.st_size;
        else if ((uint64_t)st.st_size < size && (prot & 0x04))
            (void)ftruncate(fd, (off_t)size);        /* a mapping grows the file */
    }
    h->fd = fd;
    h->find = (intptr_t)size;
    h->done = (int32_t)prot;
    return (HANDLE)h;
}

HANDLE WINAPI CreateFileMappingA(HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD prot, DWORD hi, DWORD lo, LPCSTR name)
{
    (void)sa; (void)name;
    return make_mapping(file, prot, hi, lo);
}
HANDLE WINAPI CreateFileMappingW(HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD prot, DWORD hi, DWORD lo, LPCWSTR name)
{
    (void)sa; (void)name;
    return make_mapping(file, prot, hi, lo);
}
HANDLE WINAPI OpenFileMappingW(DWORD access, BOOL inherit, LPCWSTR name)
{
    (void)access; (void)inherit; (void)name;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return NULL;
}

#define FILE_MAP_WRITE 0x0002
#define FILE_MAP_COPY  0x0001

LPVOID WINAPI MapViewOfFileEx(HANDLE hm, DWORD access, DWORD hi, DWORD lo, SIZE_T bytes, LPVOID where)
{
    W32L_Handle *h = (W32L_Handle *)hm;
    uint64_t     off = ((uint64_t)hi << 32) | lo;
    size_t       len;
    int          prot = PROT_READ, flags = MAP_SHARED, i;
    void        *p;

    if (!h || h->kind != MAPPING_KIND) {
        SetLastError(6);
        return NULL;
    }
    len = bytes ? bytes : (size_t)((uint64_t)h->find - off);
    /* FILE_MAP_ALL_ACCESS has the copy bit too; write wins (shared), and only a
     * bare FILE_MAP_COPY is copy-on-write. */
    if (access & FILE_MAP_WRITE) {
        prot |= PROT_WRITE;
    } else if (access & FILE_MAP_COPY) {
        prot |= PROT_WRITE;
        flags = MAP_PRIVATE;
    }
    p = mmap(where, len, prot, flags | (where ? MAP_FIXED : 0), h->fd, (off_t)off);
    if (p == MAP_FAILED && (prot & PROT_WRITE) && flags == MAP_SHARED)
        p = mmap(where, len, prot, MAP_PRIVATE | (where ? MAP_FIXED : 0), h->fd, (off_t)off);
    if (p == MAP_FAILED) {
        SetLastError(errno == ENOMEM ? 8 : 5);
        return NULL;
    }
    pthread_mutex_lock(&views_lock);
    for (i = 0; i < 256; i++)
        if (!views[i].addr) {
            views[i].addr = p;
            views[i].len = len;
            break;
        }
    pthread_mutex_unlock(&views_lock);
    return p;
}

LPVOID WINAPI MapViewOfFile(HANDLE hm, DWORD access, DWORD hi, DWORD lo, SIZE_T bytes)
{
    return MapViewOfFileEx(hm, access, hi, lo, bytes, NULL);
}

BOOL WINAPI UnmapViewOfFile(LPCVOID addr)
{
    int    i;
    size_t len = 0;

    pthread_mutex_lock(&views_lock);
    for (i = 0; i < 256; i++)
        if (views[i].addr == addr) {
            len = views[i].len;
            views[i].addr = NULL;
            break;
        }
    pthread_mutex_unlock(&views_lock);
    if (!len) {
        SetLastError(487);      /* ERROR_INVALID_ADDRESS */
        return FALSE;
    }
    return munmap((void *)addr, len) == 0;
}

BOOL WINAPI FlushViewOfFile(LPCVOID addr, SIZE_T n)
{
    return msync((void *)((uintptr_t)addr & ~4095u), n + ((uintptr_t)addr & 4095u), MS_ASYNC) == 0;
}

/* ---- memory information ---------------------------------------------------------------------------- */

typedef struct {
    DWORD  oem;
    DWORD  pageSize;
    LPVOID minApp, maxApp;
    DWORD  activeMask, processors, type, granularity;
    WORD   level, revision;
} SYSTEM_INFO_;

void WINAPI GetSystemInfo(SYSTEM_INFO_ *si)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);

    memset(si, 0, sizeof *si);
    si->pageSize = 4096;
    si->minApp = (LPVOID)0x10000;
    si->maxApp = (LPVOID)0x7FFEFFFF;
    si->processors = n > 0 ? (DWORD)n : 1;
    si->activeMask = (1u << (si->processors > 31 ? 31 : si->processors)) - 1;
    si->type = 586;
    si->granularity = 65536;
    si->level = 6;
}

typedef struct {
    LPVOID base, allocBase;
    DWORD  allocProt;
    SIZE_T size;
    DWORD  state, prot, type;
} MEM_INFO;

static DWORD prot_from_maps(const char *perms)
{
    int r = perms[0] == 'r', w = perms[1] == 'w', x = perms[2] == 'x';

    if (x)
        return w ? 0x40 : (r ? 0x20 : 0x10);
    if (w)
        return 0x04;
    return r ? 0x02 : 0x01;
}

SIZE_T WINAPI VirtualQuery(LPCVOID addr, MEM_INFO *out, SIZE_T len)
{
    FILE      *f = fopen("/proc/self/maps", "r");
    char       line[512], perms[8];
    uintptr_t  a = (uintptr_t)addr & ~4095u, lo, hi, next = 0;
    int        found = 0;

    (void)len;
    memset(out, 0, sizeof *out);
    if (!f)
        return 0;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%lx-%lx %7s", (unsigned long *)&lo, (unsigned long *)&hi, perms) != 3)
            continue;
        if (a >= lo && a < hi) {
            out->base = (LPVOID)a;
            out->allocBase = (LPVOID)lo;
            out->size = hi - a;
            out->state = 0x1000;                    /* MEM_COMMIT */
            out->prot = out->allocProt = prot_from_maps(perms);
            out->type = 0x20000;                    /* MEM_PRIVATE */
            found = 1;
            break;
        }
        if (lo > a && (!next || lo < next))
            next = lo;
    }
    fclose(f);
    if (!found) {
        out->base = (LPVOID)a;
        out->size = next ? next - a : 0x80000000u - a;
        out->state = 0x10000;                       /* MEM_FREE */
        out->prot = 0x01;
    }
    return sizeof *out;
}

BOOL WINAPI VirtualProtect(LPVOID addr, SIZE_T size, DWORD prot, LPDWORD old)
{
    int p = 0;

    if (old)
        *old = 0x04;
    switch (prot & 0xFF) {
    case 0x01: p = PROT_NONE; break;
    case 0x02: p = PROT_READ; break;
    case 0x04: case 0x08: p = PROT_READ | PROT_WRITE; break;
    case 0x10: p = PROT_EXEC; break;
    case 0x20: p = PROT_READ | PROT_EXEC; break;
    default:   p = PROT_READ | PROT_WRITE | PROT_EXEC; break;
    }
    return mprotect((void *)((uintptr_t)addr & ~4095u), size + ((uintptr_t)addr & 4095u), p) == 0;
}

UINT  WINAPI HeapCompact(HANDLE heap, DWORD flags) { (void)heap; (void)flags; return 0; }
BOOL  WINAPI HeapValidate(HANDLE heap, DWORD flags, LPCVOID p) { (void)heap; (void)flags; (void)p; return TRUE; }
BOOL  WINAPI HeapWalk(HANDLE heap, LPVOID entry) { (void)heap; (void)entry; SetLastError(259); return FALSE; }
BOOL  WINAPI SetProcessWorkingSetSize(HANDLE p, SIZE_T lo, SIZE_T hi) { (void)p; (void)lo; (void)hi; return TRUE; }

HANDLE WINAPI LocalAlloc(UINT flags, SIZE_T n)
{
    return HeapAlloc(GetProcessHeap(), (flags & 0x40) ? 8 /* HEAP_ZERO_MEMORY */ : 0, n);
}
HANDLE WINAPI LocalFree(HANDLE h)
{
    if (h)
        HeapFree(GetProcessHeap(), 0, h);
    return NULL;
}

/* ---- synchronisation, handles, threads, processes ---------------------------------------------------- */

LONG WINAPI w32l_InterlockedCompareExchange(volatile LONG *dst, LONG exch, LONG cmp)
{
    return __sync_val_compare_and_swap(dst, cmp, exch);
}

BOOL WINAPI InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION cs, DWORD spin)
{
    (void)spin;
    InitializeCriticalSection(cs);
    return TRUE;
}

BOOL WINAPI DisableThreadLibraryCalls(HMODULE m) { (void)m; return TRUE; }

HANDLE WINAPI CreateEventW(LPSECURITY_ATTRIBUTES sa, BOOL manual, BOOL init, LPCWSTR name)
{
    U(n, name);
    return CreateEventA(sa, manual, init, name ? n : NULL);
}
HANDLE WINAPI CreateMutexW(LPSECURITY_ATTRIBUTES sa, BOOL owner, LPCWSTR name)
{
    U(n, name);
    return CreateMutexA(sa, owner, name ? n : NULL);
}
HANDLE WINAPI OpenEventW(DWORD access, BOOL inherit, LPCWSTR name)
{
    (void)access; (void)inherit; (void)name;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return NULL;
}

/* A counting semaphore is not modelled: an auto-reset event stands in, which
 * is right for the compiler's use (a one-token handoff). */
HANDLE WINAPI CreateSemaphoreW(LPSECURITY_ATTRIBUTES sa, LONG initial, LONG max, LPCWSTR name)
{
    (void)max;
    return CreateEventW(sa, FALSE, initial > 0, name);
}
BOOL WINAPI ReleaseSemaphore(HANDLE h, LONG n, LPLONG prev)
{
    (void)n;
    if (prev)
        *prev = 0;
    return SetEvent(h);
}

BOOL WINAPI DuplicateHandle(HANDLE srcProc, HANDLE src, HANDLE dstProc, HANDLE *out, DWORD access,
                            BOOL inherit, DWORD options)
{
    W32L_Handle *h = (W32L_Handle *)src;

    (void)srcProc; (void)dstProc; (void)access; (void)inherit; (void)options;
    if (!h || src == INVALID_HANDLE_VALUE) {
        SetLastError(6);
        return FALSE;
    }
    if ((intptr_t)src < 0 && (intptr_t)src > -0x10000) {    /* pseudo-handles */
        *out = src;
        return TRUE;
    }
    __sync_add_and_fetch(&h->refs, 1);
    *out = src;
    return TRUE;
}

void  WINAPI ExitThread(DWORD code) { (void)code; pthread_exit(NULL); }
DWORD WINAPI ResumeThread(HANDLE t) { (void)t; return 0; }
BOOL  WINAPI GetExitCodeProcess(HANDLE p, LPDWORD code) { (void)p; (void)code; SetLastError(6); return FALSE; }
HANDLE WINAPI OpenProcess(DWORD access, BOOL inherit, DWORD pid) { (void)access; (void)inherit; (void)pid; SetLastError(5); return NULL; }

BOOL WINAPI CreateProcessA(LPCSTR app, LPSTR cmd, LPVOID pa, LPVOID ta, BOOL inherit, DWORD flags,
                           LPVOID env, LPCSTR dir, LPVOID si, LPVOID pi)
{
    (void)app; (void)cmd; (void)pa; (void)ta; (void)inherit; (void)flags; (void)env; (void)dir; (void)si; (void)pi;
    SetLastError(5);            /* ERROR_ACCESS_DENIED: a host that runs programs provides its own */
    return FALSE;
}
BOOL WINAPI CreateProcessW(LPCWSTR app, LPWSTR cmd, LPVOID pa, LPVOID ta, BOOL inherit, DWORD flags,
                           LPVOID env, LPCWSTR dir, LPVOID si, LPVOID pi)
{
    (void)app; (void)cmd; (void)pa; (void)ta; (void)inherit; (void)flags; (void)env; (void)dir; (void)si; (void)pi;
    SetLastError(5);
    return FALSE;
}
BOOL WINAPI CreatePipe(HANDLE *r, HANDLE *w, LPSECURITY_ATTRIBUTES sa, DWORD size)
{
    (void)r; (void)w; (void)sa; (void)size;
    SetLastError(5);
    return FALSE;
}
BOOL WINAPI PeekNamedPipe(HANDLE h, LPVOID b, DWORD n, LPDWORD got, LPDWORD avail, LPDWORD left)
{
    (void)h; (void)b; (void)n; (void)got; (void)avail; (void)left;
    SetLastError(109);          /* ERROR_BROKEN_PIPE */
    return FALSE;
}

/* ---- console, locale, misc ------------------------------------------------------------------------------ */

BOOL WINAPI GetConsoleScreenBufferInfo(HANDLE h, LPVOID info) { (void)h; (void)info; SetLastError(6); return FALSE; }
BOOL WINAPI SetConsoleMode(HANDLE h, DWORD mode) { (void)h; (void)mode; return TRUE; }
BOOL WINAPI GetNumberOfConsoleInputEvents(HANDLE h, LPDWORD n) { (void)h; if (n) *n = 0; return FALSE; }
BOOL WINAPI PeekConsoleInputA(HANDLE h, LPVOID b, DWORD n, LPDWORD got) { (void)h; (void)b; (void)n; if (got) *got = 0; return FALSE; }
BOOL WINAPI ReadConsoleInputA(HANDLE h, LPVOID b, DWORD n, LPDWORD got) { (void)h; (void)b; (void)n; if (got) *got = 0; return FALSE; }
BOOL WINAPI ReadConsoleInputW(HANDLE h, LPVOID b, DWORD n, LPDWORD got) { (void)h; (void)b; (void)n; if (got) *got = 0; return FALSE; }
BOOL WINAPI ReadConsoleA(HANDLE h, LPVOID b, DWORD n, LPDWORD got, LPVOID c) { (void)h; (void)b; (void)n; (void)c; if (got) *got = 0; return FALSE; }
BOOL WINAPI ReadConsoleW(HANDLE h, LPVOID b, DWORD n, LPDWORD got, LPVOID c) { (void)h; (void)b; (void)n; (void)c; if (got) *got = 0; return FALSE; }
BOOL WINAPI Beep(DWORD freq, DWORD ms) { (void)freq; (void)ms; return TRUE; }
UINT WINAPI SetErrorMode(UINT mode) { (void)mode; return 0; }

LCID  WINAPI GetThreadLocale(void) { return 0x0409; }
WORD  WINAPI GetUserDefaultUILanguage(void) { return 0x0409; }
BOOL  WINAPI IsDBCSLeadByteEx(UINT cp, BYTE c) { (void)cp; (void)c; return FALSE; }
int32_t WINAPI lstrlenW(LPCWSTR s) { int32_t n = 0; while (s && s[n]) n++; return n; }
int32_t WINAPI lstrcmpiW(LPCWSTR a, LPCWSTR b)
{
    for (;; a++, b++) {
        WCHAR x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = (WCHAR)(x + 32);
        if (y >= 'A' && y <= 'Z') y = (WCHAR)(y + 32);
        if (x != y)
            return x < y ? -1 : 1;
        if (!x)
            return 0;
    }
}

BOOL WINAPI GetComputerNameW(LPWSTR out, LPDWORD n)
{
    char host[64];

    if (gethostname(host, sizeof host) != 0)
        strcpy(host, "LOCALHOST");
    {
        DWORD need = (DWORD)strlen(host);
        if (!n || !out || *n <= need) {
            if (n)
                *n = need + 1;
            SetLastError(111);      /* ERROR_BUFFER_OVERFLOW */
            return FALSE;
        }
        *n = put_wide(host, out, *n);
    }
    return TRUE;
}

/* A message the host has no catalogue for: its number, as text. The compiler
 * reads its own messages from clui.dll resources, not from here. */
DWORD WINAPI FormatMessageW(DWORD flags, LPCVOID src, DWORD id, DWORD lang, LPWSTR out, DWORD cap, LPVOID args)
{
    WCHAR msg[64];
    char  a[64];
    DWORD n;

    (void)flags; (void)src; (void)lang; (void)args;
    snprintf(a, sizeof a, "Error %u", (unsigned)id);
    n = (DWORD)u2w(a, msg, 64);
    if (flags & 0x100) {            /* FORMAT_MESSAGE_ALLOCATE_BUFFER */
        WCHAR *p = (WCHAR *)LocalAlloc(0, (n + 1) * sizeof(WCHAR));
        if (!p)
            return 0;
        memcpy(p, msg, (n + 1) * sizeof(WCHAR));
        *(WCHAR **)out = p;
        return n;
    }
    if (!out || n + 1 > cap)
        return 0;
    memcpy(out, msg, (n + 1) * sizeof(WCHAR));
    return n;
}

/* ---- module handles --------------------------------------------------------------------------------------- */

DWORD WINAPI GetModuleFileNameW(HMODULE m, LPWSTR out, DWORD cap)
{
    char a[PATH_BUF];

    GetModuleFileNameA(m, a, sizeof a);
    return put_wide(a, out, cap);
}
HMODULE WINAPI GetModuleHandleW(LPCWSTR name)
{
    U(a, name);
    return GetModuleHandleA(name ? a : NULL);
}
HMODULE WINAPI LoadLibraryW(LPCWSTR name)
{
    U(a, name);
    return LoadLibraryA(a);
}
HMODULE WINAPI LoadLibraryExW(LPCWSTR name, HANDLE file, DWORD flags)
{
    (void)file; (void)flags;
    return LoadLibraryW(name);
}

typedef struct { DWORD size, major, minor, build, platform; WCHAR csd[128];
                 WORD spMajor, spMinor, suite; BYTE product, reserved; } OSVERSIONINFOEXW_;

BOOL WINAPI GetVersionExW(OSVERSIONINFOEXW_ *v)
{
    if (!v || v->size < 20 + 256)
        return FALSE;
    v->major = w32l_os_version.major;
    v->minor = w32l_os_version.minor;
    v->build = w32l_os_version.build;
    v->platform = w32l_os_version.platform;
    v->csd[0] = 0;
    if (v->size >= sizeof *v) {
        v->spMajor = v->spMinor = v->suite = 0;
        v->product = 1;             /* VER_NT_WORKSTATION */
    }
    return TRUE;
}

/* ---- ADVAPI32, SHELL32, msvcrt ---------------------------------------------------------------------------- */

#define ERROR_CALL_NOT_IMPLEMENTED 120

LONG WINAPI RegOpenKeyExW(HKEY k, LPCWSTR sub, DWORD opts, DWORD sam, PHKEY out)
{
    (void)k; (void)sub; (void)opts; (void)sam; (void)out;
    return ERROR_FILE_NOT_FOUND;
}
LONG WINAPI RegQueryValueExW(HKEY k, LPCWSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD n)
{
    (void)k; (void)name; (void)res; (void)type; (void)data; (void)n;
    return ERROR_FILE_NOT_FOUND;
}
BOOL WINAPI OpenProcessToken(HANDLE p, DWORD access, PHANDLE tok) { (void)p; (void)access; (void)tok; SetLastError(5); return FALSE; }
BOOL WINAPI GetTokenInformation(HANDLE t, int32_t cls, LPVOID info, DWORD len, LPDWORD ret)
{ (void)t; (void)cls; (void)info; (void)len; (void)ret; SetLastError(5); return FALSE; }

BOOL WINAPI CryptAcquireContextA(ULONG_PTR *prov, LPCSTR c, LPCSTR p, DWORD type, DWORD flags)
{ (void)c; (void)p; (void)type; (void)flags; *prov = 1; return TRUE; }
BOOL WINAPI CryptAcquireContextW(ULONG_PTR *prov, LPCWSTR c, LPCWSTR p, DWORD type, DWORD flags)
{ (void)c; (void)p; (void)type; (void)flags; *prov = 1; return TRUE; }
BOOL WINAPI CryptReleaseContext(ULONG_PTR prov, DWORD flags) { (void)prov; (void)flags; return TRUE; }
BOOL WINAPI CryptGenRandom(ULONG_PTR prov, DWORD n, BYTE *buf)
{
    FILE  *f = fopen("/dev/urandom", "rb");
    size_t got = 0;

    (void)prov;
    if (f) {
        got = fread(buf, 1, n, f);
        fclose(f);
    }
    return got == n;
}
BOOL WINAPI CryptCreateHash(ULONG_PTR p, DWORD alg, ULONG_PTR key, DWORD flags, ULONG_PTR *h)
{ (void)p; (void)alg; (void)key; (void)flags; (void)h; SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE; }
BOOL WINAPI CryptHashData(ULONG_PTR h, const BYTE *d, DWORD n, DWORD flags)
{ (void)h; (void)d; (void)n; (void)flags; SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE; }
BOOL WINAPI CryptGetHashParam(ULONG_PTR h, DWORD p, BYTE *d, DWORD *n, DWORD flags)
{ (void)h; (void)p; (void)d; (void)n; (void)flags; SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE; }
BOOL WINAPI CryptDestroyHash(ULONG_PTR h) { (void)h; return TRUE; }

HRESULT WINAPI SHGetFolderPathW(HWND hwnd, int32_t csidl, HANDLE token, DWORD flags, LPWSTR out)
{
    char    a[PATH_BUF];
    HRESULT hr = SHGetFolderPathA(hwnd, csidl, token, flags, a);

    if (hr >= 0)
        put_wide(a, out, MAX_PATH_W);
    return hr;
}

/* ---- the table -------------------------------------------------------------------------------------------- */

#define XK(fn) { "KERNEL32.dll", #fn, (const void *)&fn }
#define XA(fn) { "ADVAPI32.dll", #fn, (const void *)&fn }
extern const W32L_Export w32l_exports_w[];
extern const size_t      w32l_exports_w_count;
const W32L_Export w32l_exports_w[] = {
    XK(GetEnvironmentVariableA), XK(GetEnvironmentVariableW), XK(SetEnvironmentVariableW),
    XK(ExpandEnvironmentStringsA), XK(ExpandEnvironmentStringsW), XK(GetCommandLineW),
    XK(GetStartupInfoW), XK(CreateDirectoryW), XK(DeleteFileW), XK(RemoveDirectoryW),
    XK(SetCurrentDirectoryW), XK(GetFileAttributesW), XK(SetFileAttributesW),
    XK(GetCurrentDirectoryW), XK(GetFullPathNameW), XK(GetLongPathNameW), XK(GetShortPathNameW),
    XK(GetSystemDirectoryW), XK(GetTempPathW), XK(GetTempFileNameW), XK(MoveFileA), XK(MoveFileW),
    XK(CopyFileW), XK(GetFileAttributesExW), XK(GetFileInformationByHandle), XK(GetFileSize),
    XK(GetFileTime), XK(SetFileTime), XK(LocalFileTimeToFileTime), XK(SystemTimeToFileTime),
    XK(DosDateTimeToFileTime), XK(FileTimeToDosDateTime),
    XK(SetLocalTime), XK(LockFile), XK(UnlockFile), XK(LockFileEx), XK(UnlockFileEx),
    XK(FindFirstFileW), XK(FindNextFileW), XK(GetDriveTypeW), XK(GetLogicalDrives),
    XK(GetDiskFreeSpaceA), XK(GetDiskFreeSpaceExW), XK(CreateFileMappingA), XK(CreateFileMappingW),
    XK(OpenFileMappingW), XK(MapViewOfFile), XK(MapViewOfFileEx), XK(UnmapViewOfFile),
    XK(FlushViewOfFile), XK(GetSystemInfo), XK(VirtualQuery), XK(VirtualProtect), XK(HeapCompact),
    XK(HeapValidate), XK(HeapWalk), XK(SetProcessWorkingSetSize), XK(LocalAlloc), XK(LocalFree),
    { "KERNEL32.dll", "InterlockedCompareExchange", (const void *)&w32l_InterlockedCompareExchange },
    XK(InitializeCriticalSectionAndSpinCount), XK(DisableThreadLibraryCalls), XK(CreateEventW),
    XK(CreateMutexW), XK(OpenEventW), XK(CreateSemaphoreW), XK(ReleaseSemaphore), XK(DuplicateHandle),
    XK(ExitThread), XK(ResumeThread), XK(GetExitCodeProcess), XK(OpenProcess), XK(CreateProcessA),
    XK(CreateProcessW), XK(CreatePipe), XK(PeekNamedPipe), XK(GetConsoleScreenBufferInfo),
    XK(SetConsoleMode), XK(GetNumberOfConsoleInputEvents), XK(PeekConsoleInputA),
    XK(ReadConsoleInputA), XK(ReadConsoleInputW), XK(ReadConsoleA), XK(ReadConsoleW), XK(Beep),
    XK(SetErrorMode), XK(GetThreadLocale), XK(GetUserDefaultUILanguage), XK(IsDBCSLeadByteEx),
    XK(lstrlenW), XK(lstrcmpiW), XK(GetComputerNameW), XK(FormatMessageW), XK(GetModuleFileNameW),
    XK(GetModuleHandleW), XK(LoadLibraryW), XK(LoadLibraryExW), XK(GetVersionExW),
    XA(RegOpenKeyExW), XA(RegQueryValueExW), XA(OpenProcessToken), XA(GetTokenInformation),
    XA(CryptAcquireContextA), XA(CryptAcquireContextW), XA(CryptReleaseContext), XA(CryptGenRandom),
    XA(CryptCreateHash), XA(CryptHashData), XA(CryptGetHashParam), XA(CryptDestroyHash),
    { "SHELL32.dll", "SHGetFolderPathW", (const void *)&SHGetFolderPathW },
    { "msvcrt.dll", "_getdrives", (const void *)&w32l_getdrives },
};
const size_t w32l_exports_w_count = sizeof w32l_exports_w / sizeof w32l_exports_w[0];
