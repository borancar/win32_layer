/* kernel32.cpp -- time, modules and the import table, threads and events,
 * memory probes, the exception hook, the registry, drives, and the
 * Winsock names control.c uses. See platform.h for the layer's shape.
 */
#include "platform.h"
#include "handle.h"
#include <winsock2.h>
#include <io.h>
#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>
#include <dplobby.h>
#include <direct.h>

#include <SDL3/SDL.h>

#include <pthread.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* winsock2.h routes control.c's setsockopt through the shim below; this
 * file calls the real one. */
#undef setsockopt

/* ---- logging ---------------------------------------------------------------- */

void w32l_plat_log(const char *fmt, ...)
{
    static int quiet = -1;
    va_list ap;

    if (quiet < 0) {
        const char *q = getenv("W32L_PLATFORM_QUIET");
        quiet = q && *q && *q != '0';
    }
    if (quiet)
        return;
    fputs("platform: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void w32l_plat_debug(const char *fmt, ...)
{
    static int on = -1;
    va_list ap;

    if (on < 0) {
        const char *d = getenv("W32L_PLATFORM_DEBUG");
        on = d && *d && *d != '0';
    }
    if (!on)
        return;
    fputs("platform: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* ---- last error --------------------------------------------------------------- */

static __thread DWORD w32l_last_error;

DWORD WINAPI GetLastError(void) { return w32l_last_error; }
void  WINAPI SetLastError(DWORD err) { w32l_last_error = err; }

/* ---- time ------------------------------------------------------------------ */

/* All four clocks are one clock, so that under W32L_LOCKSTEP the game's
 * frame clock, its animation ticks and its timestamps agree with the
 * pump count and with nothing else. See platform.h. */
DWORD WINAPI GetTickCount(void)
{
    return (DWORD)(w32l_host_clock_ns() / 1000000ULL);
}

DWORD WINAPI timeGetTime(void)
{
    return (DWORD)(w32l_host_clock_ns() / 1000000ULL);
}

void WINAPI Sleep(DWORD ms)
{
    if (w32l_host_lockstep() && w32l_host_on_game_thread()) {
        w32l_host_clock_advance_ms(ms);
        return;
    }
    if (ms == 0)
        sched_yield();
    else
        SDL_Delay(ms);
}

BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *out)
{
    out->QuadPart = (LONGLONG)w32l_host_qpc_ns();
    return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *out)
{
    out->QuadPart = w32l_host_lockstep() ? 1000000000LL
                                        : (LONGLONG)SDL_GetPerformanceFrequency();
    return TRUE;
}

/* ---- modules and the import table --------------------------------------------- */

/* The original's IAT is carried in .origdat and bound at startup by the
 * application's import binder, which asks for each entry by module and name.
 * Sites in ArmyMen2's src/game then call through those slots -- GetTickCount from the
 * AI and comm timers, IntersectRect from the row layer -- so the answers have
 * to be real functions with the right convention.
 *
 * A name in a module we emulate that is not in the table is NULL from
 * GetProcAddress (as on Windows) and, to a PE loader, w32l_export_lookup's
 * NULL, which the loader turns into a stub of its own that names it. Most of
 * the kernel32 list is the MSVC CRT's own imports, which nothing in this tree
 * calls. smackw32 has no entries: ArmyMen2's native port calls its own
 * smackw32 (src/game/win32/smackw32.cpp, over libsmacker) by name. */

extern const W32L_Export w32l_exports_w[];   /* kernel32w.cpp */
extern const size_t      w32l_exports_w_count;

#define X(mod, fn) { mod, #fn, (const void *)&fn }
static const W32L_Export w32l_exports[] = {
    X("ADVAPI32.dll", RegCreateKeyExA), X("ADVAPI32.dll", RegCloseKey),
    X("ADVAPI32.dll", RegQueryValueExA), X("ADVAPI32.dll", RegSetValueExA),
    X("ADVAPI32.dll", RegEnumKeyExA), X("ADVAPI32.dll", RegDeleteKeyA),
    X("ADVAPI32.dll", RegDeleteValueA), X("ADVAPI32.dll", RegFlushKey),
    X("ADVAPI32.dll", RegOpenKeyA), X("ADVAPI32.dll", RegCreateKeyA),
    X("DDRAW.dll", DirectDrawCreate),
    X("DINPUT.dll", DirectInputCreateA),
    X("DSOUND.dll", DirectSoundCreate),
    X("GDI32.dll", RealizePalette), X("GDI32.dll", CreatePalette),
    X("GDI32.dll", SetSystemPaletteUse), X("GDI32.dll", GetPixel),
    X("GDI32.dll", SelectObject), X("GDI32.dll", GetSystemPaletteEntries),
    X("GDI32.dll", CreateFontA), X("GDI32.dll", TextOutA),
    X("GDI32.dll", GetTextExtentPoint32A), X("GDI32.dll", SetTextColor),
    X("GDI32.dll", SetBkMode), X("GDI32.dll", DeleteObject),
    X("GDI32.dll", SelectPalette), X("GDI32.dll", GetDeviceCaps),
    X("KERNEL32.dll", ReleaseMutex), X("KERNEL32.dll", CreateMutexA),
    X("KERNEL32.dll", OpenMutexA), X("KERNEL32.dll", GetExitCodeThread),
    X("KERNEL32.dll", CloseHandle), X("KERNEL32.dll", SetEvent),
    X("KERNEL32.dll", WaitForSingleObject), X("KERNEL32.dll", WaitForMultipleObjects),
    X("KERNEL32.dll", GetTickCount), X("KERNEL32.dll", GetProcAddress),
    X("KERNEL32.dll", GlobalFree), X("KERNEL32.dll", GlobalAlloc),
    X("KERNEL32.dll", Sleep), X("KERNEL32.dll", GetVolumeInformationA),
    X("KERNEL32.dll", GetDriveTypeA), X("KERNEL32.dll", GetLogicalDriveStringsA),
    X("KERNEL32.dll", QueryPerformanceCounter), X("KERNEL32.dll", QueryPerformanceFrequency),
    X("KERNEL32.dll", lstrlenA), X("KERNEL32.dll", GetModuleFileNameA),
    X("KERNEL32.dll", IsBadReadPtr), X("KERNEL32.dll", IsBadWritePtr),
    X("KERNEL32.dll", LoadLibraryA), X("KERNEL32.dll", FreeLibrary),
    X("KERNEL32.dll", SetThreadPriority), X("KERNEL32.dll", CreateThread),
    X("KERNEL32.dll", CreateEventA), X("KERNEL32.dll", GetModuleHandleA),
    X("KERNEL32.dll", SetCurrentDirectoryA), X("KERNEL32.dll", GetCurrentDirectoryA),
    X("KERNEL32.dll", GetLastError),
    /* The rest of KERNEL32 is what the MSVC 6 CRT inside the original
     * imports, provided by kernel32crt.cpp for the PE loader. */
    X("KERNEL32.dll", CompareStringA), X("KERNEL32.dll", CompareStringW),
    X("KERNEL32.dll", CreateDirectoryA), X("KERNEL32.dll", CreateFileA),
    X("KERNEL32.dll", DeleteFileA), X("KERNEL32.dll", ExitProcess),
    X("KERNEL32.dll", FileTimeToLocalFileTime), X("KERNEL32.dll", FileTimeToSystemTime),
    X("KERNEL32.dll", FindClose), X("KERNEL32.dll", FindFirstFileA),
    X("KERNEL32.dll", FindNextFileA), X("KERNEL32.dll", FlushFileBuffers),
    X("KERNEL32.dll", FreeEnvironmentStringsA), X("KERNEL32.dll", FreeEnvironmentStringsW),
    X("KERNEL32.dll", GetACP), X("KERNEL32.dll", GetCommandLineA),
    X("KERNEL32.dll", GetCPInfo), X("KERNEL32.dll", GetCurrentProcess),
    X("KERNEL32.dll", GetEnvironmentStrings), X("KERNEL32.dll", GetEnvironmentStringsW),
    X("KERNEL32.dll", GetFileAttributesA), X("KERNEL32.dll", GetFileType),
    X("KERNEL32.dll", GetFullPathNameA), X("KERNEL32.dll", GetLocalTime),
    X("KERNEL32.dll", GetOEMCP), X("KERNEL32.dll", GetStartupInfoA),
    X("KERNEL32.dll", GetStdHandle), X("KERNEL32.dll", GetStringTypeA),
    X("KERNEL32.dll", GetStringTypeW), X("KERNEL32.dll", GetSystemTime),
    X("KERNEL32.dll", GetTimeZoneInformation), X("KERNEL32.dll", GetVersion),
    X("KERNEL32.dll", HeapAlloc), X("KERNEL32.dll", HeapCreate),
    X("KERNEL32.dll", HeapDestroy), X("KERNEL32.dll", HeapFree),
    X("KERNEL32.dll", HeapReAlloc), X("KERNEL32.dll", HeapSize),
    { "KERNEL32.dll", "InterlockedExchange", (const void *)&w32l_InterlockedExchange },
    X("KERNEL32.dll", IsBadCodePtr), X("KERNEL32.dll", LCMapStringA),
    X("KERNEL32.dll", LCMapStringW), X("KERNEL32.dll", MultiByteToWideChar),
    X("KERNEL32.dll", ReadFile), X("KERNEL32.dll", RemoveDirectoryA),
    X("KERNEL32.dll", RtlUnwind), X("KERNEL32.dll", SetEndOfFile),
    X("KERNEL32.dll", SetEnvironmentVariableA), X("KERNEL32.dll", SetFileAttributesA),
    X("KERNEL32.dll", SetFilePointer), X("KERNEL32.dll", SetHandleCount),
    X("KERNEL32.dll", SetStdHandle), X("KERNEL32.dll", SetUnhandledExceptionFilter),
    X("KERNEL32.dll", TerminateProcess), X("KERNEL32.dll", UnhandledExceptionFilter),
    X("KERNEL32.dll", VirtualAlloc), X("KERNEL32.dll", VirtualFree),
    X("KERNEL32.dll", WideCharToMultiByte), X("KERNEL32.dll", WriteFile),
    X("USER32.dll", PostMessageA), X("USER32.dll", EndPaint),
    X("USER32.dll", BeginPaint), X("USER32.dll", PostQuitMessage),
    X("USER32.dll", RedrawWindow), X("USER32.dll", GetUpdateRect),
    X("USER32.dll", DefWindowProcA), X("USER32.dll", SetRect),
    X("USER32.dll", GetSystemMetrics), X("USER32.dll", IsIconic),
    X("USER32.dll", ClientToScreen), X("USER32.dll", GetClientRect),
    X("USER32.dll", SetWindowPos), X("USER32.dll", GetWindowRect),
    X("USER32.dll", SystemParametersInfoA), X("USER32.dll", SetWindowLongA),
    X("USER32.dll", AdjustWindowRectEx), X("USER32.dll", GetMenu),
    X("USER32.dll", TranslateMessage), X("USER32.dll", GetWindowLongA),
    X("USER32.dll", DispatchMessageA), X("USER32.dll", GetFocus),
    X("USER32.dll", SetFocus), X("USER32.dll", UpdateWindow),
    X("USER32.dll", GetDC), X("USER32.dll", ShowCursor),
    X("USER32.dll", wsprintfA), X("USER32.dll", MessageBoxA),
    X("USER32.dll", ReleaseDC), X("USER32.dll", GetActiveWindow),
    X("USER32.dll", RegisterClassA), X("USER32.dll", LoadIconA),
    X("USER32.dll", LoadCursorA), X("USER32.dll", SetCursor),
    X("USER32.dll", CreateWindowExA), X("USER32.dll", IntersectRect),
    X("USER32.dll", PeekMessageA), X("USER32.dll", DestroyWindow),
    X("WINMM.dll", mmioDescend), X("WINMM.dll", mmioRead), X("WINMM.dll", mmioAdvance),
    X("WINMM.dll", timeBeginPeriod), X("WINMM.dll", timeSetEvent),
    X("WINMM.dll", timeKillEvent), X("WINMM.dll", timeEndPeriod),
    X("WINMM.dll", mmioGetInfo), X("WINMM.dll", mmioClose), X("WINMM.dll", mmioSetInfo),
    X("WINMM.dll", mmioSeek), X("WINMM.dll", mmioOpenA), X("WINMM.dll", mmioAscend),
    X("ole32.dll", CoInitialize), X("ole32.dll", CoCreateInstance),
    /* Added for Commandos: the rest of what a CRT-and-window game imports
     * (kernel32ext.cpp, user32ext.cpp, and implementations that were already
     * here but had no export). */
    X("KERNEL32.dll", CopyFileA), X("KERNEL32.dll", CreateFileW),
    X("KERNEL32.dll", DebugBreak), X("KERNEL32.dll", EnumSystemLocalesA),
    X("KERNEL32.dll", FatalAppExitA), X("KERNEL32.dll", GetConsoleCP),
    X("KERNEL32.dll", GetConsoleMode), X("KERNEL32.dll", GetConsoleOutputCP),
    X("KERNEL32.dll", GetCurrentProcessId),
    X("KERNEL32.dll", GetCurrentThread),
    X("KERNEL32.dll", GetCurrentThreadId), X("KERNEL32.dll", GetDateFormatA),
    X("KERNEL32.dll", GetLocaleInfoA), X("KERNEL32.dll", GetLocaleInfoW),
    X("KERNEL32.dll", GetProcessHeap),
    X("KERNEL32.dll", GetSystemDefaultLangID),
    X("KERNEL32.dll", GetSystemTimeAsFileTime),
    X("KERNEL32.dll", GetTimeFormatA), X("KERNEL32.dll", GetUserDefaultLCID),
    X("KERNEL32.dll", GetVersionExA), X("KERNEL32.dll", IsDebuggerPresent),
    X("KERNEL32.dll", IsValidCodePage), X("KERNEL32.dll", IsValidLocale),
    X("KERNEL32.dll", lstrcatA), X("KERNEL32.dll", lstrcmpA),
    X("KERNEL32.dll", lstrcmpiA), X("KERNEL32.dll", lstrcpyA),
    X("KERNEL32.dll", lstrcpynA), X("KERNEL32.dll", OutputDebugStringA),
    X("KERNEL32.dll", RaiseException),
    X("KERNEL32.dll", SetConsoleCtrlHandler), X("KERNEL32.dll", TlsAlloc),
    X("KERNEL32.dll", TlsFree), X("KERNEL32.dll", TlsGetValue),
    X("KERNEL32.dll", TlsSetValue), X("KERNEL32.dll", WriteConsoleA),
    X("KERNEL32.dll", WriteConsoleW),
    X("KERNEL32.dll", DeleteCriticalSection),
    X("KERNEL32.dll", EnterCriticalSection),
    X("KERNEL32.dll", InitializeCriticalSection),
    X("KERNEL32.dll", LeaveCriticalSection), X("KERNEL32.dll", SetLastError),
    { "KERNEL32.dll", "InterlockedIncrement", (const void *)&w32l_InterlockedIncrement },
    { "KERNEL32.dll", "InterlockedDecrement", (const void *)&w32l_InterlockedDecrement },
    X("USER32.dll", BringWindowToTop), X("USER32.dll", CheckDlgButton),
    X("USER32.dll", CreateDialogParamA), X("USER32.dll", DestroyMenu),
    X("USER32.dll", DialogBoxParamA), X("USER32.dll", EndDialog),
    X("USER32.dll", GetCursorPos), X("USER32.dll", GetDesktopWindow),
    X("USER32.dll", GetDlgCtrlID), X("USER32.dll", GetDlgItem),
    X("USER32.dll", GetDlgItemInt), X("USER32.dll", GetDlgItemTextA),
    X("USER32.dll", GetForegroundWindow), X("USER32.dll", GetKeyState),
    X("USER32.dll", GetMenuItemCount), X("USER32.dll", GetMenuItemInfoA),
    X("USER32.dll", GetSubMenu), X("USER32.dll", InvalidateRect),
    X("USER32.dll", IsDlgButtonChecked), X("USER32.dll", LoadMenuA),
    X("USER32.dll", MapVirtualKeyA), X("USER32.dll", MoveWindow),
    X("USER32.dll", RegisterClassExA), X("USER32.dll", ReleaseCapture),
    X("USER32.dll", SendDlgItemMessageA), X("USER32.dll", SendMessageA),
    X("USER32.dll", SetActiveWindow), X("USER32.dll", SetCapture),
    X("USER32.dll", SetCursorPos), X("USER32.dll", SetDlgItemInt),
    X("USER32.dll", SetDlgItemTextA), X("USER32.dll", SetWindowTextA),
    X("USER32.dll", ShowWindow), X("USER32.dll", TrackPopupMenu),
    X("USER32.dll", UnregisterClassA), X("USER32.dll", WindowFromPoint),
    X("USER32.dll", wvsprintfA),
    X("WINMM.dll", timeGetTime),
    X("ADVAPI32.dll", RegOpenKeyExA),
    X("ADVAPI32.dll", RegQueryInfoKeyA),
    X("ADVAPI32.dll", RegEnumValueA),
    X("SHLWAPI.dll", PathFileExistsA),
    X("SHLWAPI.dll", PathAppendA),
    X("SHELL32.dll", SHGetFolderPathA),
    X("SHELL32.dll", SHCreateDirectoryExA),
    X("SHELL32.dll", DragAcceptFiles),
    X("SHELL32.dll", DragQueryFileA),
    X("SHELL32.dll", DragFinish),
    X("ole32.dll", CoUninitialize),
    X("GDI32.dll", SetBkColor),
    X("GDI32.dll", SetPixel),
    X("GDI32.dll", ExtTextOutA),
    X("GDI32.dll", GetTextExtentPointA),
    X("GDI32.dll", GetStockObject),
};
#undef X

static const char *const w32l_modules[] = {
    "ADVAPI32.dll", "DDRAW.dll", "DINPUT.dll", "DSOUND.dll", "GDI32.dll",
    "KERNEL32.dll", "USER32.dll", "WINMM.dll", "ole32.dll", "smackw32.dll",
};

HMODULE WINAPI LoadLibraryA(LPCSTR name)
{
    size_t i;

    if (!name)
        return NULL;
    for (i = 0; i < sizeof w32l_modules / sizeof w32l_modules[0]; i++)
        if (!strcasecmp(w32l_modules[i], name))
            return (HMODULE)(uintptr_t)(i + 1);
    /* cpuinf32.dll is the one the game itself asks for, and answering NULL
     * takes the path the original takes without it: "Missing cpuinf32.dll"
     * and the default machine speed. */
    w32l_last_error = ERROR_FILE_NOT_FOUND;
    return NULL;
}

BOOL WINAPI FreeLibrary(HMODULE mod) { (void)mod; return TRUE; }

HMODULE WINAPI GetModuleHandleA(LPCSTR name)
{
    if (!name)
        return (HMODULE)(uintptr_t)0x400000u;
    return LoadLibraryA(name);
}

FARPROC WINAPI GetProcAddress(HMODULE mod, LPCSTR name)
{
    size_t      idx = (size_t)(uintptr_t)mod;
    const char *module;
    size_t      i;
    char        ordinal[16];

    if (idx == 0 || idx > sizeof w32l_modules / sizeof w32l_modules[0])
        return NULL;
    module = w32l_modules[idx - 1];

    /* An import by ordinal arrives as a small integer. The one the game has
     * is DSOUND's #1, DirectSoundCreate. */
    if ((uintptr_t)name < 0x10000) {
        if (!strcasecmp(module, "DSOUND.dll") && (uintptr_t)name == 1)
            name = "DirectSoundCreate";
        else {
            snprintf(ordinal, sizeof ordinal, "#%u", (unsigned)(uintptr_t)name);
            name = ordinal;
        }
    }

    for (i = 0; i < sizeof w32l_exports / sizeof w32l_exports[0]; i++)
        if (!strcmp(w32l_exports[i].module, module) &&
            !strcmp(w32l_exports[i].name, name))
            return (FARPROC)w32l_exports[i].fn;
    for (i = 0; i < w32l_exports_w_count; i++)
        if (!strcmp(w32l_exports_w[i].module, module) &&
            !strcmp(w32l_exports_w[i].name, name))
            return (FARPROC)w32l_exports_w[i].fn;

    /* What Windows does for an export the module does not have: NULL and
     * ERROR_PROC_NOT_FOUND. A program probes for newer entries this way
     * (the MSVC 7+ CRT asks for FlsAlloc and falls back to TlsAlloc if it is
     * not there), so a trap here makes it believe the entry exists and call
     * it. The layer claims Windows 98 SE (GetVersion), which has none of those. */
    w32l_plat_debug("GetProcAddress(%s, %s): not provided", module, name);
    SetLastError(127);   /* ERROR_PROC_NOT_FOUND */
    return NULL;
}

/* The loader's view of the table: NULL for a name we do not provide, so it
 * can make a trap that says which name, rather than the shared one. */
const void *w32l_export_lookup(const char *module, const char *name)
{
    size_t i;

    for (i = 0; i < sizeof w32l_exports / sizeof w32l_exports[0]; i++)
        if (!strcasecmp(w32l_exports[i].module, module) &&
            !strcmp(w32l_exports[i].name, name))
            return w32l_exports[i].fn;
    for (i = 0; i < w32l_exports_w_count; i++)
        if (!strcasecmp(w32l_exports_w[i].module, module) &&
            !strcmp(w32l_exports_w[i].name, name))
            return w32l_exports_w[i].fn;
    return w32l_stub_lookup(module, name);
}

/* The application's description of itself (platform.h). Weak: the
 * application's own definition replaces it. */
extern "C" __attribute__((weak)) const W32L_AppInfo w32l_app_info = {
    "Win32 application", "0.0", "org.win32_layer.app", "C:\\APP.EXE"
};

/* CANONICAL, like the environment (kernel32crt.cpp): the CRT's startup
 * copies this into its heap as the program name, so a binary's own path
 * here -- two builds of one program run side by side -- shifted every heap
 * address after it between the two. It comes from w32l_app_info. */
DWORD WINAPI GetModuleFileNameA(HMODULE mod, LPSTR out, DWORD cap)
{
    const char *kName = w32l_app_info.module_path;
    size_t n = strlen(kName);

    (void)mod;
    if (!out || cap == 0)
        return 0;
    if (n > cap - 1)
        n = cap - 1;
    memcpy(out, kName, n);
    out[n] = 0;
    return (DWORD)n;
}

/* ---- memory probes and heaps --------------------------------------------- */

/* A page is readable if mincore knows it: ENOMEM says it is not mapped.
 * That is what the original's logger guard needs -- runtime.cpp asks
 * whether a "format string" the game handed it with an empty argument
 * frame is a string at all. */
static int32_t w32l_page_mapped(const void *p)
{
    uintptr_t page = (uintptr_t)p & ~(uintptr_t)4095;
    unsigned char vec;

    return mincore((void *)page, 4096, &vec) == 0;
}

static BOOL w32l_range_bad(const void *p, UINT_PTR n)
{
    uintptr_t a, end;

    if (!p)
        return TRUE;
    if (n == 0)
        return FALSE;
    a = (uintptr_t)p & ~(uintptr_t)4095;
    end = (uintptr_t)p + n - 1;
    for (; a <= end; a += 4096)
        if (!w32l_page_mapped((const void *)a))
            return TRUE;
    return FALSE;
}

BOOL WINAPI IsBadReadPtr(LPCVOID p, UINT_PTR n) { return w32l_range_bad(p, n); }
BOOL WINAPI IsBadWritePtr(LPVOID p, UINT_PTR n) { return w32l_range_bad(p, n); }

BOOL WINAPI IsBadStringPtrA(LPCSTR p, UINT_PTR max)
{
    uintptr_t a;

    if (!p)
        return TRUE;
    for (a = (uintptr_t)p; max > 0; a++, max--) {
        if ((a & 4095) == 0 || a == (uintptr_t)p)
            if (!w32l_page_mapped((const void *)a))
                return TRUE;
        if (*(const char *)a == 0)
            return FALSE;
    }
    return FALSE;
}

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T bytes)
{
    void *p = malloc(bytes ? bytes : 1);

    if (p && (flags & GMEM_ZEROINIT))
        memset(p, 0, bytes);
    return (HGLOBAL)p;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL mem)
{
    free(mem);
    return NULL;
}

/* ---- handles: threads, events, mutexes, files ------------------------------ */

/* One lock and one condition for every handle. The game has two threads
 * besides the main one and they rendezvous a handful of times a second, so
 * a shared condition is simpler than one per object and costs nothing. The
 * record itself is handle.h's, because kernel32crt.cpp makes the file and
 * find-file kinds for the original's CRT. */
static pthread_mutex_t w32l_h_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  w32l_h_cond = PTHREAD_COND_INITIALIZER;

W32L_Handle *w32l_handle_new(int32_t kind)
{
    W32L_Handle *h = (W32L_Handle *)calloc(1, sizeof *h);

    if (h) {
        h->kind = kind;
        h->refs = 1;
        h->fd = -1;
    }
    return h;
}

void w32l_handle_release(W32L_Handle *h)
{
    int32_t gone;

    pthread_mutex_lock(&w32l_h_lock);
    gone = --h->refs == 0;
    pthread_mutex_unlock(&w32l_h_lock);
    if (!gone)
        return;
    if (h->kind == W32L_H_FILE && !h->keepFd && h->fd >= 0)
        close(h->fd);
    if (h->kind == W32L_H_MAPPING && h->fd >= 0)
        close(h->fd);
    if (h->kind == W32L_H_FIND && h->find)
        _findclose(h->find);
    free(h);
}

/* Called on every thread the game creates, before its start routine, when
 * set. The PE loader hangs each thread's own TEB on it. */
void (*w32l_thread_attach)(void);

/* Call a thread's start routine without trusting its convention. The
 * game's packet thread is cdecl behind an LPTHREAD_START_ROUTINE cast --
 * dplay.cpp records the original's mismatch -- so it does not pop its
 * argument, and a plain stdcall call here left the trampoline four bytes
 * off and returning into address 8 at exit. Windows survives it because
 * its thread stub never returns through that frame; this restores the
 * stack pointer around the call, which works for either convention. */
static DWORD w32l_call_thread_start(LPTHREAD_START_ROUTINE fn, LPVOID param)
{
    DWORD rc;

    __asm__ volatile(
        "mov %%esp, %%esi\n\t"
        "push %[param]\n\t"
        "call *%[fn]\n\t"
        "mov %%esi, %%esp\n\t"
        : "=a"(rc)
        : [fn] "r"(fn), [param] "r"(param)
        : "esi", "ecx", "edx", "memory", "cc");
    return rc;
}

static void *w32l_thread_main(void *arg)
{
    W32L_Handle *h = (W32L_Handle *)arg;
    DWORD       rc;

    if (w32l_thread_attach)
        w32l_thread_attach();
    rc = w32l_call_thread_start(h->start, h->param);

    pthread_mutex_lock(&w32l_h_lock);
    h->exitCode = rc;
    h->done = 1;
    pthread_cond_broadcast(&w32l_h_cond);
    pthread_mutex_unlock(&w32l_h_lock);
    w32l_handle_release(h);
    return NULL;
}

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES attr, SIZE_T stack,
                           LPTHREAD_START_ROUTINE start, LPVOID param,
                           DWORD flags, LPDWORD id)
{
    W32L_Handle    *h = w32l_handle_new(W32L_H_THREAD);
    pthread_attr_t pa;

    (void)attr; (void)flags;
    if (!h)
        return NULL;
    h->start = start;
    h->param = param;
    h->exitCode = STILL_ACTIVE;
    h->refs = 2;                       /* the caller's handle and the thread */
    pthread_attr_init(&pa);
    if (stack)
        pthread_attr_setstacksize(&pa, stack < 65536 ? 65536 : stack);
    if (pthread_create(&h->thread, &pa, w32l_thread_main, h) != 0) {
        pthread_attr_destroy(&pa);
        free(h);
        return NULL;
    }
    pthread_attr_destroy(&pa);
    pthread_detach(h->thread);
    if (id)
        *id = (DWORD)(uintptr_t)h;
    return (HANDLE)h;
}

BOOL WINAPI GetExitCodeThread(HANDLE thread, LPDWORD code)
{
    W32L_Handle *h = (W32L_Handle *)thread;

    if (!h || h->kind != W32L_H_THREAD)
        return FALSE;
    pthread_mutex_lock(&w32l_h_lock);
    *code = h->done ? h->exitCode : STILL_ACTIVE;
    pthread_mutex_unlock(&w32l_h_lock);
    return TRUE;
}

BOOL WINAPI SetThreadPriority(HANDLE thread, int32_t prio)
{
    (void)thread; (void)prio;
    return TRUE;
}

HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES attr, BOOL manualReset,
                           BOOL initialState, LPCSTR name)
{
    W32L_Handle *h = w32l_handle_new(W32L_H_EVENT);

    (void)attr; (void)name;
    if (!h)
        return NULL;
    h->manualReset = manualReset;
    h->signaled = initialState;
    return (HANDLE)h;
}

