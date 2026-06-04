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
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);
  cp_info* entry = &frame->constant_pool[cp_idx];

  // classe do método
  u2 class_idx = entry->info.methodref_info.class_index;
  const char* class_name = cp_class_name(frame->constant_pool, class_idx);

  // name_and_type
  u2 nt_idx = entry->info.methodref_info.name_and_type_index;
  cp_info* nt_entry = &frame->constant_pool[nt_idx];

  // descritor
  const char* method_name = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.name_index);
  
  const char* descriptor = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.descriptor_index);

  if (strcmp(class_name, "java/io/PrintStream") == 0 && 
      strcmp(method_name, "println") == 0)
    handle_sysout(frame, ctx, descriptor[1]);
}

void handle_invokestatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);
  cp_info* entry = &frame->constant_pool[cp_idx];

  u2 class_idx = entry->info.methodref_info.class_index;
  const char* class_name = cp_class_name(frame->constant_pool, class_idx);

  // name_and_type
  u2 nt_idx = entry->info.methodref_info.name_and_type_index;
  cp_info* nt_entry = &frame->constant_pool[nt_idx];

  // descritor
  const char* method_name = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.name_index);
  const char* descriptor = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.descriptor_index);


  // procura classe 
  LoadedClass* class = find_class_by_name(ctx, class_name);
  if (class == NULL) // Carregar a classe
  {
    class = load_class(ctx, class_name); // Possivel encerramento da execução
  }

  // procura método na classe base, e nas superclasses
  method_info* m = find_method(class->cf, method_name, descriptor);
  if (m == NULL)
  {
    class = find_superclass_with_method(ctx, class, method_name, descriptor);
    if (class == NULL) goto err_no_such_method;
  }

  // garantido que existe
  m = find_method(class->cf, method_name, descriptor);

  if (m->access_flags & ACC_PRIVATE)
    goto err_no_such_method;

  int args = count_args_size(descriptor);
  push_frame(&ctx->t, new_frame(class->cf, m));
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
