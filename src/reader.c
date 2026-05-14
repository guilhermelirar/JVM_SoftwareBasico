#include "common/reader.h"
#include <stdlib.h>

u1 *load_file(FILE *f, u4 *size) {
  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);

  u1 *buf = malloc(*size);
  fread(buf, 1, *size, f);
  return buf;
}
