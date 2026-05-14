#include <stdio.h>
#include <stdlib.h>
#include "common/reader.h"
#include "common/classfile.h"
#include "display/display.h"
#include "common/classfile_reader.h"


int main(int argc, const char **argv) {
  ClassFile* class;

  if (argc < 2 || argc > 3) {
    printf("Usage:\n  %s <.class> [output_file]\n", argv[0]);
    return 1;
  }

  const char* filepath = argv[1];
  
  FILE* file = fopen(filepath, "rb");
  Reader reader = { NULL, 0 , 0};
  reader.buf = load_file(file, &reader.size);

  class = read_class(&reader);
  free(reader.buf);
  if (!class) return 1;

  FILE* output_file = argc == 3 ? fopen(argv[2], "w") : stdout;

  printclass(class, output_file);
  free_classfile(class);

  fclose(file);
  if (argc == 3) fclose(output_file);
  return 0;
}
