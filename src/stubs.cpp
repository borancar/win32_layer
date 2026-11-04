/* stubs.cpp -- zero-answering stand-ins for whole modules a program imports
 * but whose real implementation is not (yet) in this layer.
 *
 * w32l_export_lookup falls through to w32l_stub_lookup. Each name gets a
 * small generated function:
 *
 *     pushad; push &entry; call w32l_stub_hit; add esp,4; popad
 *     mov eax, <answer>; ret <argument bytes>
 *
 * so the stack stays balanced whatever the caller passed (a stdcall callee
 * pops its own arguments; an answer without the right `ret N` corrupts the
 * stack one call later, far from the cause), and the FIRST call of every name
 * is logged. That log is the list of what the program really uses, which is
 * what to implement first.
 *
 *   mss32.dll   Miles Sound System. Its imports are stdcall-decorated
 *               (_AIL_open_stream@12), so the byte count is in the name and
 *               the whole module is covered without a table. A real provider
 *               (OpenMiles, GPLv3) is linked per game instead of this; see
 *               the windows-game-reconstruction skill.
 *   WSOCK32.dll Winsock, imported by ordinal; the table below gives names and
 *               argument bytes. Everything answers failure, so a game takes
 *               its no-network path.
 *
 * Entries with a real implementation are not stubbed: a zero from
 * AIL_mem_alloc_lock would be a NULL the game dereferences.
 */
#include "platform.h"

#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

namespace {

struct Stub {
    char     *module;
    char     *name;
    uint32_t  argbytes;
    int32_t   answer;
    int32_t   hit;
    uint8_t  *code;
};

/* ---- real implementations ----------------------------------------------- */

void *WINAPI stub_AIL_mem_alloc_lock(uint32_t size) { return malloc(size ? size : 1); }
int32_t WINAPI stub_AIL_mem_free_lock(void *p) { free(p); return 0; }

struct Real { const char *module, *name; const void *fn; };
const Real kReal[] = {
    { "mss32.dll", "_AIL_mem_alloc_lock@4", (const void *)&stub_AIL_mem_alloc_lock },
    { "mss32.dll", "_AIL_mem_free_lock@4",  (const void *)&stub_AIL_mem_free_lock },
};

/* ---- Winsock by ordinal --------------------------------------------------- */

const int32_t kSocketError = -1;
const int32_t kWsaSysNotReady = 10091;

struct WsEntry { uint16_t ordinal; const char *name; uint32_t argbytes; int32_t answer; };
const WsEntry kWinsock[] = {
    {   1, "accept",        12, kSocketError },
    {   2, "bind",          12, kSocketError },
    {   3, "closesocket",    4, 0 },
    {   4, "connect",       12, kSocketError },
    {   9, "htons",          4, 0 },
    {  10, "ioctlsocket",   12, kSocketError },
    {  11, "inet_addr",      4, -1 },
    {  12, "inet_ntoa",      4, 0 },
    {  13, "listen",         8, kSocketError },
    {  16, "recv",          16, kSocketError },
    {  18, "select",        20, kSocketError },
    {  19, "send",          16, kSocketError },
    {  21, "setsockopt",    20, kSocketError },
    {  23, "socket",        12, kSocketError },
    {  52, "gethostbyname",  4, 0 },
    {  57, "gethostname",    8, kSocketError },
    { 115, "WSAStartup",     8, kWsaSysNotReady },
    { 116, "WSACleanup",     0, 0 },
};

/* ---- generated code -------------------------------------------------------- */

const size_t kPage = 64 * 1024;
const size_t kCodeSize = 32;
Stub    *g_stubs[2048];
size_t   g_nstubs;
uint8_t *g_page;
size_t   g_used;

void stub_hit(Stub *s)
{
    if (!s->hit) {
        s->hit = 1;
        w32l_plat_log("stub called: %s!%s (pops %u bytes, answers %d)",
                      s->module, s->name, (unsigned)s->argbytes, (int)s->answer);
    }
}

const void *stub_make(const char *module, const char *name, uint32_t argbytes, int32_t answer)
{
    size_t   i;
    Stub    *s;
    uint8_t *c;
    uint32_t rel, ptr;

    for (i = 0; i < g_nstubs; i++)
        if (!strcasecmp(g_stubs[i]->module, module) && !strcmp(g_stubs[i]->name, name))
            return g_stubs[i]->code;
    if (g_nstubs >= sizeof g_stubs / sizeof g_stubs[0])
        return NULL;
    if (!g_page) {
        void *m = mmap(NULL, kPage, PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m == MAP_FAILED)
            return NULL;
        g_page = (uint8_t *)m;
    }
    if (g_used + kCodeSize > kPage)
        return NULL;

    s = (Stub *)calloc(1, sizeof *s);
    s->module = strdup(module);
    s->name = strdup(name);
    s->argbytes = argbytes;
    s->answer = answer;
    c = g_page + g_used;
    g_used += kCodeSize;
    s->code = c;

    /* stub_hit is cdecl; as a plain C++ function in this TU it is
     * __cdecl on i386, which is what the call below assumes. */
    ptr = (uint32_t)(uintptr_t)s;
    i = 0;
    c[i++] = 0x60;                                      /* pushad */
    c[i++] = 0x68; memcpy(c + i, &ptr, 4); i += 4;      /* push s */
    c[i++] = 0xE8;                                      /* call stub_hit */
    rel = (uint32_t)((uintptr_t)&stub_hit - (uintptr_t)(c + i + 4));
    memcpy(c + i, &rel, 4); i += 4;
    c[i++] = 0x83; c[i++] = 0xC4; c[i++] = 0x04;        /* add esp,4 */
    c[i++] = 0x61;                                      /* popad */
    c[i++] = 0xB8; memcpy(c + i, &answer, 4); i += 4;   /* mov eax,answer */
    if (argbytes) {
        uint16_t n = (uint16_t)argbytes;
        c[i++] = 0xC2; memcpy(c + i, &n, 2); i += 2;    /* ret n */
    } else {
        c[i++] = 0xC3;                                  /* ret */
    }
    g_stubs[g_nstubs++] = s;
    return c;
}

} // namespace

/* NULL when the module is not one we stub, or the name is not decodable. */
const void *w32l_stub_lookup(const char *module, const char *name)
{
    size_t i;

    if (!strcasecmp(module, "mss32.dll")) {
        const char *at;

        for (i = 0; i < sizeof kReal / sizeof kReal[0]; i++)
            if (!strcmp(kReal[i].name, name))
                return kReal[i].fn;
        at = strrchr(name, '@');
        if (!at || !at[1])
            return NULL;
        return stub_make(module, name, (uint32_t)strtoul(at + 1, NULL, 10), 0);
    }
    if (!strcasecmp(module, "WSOCK32.dll") && name[0] == '#') {
        unsigned ord = (unsigned)strtoul(name + 1, NULL, 10);

        for (i = 0; i < sizeof kWinsock / sizeof kWinsock[0]; i++)
            if (kWinsock[i].ordinal == ord)
                return stub_make(module, kWinsock[i].name,
                                 kWinsock[i].argbytes, kWinsock[i].answer);
    }
    return NULL;
}
