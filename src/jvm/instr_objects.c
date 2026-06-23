#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
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

