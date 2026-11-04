/* cinepak.cpp -- a Cinepak ("cvid") video decoder.
 *
 * Cinepak codes a frame as horizontal strips, each a list of chunks: V1 and
 * V4 codebooks (256 vectors of four luma values and one chroma pair, held
 * here already as four RGB pixels) and the vectors that pick codebook entries
 * for each 4x4 block, with a skip bit per block in inter frames. The decoder
 * keeps the picture between calls, which is what an inter frame draws on.
 *
 * The format is described in the public Cinepak documentation and
 * implemented in several free decoders; this is written from that
 * description. Colour only (the 6-byte codebook vectors); the grey-scale
 * 4-byte form is read but copied as grey.
 */
#include "video.h"

#include <stdlib.h>
#include <string.h>

namespace {

struct Strip {
    uint8_t v4[256][12];
    uint8_t v1[256][12];
    int32_t y1, y2;
};

inline uint8_t clip8(int32_t v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }
inline uint32_t be16(const uint8_t *p) { return (uint32_t)(p[0] << 8 | p[1]); }
inline uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* A codebook chunk: bit 0x01 = partial update (a flag word per 32 entries),
 * bit 0x04 = grey (4 bytes an entry) rather than colour (6). */
void read_codebook(uint8_t cb[256][12], uint32_t id, const uint8_t *data, const uint8_t *eod)
{
    int32_t  n = (id & 0x04) ? 4 : 6;
    uint32_t flag = 0, mask = 0;
    int32_t  i;

    for (i = 0; i < 256; i++) {
        if ((id & 0x01) && !(mask >>= 1)) {
            if (data + 4 > eod)
                break;
            flag = be32(data);
            data += 4;
            mask = 0x80000000u;
        }
        if (!(id & 0x01) || (flag & mask)) {
            uint8_t *p = cb[i];
            int32_t  k;

            if (data + n > eod)
                break;
            for (k = 0; k < 4; k++) {
                uint8_t y = *data++;
                p[k * 3] = p[k * 3 + 1] = p[k * 3 + 2] = y;
            }
            if (n == 6) {
                int32_t u = (int8_t)*data++;
                int32_t v = (int8_t)*data++;
                for (k = 0; k < 4; k++) {
                    int32_t y = p[k * 3];
                    p[k * 3]     = clip8(y + v * 2);
                    p[k * 3 + 1] = clip8(y - u / 2 - v);
                    p[k * 3 + 2] = clip8(y + u * 2);
                }
            }
        }
    }
}

/* One codebook entry's 2x2 pixels into the picture at (x,y). */
inline void put2x2(uint8_t *row0, uint8_t *row1, const uint8_t *p)
{
    memcpy(row0, p, 6);
    memcpy(row1, p + 6, 6);
}

void read_vectors(Cinepak *c, Strip *s, uint32_t id, const uint8_t *data, const uint8_t *eod)
{
    uint32_t flag = 0, mask = 0;
    int32_t  x, y, stride = c->width * 3;

    for (y = s->y1; y < s->y2 && y < c->height; y += 4) {
        for (x = 0; x < c->width; x += 4) {
            uint8_t *ip = c->rgb + (size_t)y * (size_t)stride + (size_t)x * 3;

            if ((id & 0x01) && !(mask >>= 1)) {
                if (data + 4 > eod)
                    return;
                flag = be32(data);
                data += 4;
                mask = 0x80000000u;
            }
            if (!(id & 0x01) || (flag & mask)) {
                if (!(id & 0x02) && !(mask >>= 1)) {
                    if (data + 4 > eod)
                        return;
                    flag = be32(data);
                    data += 4;
                    mask = 0x80000000u;
                }
                if ((id & 0x02) || (~flag & mask)) {
                    /* V1: one entry, each of its four pixels a 2x2 block. */
                    const uint8_t *p;
                    int32_t        k;
                    if (data >= eod)
                        return;
                    p = s->v1[*data++];
                    for (k = 0; k < 4; k++) {
                        uint8_t *q = ip + (k >> 1) * 2 * stride + (k & 1) * 6;
                        uint8_t *q1 = q + stride;
                        memcpy(q, p + k * 3, 3);
                        memcpy(q + 3, p + k * 3, 3);
                        memcpy(q1, p + k * 3, 3);
                        memcpy(q1 + 3, p + k * 3, 3);
                    }
                } else if (flag & mask) {
                    /* V4: four entries, one 2x2 block each. */
                    int32_t k;
                    if (data + 4 > eod)
                        return;
                    for (k = 0; k < 4; k++) {
                        uint8_t *q = ip + (k >> 1) * 2 * stride + (k & 1) * 6;
                        put2x2(q, q + stride, s->v4[*data++]);
                    }
                }
            }
        }
    }
}

} // namespace

Cinepak *cinepak_open(int32_t width, int32_t height)
{
    Cinepak *c = (Cinepak *)calloc(1, sizeof *c);

    if (!c)
        return NULL;
    c->width = width;
    c->height = height;
    c->rgb = (uint8_t *)calloc((size_t)width * (size_t)height * 3 + 64, 1);
    c->strips = calloc(32, sizeof(Strip));
    if (!c->rgb || !c->strips) {
        cinepak_close(c);
        return NULL;
    }
    return c;
}

void cinepak_close(Cinepak *c)
{
    if (!c)
        return;
    free(c->rgb);
    free(c->strips);
    free(c);
}

int32_t cinepak_decode(Cinepak *c, const uint8_t *data, uint32_t size)
{
    const uint8_t *eof = data + size;
    Strip         *strips = (Strip *)c->strips;
    uint32_t       flags, nstrips, i;
    int32_t        y0 = 0;

    if (size < 10)
        return -1;
    flags = data[0];
    nstrips = be16(data + 8);
    data += 10;
    if (nstrips > 32)
        nstrips = 32;
    for (i = 0; i < nstrips; i++) {
        Strip         *s = &strips[i];
        uint32_t       ssize;
        const uint8_t *eos;

        if (data + 12 > eof)
            return -1;
        /* Without the "new codebooks" flag a strip starts from the last one's. */
        if (i > 0 && !(flags & 1)) {
            memcpy(s->v4, strips[i - 1].v4, sizeof s->v4);
            memcpy(s->v1, strips[i - 1].v1, sizeof s->v1);
        }
        s->y1 = y0;
        s->y2 = y0 + (int32_t)be16(data + 8);
        ssize = be16(data + 2);
        if (ssize < 12 || data + ssize > eof)
            ssize = (uint32_t)(eof - data);
        eos = data + ssize;
        data += 12;
        while (data + 4 <= eos) {
            uint32_t id = be16(data);
            uint32_t csize = be16(data + 2);
            const uint8_t *body = data + 4;
            const uint8_t *end;

            if (csize < 4)
                break;
            end = data + csize;
            if (end > eos)
                end = eos;
            switch (id >> 8) {
            case 0x20: case 0x21: case 0x24: case 0x25:
                read_codebook(s->v4, id >> 8, body, end);
                break;
            case 0x22: case 0x23: case 0x26: case 0x27:
                read_codebook(s->v1, id >> 8, body, end);
                break;
            case 0x30: case 0x31: case 0x32:
                read_vectors(c, s, id >> 8, body, end);
                break;
            default:
                break;
            }
            data = end;
        }
        data = eos;
        y0 = s->y2;
    }
    return 0;
}
