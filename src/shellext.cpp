/* shellext.cpp -- advapi32's registry reads, shlwapi's path helpers, shell32's
 * folders and drag-and-drop.
 *
 * There is no registry: every key a program opens is absent, so it takes its
 * defaults. Folders are fixed Windows-style paths on drive C: (which the host
 * maps with W32L_DRIVE_C). Drag and drop never happens. */
#include "platform.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

/* ---- registry: nothing is there ---------------------------------------------------- */

LONG WINAPI RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD options, DWORD access, PHKEY out)
{
    (void)key; (void)sub; (void)options; (void)access;
    if (out)
        *out = NULL;
    return ERROR_FILE_NOT_FOUND;
}

LONG WINAPI RegQueryInfoKeyA(HKEY key, LPSTR cls, LPDWORD clsLen, LPDWORD reserved,
                             LPDWORD subKeys, LPDWORD maxSubKey, LPDWORD maxClass,
                             LPDWORD values, LPDWORD maxValueName, LPDWORD maxValueData,
                             LPDWORD secDesc, LPFILETIME lastWrite)
{
    (void)key; (void)cls; (void)reserved;
    if (clsLen) *clsLen = 0;
    if (subKeys) *subKeys = 0;
    if (maxSubKey) *maxSubKey = 0;
    if (maxClass) *maxClass = 0;
    if (values) *values = 0;
    if (maxValueName) *maxValueName = 0;
    if (maxValueData) *maxValueData = 0;
    if (secDesc) *secDesc = 0;
    if (lastWrite) lastWrite->dwLowDateTime = lastWrite->dwHighDateTime = 0;
    return ERROR_SUCCESS;
}

LONG WINAPI RegEnumValueA(HKEY key, DWORD index, LPSTR name, LPDWORD nameLen,
                          LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD dataLen)
{
    (void)key; (void)index; (void)name; (void)nameLen; (void)reserved;
    (void)type; (void)data; (void)dataLen;
    return ERROR_NO_MORE_ITEMS;
}

/* ---- shlwapi ------------------------------------------------------------------------- */

BOOL WINAPI PathFileExistsA(LPCSTR path)
{
    return path && GetFileAttributesA(path) != 0xFFFFFFFFu;
}

/* Join with exactly one backslash between. */
BOOL WINAPI PathAppendA(LPSTR path, LPCSTR more)
{
    size_t n;

    if (!path || !more)
        return FALSE;
    while (*more == '\\')
        more++;
    n = strlen(path);
    if (n && path[n - 1] != '\\' && path[n - 1] != '/')
        path[n++] = '\\';
    strcpy(path + n, more);
    return TRUE;
}

/* ---- shell32 ------------------------------------------------------------------------------ */

/* The per-user folders live in a real directory the program can write:
 * $W32L_USERDIR, or ./userdata under the current directory. A native path,
 * so it needs no drive mapping. */
int32_t WINAPI SHCreateDirectoryExA(HWND, LPCSTR, LPSECURITY_ATTRIBUTES);

HRESULT WINAPI SHGetFolderPathA(HWND hwnd, int32_t csidl, HANDLE token, DWORD flags, LPSTR out)
{
    char        root[MAX_PATH - 32];
    const char *env = getenv("W32L_USERDIR");
    const char *sub = (csidl & 0xFF) == (CSIDL_APPDATA & 0xFF) ? "AppData" : "Documents";
    char        dir[MAX_PATH];

    (void)hwnd; (void)token; (void)flags;
    if (!out)
        return E_POINTER;
    if (env && *env) {
        snprintf(root, sizeof root, "%s", env);
    } else {
        char cwd[MAX_PATH - 64];
        if (!getcwd(cwd, sizeof cwd))
            strcpy(cwd, ".");
        snprintf(root, sizeof root, "%s/userdata", cwd);
    }
    snprintf(dir, sizeof dir, "%s/%s", root, sub);
    SHCreateDirectoryExA(NULL, dir, NULL);
    strcpy(out, dir);
    return S_OK;
}

/* Every missing component, outermost first. */
int32_t WINAPI SHCreateDirectoryExA(HWND hwnd, LPCSTR path, LPSECURITY_ATTRIBUTES sa)
{
    char   buf[MAX_PATH * 2];
    size_t i, n;

    (void)hwnd;
    if (!path)
        return ERROR_INVALID_PARAMETER;
    n = strlen(path);
    if (n >= sizeof buf)
        return ERROR_INVALID_PARAMETER;
    memcpy(buf, path, n + 1);
    for (i = 1; i <= n; i++)
        if (buf[i] == '\\' || buf[i] == '/' || buf[i] == 0) {
            char c = buf[i];
            buf[i] = 0;
            if (i > 2 || buf[1] != ':')     /* not the drive itself */
                CreateDirectoryA(buf, sa);
            buf[i] = c;
        }
    return ERROR_SUCCESS;
}

void WINAPI DragAcceptFiles(HWND hwnd, BOOL accept) { (void)hwnd; (void)accept; }
UINT WINAPI DragQueryFileA(HANDLE drop, UINT index, LPSTR out, UINT cch)
{
    (void)drop; (void)index; (void)out; (void)cch;
    return 0;
}
void WINAPI DragFinish(HANDLE drop) { (void)drop; }
