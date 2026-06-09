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

  if (cf == NULL)
  {
    fprintf(stderr, "Aborting...\n");
    exit(1);
  }

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

void initialize_class(JVM_Context* ctx, LoadedClass* loaded)
{
  if (loaded->is_initialized == true) return;

  // Inicializando classe atual e superclasses
  LoadedClass* curr = loaded;
  do {
    alloc_static_fields(curr); // inicializa campos estáticos

    // Empilhando <clinit> se houver
    method_info* clinit = find_method(curr->cf, "<clinit>", "()V");
    if (clinit != NULL)
    {
      Frame* clinit_frame = new_frame(curr, clinit);
      push_frame(&ctx->t, clinit_frame);
    }

    curr->is_initialized = true; // <clinit> em progresso ou encerrado
    curr = curr->super;  // Para inicializar a superclasse 
  } while (curr != NULL);
}

LoadedClass* load_class(JVM_Context* ctx, const char* name) 
{
  char path[512];
  snprintf(path, sizeof(path), "%s%s.class", ctx->base_dir, name);
  ClassFile* cf = ClassFile_from_path(path);

  // Se há superclasse carregável por esta implementação da JVM
  LoadedClass* super_loaded = NULL;
  const char* super_name = get_superclass_name(cf);
  if (super_name != NULL && strcmp(super_name, "java/lang/Object") != 0)
  {
    super_loaded = load_class(ctx, get_superclass_name(cf)); 
  }

  LoadedClass* loaded = &ctx->method_area[ctx->classes_count++];
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

