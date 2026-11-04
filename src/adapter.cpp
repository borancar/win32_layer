/* adapter.cpp -- calling a C (cdecl) function from a stdcall import slot.
 *
 * A PE game calls its imports __stdcall: the callee pops the arguments. A
 * provider built for Linux (OpenMiles for mss32, say) takes them __cdecl.
 * w32l_stdcall_adapter makes a small function per provider entry that
 * re-pushes the caller's `argbytes` bytes of arguments, calls the provider
 * with a 16-byte-aligned stack (the i386 System V rule a Linux-built library
 * assumes and a Windows program does not keep), and returns with
 * `ret argbytes`:
 *
 *     push ebp; mov ebp,esp; push ebx,esi,edi; and esp,-16; sub esp,pad
 *     push [ebp+8+4*(n-1)] ... push [ebp+8]       ; arguments, last first
 *     mov eax,fn; call eax
 *     lea esp,[ebp-12]; pop edi,esi,ebx; pop ebp; ret argbytes
 *
 * The result stays in eax (edx:eax, st0) as the provider left it. Arguments
 * are copied as raw 32-bit words, so pointers, ints and floats all work;
 * a callee that wants a translated argument (a Windows path, say) is given a
 * wrapper in C as `fn` instead.
 */
#include "platform.h"

#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

const void *w32l_stdcall_adapter(const void *fn, uint32_t argbytes)
{
    static uint8_t *page;
    static size_t   used;
    const size_t    kPage = 64 * 1024;
    uint32_t        n = argbytes / 4, pad, k;
    uint8_t        *c;
    size_t          i = 0;
    uint32_t        imm;

    if (argbytes % 4 || n > 28)
        return NULL;
    if (!page) {
        void *m = mmap(NULL, kPage, PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m == MAP_FAILED)
            return NULL;
        page = (uint8_t *)m;
    }
    if (used + 160 > kPage)
        return NULL;
    c = page + used;
    pad = (16 - (n * 4) % 16) % 16;

    c[i++] = 0x55;                                          /* push ebp */
    c[i++] = 0x89; c[i++] = 0xE5;                           /* mov ebp,esp */
    /* A stdcall callee keeps ebx, esi, edi; a PIC library keeps ebx for its
     * own use and the game relies on its own copy surviving the call. */
    c[i++] = 0x53; c[i++] = 0x56; c[i++] = 0x57;            /* push ebx; push esi; push edi */
    c[i++] = 0x83; c[i++] = 0xE4; c[i++] = 0xF0;            /* and esp,-16 */
    if (pad) {
        c[i++] = 0x83; c[i++] = 0xEC; c[i++] = (uint8_t)pad; /* sub esp,pad */
    }
    for (k = n; k > 0; k--) {
        c[i++] = 0xFF; c[i++] = 0x75; c[i++] = (uint8_t)(8 + 4 * (k - 1)); /* push [ebp+d8] */
    }
    imm = (uint32_t)(uintptr_t)fn;
    c[i++] = 0xB8; memcpy(c + i, &imm, 4); i += 4;          /* mov eax,fn */
    c[i++] = 0xFF; c[i++] = 0xD0;                           /* call eax */
    c[i++] = 0x8D; c[i++] = 0x65; c[i++] = 0xF4;            /* lea esp,[ebp-12] */
    c[i++] = 0x5F; c[i++] = 0x5E; c[i++] = 0x5B;            /* pop edi; pop esi; pop ebx */
    c[i++] = 0x5D;                                          /* pop ebp */
    if (argbytes) {
        uint16_t r = (uint16_t)argbytes;
        c[i++] = 0xC2; memcpy(c + i, &r, 2); i += 2;        /* ret argbytes */
    } else {
        c[i++] = 0xC3;
    }
    used += (i + 15) & ~(size_t)15;
    return c;
}