BOOL WINAPI SetEvent(HANDLE ev)
{
    W32L_Handle *h = (W32L_Handle *)ev;

    if (!h || h->kind != W32L_H_EVENT)
        return FALSE;
    pthread_mutex_lock(&w32l_h_lock);
    h->signaled = 1;
    pthread_cond_broadcast(&w32l_h_cond);
    pthread_mutex_unlock(&w32l_h_lock);
    return TRUE;
}

BOOL WINAPI ResetEvent(HANDLE ev)
{
    W32L_Handle *h = (W32L_Handle *)ev;

    if (!h || h->kind != W32L_H_EVENT)
        return FALSE;
    pthread_mutex_lock(&w32l_h_lock);
    h->signaled = 0;
    pthread_mutex_unlock(&w32l_h_lock);
    return TRUE;
}

/* The one-instance mutex. Nothing here is cross-process, so a mutex is a
 * handle that exists: CreateMutexA makes one, OpenMutexA finds none, and
 * the game reads that as "no other copy running". */
HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES attr, BOOL owner, LPCSTR name)
{
    W32L_Handle *h = w32l_handle_new(W32L_H_MUTEX);

    (void)attr; (void)owner; (void)name;
    if (!h)
        return NULL;
    w32l_last_error = ERROR_SUCCESS;
    return (HANDLE)h;
}

