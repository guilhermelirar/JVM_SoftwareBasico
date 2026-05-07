#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdio.h>

typedef uint8_t  u1;  
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

typedef struct {
  u1 *buf;
  u4 size;
  u4 pos;
} Reader;

static inline u1 read_u1(Reader *r) {
  return r->buf[r->pos++];
}

static inline u2 read_u2(Reader *r) {
  u2 v = ((u2)r->buf[r->pos] << 8) |
         ((u2)r->buf[r->pos + 1]);
  r->pos += 2;
  return v;
}

static inline u4 read_u4(Reader *r) {
  u4 v = ((u4)r->buf[r->pos]     << 24) |
         ((u4)r->buf[r->pos + 1] << 16) |
         ((u4)r->buf[r->pos + 2] << 8)  |
          (u4)r->buf[r->pos + 3];
  r->pos += 4;
  return v;
}

u1 *load_file(FILE *f, u4 *size); 

#endif
