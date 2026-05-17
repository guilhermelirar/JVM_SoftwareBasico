#include "jvm/jvm.h"
#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
        strcmp(name, cp_get_utf8(cf, m->name_index)) == 0 && 
        strcmp(descriptor, cp_get_utf8(cf, m->descriptor_index)) == 0
        ) 
    {
      return m; 
    }
  }

  // Método não encontrado
  return NULL;
}

Frame* new_frame(ClassFile* cf, method_info* method)
{
  Frame* frame = (Frame*)malloc(sizeof(Frame));
  if (frame == NULL) return NULL;

  Code_attribute* code = NULL;
  for (u4 i = 0; i < method->attributes_count; i++)
  {
    if (strcmp("Code", cp_get_utf8(cf, 
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

  return frame;
}

JVM_Context* jvm_init(ClassFile* main_class)
{
  // Busca por main (obrigatório)
  method_info* main = find_method(main_class, "main", "([Ljava/lang/String;)V");
  if (main == NULL || 
      !(main->access_flags & ACC_PUBLIC) || 
      !(main->access_flags & ACC_STATIC)) 
  {
    printf("ERROR: method main (public void main(String[] args))" 
        "not found in class \"%s\"\n", 
        cp_class_name(main_class, main_class->this_class));
    return NULL;
  }

  JVM_Context* ctx = (JVM_Context*)malloc(sizeof(JVM_Context));
  if (ctx == NULL) return NULL;

  ctx->heap.capacity = JVM_HEAP_CAPACITY;
  ctx->heap.count = 0;
  ctx->t.frame_ptr = -1;

  // TODO resto
  return ctx;
}

void terminateJVM(JVM_Context *ctx)
{
  while (ctx->t.frame_ptr >= 0) 
  {
    free_frame(ctx->t.frames[ctx->t.frame_ptr]);
    ctx->t.frame_ptr--;
  }
  // TODO percorrer estrutura para liberar outros ponteiros
  free(ctx);
}

void free_frame(Frame *f)
{
  if (f == NULL) return;
  free(f->operand_stack);
  free(f->locals);
  free(f);
}