HANDLE WINAPI OpenMutexA(DWORD access, BOOL inherit, LPCSTR name)
{
    (void)access; (void)inherit; (void)name;
    w32l_last_error = ERROR_FILE_NOT_FOUND;
    return NULL;
}

BOOL WINAPI ReleaseMutex(HANDLE m)
{
    W32L_Handle *h = (W32L_Handle *)m;
    return h && h->kind == W32L_H_MUTEX;
}

/* Wait on the shared condition until `ready` says so or the deadline
 * passes. Called with the lock held. */
static DWORD w32l_wait_until(int32_t (*ready)(HANDLE *, DWORD, BOOL),
                            HANDLE *hs, DWORD n, BOOL all, DWORD ms)
{
    struct timespec deadline;

    if (ms != INFINITE) {
        struct timeval now;
        gettimeofday(&now, NULL);
        deadline.tv_sec = now.tv_sec + ms / 1000;
        deadline.tv_nsec = (now.tv_usec + (ms % 1000) * 1000) * 1000;
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }
    }
    for (;;) {
        int32_t r = ready(hs, n, all);
        if (r >= 0)
            return (DWORD)r;
        if (ms == INFINITE) {
            pthread_cond_wait(&w32l_h_cond, &w32l_h_lock);
        } else if (pthread_cond_timedwait(&w32l_h_cond, &w32l_h_lock, &deadline)
                   == ETIMEDOUT) {
            r = ready(hs, n, all);
            return r >= 0 ? (DWORD)r : WAIT_TIMEOUT;
        }
    }
}

