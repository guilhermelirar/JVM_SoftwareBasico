#include "jvm/jvm.h"
#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "common/bytecode.h"
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_MODE
  #include <stdio.h>
  #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...) do {} while (0)
#endif

static Code_attribute* find_code_attr(cp_info* cp, method_info* m)
{
  Code_attribute* code = NULL;
  for (u4 i = 0; i < m->attributes_count; i++)
  {
    if (strcmp("Code", cp_get_utf8(cp, 
            m->attributes[i].attribute_name_index)) == 0)
    {
      code = m->attributes[i].info.code_attribute;
      return code;
    }
  }
  return NULL;
}

void init_RuntimeMethod(LoadedClass* holder_class, method_info* m_info, 
    RuntimeMethod* runtime_m)
{
  if (runtime_m == NULL || holder_class == NULL) return;
  cp_info* cp = holder_class->cf->constant_pool;

  runtime_m->holder_class = holder_class;
  runtime_m->info = m_info;
  runtime_m->code_attr = find_code_attr(cp, m_info);
  runtime_m->name = cp_get_utf8(cp, m_info->name_index);
  runtime_m->descriptor = cp_get_utf8(cp, m_info->descriptor_index);
  runtime_m->args_size = count_args_size(runtime_m->descriptor);
}

method_info* find_method(ClassFile *cf, const char* name, 
    const char* descriptor) 
{
  if (cf == NULL) return NULL;

  // Tenta procurar o método
  method_info* m = NULL;
  for (u2 method_idx = 0; method_idx < cf->methods_count; method_idx++)
  {
    m = &cf->methods[method_idx];
    if (
        strcmp(name, 
          cp_get_utf8(cf->constant_pool, m->name_index)) == 0 && 
        strcmp(descriptor, 
          cp_get_utf8(cf->constant_pool, m->descriptor_index)) == 0
        ) 
    {
      return m; 
    }
  }

  // Método não encontrado
  return NULL;
}

LoadedClass* find_superclass_with_method(JVM_Context* ctx, 
    LoadedClass* base_class, const char* method_name, const char* descriptor)
{
  if (base_class->cf->super_class == 0) return NULL;

  const char* super_name = cp_class_name(
      base_class->cf->constant_pool,
      base_class->cf->super_class
  );

  // acha super_classe, se não achar (for Object) retorna NULL
  LoadedClass* super_class = find_class_by_name(ctx, super_name);
  if (super_class == NULL) return NULL;

  // se achar na super classe, a retorna
  method_info* method = find_method(super_class->cf, method_name, descriptor);
  if (method != NULL) return super_class; 

  // procura na hierarquia
  return find_superclass_with_method(ctx, super_class, 
      method_name, descriptor);
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
  
  return NULL;
}

Frame* new_frame(RuntimeMethod* method)
{
  Frame* frame = (Frame*)malloc(sizeof(Frame));
  if (frame == NULL) return NULL;


  frame->locals = (u4*)calloc(method->code_attr->max_locals, sizeof(u4));
  frame->operand_stack = (u4*)calloc(method->code_attr->max_stack, sizeof(u4));
  frame->stack_ptr = -1;
  
  frame->pc = method->code_attr->code;
  frame->method = *method;
  return frame;

  exit(1);
}

JVM_Context* jvm_init()
{
  JVM_Context* ctx = (JVM_Context*)malloc(sizeof(JVM_Context));
  if (ctx == NULL) return NULL;

  ctx->objects.capacity = JVM_HEAP_CAPACITY;
  ctx->objects.count = 0;

  ctx->classes_count = 0;
  ctx->strings.count = 0;
  ctx->strings.capacity = JVM_STRING_TABLE_SIZE;

  ctx->t.frame_ptr = -1;
 
  return ctx;
}

void run_method(JVM_Context *ctx, int frame_ptr)
{
  while (ctx->t.frame_ptr >= frame_ptr)
  {
    Frame* frame = current_frame(ctx);
    u1 opcode = *frame->pc++;
    
    DEBUG_PRINT("[DEBUG_RUN] frame_ptr=%2d | pc=%3u | opc=0x%02X (%s)\n", 
      ctx->t.frame_ptr, frame->pc - 1, opcode, opcode_table[opcode].name);
    
    DISPATCH_TABLE[opcode](ctx, opcode);
  }
}

int count_args_size(const char* descriptor) 
{
  int count = 0;
  int i = 1; // Pula '('
  while (descriptor[i] != ')')
  {
    // Arrays sempre ocupam 1 slot
    if (descriptor[i] == '[')
    {
      while (descriptor[i] == '[') i++; // Pula dimensões do array
     
      // Pula nome de classe
      if (descriptor[i] == 'L') 
      {
        while(descriptor[i] != ';') i++;
      }
      
      // Pula ';' em caso de classe, ou descritor simples
      // que não precisa ser tratado
      i++;
      count++;
      continue;
    }

    // Long e Double usam slot extra
    if (descriptor[i] == 'D' || descriptor[i] == 'J')
      count++;

    // Pular nome de classe
    if (descriptor[i] == 'L')
      while(descriptor[i] != ';') i++;

    count++;
    i++;
  }

  return count;
}

