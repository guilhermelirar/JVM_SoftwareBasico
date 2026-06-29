// src/jvm/class_loader.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "common/reader.h"
#include "jvm/jvmtypes.h"
#include "jvm/jvm.h"
#include "jvm/utils.h"

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

  if (cf == NULL)
  {
    fprintf(stderr, "Aborting...\n");
    exit(1);
  }

  return cf;
}

static attribute_info* get_constant_value(LoadedClass* class, field_info* f)
{
  for (int i = 0; i < f->attributes_count; i++)
  {
    const char* attr_name = cp_get_utf8(class->cf->constant_pool, 
        f->attributes[i].attribute_name_index);

    if (strcmp(attr_name, "ConstantValue") == 0)
    {
      return &f->attributes[i];
    }
  }

  return NULL;
}

static void init_static_fields(JVM_Context* ctx, LoadedClass* class)
{
  int offset = 0;
  for (int i = 0; i < class->cf->fields_count; i++)
  {
    field_info* f = &class->cf->fields[i];
    const char* descriptor = cp_get_utf8(class->cf->constant_pool, 
          f->descriptor_index);
   
    if (f->access_flags & ACC_STATIC)
    {

      attribute_info* cv = get_constant_value(class, f);
      if (cv)
      {
        cp_info* cv_info = &class->cf->
          constant_pool[cv->info.constantvalue_index];

        switch (descriptor[0])
        {
          case 'I':
          case 'Z':
          case 'B':
          case 'C':
          case 'S':
            class->static_fields[offset] = cv_info->info.int_info.bytes;
            break;

          case 'F':
            class->static_fields[offset] = cv_info->info.float_info.bytes;
            break;

          case 'J':
            class->static_fields[offset] = cv_info->info.long_info.h_bytes;
            class->static_fields[offset+1] = cv_info->info.long_info.l_bytes;
            break;

          case 'D':
            class->static_fields[offset] = cv_info->info.double_info.h_bytes;
            class->static_fields[offset+1] = cv_info->info.double_info.l_bytes;
            break;

          case 'L':
            if (strcmp("Ljava/lang/String;", descriptor) == 0)
            {
              u4 str_idx = load_string(ctx, class, 
                  cv_info->info.string_info.string_index);

              class->static_fields[offset] = str_idx;
            } 

          default:
            break;
        }
      }
    }
    
    if (descriptor[0] == 'J' || descriptor[0] == 'D')
      offset++;

    offset++;

  }
}

// inicializa static_fields_size e instance_size
// isto é, número de slots de 32 bits para representar
// campos estáticos e de instância
static void resolve_fields_size(LoadedClass* class)
{
  class->static_fields_size = 0;
  class->instance_size = 0;

  for (int i = 0; i < class->cf->fields_count; i++)
  {
    u1 field_size;
    field_info* f = &class->cf->fields[i];

    const char* descriptor = cp_get_utf8(class->cf->constant_pool, 
        f->descriptor_index);
      
    // double e long ocupam dois slots
    field_size = (descriptor[0] == 'J' || descriptor[0] == 'D') ? 2 : 1;

    if (f->access_flags & ACC_STATIC)
      class->static_fields_size += field_size;
    else
      class->instance_size += field_size;
  }

  if (class->super != NULL)
    class->instance_size += class->super->instance_size;

  class->static_fields = (u4*)calloc(class->static_fields_size, sizeof(u4));
}

void initialize_class(JVM_Context* ctx, LoadedClass* loaded)
{
  if (loaded == NULL || loaded->is_initialized == true) return;

  // empilha clinit, para ser executado depois da superclasse (se houver)
  method_info* clinit = find_method(loaded->cf, "<clinit>", "()V");
  if (clinit != NULL)
  {
    RuntimeMethod rclinit;
    init_RuntimeMethod(loaded, clinit, &rclinit);
    Frame* clinit_frame = new_frame(&rclinit);
    push_frame(ctx, clinit_frame);
  }

  // marca como inicializado para evitar loop
  loaded->is_initialized = true; 
  if (loaded->super != NULL) {
    initialize_class(ctx, loaded->super);
  }

  // descorbrir tamanho de static fields e instance fields 
  // em termos de slots de 32 bits
  resolve_fields_size(loaded); 
  init_static_fields(ctx, loaded); 
}


LoadedClass* load_class(JVM_Context* ctx, const char* name) 
{
  // Caso especial: java/lang/Object ou String
  if (strcmp("java/lang/Object", name) == 0 ||
      strcmp("java/lang/String", name) == 0 ||
      strcmp("java/io/PrintStream", name) == 0) 
  {
    LoadedClass* loaded = &ctx->method_area[ctx->classes_count++];
    loaded->name = mystrdup(name);
    loaded->cf = NULL;
    loaded->super = NULL;
    loaded->static_fields = NULL;
    loaded->cp = NULL;
    loaded->is_initialized = true; 
    loaded->instance_size = 0; 
    return loaded; 
  }

  ClassFile* cf = NULL;
  LoadedClass* super_loaded = NULL;

  char path[512];
  snprintf(path, sizeof(path), "%s%s.class", ctx->base_dir, name);
  cf = ClassFile_from_path(path);

  const char* super_name = get_superclass_name(cf);
  if (super_name != NULL || !strcmp(super_name, "java/lang/Object")) 
  {
    super_loaded = get_class(ctx, super_name); 
  }

  LoadedClass* loaded = &ctx->method_area[ctx->classes_count++];
  loaded->name = mystrdup(name);
  loaded->cf = cf;
  loaded->static_fields = NULL;
  loaded->cp = (Resolved_cp_info*)calloc(cf->constant_pool_count, 
      sizeof(Resolved_cp_info));
  loaded->is_initialized = false;
  loaded->super = super_loaded;

  return loaded;
}

LoadedClass* get_class(JVM_Context* ctx, const char* name)
{
  LoadedClass* class = find_class_by_name(ctx, name);
  if (class == NULL)
    class = load_class(ctx, name);

  return class;
}