/* Is handle i satisfied? Consumes an auto-reset event when it is. */
static int32_t w32l_handle_ready(W32L_Handle *h, int32_t consume)
{
    switch (h->kind) {
    case W32L_H_THREAD:
        return h->done;
    case W32L_H_EVENT:
        if (!h->signaled)
            return 0;
        if (consume && !h->manualReset)
            h->signaled = 0;
        return 1;
    case W32L_H_MUTEX:
        return 1;
    }
    return 1;
}

static int32_t w32l_ready_any(HANDLE *hs, DWORD n, BOOL all)
{
    DWORD i;

    if (all) {
        for (i = 0; i < n; i++)
            if (!w32l_handle_ready((W32L_Handle *)hs[i], 0))
                return -1;
        for (i = 0; i < n; i++)
            w32l_handle_ready((W32L_Handle *)hs[i], 1);
        return WAIT_OBJECT_0;
    }
    for (i = 0; i < n; i++)
        if (w32l_handle_ready((W32L_Handle *)hs[i], 1))
            return (int32_t)(WAIT_OBJECT_0 + i);
    return -1;
}

DWORD WINAPI WaitForSingleObject(HANDLE h, DWORD ms)
{
    DWORD rc;

    if (!h)
        return WAIT_FAILED;
    pthread_mutex_lock(&w32l_h_lock);
    rc = w32l_wait_until(w32l_ready_any, &h, 1, FALSE, ms);
    pthread_mutex_unlock(&w32l_h_lock);
    return rc;
}

