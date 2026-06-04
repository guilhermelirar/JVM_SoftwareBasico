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
      strcmp(method_name, "println") == 0)
    handle_sysout(frame, ctx, descriptor[1]);
}

void handle_invokestatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  cp_info* cp = constant_pool(frame);
  u2 cp_idx = fetch_u2(&frame->pc);
  cp_info* entry = &cp[cp_idx];

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

  LoadedClass* this = frame->method.holder_class;
  LoadedClass* class = get_class(ctx, class_name);
  LoadedClass** class_with_method_pp = &class;

  method_info* m = lookup_method(class_with_method_pp, 
     method_name, descriptor);

  if (m == NULL) goto err_no_such_method;

  if ((m->access_flags & ACC_PRIVATE) && this != class)
    goto err_no_such_method; // TODO mensagem de erro

  if (m->access_flags & ACC_PROTECTED)
  {
    LoadedClass* curr = this;
    do {
      // Faz parte da hierarquia
      if (curr == class)
      {
        goto method_access_ok;

      }
      curr = curr->super;
    } while (curr != NULL);

    goto err_no_such_method; // TODO mensagem de falha de acesso
  }

method_access_ok:
  if (class->is_initialized == false)
  {
    initialize_class(ctx, class);
    frame->pc = frame->pc - 3; 
    return;
  }

  int args = count_args_size(descriptor);
  push_frame(&ctx->t, new_frame(*class_with_method_pp, m));
  Frame* new_f = current_frame(ctx);

  for (int i = args-1; i >= 0; i--) {
    new_f->locals[i] = frame->operand_stack[frame->stack_ptr--];
  }

  return;
err_no_such_method:
  fprintf(stderr, 
      "Fatal Error: NoSuchMethodError." 
      "Could not find method '%s%s' in hierarchy.\n", 
      method_name, descriptor);
  exit(1); 
}
