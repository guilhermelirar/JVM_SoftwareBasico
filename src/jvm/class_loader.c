// src/jvm/class_loader.h
#include <stdio.h>
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "common/reader.h"

ClassFile* ClassFile_from_path(const char *path)
{
  FILE* file = fopen(path, "rb");

  if (file == NULL)
  {
    fprintf(stderr, 
        "ERROR: could not load .class \"%s\":", 
        path);
    perror("");
    exit(1);
  }

  Reader r = {NULL, 0, 0};
  r.buf = load_file(file, &r.size);

  ClassFile* cf = read_class(&r);
  free(r.buf);
  fclose(file);

  return cf;
}
