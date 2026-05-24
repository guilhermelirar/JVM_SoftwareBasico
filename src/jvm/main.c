#include <stdio.h>
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"

int main(int argc, char** argv) 
{
  if (argc < 2) return 1;

  
  FILE* file = fopen(argv[1], "rb");
  
  if (file == NULL)
  {
    fprintf(stderr, 
        "ERROR: classfile \"%s\" does not exist or could not be opened\n", 
        argv[1]);
    exit(1);
  }
  

  Reader reader = { NULL, 0 , 0};
  reader.buf = load_file(file, &reader.size);

  ClassFile* main_class = read_class(&reader);
  free(reader.buf);

  JVM_Context* ctx = jvm_init(main_class);
  jvm_run(ctx);

  free_classfile(main_class);
  terminateJVM(ctx);
  fclose(file);
  return 0;
}
