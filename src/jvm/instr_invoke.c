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

/*
 * Procura o método a partir da classe filha, para 
 * inicializar o atributo code com o atributo de 
 * métodos sobrescritos se houver
 * */
static void get_actual_instance_method(JVM_Context* ctx, 
    RuntimeMethod* resolved, LoadedClass* object_ref_c)
{
  LoadedClass* curr = object_ref_c; 
  method_info* new_mi = NULL;
  do {
    new_mi = find_method(curr->cf, resolved->name, resolved->descriptor);
    if (new_mi != NULL) break;
    curr = curr->super;
  } while (curr != NULL);

  // TODO throw
  if (new_mi == NULL || new_mi->access_flags & ACC_ABSTRACT)
  {
    terminateJVM(ctx);
    fprintf(stderr, "AbstractMethodError\n");
    exit(1);
  }

  // não fará alteração se um dos valores for null
  init_RuntimeMethod(curr, new_mi, resolved); 
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

  u4 this_ref = frame->operand_stack[frame->stack_ptr - method.args_size + 1];

  if (!this_ref) // TODO throw tmp
  {
    fprintf(stderr, "NullPointerException\n");
    terminateJVM(ctx);
    exit(1);
  }

  Object* this_ref_o = &ctx->objects.entries[this_ref];
  LoadedClass* this_ref_c = this_ref_o->clazz;
  if (this_ref_c != method.holder_class)
  {
    if (!extends(this_ref_c, method.holder_class) || 
        method.info->access_flags & ACC_PRIVATE)
      goto illegal_acc;

    get_actual_instance_method(ctx, &method, this_ref_c);
  }
  invoke_method(ctx, &method);
  return;

illegal_acc:
  // TODO
  fprintf(stderr, "IllegalAccessError\n");
  terminateJVM(ctx);
  exit(1);
}

void handle_invokeinterface(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);
  u1 count = *frame->pc++; // contagem de slots dos argumentos
  frame->pc++; // 0 obrigatório

  RuntimeMethod interface_method = *resolve_interface_method(ctx, cp_idx); 
  u4 this_ref = frame->operand_stack[frame->stack_ptr - count + 1];

  if (!this_ref) // TODO throw tmp
  {
    fprintf(stderr, "NullPointerException\n");
    terminateJVM(ctx);
    exit(1);
  }

  Object* this_ref_o = &ctx->objects.entries[this_ref];
  LoadedClass* this_ref_c = this_ref_o->clazz;
  if (this_ref_c != interface_method.holder_class)
    get_actual_instance_method(ctx, &interface_method, this_ref_c);

  invoke_method(ctx, &interface_method);
}

void handle_invokestatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  // Encontra o método e inicializa a classe caso não tenha sido 
  // inicializada
  RuntimeMethod method = *resolve_method(ctx, cp_idx);
  if (!method.holder_class->is_initialized)
  {
    initialize_class(ctx, method.holder_class);
    frame->pc = frame->pc - 3; // Volta o PC para re-executar
    return;
  }

  invoke_method(ctx, &method);
  return;
}

void handle_invokespecial(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeMethod* resolved_m_ptr = resolve_method(ctx, cp_idx);

  if (resolved_m_ptr == NULL)
  {
    fprintf(stderr, "MethodNotFoundError");
    goto terminate;
  }

  // java/lang/Object.<init>
  if (strcmp(resolved_m_ptr->holder_class->name, "java/lang/Object") == 0)
  {
    pop_operand(frame);
    return;
  }

  u4 objectref = frame->operand_stack[frame->stack_ptr - 
    resolved_m_ptr->args_size + 1];

  if (!objectref)
  {
    fprintf(stderr, "NullPointerException");
    goto terminate;
  }

  Object* obj = &ctx->objects.entries[objectref];

  // Cria cópia do resolved_m_ptr para caso de houver sobrescrita
  RuntimeMethod resolved_m = *resolved_m_ptr;
  get_actual_instance_method(ctx, &resolved_m, obj->clazz);
  invoke_method(ctx, &resolved_m);
  return;
terminate:
  terminateJVM(ctx);
  exit(1);
}