void terminateJVM(JVM_Context *ctx)
{
  while (ctx->t.frame_ptr >= 0) 
  {
    free_frame(ctx->t.frames[ctx->t.frame_ptr]);
    ctx->t.frame_ptr--;
  }

  while (ctx->classes_count--)
  {
    free_classfile(ctx->method_area[ctx->classes_count].cf);
    free(ctx->method_area[ctx->classes_count].static_fields);
    free(ctx->method_area[ctx->classes_count].cp);  // constant pool resolvida
  }

  free(ctx);
}

void free_frame(Frame *f)
{
  if (f == NULL) return;
  free(f->operand_stack);
  free(f->locals);
  free(f);
}

const instruction_handler DISPATCH_TABLE[256] = {
#include "jvm/dispatch_table.def"
};

void jvm_run(JVM_Context* ctx, const char* entry_class_name)
{
  stack_main_frame(ctx, entry_class_name);

  while (ctx->t.frame_ptr >= 0)
  {
    Frame* frame = current_frame(ctx);
    u1 opcode = *frame->pc++;
    
    DEBUG_PRINT("[DEBUG_RUN] frame_ptr=%2d | pc=%3u | opc=0x%02X (%s)\n", 
      ctx->t.frame_ptr, frame->pc - 1, opcode, opcode_table[opcode].name);
    
    DISPATCH_TABLE[opcode](ctx, opcode);
  }
}

method_info* lookup_method(LoadedClass** base_class_pp, 
    const char* method_name, const char* method_descriptor)
{
  if (base_class_pp == NULL || *base_class_pp == NULL) return NULL;

  method_info* m = NULL;
  LoadedClass* curr = *base_class_pp;
 
  // Procura na atual e nas superclasse até achar
  do {
     m = find_method(curr->cf, method_name, method_descriptor);
     if (m != NULL)
     {
       *base_class_pp = curr;
       return m;
     }

     curr = curr->super;
  } while (curr != NULL); 

  return NULL;
}

void stack_main_frame(JVM_Context* ctx, const char* entry_class_name)
{
  if (ctx == NULL) return;

  LoadedClass* entry_class_loaded = get_class(ctx, entry_class_name);

  method_info* main; 
  LoadedClass* curr = entry_class_loaded;
 
  // Procura na atual e nas superclasse até achar
  do {
     main = find_method(curr->cf, "main", "([Ljava/lang/String;)V");
     if (main != NULL) break;
     curr = curr->super;
  } while (curr != NULL); 

  // Testando se flags de acesso estão corretas
  u2 required = ACC_STATIC | ACC_PUBLIC;
  if (main == NULL || (main->access_flags & required) != required)
    goto err_main_not_found;

  RuntimeMethod main_method;
  init_RuntimeMethod(curr, main, &main_method);
  Frame* f = new_frame(&main_method);
  push_frame(&ctx->t, f);

  initialize_class(ctx, entry_class_loaded); // <clinit> e static fields
  
  return;

err_main_not_found:
  fprintf(stderr, 
      "Error: Main method not found"
      " in class %s, please define the main method as:\n"
      "public static void main(String[] args)\n",
      entry_class_name);
  terminateJVM(ctx);
  exit(1);
}


static field_info* field_by_name_and_type(ClassFile* cf, 
    const char* name, const char* descriptor, u2* offset)
{
  field_info* f;
  *offset = 0;
  for (int i = 0; i < cf->fields_count; i++)
  {
    f = &cf->fields[i];
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

bool extends(LoadedClass* class_a, LoadedClass* class_b)
{
  do {
    if (class_a == class_b) return true;
    class_a = class_a->super;
  } while (class_a != NULL);
  
  return false;
}

RuntimeField* resolve_field(JVM_Context* ctx, u2 cp_idx)
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
    f = field_by_name_and_type(clazz->cf, name, descriptor, &field_idx);
    if (f != NULL) 
    {
      res->tag = CP_RESOLVED_FIELD;
      res->info.field.holder_class = clazz;
      res->info.field.descriptor = descriptor;
      res->info.field.name = name;
      res->info.field.access_flags = f->access_flags;
      res->info.field.attributes = f->attributes;
      res->info.field.attributes_count = f->attributes_count;
      res->info.field.index = field_idx;
      return &res->info.field;
    }
    clazz = clazz->super;
  } while (clazz != NULL);

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
