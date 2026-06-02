// src/jvm/class_loader.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void alloc_static_fields(LoadedClass* class)
{
  u2 needed_slots = 0;

  for (int i = 0; i < class->cf->fields_count; i++)
  {
    field_info* f = &class->cf->fields[i];

    if (f->access_flags & ACC_STATIC)
    {
      const char* descriptor = cp_get_utf8(class->cf->constant_pool, 
          f->descriptor_index);
      
      // ocupam dois slots
      if (descriptor[0] == 'J' || descriptor[0] == 'D')
        needed_slots++;

      needed_slots++;
    }
  }

  class->static_fields = (u4*)calloc(needed_slots, sizeof(u4));
}

static inline void load_super(JVM_Context* ctx, ClassFile* cf)
{
  const char* super_name = cp_class_name(cf->constant_pool, 
      cf->super_class);
  
  if (strcmp(super_name, "java/lang/Object"))
  {
    load_class(ctx, cp_class_name(cf->constant_pool, 
          cf->super_class));
  }
}

void load_class(JVM_Context* ctx, const char* name) 
{
  char path[512];
  snprintf(path, sizeof(path), "%s%s%s", ctx->base_dir, name, ".class");
  ClassFile* cf = ClassFile_from_path(path);
  
  load_super(ctx, cf);

  // Colocando na área de métodos
  ctx->method_area[ctx->classes_count].cf = cf;
  alloc_static_fields(&ctx->method_area[ctx->classes_count]);
  ctx->classes_count++;

  method_info* clinit = find_method(cf, "<clinit>", "()V");
  if (clinit != NULL)
  {
    Frame* clinit_frame = new_frame(cf, clinit);
    push_frame(&ctx->t, clinit_frame);
    run_method(ctx, ctx->t.frame_ptr);
  }
}

void load_main_class(JVM_Context *ctx, const char *path)
{
  // carregando classfile
  ClassFile* main_class = ClassFile_from_path(path);

  load_super(ctx, main_class); // inicialização das superclasses

  ctx->method_area[ctx->classes_count].cf = main_class;
  alloc_static_fields(&ctx->method_area[ctx->classes_count]);
  ctx->classes_count++;

  method_info* clinit = find_method(main_class, "<clinit>", "()V");
  if (clinit != NULL)
  {
    Frame* clinit_frame = new_frame(main_class, clinit);
    push_frame(&ctx->t, clinit_frame);
    run_method(ctx, ctx->t.frame_ptr);
  }

  method_info* main = find_method(main_class, 
      "main", "([Ljava/lang/String;)V");
  
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

