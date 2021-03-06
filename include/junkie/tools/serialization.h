// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#ifndef SERIALIZATION_H_111031
#define SERIALIZATION_H_111031
#include <stdint.h>
#include <assert.h>
#include <string.h>

static inline void serialize_1(uint8_t **buf, uint_least8_t v)
{
    **buf = v;
    (*buf)++;
}

static inline void serialize_2(uint8_t **buf, uint_least16_t v)
{
    serialize_1(buf, v);
    serialize_1(buf, v>>8U);
}

static inline void serialize_3(uint8_t **buf, uint_least32_t v)
{
    serialize_2(buf, v);
    serialize_1(buf, v>>16U);
}

static inline void serialize_4(uint8_t **buf, uint_least32_t v)
{
    serialize_2(buf, v);
    serialize_2(buf, v>>16U);
}

static inline void serialize_8(uint8_t **buf, uint_least64_t v)
{
    serialize_4(buf, v);
    serialize_4(buf, v>>32ULL);
}

static inline void serialize_n(uint8_t **buf, void const *src, size_t n)
{
    memcpy(*buf, src, n);
    (*buf) += n;
}

static inline void serialize_str(uint8_t **buf, char const *s)
{
    unsigned c;
    for (c = 0; s[c] != '\0'; c++) {
        (*buf)[c+2] = s[c];
    }
    (*buf)[0] = c;  // Note: must be same endianness than serialize_2()
    (*buf)[1] = c>>8;
    *buf += c+2;
}

static inline uint_least8_t deserialize_1(uint8_t const **buf)
{
    return *((*buf)++);
}

static inline uint_least16_t deserialize_2(uint8_t const **buf)
{
    return deserialize_1(buf) + ((uint_least16_t)deserialize_1(buf)<<8U);
}

static inline uint_least32_t deserialize_3(uint8_t const **buf)
{
    return deserialize_2(buf) + ((uint_least32_t)deserialize_1(buf)<<16U);
}

static inline uint_least32_t deserialize_4(uint8_t const **buf)
{
    return deserialize_2(buf) + ((uint_least32_t)deserialize_2(buf)<<16U);
}

static inline uint_least64_t deserialize_8(uint8_t const **buf)
{
    return deserialize_4(buf) + ((uint_least64_t)deserialize_4(buf)<<32ULL);
}

static inline void deserialize_n(uint8_t const **buf, void *dst, size_t n)
{
    memcpy(dst, *buf, n);
    (*buf) += n;
}

// returns the length of the result string that was copied in dst, not including the terminating nul
static inline unsigned deserialize_str(uint8_t const **buf, char *dst, size_t max_len)
{
    unsigned n = deserialize_2(buf);
    assert(n < max_len);
    deserialize_n(buf, (uint8_t *)dst, n);
    dst[n] = '\0';
    return n;
}

#endif
