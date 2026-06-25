#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static u4 new_object(JVM_Context* ctx, LoadedClass* clazz)
{
  if (ctx->objects.count == ctx->objects.capacity)
    return 0; // NULL
  u4 idx = ctx->objects.count++;
  ctx->objects.entries[idx].type = OBJ_INSTANCE;
  ctx->objects.entries[idx].clazz = clazz; 
  ctx->objects.entries[idx].content.fields = 
    (u4*)calloc(clazz->instance_size, sizeof(u4));

  return idx;
}

void handle_new(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u1* opc_pc = frame->pc - 1; 

  u2 cp_idx = fetch_u2(&frame->pc);
  LoadedClass* clazz = resolve_class(ctx, cp_idx);

  // instanciação de classe que não pode ser instanciada
  if (clazz->cf->access_flags & ACC_ABSTRACT ||
      clazz->cf->access_flags & ACC_INTERFACE)
  {
    // TODO exceção 
    fprintf(stderr, "Illegal instantiation");
    terminateJVM(ctx);
    exit(1);
  }

  // inicializa classe antes de executar
  if (clazz != NULL && !clazz->is_initialized)
  {
    initialize_class(ctx, clazz);
    frame->pc = opc_pc;
    return;
  }

  u4 objectref = new_object(ctx, clazz);

  push_operand(frame, objectref);
}

void handle_newarray(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u1 type = *(frame->pc++);
 
  // obtém número de elementos e inicializa número de slots
  int32_t count = (int32_t)pop_operand(frame);
  if (count < 0)
  {
    fprintf(stderr, "NegativeArraySizeException"); // TODO
    terminateJVM(ctx);
    exit(1);
  }

  int32_t arr_size = count;

  u4 arrayref = ctx->objects.count++;
  if (arrayref == JVM_HEAP_CAPACITY)
  {
    fprintf(stderr, "Out of memory\n");
    terminateJVM(ctx);
    exit(1);
  }

  // inicializando objeto
  Object* arr_obj = &ctx->objects.entries[arrayref];
  Array* array = &arr_obj->content.arr;
  arr_obj->type = OBJ_ARRAY;
  arr_obj->clazz = NULL;
  array->type = type;
  array->length = count;
  array->dimensions = 1;

  // Número de slots 
  if (type == T_LONG || type == T_DOUBLE)
  {
    arr_size *= 2;
  }
  
  array->data = (u4*)(calloc(arr_size, sizeof(u4)));

  push_operand(frame, arrayref);
}


void handle_arraylength(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u4 arrayref = pop_operand(frame);

  if (arrayref == 0 || ctx->objects.entries[arrayref].type != OBJ_ARRAY)
  {
    // TODO throw
    fprintf(stderr, "NullPointerException");
    terminateJVM(ctx);
    exit(1);
  }

  push_operand(frame, ctx->objects.entries[arrayref].content.arr.length);
}