DWORD WINAPI WaitForMultipleObjects(DWORD n, const HANDLE *hs, BOOL all, DWORD ms)
{
    HANDLE copy[64];
    DWORD  rc, i;

    if (n == 0 || n > 64)
        return WAIT_FAILED;
    for (i = 0; i < n; i++) {
        if (!hs[i])
            return WAIT_FAILED;
        copy[i] = hs[i];
    }
    pthread_mutex_lock(&w32l_h_lock);
    rc = w32l_wait_until(w32l_ready_any, copy, n, all, ms);
    pthread_mutex_unlock(&w32l_h_lock);
    return rc;
}

BOOL WINAPI CloseHandle(HANDLE h)
{
    if (!h || h == INVALID_HANDLE_VALUE)
        return FALSE;
    w32l_handle_release((W32L_Handle *)h);
    return TRUE;
}

/* ---- critical sections ------------------------------------------------------- */

void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION cs)
{
    pthread_mutex_t    *m = (pthread_mutex_t *)malloc(sizeof *m);
    pthread_mutexattr_t a;

    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &a);
    pthread_mutexattr_destroy(&a);
    cs->impl = m;
}

void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION cs)
{
    if (cs->impl) {
        pthread_mutex_destroy((pthread_mutex_t *)cs->impl);
        free(cs->impl);
        cs->impl = NULL;
    }
}

