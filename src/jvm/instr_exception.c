#include "common/classfile.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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
 
  // procura tratamento de exceção até acabar a execução
  int handler_frame_ptr = ctx->t.frame_ptr;
  Frame* handler_frame = frame;
  while (handler_frame != NULL)
  {
    exception_info* catch_info = get_catch_info(handler_frame, e);

    // tratamento encontrado, mudar pc para o handler
    if (catch_info != NULL) 
    {
      while (ctx->t.frame_ptr != handler_frame_ptr) pop_frame(&ctx->t);
      handler_frame->stack_ptr = -1; 
      push_operand(handler_frame, exc_ref); 
      handler_frame->pc = handler_frame->method.code_attr->code + 
        catch_info->handler_pc; 
      return; 
    } 
    
    handler_frame = (handler_frame_ptr > 0) ? 
      ctx->t.frames[--handler_frame_ptr] : NULL;
  }
 
  // Exceção causa fim da execução
  fatal_error(ctx, "Exception in thread \"main\" %s", e->clazz->name);
}
