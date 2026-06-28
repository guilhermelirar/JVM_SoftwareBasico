#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "common/classfile_reader.h"
#include <string.h>
#include "jvm/utils.h"

static field_info* field_by_name_and_type(ClassFile* cf, 
    const char* name, const char* descriptor, u2* offset, bool is_static)
{
  field_info* f;
  *offset = 0;
  for (int i = 0; i < cf->fields_count; i++)
  {
    f = &cf->fields[i];

    if (is_static != ((f->access_flags & ACC_STATIC) != 0)) continue;  

    const char* f_dsc = cp_get_utf8(cf->constant_pool, f->descriptor_index);
    if (strcmp(name, cp_get_utf8(cf->constant_pool, f->name_index)) == 0 &&
        strcmp(descriptor, f_dsc) == 0)
    {
      return f;
    }
    
    if (f_dsc[0] == 'J' || f_dsc[0] == 'D')
      (*offset)++; // long e double ocupam 2 slots
    (*offset)++;
  }

  return NULL;
}


RuntimeField* resolve_field(JVM_Context* ctx, u2 cp_idx, bool is_static)
{
  Frame* frame = current_frame(ctx);
  cp_info* cp = constant_pool(frame);

  cp_info* entry = &cp[cp_idx];
  if (entry->tag != CONSTANT_Fieldref) return NULL;

  Resolved_cp_info* res = &frame->method.holder_class->cp[cp_idx];
  if (res->tag != CP_UNRESOLVED) 
    return &res->info.field;

  u2 class_index = entry->info.fieldref_info.class_index;
  u2 nt_index = entry->info.fieldref_info.name_and_type_index;
  
  cp_info* nt = &cp[nt_index];

  const char* name = cp_get_utf8(cp, 
      nt->info.name_and_type_info.name_index);
  const char* descriptor = cp_get_utf8(cp, 
      nt->info.name_and_type_info.descriptor_index);

  LoadedClass* clazz = resolve_class(ctx, class_index);
  if (clazz == NULL && 
      frame->method.holder_class->cp[class_index].tag == JAVA_LANG_SYSTEM 
      && strcmp(name, "out") == 0)
  {
    res->tag = CP_RESOLVED_FIELD;
    res->info.field.index = JAVA_SYSTEM_OUT_IDX;
    return &res->info.field;
  }

  field_info* f;  
  u2 field_idx;
  do {
    f = field_by_name_and_type(clazz->cf, name, descriptor, &field_idx, 
        is_static);
    
    if (f != NULL) 
    {
      res->tag = CP_RESOLVED_FIELD;
      res->info.field.holder_class = clazz;
      res->info.field.descriptor = descriptor;
      res->info.field.name = name;
      res->info.field.access_flags = f->access_flags;
      res->info.field.attributes = f->attributes;
      res->info.field.attributes_count = f->attributes_count;
      
      // se field é estático, offset é o índice,
      // caso contrário, deve-se somar ao tamanho da 
      // instância da superclasse
      if (f->access_flags & ACC_STATIC || clazz->super == NULL)
        res->info.field.index = field_idx;
      else
        res->info.field.index = field_idx + clazz->super->instance_size;

      return &res->info.field;
    }
    clazz = clazz->super;
  } while (clazz != NULL);

  return NULL;
}

LoadedClass* find_class_by_name(JVM_Context* ctx, const char* name)
{
  LoadedClass* class = NULL;
  for (int i = 0; i < ctx->classes_count; i++)
  {
    class = &ctx->method_area[i];

    // achou classe
    if (strcmp(name, class->name) == 0)
    {
      return class;
    }
  }

  // Inicializa LoadedClass* vazia para descrever arrays
  if (name[0] == '[')
  {
    class = &ctx->method_area[ctx->classes_count++];
    class->name = mystrdup(name);
    class->cp = NULL;
    class->instance_size = 0;
    return class;
  }

  return NULL;
}

LoadedClass* resolve_class(JVM_Context* ctx, u2 cp_idx)
{
  Frame* frame = current_frame(ctx);
  cp_info* cp = constant_pool(frame);
  Resolved_cp_info* resolved_entry = &frame->method.holder_class->cp[cp_idx];
  if (resolved_entry->tag != CP_UNRESOLVED) 
    return resolved_entry->info.clazz;

  cp_info* entry = &cp[cp_idx];
  if (entry->tag != CONSTANT_Class) return NULL;

  const char* name = cp_class_name(cp, cp_idx);
  if (strcmp(name, "java/lang/System") == 0)
  {
    resolved_entry->tag = JAVA_LANG_SYSTEM;
    resolved_entry->info.clazz = NULL;
    return NULL;
  }

  LoadedClass* clazz = get_class(ctx, name);
  if (clazz == NULL) return NULL;

  resolved_entry->tag = CP_RESOLVED_CLASS;
  resolved_entry->info.clazz = clazz;
  return clazz;
}

RuntimeMethod* resolve_method(JVM_Context* ctx, u2 cp_idx)
{
  Frame* frame = current_frame(ctx);
  Resolved_cp_info* res = &frame->method.holder_class->cp[cp_idx];

  // Já resolvido
  if (res->tag == CP_RESOLVED_METHOD)
    return &res->info.method; 
  
  // Não resolvido
  cp_info* cp = constant_pool(frame);

  LoadedClass* clazz = resolve_class(ctx, 
      cp[cp_idx].info.methodref_info.class_index);

  if (strcmp(clazz->name, "java/lang/Object") == 0)
  {
    res->info.method.holder_class = clazz;
    res->tag = CP_RESOLVED_METHOD;
    return &res->info.method;
  }
  
  cp_info* nt_info = &cp[cp[cp_idx].info.\
                     methodref_info.name_and_type_index];
  
  const char* name = cp_get_utf8(
      cp, 
      nt_info->info.name_and_type_info.name_index
  );

  const char* descriptor = cp_get_utf8(
      cp, 
      nt_info->info.name_and_type_info.descriptor_index
  );

  method_info* m;
  LoadedClass* curr = clazz;
  do {
    m = find_method(curr->cf, name, descriptor);
    if (m != NULL) break;
    curr = curr->super;
  } while (curr != NULL);

  if (m == NULL)
  {
    fprintf(stderr, "Error: NoSuchMethodError");
    goto terminate;
  }

  if (frame->method.holder_class != clazz && 
      (
       m->access_flags & ACC_PRIVATE || 
       (!extends(frame->method.holder_class, clazz) && 
        m->access_flags & ACC_PROTECTED)
       )
      )
  {
    fprintf(stderr, "Error: IllegalAccessError");
    goto terminate;
  }
 
  if (m->access_flags & ACC_ABSTRACT)
  {
    fprintf(stderr, "Error: AbstractMethodError");
    goto terminate;
  }

  init_RuntimeMethod(curr, m, &res->info.method);
  res->tag = CP_RESOLVED_METHOD;
  return &res->info.method;

terminate:
  terminateJVM(ctx);
  exit(1);
}


u4 load_string(JVM_Context* ctx, LoadedClass* clazz, u4 cp_idx)
{
  cp_info* entry = &clazz->cf->constant_pool[cp_idx];
  const char* str = cp_get_utf8(clazz->cf->constant_pool, 
          entry->info.string_info.string_index);
      
  // ver se está na tabela de strings do ctx ou inserir
  for (u4 i = 0; i < ctx->strings.count; i++)
  {
    if (str == ctx->strings.strings[i])
    {
      return i;
    }
  }
  
  ctx->strings.strings[ctx->strings.count] = (char*)str;
  ctx->strings.count++;
  return ctx->strings.count - 1;
}



