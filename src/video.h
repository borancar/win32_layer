/* video.h -- the pieces of the multimedia-streaming (amstream) video path. */
#ifndef W32L_VIDEO_H
#define W32L_VIDEO_H

#include "platform.h"

/* A Cinepak decoder: feed it frames in order, read the picture from `rgb`
 * (width*height*3 bytes, R,G,B, top row first). */
struct Cinepak {
    int32_t  width, height;
    uint8_t *rgb;
    void    *strips;
};

Cinepak *cinepak_open(int32_t width, int32_t height);
void     cinepak_close(Cinepak *c);
/* 0 on success. */
int32_t  cinepak_decode(Cinepak *c, const uint8_t *data, uint32_t size);

/* CoCreateInstance for the multimedia-stream classes; REGDB_E_CLASSNOTREG when
 * `clsid` is not one of them. */
HRESULT w32l_amstream_create(REFCLSID clsid, REFIID iid, LPVOID *out);

#endif
