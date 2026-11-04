/* amstream.cpp -- DirectShow multimedia streaming, enough to play an AVI.
 *
 * A game of this era plays its videos through amstream.dll: it creates
 * CLSID_AMMultiMediaStream, adds a primary video stream (a DirectDraw one) and
 * a default audio renderer, opens a file, runs it, and then pulls frames by
 * creating a DirectDraw stream sample on a surface of its own and calling
 * Update() on it once a frame. This implements that contract directly: an AVI
 * is read in this file, a Cinepak picture is decoded (cinepak.cpp) and
 * written into the sample's surface in the surface's format, and Update()
 * waits for the frame's time on the stream's clock.
 *
 *   IAMMultiMediaStream      Initialize, AddMediaStream, OpenFile, Seek,
 *                            SetState, GetMediaStream, GetState, Release.
 *   IDirectDrawMediaStream   GetFormat, CreateSample (and the rest, no-ops).
 *   IDirectDrawStreamSample  Update, CompletionStatus, GetSurface.
 *
 * Not here (yet): audio, and codecs other than Cinepak and uncompressed RGB.
 */
#include "video.h"
#include "ddraw.h"
#include <direct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define MS_S_NOUPDATE     ((HRESULT)0x00040004)
#define MS_S_ENDOFSTREAM  ((HRESULT)0x00040003)
#define MS_E_NOSTREAM     ((HRESULT)0x80040201)
#define MS_E_NOTINIT      ((HRESULT)0x80040204)
#define STG_E_FILENOTFOUND ((HRESULT)0x80030002)

