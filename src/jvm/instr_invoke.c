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

void handle_invokevirtual(JVM_Context *ctx, u1 opc) {
  (void)opc;
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  cp_info* cp = constant_pool(frame);
  u2 cp_idx = fetch_u2(&frame->pc);
  cp_info* entry = &constant_pool(frame)[cp_idx];

  // classe do método
  u2 class_idx = entry->info.methodref_info.class_index;
  const char* class_name = cp_class_name(cp, class_idx);

  // name_and_type
  u2 nt_idx = entry->info.methodref_info.name_and_type_index;
  cp_info* nt_entry = &cp[nt_idx];

  // descritor
  const char* method_name = 
    cp_get_utf8(cp, 
        nt_entry->info.name_and_type_info.name_index);
  
  const char* descriptor = 
    cp_get_utf8(cp, 
        nt_entry->info.name_and_type_info.descriptor_index);

  if (strcmp(class_name, "java/io/PrintStream") == 0 && 
    (strcmp(method_name, "println") == 0 
     || strcmp(method_name, "print") == 0))
  {
    handle_sysout(frame, ctx, descriptor[1], method_name);
  }
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

  int args = count_args_size(method->descriptor);
  push_frame(&ctx->t, new_frame(method));
  Frame* new_f = current_frame(ctx);

  for (int i = args-1; i >= 0; i--) {
    new_f->locals[i] = pop_operand(frame);
  }
}
