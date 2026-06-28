#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include "common/classfile.h"
#include "common/bytecode.h" // IWYU pragma: keep

void invoke_method(JVM_Context *ctx, RuntimeMethod *target_method)
{
  Frame* frame = current_frame(ctx);
  Frame* next_frame = new_frame(target_method);
    
  int total_params = target_method->args_size; 
    
  for (int i = total_params - 1; i >= 0; i--) {
    next_frame->locals[i] = pop_operand(frame);
  }
    
  push_frame(&ctx->t, next_frame);
}

void handle_invokevirtual(JVM_Context *ctx, u1 opc) {
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeMethod method = *resolve_method(ctx, cp_idx);

  // Tratando caso de print e println
  if (strcmp(method.holder_class->name, "java/io/PrintStream") == 0 && 
    (strcmp(method.name, "println") == 0 
     || strcmp(method.name, "print") == 0))
  {
    handle_sysout(frame, ctx, method.descriptor[1], method.name);
    return;
  }

  invoke_method(ctx, &method);
}

void handle_invokestatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeMethod* method = resolve_method(ctx, cp_idx);
  if (!method->holder_class->is_initialized)
  {
    initialize_class(ctx, method->holder_class);
    frame->pc = frame->pc - 3; // Volta o PC para re-executar
    return;
  }

  invoke_method(ctx, method);
}

void handle_invokespecial(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeMethod* resolved_m = resolve_method(ctx, cp_idx);

  // java/lang/Object.<init>
  if (strcmp(resolved_m->holder_class->name, "java/lang/Object") == 0)
  {
    pop_operand(frame);
    return;
  }

  bool ACC_SUPER_set = frame->method.holder_class->
    cf->access_flags & ACC_SUPER;
  
  bool current_extends_holder = extends(frame->method.holder_class, 
      resolved_m->holder_class);

  bool not_init = strcmp("<init>", resolved_m->name);
  
  if (!(ACC_SUPER_set && current_extends_holder && not_init))
    return invoke_method(ctx, resolved_m);
  
  // TODO caso especial
}