namespace {

const GUID kCLSID_AMMultiMediaStream = { 0x49c47ce5, 0x9ba4, 0x11d0, { 0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45 } };
const GUID kIID_IAMMultiMediaStream  = { 0xbebe595c, 0x9a6f, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IMultiMediaStream    = { 0xb502d1bc, 0x9a57, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IMediaStream         = { 0xb502d1bd, 0x9a57, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IDirectDrawMediaStream = { 0xf4104fce, 0x9a70, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IStreamSample        = { 0xb502d1be, 0x9a57, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IDirectDrawStreamSample = { 0xf4104fcf, 0x9a70, 0x11d0, { 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
const GUID kIID_IUnknown             = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const GUID kMSPID_PrimaryVideo       = { 0xa35ff56a, 0x9fda, 0x11d0, { 0x8f, 0xdf, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };

#define STREAMSTATE_STOP 0
#define STREAMSTATE_RUN  1

/* ---- the AVI ------------------------------------------------------------------ */

struct Chunk { uint32_t offset, size; };

struct Avi {
    FILE    *fp;
    int32_t  width, height, bitcount;
    uint32_t fourcc;                    /* the video codec, 'cvid' or 0 for RGB */
    uint32_t rate, scale;               /* frames: rate/scale a second */
    Chunk   *video;
    uint32_t nvideo;
    uint32_t cap;
};

uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

void avi_close(Avi *a)
{
    if (!a)
        return;
    if (a->fp)
        fclose(a->fp);
    free(a->video);
    free(a);
}

/* Walk the RIFF: the video stream's format from hdrl, and every '..dc'/'..db'
 * chunk of the video stream (stream 0 is assumed to be it; the video is
 * whichever stream's strh says "vids"). */
Avi *avi_open(const char *path)
{
    Avi     *a = (Avi *)calloc(1, sizeof *a);
    uint8_t  hdr[12];
    long     end;
    int32_t  stream = -1, videoStream = -1;

    if (!a)
        return NULL;
    a->fp = fopen(path, "rb");
    if (!a->fp || fread(hdr, 1, 12, a->fp) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "AVI ", 4)) {
        avi_close(a);
        return NULL;
    }
    fseek(a->fp, 0, SEEK_END);
    end = ftell(a->fp);
    fseek(a->fp, 12, SEEK_SET);

    /* Top level: LIST hdrl (parsed), LIST movi (indexed), the rest skipped. */
    for (;;) {
        uint8_t  h[12];
        long     at = ftell(a->fp);
        uint32_t size;

        if (at + 8 > end || fread(h, 1, 8, a->fp) != 8)
            break;
        size = le32(h + 4);
        if (!memcmp(h, "LIST", 4) && size >= 4) {
            uint8_t type[4];
            long    listEnd = at + 8 + (long)size;
            if (fread(type, 1, 4, a->fp) != 4)
                break;
            if (!memcmp(type, "hdrl", 4)) {
                /* Inside: avih, then LIST strl each with strh and strf. */
                while (ftell(a->fp) + 8 <= listEnd) {
                    uint8_t  ch[12];
                    long     cat = ftell(a->fp);
                    uint32_t csize;
                    if (fread(ch, 1, 8, a->fp) != 8)
                        break;
                    csize = le32(ch + 4);
                    if (!memcmp(ch, "LIST", 4)) {
                        long    sEnd = cat + 8 + (long)csize;
                        uint8_t st[4];
                        if (fread(st, 1, 4, a->fp) != 4)
                            break;
                        if (!memcmp(st, "strl", 4)) {
                            stream++;
                            while (ftell(a->fp) + 8 <= sEnd) {
                                uint8_t  sh[8];
                                uint32_t ssize;
                                long     sat = ftell(a->fp);
                                uint8_t  body[64];
                                if (fread(sh, 1, 8, a->fp) != 8)
                                    break;
                                ssize = le32(sh + 4);
                                if (!memcmp(sh, "strh", 4) && ssize >= 36) {
                                    if (fread(body, 1, 36, a->fp) != 36)
                                        break;
                                    if (!memcmp(body, "vids", 4)) {
                                        videoStream = stream;
                                        a->fourcc = le32(body + 4);
                                        a->scale = le32(body + 20);
                                        a->rate = le32(body + 24);
                                    }
                                } else if (!memcmp(sh, "strf", 4) && ssize >= 16 && stream == videoStream) {
                                    if (fread(body, 1, 16, a->fp) != 16)
                                        break;
                                    a->width = (int32_t)le32(body + 4);
                                    a->height = (int32_t)le32(body + 8);
                                    a->bitcount = le16(body + 14);
                                }
                                fseek(a->fp, sat + 8 + (long)ssize + (long)(ssize & 1), SEEK_SET);
                            }
                        }
                        fseek(a->fp, sEnd + (long)(csize & 1), SEEK_SET);
                    } else {
                        fseek(a->fp, cat + 8 + (long)csize + (long)(csize & 1), SEEK_SET);
                    }
                }
            } else if (!memcmp(type, "movi", 4)) {
                char     want[16];
                uint32_t idx = 0;
                (void)idx;
                snprintf(want, sizeof want, "%02dd", videoStream < 0 ? 0 : videoStream);
                while (ftell(a->fp) + 8 <= listEnd) {
                    uint8_t  ch[8];
                    long     cat = ftell(a->fp);
                    uint32_t csize;
                    if (fread(ch, 1, 8, a->fp) != 8)
                        break;
                    csize = le32(ch + 4);
                    if (!memcmp(ch, "LIST", 4)) {
                        /* A 'rec ' group: its chunks follow the 4-byte type, in line. */
                        fseek(a->fp, cat + 12, SEEK_SET);
                        continue;
                    }
                    if (ch[0] == want[0] && ch[1] == want[1] && ch[2] == 'd' && (ch[3] == 'c' || ch[3] == 'b')) {
                        if (a->nvideo == a->cap) {
                            a->cap = a->cap ? a->cap * 2 : 1024;
                            a->video = (Chunk *)realloc(a->video, a->cap * sizeof(Chunk));
                            if (!a->video) {
                                avi_close(a);
                                return NULL;
                            }
                        }
                        a->video[a->nvideo].offset = (uint32_t)(cat + 8);
                        a->video[a->nvideo].size = csize;
                        a->nvideo++;
                    }
                    fseek(a->fp, cat + 8 + (long)csize + (long)(csize & 1), SEEK_SET);
                }
            }
            fseek(a->fp, listEnd + (long)(size & 1), SEEK_SET);
        } else {
            fseek(a->fp, at + 8 + (long)size + (long)(size & 1), SEEK_SET);
        }
    }
    if (!a->width || !a->height || !a->nvideo || !a->rate || !a->scale) {
        w32l_plat_log("amstream: unusable AVI: %dx%d frames=%u rate=%u/%u", (int)a->width, (int)a->height,
                      (unsigned)a->nvideo, (unsigned)a->rate, (unsigned)a->scale);
        avi_close(a);
        return NULL;
    }
    if (a->height < 0)
        a->height = -a->height;
    return a;
}

/* ---- the objects ------------------------------------------------------------------ */

struct Stream;

struct MMStream {
    const void  *lpVtbl;
    ULONG        refs;
    Avi         *avi;
    Cinepak     *cvid;
    Stream      *video;
    int32_t      wantsVideo;
    uint32_t     state;
    uint32_t     next;                  /* the next frame to deliver */
    uint64_t     startNs;               /* the clock when the stream last started */
    uint32_t     startFrame;
};

struct Stream {
    const void *lpVtbl;
    ULONG       refs;
    MMStream   *owner;
};

struct Sample {
    const void              *lpVtbl;
    ULONG                    refs;
    Stream                  *stream;
    IDirectDrawSurface      *surface;
    RECT                     rect;
    int32_t                  hasRect;
};

uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void sleep_until(uint64_t ns)
{
    uint64_t now = now_ns();
    if (ns > now) {
        struct timespec ts;
        uint64_t d = ns - now;
        ts.tv_sec = (time_t)(d / 1000000000ULL);
        ts.tv_nsec = (long)(d % 1000000000ULL);
        nanosleep(&ts, NULL);
    }
}

HRESULT STDMETHODCALLTYPE mm_unsupported() { return E_NOTIMPL; }

/* ---- IDirectDrawStreamSample ---- */

HRESULT STDMETHODCALLTYPE Sample_QI(Sample *s, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, kIID_IUnknown) || IsEqualGUID(iid, kIID_IStreamSample) ||
        IsEqualGUID(iid, kIID_IDirectDrawStreamSample)) {
        s->refs++;
        *out = s;
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE Sample_AddRef(Sample *s) { return ++s->refs; }
ULONG STDMETHODCALLTYPE Sample_Release(Sample *s)
{
    ULONG n = --s->refs;
    if (!n) {
        if (s->surface)
            s->surface->lpVtbl->Release(s->surface);
        free(s);
    }
    return n;
}
HRESULT STDMETHODCALLTYPE Sample_GetMediaStream(Sample *s, void **out)
{
    s->stream->refs++;
    *out = s->stream;
    return S_OK;
}

/* Write the decoded picture into the surface, in the surface's format. */
void blit_frame(MMStream *m, Sample *s)
{
    DDSURFACEDESC d;
    int32_t       w = m->avi->width, h = m->avi->height, x, y;
    const uint8_t *src = m->cvid->rgb;

    memset(&d, 0, sizeof d);
    d.dwSize = sizeof d;
    if (s->surface->lpVtbl->Lock(s->surface, NULL, &d, 0, NULL) != DD_OK)
        return;
    if (w > (int32_t)d.dwWidth)
        w = (int32_t)d.dwWidth;
    if (h > (int32_t)d.dwHeight)
        h = (int32_t)d.dwHeight;
    for (y = 0; y < h; y++) {
        uint8_t *row = (uint8_t *)d.lpSurface + (size_t)y * (size_t)d.lPitch;
        const uint8_t *p = src + (size_t)y * (size_t)m->avi->width * 3;
        if (d.ddpfPixelFormat.dwRGBBitCount == 16) {
            for (x = 0; x < w; x++, p += 3)
                ((uint16_t *)row)[x] = (uint16_t)(((p[0] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[2] >> 3));
        } else if (d.ddpfPixelFormat.dwRGBBitCount == 32) {
            for (x = 0; x < w; x++, p += 3)
                ((uint32_t *)row)[x] = (uint32_t)(p[0] << 16 | p[1] << 8 | p[2]);
        }
    }
    s->surface->lpVtbl->Unlock(s->surface, d.lpSurface);
}

HRESULT STDMETHODCALLTYPE Sample_Update(Sample *s, DWORD flags, HANDLE ev, LPVOID apc, DWORD data)
{
    MMStream *m = s->stream->owner;
    uint64_t  due;
    uint8_t  *buf;
    Chunk    *c;

    (void)flags; (void)ev; (void)apc; (void)data;
    if (!m->avi)
        return MS_S_ENDOFSTREAM;
    if (m->state != STREAMSTATE_RUN)
        return MS_S_NOUPDATE;
    if (m->next >= m->avi->nvideo)
        return MS_S_ENDOFSTREAM;

    /* Frame n is due n * scale / rate seconds after the stream started. */
    due = m->startNs + (uint64_t)(m->next - m->startFrame) * 1000000000ULL * m->avi->scale / m->avi->rate;
    sleep_until(due);

    c = &m->avi->video[m->next++];
    buf = (uint8_t *)malloc(c->size ? c->size : 1);
    if (!buf)
        return E_OUTOFMEMORY;
    fseek(m->avi->fp, (long)c->offset, SEEK_SET);
    if (fread(buf, 1, c->size, m->avi->fp) == c->size && c->size > 0)
        cinepak_decode(m->cvid, buf, c->size);
    free(buf);
    blit_frame(m, s);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE Sample_CompletionStatus(Sample *s, DWORD flags, DWORD ms)
{
    (void)s; (void)flags; (void)ms;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE Sample_GetSurface(Sample *s, IDirectDrawSurface **surf, RECT *rc)
{
    if (surf) {
        s->surface->lpVtbl->AddRef(s->surface);
        *surf = s->surface;
    }
    if (rc)
        *rc = s->rect;
    return S_OK;
}

/* IUnknown + IStreamSample (3..7) + IDirectDrawStreamSample (8,9). */
const void *kSampleVtbl[] = {
    (void *)Sample_QI, (void *)Sample_AddRef, (void *)Sample_Release,
    (void *)Sample_GetMediaStream, (void *)mm_unsupported, (void *)mm_unsupported,
    (void *)Sample_Update, (void *)Sample_CompletionStatus,
    (void *)Sample_GetSurface, (void *)mm_unsupported,
};

/* ---- IDirectDrawMediaStream ---- */

HRESULT STDMETHODCALLTYPE Stream_QI(Stream *s, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, kIID_IUnknown) || IsEqualGUID(iid, kIID_IMediaStream) ||
        IsEqualGUID(iid, kIID_IDirectDrawMediaStream)) {
        s->refs++;
        *out = s;
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE Stream_AddRef(Stream *s) { return ++s->refs; }
ULONG STDMETHODCALLTYPE Stream_Release(Stream *s)
{
    ULONG n = --s->refs;
    if (!n)
        free(s);
    return n;
}
HRESULT STDMETHODCALLTYPE Stream_GetMultiMediaStream(Stream *s, void **out)
{
    s->owner->refs++;
    *out = s->owner;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE Stream_GetFormat(Stream *s, LPDDSURFACEDESC cur, void **pal, LPDDSURFACEDESC want, DWORD *flags)
{
    MMStream *m = s->owner;

    (void)want;
    if (!m->avi)
        return MS_E_NOTINIT;
    if (pal)
        *pal = NULL;
    if (flags)
        *flags = 0;
    if (cur) {
        memset(cur, 0, sizeof *cur);
        cur->dwSize = sizeof *cur;
        cur->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        cur->dwWidth = (DWORD)m->avi->width;
        cur->dwHeight = (DWORD)m->avi->height;
        cur->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        cur->ddpfPixelFormat.dwSize = sizeof cur->ddpfPixelFormat;
        cur->ddpfPixelFormat.dwFlags = DDPF_RGB;
        if (w32l_display_bpp > 8) {
            cur->ddpfPixelFormat.dwRGBBitCount = 16;
            cur->ddpfPixelFormat.dwRBitMask = 0xF800;
            cur->ddpfPixelFormat.dwGBitMask = 0x07E0;
            cur->ddpfPixelFormat.dwBBitMask = 0x001F;
        } else {
            cur->ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
            cur->ddpfPixelFormat.dwRGBBitCount = 8;
        }
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE Stream_CreateSample(Stream *s, IDirectDrawSurface *surf, const RECT *rc, DWORD flags, void **out)
{
    Sample *smp;

    (void)flags;
    if (!surf || !out)
        return E_POINTER;
    smp = (Sample *)calloc(1, sizeof *smp);
    if (!smp)
        return E_OUTOFMEMORY;
    smp->lpVtbl = kSampleVtbl;
    smp->refs = 1;
    smp->stream = s;
    s->refs++;
    smp->surface = surf;
    surf->lpVtbl->AddRef(surf);
    if (rc) {
        smp->rect = *rc;
        smp->hasRect = 1;
    } else {
        smp->rect.right = s->owner->avi ? s->owner->avi->width : 0;
        smp->rect.bottom = s->owner->avi ? s->owner->avi->height : 0;
    }
    *out = smp;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE Stream_GetTimePerFrame(Stream *s, LONGLONG *t)
{
    MMStream *m = s->owner;
    if (!m->avi || !t)
        return E_POINTER;
    *t = (LONGLONG)10000000 * m->avi->scale / m->avi->rate;
    return S_OK;
}

/* IUnknown, IMediaStream (3..8), IDirectDrawMediaStream (9..14). */
const void *kStreamVtbl[] = {
    (void *)Stream_QI, (void *)Stream_AddRef, (void *)Stream_Release,
    (void *)Stream_GetMultiMediaStream, (void *)mm_unsupported, (void *)mm_unsupported,
    (void *)mm_unsupported, (void *)mm_unsupported, (void *)mm_unsupported,
    (void *)Stream_GetFormat, (void *)mm_unsupported, (void *)mm_unsupported,
    (void *)mm_unsupported, (void *)Stream_CreateSample, (void *)Stream_GetTimePerFrame,
};

/* ---- IAMMultiMediaStream ---- */

HRESULT STDMETHODCALLTYPE MM_QI(MMStream *m, REFIID iid, LPVOID *out)
{
    if (IsEqualGUID(iid, kIID_IUnknown) || IsEqualGUID(iid, kIID_IMultiMediaStream) ||
        IsEqualGUID(iid, kIID_IAMMultiMediaStream)) {
        m->refs++;
        *out = m;
        return S_OK;
    }
    *out = NULL;
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE MM_AddRef(MMStream *m) { return ++m->refs; }
ULONG STDMETHODCALLTYPE MM_Release(MMStream *m)
{
    ULONG n = --m->refs;
    if (!n) {
        /* The file and decoder go; the small object stays, because streams
         * and samples the program still holds point back at it. */
        avi_close(m->avi);
        cinepak_close(m->cvid);
        m->avi = NULL;
        m->cvid = NULL;
    }
    return n;
}
HRESULT STDMETHODCALLTYPE MM_GetMediaStream(MMStream *m, const GUID *purpose, void **out)
{
    if (!m->video || !IsEqualGUID(*purpose, kMSPID_PrimaryVideo)) {
        *out = NULL;
        return MS_E_NOSTREAM;
    }
    m->video->refs++;
    *out = m->video;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE MM_GetState(MMStream *m, DWORD *state)
{
    *state = m->state;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE MM_SetState(MMStream *m, DWORD state)
{
    if (state == STREAMSTATE_RUN && m->state != STREAMSTATE_RUN) {
        m->startNs = now_ns();
        m->startFrame = m->next;
    }
    m->state = state;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE MM_Seek(MMStream *m, LONGLONG t)
{
    uint64_t frame = m->avi ? (uint64_t)t * m->avi->rate / ((uint64_t)10000000 * m->avi->scale) : 0;

    /* Only a seek to the start is supported (a Cinepak stream needs its
     * key frame); anything else restarts from the beginning. */
    (void)frame;
    m->next = 0;
    m->startFrame = 0;
    m->startNs = now_ns();
    if (m->cvid)
        memset(m->cvid->rgb, 0, (size_t)m->cvid->width * (size_t)m->cvid->height * 3);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE MM_Initialize(MMStream *m, int32_t type, DWORD flags, void *graph)
{
    (void)m; (void)type; (void)flags; (void)graph;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE MM_AddMediaStream(MMStream *m, IUnknown *dd, const GUID *purpose, DWORD flags, void **out)
{
    (void)dd; (void)flags;
    if (out)
        *out = NULL;
    if (IsEqualGUID(*purpose, kMSPID_PrimaryVideo)) {
        Stream *s = (Stream *)calloc(1, sizeof *s);
        if (!s)
            return E_OUTOFMEMORY;
        s->lpVtbl = kStreamVtbl;
        s->refs = 1;                /* the multimedia stream's own */
        s->owner = m;
        m->video = s;
        if (out) {
            s->refs++;
            *out = s;
        }
    }
    return S_OK;                    /* audio: accepted, and silent for now */
}
HRESULT STDMETHODCALLTYPE MM_OpenFile(MMStream *m, const uint16_t *wpath, DWORD flags)
{
    char path[1024], native[W32L_NATIVE_PATH_MAX];
    size_t i;

    (void)flags;
    for (i = 0; wpath && wpath[i] && i < sizeof path - 1; i++)
        path[i] = (char)wpath[i];
    path[i] = 0;
    avi_close(m->avi);
    m->avi = avi_open(w32l_native_path(path, native));
    if (!m->avi) {
        w32l_plat_log("amstream: cannot open %s (%s)", path, native);
        return STG_E_FILENOTFOUND;
    }
    cinepak_close(m->cvid);
    m->cvid = cinepak_open(m->avi->width, m->avi->height);
    if (!m->cvid)
        return E_OUTOFMEMORY;
    m->next = 0;
    w32l_plat_log("amstream: %s %dx%d %u/%u fps, %u frames, codec %.4s", path, (int)m->avi->width,
                  (int)m->avi->height, (unsigned)m->avi->rate, (unsigned)m->avi->scale,
                  (unsigned)m->avi->nvideo, (const char *)&m->avi->fourcc);
    return S_OK;
}

/* IUnknown, IMultiMediaStream (3..11), IAMMultiMediaStream (12..18). */
const void *kMMVtbl[] = {
    (void *)MM_QI, (void *)MM_AddRef, (void *)MM_Release,
    (void *)mm_unsupported,         /* GetInformation */
    (void *)MM_GetMediaStream,
    (void *)mm_unsupported,         /* EnumMediaStreams */
    (void *)MM_GetState, (void *)MM_SetState,
    (void *)mm_unsupported,         /* GetTime */
    (void *)mm_unsupported,         /* GetDuration */
    (void *)MM_Seek,
    (void *)mm_unsupported,         /* GetEndOfStreamEventHandle */
    (void *)MM_Initialize,
    (void *)mm_unsupported,         /* GetFilterGraph */
    (void *)mm_unsupported,         /* GetFilter */
    (void *)MM_AddMediaStream, (void *)MM_OpenFile,
    (void *)mm_unsupported,         /* OpenMoniker */
    (void *)mm_unsupported,         /* Renderer */
};

} // namespace

HRESULT w32l_amstream_create(REFCLSID clsid, REFIID iid, LPVOID *out)
{
    MMStream *m;

    if (!IsEqualGUID(clsid, kCLSID_AMMultiMediaStream))
        return REGDB_E_CLASSNOTREG;
    if (!IsEqualGUID(iid, kIID_IAMMultiMediaStream) && !IsEqualGUID(iid, kIID_IMultiMediaStream) &&
        !IsEqualGUID(iid, kIID_IUnknown))
        return E_NOINTERFACE;
    m = (MMStream *)calloc(1, sizeof *m);
    if (!m)
        return E_OUTOFMEMORY;
    m->lpVtbl = kMMVtbl;
    m->refs = 1;
    *out = m;
    return S_OK;
}
