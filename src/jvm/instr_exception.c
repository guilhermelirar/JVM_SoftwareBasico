#include "common/bytecode.h"
#include "common/classfile.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static exception_info* get_catch_info(Frame* frame, Object* exception)
{
  const char* exc_name = exception->clazz->name; 

  ClassFile* cf = frame->method.holder_class->cf;
  u2 exception_table_len = frame->method.code_attr->exception_table_length;
  exception_info* exc_info = frame->method.code_attr->exception_table;
 
  // Procura por um catch_type com o nome da classe da esceção
  for (int i = 0; i < exception_table_len; i++, exc_info++)
  {
    u2 catch_type = exc_info->catch_type;
    if (!catch_type) continue;

    const char* catch_type_name = cp_class_name(cf->constant_pool, 
      exc_info->catch_type);

    if (strcmp(catch_type_name, exc_name) == 0)
      return exc_info;
  }

  return NULL;
}

void handle_athrow(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u4 exc_ref = pop_operand(frame);
  Object* e = get_object(ctx, exc_ref);
  
  while (frame != NULL)
  {
    exception_info* catch_info = get_catch_info(frame, e);

    if (catch_info != NULL) 
    {
      push_operand(frame, exc_ref); 
      frame->pc = frame->method.code_attr->code + catch_info->handler_pc; 
      return; 
    } 
    
    pop_frame(&ctx->t);
    frame = (ctx->t.frame_ptr > 0) ? current_frame(ctx) : NULL;
  }

  terminateJVM(ctx);
  exit(1);
}