void WINAPI EnterCriticalSection(LPCRITICAL_SECTION cs)
{
    if (!cs->impl)
        InitializeCriticalSection(cs);
    pthread_mutex_lock((pthread_mutex_t *)cs->impl);
}

void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION cs)
{
    if (cs->impl)
        pthread_mutex_unlock((pthread_mutex_t *)cs->impl);
}

/* ---- the exception hook ------------------------------------------------------- */

/* runtime.cpp registers one handler, whose job is to name a call into the
 * original's .text range -- the int3 gap -- and its caller. Natively that
 * range is not executable at all, so the call arrives as SIGSEGV with EIP
 * at the callee; SIGTRAP and SIGILL are taken too, for the same shape by
 * other means. The handler is given the Win32 view of the fault, and
 * whatever it says the process then dies the ordinary way, with a line on
 * stderr first so a native run does not have to find the log. */
static PVECTORED_EXCEPTION_HANDLER w32l_veh;

static void w32l_signal(int sig, siginfo_t *info, void *uc_)
{
    ucontext_t        *uc = (ucontext_t *)uc_;
    EXCEPTION_RECORD   er;
    CONTEXT            ctx;
    EXCEPTION_POINTERS ep;
    uintptr_t          eip = (uintptr_t)uc->uc_mcontext.gregs[REG_EIP];
    uintptr_t          esp = (uintptr_t)uc->uc_mcontext.gregs[REG_ESP];

    memset(&er, 0, sizeof er);
    memset(&ctx, 0, sizeof ctx);
    er.ExceptionCode = sig == SIGSEGV ? EXCEPTION_ACCESS_VIOLATION
                     : sig == SIGTRAP ? EXCEPTION_BREAKPOINT
                     : EXCEPTION_ILLEGAL_INSTRUCTION;
    er.ExceptionAddress = (PVOID)eip;
    er.NumberParameters = 2;
    er.ExceptionInformation[0] = 0;
    er.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
    ctx.Eip = (DWORD)eip;
    ctx.Esp = (DWORD)esp;
    ctx.Eax = uc->uc_mcontext.gregs[REG_EAX];
    ctx.Ebx = uc->uc_mcontext.gregs[REG_EBX];
    ctx.Ecx = uc->uc_mcontext.gregs[REG_ECX];
    ctx.Edx = uc->uc_mcontext.gregs[REG_EDX];
    ctx.Esi = uc->uc_mcontext.gregs[REG_ESI];
    ctx.Edi = uc->uc_mcontext.gregs[REG_EDI];
    ctx.Ebp = uc->uc_mcontext.gregs[REG_EBP];
    ep.ExceptionRecord = &er;
    ep.ContextRecord = &ctx;

    fprintf(stderr, "platform: fatal signal %d at eip=0x%08lx addr=%p "
            "(return address 0x%08lx)\n", sig, (unsigned long)eip,
            info->si_addr, (unsigned long)(esp ? *(uintptr_t *)esp : 0));
    if (w32l_veh && w32l_veh(&ep) == EXCEPTION_CONTINUE_EXECUTION)
        return;
    signal(sig, SIG_DFL);
    raise(sig);
}

