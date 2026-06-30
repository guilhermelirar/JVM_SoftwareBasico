#include "common/classfile.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static u4 get_current_line(Frame* f)
{
  Code_attribute* code_attr = f->method.code_attr;
  cp_info* cp = f->method.holder_class->cf->constant_pool;
  u4 pc = f->pc - code_attr->code - 1;
  for (int i = 0; i < code_attr->attributes_count; i++)
  {
    if (strcmp("LineNumberTable", 
          cp_get_utf8(cp, code_attr->attributes[i].attribute_name_index)) == 0)
      {
        LineNumberTable_attribute* line_number_attr = 
          code_attr->attributes[i].info.line_number_table_attribute;

        // procura linha do pc atual
        u4 line = line_number_attr->line_number_table[0].line_number;
        for (int j = 0; j < line_number_attr->line_number_table_length; j++)
        {
          if (line_number_attr->line_number_table[j].start_pc > pc)
          {
            return line;
          }

          line = line_number_attr->line_number_table[j].line_number;
        }

        return line;
      }
  }

  return 0;
}

static void fatal_error(JVM_Context* ctx, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);

  Frame* f = current_frame(ctx);
  while (f != NULL)
  {
    const char* class_name = f->method.holder_class->name;
    const char* method_name = f->method.name;
    u4 line = get_current_line(f);
    fprintf(stderr, "\tat %s.%s", class_name, method_name);
    if (line)
      fprintf(stderr, "(%s.java:%u)\n", class_name, line);
    else 
      fprintf(stderr, "(%s.java)\n", class_name);

    pop_frame(&ctx->t);
    f = (ctx->t.frame_ptr >= 0) ? current_frame(ctx) : NULL;
  }

  terminateJVM(ctx);
  exit(1);
}

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
  fatal_error(ctx, "Exception in thread \"main\" %s\n", e->clazz->name);
}
