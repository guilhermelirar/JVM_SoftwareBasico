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
    if (strcmp(name, cp_class_name(class->cf->constant_pool, 
            class->cf->this_class)) == 0)
    {
      return class;
    }
  }
  
  return NULL;
}

Frame* new_frame(ClassFile* cf, method_info* method)
{
  Frame* frame = (Frame*)malloc(sizeof(Frame));
  if (frame == NULL) return NULL;

  Code_attribute* code = NULL;
  for (u4 i = 0; i < method->attributes_count; i++)
  {
    if (strcmp("Code", cp_get_utf8(cf->constant_pool, 
            method->attributes[i].attribute_name_index)) == 0)
    {
      code = method->attributes[i].info.code_attribute;
      break;
    }
  }

  frame->constant_pool = cf->constant_pool;
  frame->locals = (u4*)calloc(code->max_locals, sizeof(u4));
  frame->operand_stack = (u4*)calloc(code->max_stack, sizeof(u4));
  frame->pc = 0;
  frame->code = code->code;
  frame->stack_ptr = -1;

  return frame;
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
    u1 opcode = fetch_u1(frame->code, &frame->pc);
    
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

void jvm_run(JVM_Context* ctx)
{
  while (ctx->t.frame_ptr >= 0)
  {
    Frame* frame = current_frame(ctx);
    u1 opcode = fetch_u1(frame->code, &frame->pc);
    
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
  LoadedClass** main_class_loaded_pp = &entry_class_loaded;

  method_info* main_method = lookup_method(main_class_loaded_pp, 
      "main", "([Ljava.lang.String)V;");

  // Testando se flags de acesso estão corretas
  u2 required = ACC_STATIC | ACC_PUBLIC;
  if (main_method == NULL || 
      (main_method->access_flags & required) != required)
    goto err_main_not_found;

  initialize_class(ctx, entry_class_loaded); // <clinit> e static fields

  Frame* f = new_frame((*main_class_loaded_pp)->cf, main_method);
  push_frame(&ctx->t, f);
  
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
