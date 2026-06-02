// src/jvm/class_loader.h
#include <stdio.h>
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "common/reader.h"
#include "jvm/jvmtypes.h"
#include "jvm/jvm.h"

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

void load_class(JVM_Context* ctx, const char* name) {
  char result[512];
  snprintf(result, sizeof(result), "%s%s%s", ctx->base_dir, name, ".class");

  printf("%s\n", result);
};

void load_main_class(JVM_Context *ctx, const char *path)
{
  // carregando classfile
  ClassFile* main_class = ClassFile_from_path(path);

  if (main_class->super_class) // super classe não é java/lang/Object
  {
    load_class(ctx, cp_class_name(main_class->constant_pool, 
          main_class->super_class));
  }

  method_info* clinit = find_method(main_class, "<clinit>", "()V");
  if (clinit)
  {
    Frame* clinit_frame = new_frame(main_class, clinit);
    push_frame(&ctx->t, clinit_frame);
    run_method(ctx, ctx->t.frame_ptr);
  }

  method_info* main = find_method(main_class, "main", "([Ljava/lang/String;)V");
  if (main == NULL || 
      !(main->access_flags & ACC_PUBLIC) || 
      !(main->access_flags & ACC_STATIC)) 
  {
    printf("ERROR: method main (public void main(String[] args))" 
        "not found in class \"%s\"\n", 
        cp_class_name(main_class->constant_pool, main_class->this_class));
  }

  Frame* main_frame = new_frame(main_class, main);
  push_frame(&ctx->t, main_frame);
  return;
}