PVOID WINAPI AddVectoredExceptionHandler(ULONG first,
                                         PVECTORED_EXCEPTION_HANDLER handler)
{
    struct sigaction sa;

    (void)first;
    w32l_veh = handler;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = w32l_signal;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    return (PVOID)handler;
}

/* ---- registry ----------------------------------------------------------------- */


/* ---- drives ---------------------------------------------------------------------- */

/* One fixed drive and no CD. The CD check in this retail binary is patched
 * out already; what remains asks these three and takes the no-disc path. */
DWORD WINAPI GetLogicalDriveStringsA(DWORD cap, LPSTR out)
{
    static const char drives[] = "C:\\\0";
    if (cap >= sizeof drives)
        memcpy(out, drives, sizeof drives);
    return sizeof drives - 1;
}

UINT WINAPI GetDriveTypeA(LPCSTR root)
{
    (void)root;
    return DRIVE_FIXED;
}

BOOL WINAPI GetVolumeInformationA(LPCSTR root, LPSTR volName, DWORD volCap,
                                  LPDWORD serial, LPDWORD maxComp,
                                  LPDWORD fsFlags, LPSTR fsName, DWORD fsCap)
{
    (void)root;
    if (volName && volCap)
        volName[0] = 0;
    if (serial)
        *serial = 0;
    if (maxComp)
        *maxComp = 255;
    if (fsFlags)
        *fsFlags = 0;
    if (fsName && fsCap)
        fsName[0] = 0;
    return FALSE;
}

