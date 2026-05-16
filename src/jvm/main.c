#include <stdio.h>
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "jvm/jvm.h"

int main(int argc, char** argv) 
{
  printf("Hello world\n");
 
  if (argc < 2) return 1;
  
  FILE* file = fopen(argv[1], "rb");
  Reader reader = { NULL, 0 , 0};
  reader.buf = load_file(file, &reader.size);

  ClassFile* main_class = read_class(&reader);
  free(reader.buf);

  jvm_init(main_class);

  free_classfile(main_class);
  fclose(file);
  return 0;
}