BOOL WINAPI SetCurrentDirectoryA(LPCSTR path)
{
    char native[W32L_NATIVE_PATH_MAX];

    if (!path)
        return FALSE;
    if (chdir(w32l_native_path(path, native)) != 0) {
        w32l_last_error = errno == ENOENT ? ERROR_PATH_NOT_FOUND : ERROR_ACCESS_DENIED;
        return FALSE;
    }
    return TRUE;
}

DWORD WINAPI GetCurrentDirectoryA(DWORD cap, LPSTR out)
{
    char buf[W32L_NATIVE_PATH_MAX];

    if (!getcwd(buf, sizeof buf))
        return 0;
    if (strlen(buf) + 1 > cap)
        return (DWORD)strlen(buf) + 1;
    strcpy(out, buf);
    return (DWORD)strlen(buf);
}

/* ---- COM ------------------------------------------------------------------------ */

HRESULT WINAPI CoInitialize(LPVOID reserved) { (void)reserved; return S_OK; }
void    WINAPI CoUninitialize(void) { }

/* ---- winsock ------------------------------------------------------------------------ */

int32_t WINAPI WSAStartup(WORD version, LPWSADATA out)
{
    if (out) {
        memset(out, 0, sizeof *out);
        out->wVersion = version;
        out->wHighVersion = version;
    }
    return 0;
}

int32_t WINAPI WSACleanup(void) { return 0; }
int32_t WINAPI WSAGetLastError(void) { return errno; }
int32_t WINAPI closesocket(SOCKET s) { return close(s); }

int32_t w32l_ws_setsockopt(SOCKET s, int32_t level, int32_t name,
                          const char *val, int32_t len)
{
    /* Winsock's receive timeout is a DWORD of milliseconds; POSIX wants a
     * timeval. control.c passes the former. */
    if (level == SOL_SOCKET && name == SO_RCVTIMEO && len == 4) {
        struct timeval tv;
        DWORD ms = *(const DWORD *)val;
        tv.tv_sec = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;
        return setsockopt(s, level, name, &tv, sizeof tv);
    }
    return setsockopt(s, level, name, val, (socklen_t)len);
}
