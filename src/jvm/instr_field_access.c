#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "common/classfile.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"


// 178
void handle_getstatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u1* opc_pc = frame->pc - 1; 
  u2 cp_idx = fetch_u2(&frame->pc);

  // encontrando cpinfo
  cp_info* cp_entry = &constant_pool(frame)[cp_idx];
  u2 class_index = cp_entry->info.fieldref_info.class_index;

  LoadedClass* clazz = resolve_class(ctx, class_index);
  RuntimeField* field = resolve_field(ctx, cp_idx, true); // is_static = true

  if (field->index == JAVA_SYSTEM_OUT_IDX)
  {
    push_operand(frame, JVM_HANDLE_SYSOUT);
    return;
  }

  // PC Rewind
  if (!clazz->is_initialized)
  {
    initialize_class(ctx, clazz);
    frame->pc = opc_pc;
    return;
  }

  if (field->descriptor[0] == 'J' || field->descriptor[0] == 'D')
  {
    u4 val_h = field->holder_class->static_fields[field->index];
    u4 val_l = field->holder_class->static_fields[field->index+1]; 
    push_operand2(frame, ((u8)val_h << 32) | val_l);
    return;
  }

  push_operand(frame, field->holder_class->static_fields[field->index]);  
}

// 179 
void handle_putstatic(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u1* opc_pc = frame->pc - 1;
  u2 cp_idx = fetch_u2(&frame->pc); // indice para fields_count

  // encontrando cpinfo
  cp_info* cp_entry = &constant_pool(frame)[cp_idx];
  u2 class_index = cp_entry->info.fieldref_info.class_index;

  LoadedClass* clazz = resolve_class(ctx, class_index);
  RuntimeField* field = resolve_field(ctx, cp_idx, true); // is_static = true

  // PC Rewind
  if (!clazz->is_initialized)
  {
    initialize_class(ctx, clazz);
    frame->pc = opc_pc;
    return;
  }

  if (field->descriptor[0] == 'J' || field->descriptor[0] == 'D')
  {
    u8 value = pop_operand2(frame);
    field->holder_class->static_fields[field->index] = (u4)(value >> 32);
    field->holder_class->static_fields[field->index + 1] = (u4)value;
    return;
  }

  field->holder_class->static_fields[field->index] = pop_operand(frame); 
}


void handle_getfield(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeField* field = resolve_field(ctx, cp_idx, false);

  bool is_cat2 = field->descriptor[0] == 'J' || field->descriptor[0] == 'D';

  u4 obj_ref = pop_operand(frame);
  if (!obj_ref) goto null_ptr;
  Object* obj = &ctx->objects.entries[obj_ref];

  // se protegido, e resolução aconteceu, classe de obj 
  // deve ser subclasse da classe atual ou classe atual
  if ((field->access_flags & ACC_PROTECTED) && 
      !extends(obj->clazz, frame->method.holder_class))
    goto illegal_access;

  // mesma coisa se campo privado e classe não é atual 
  if ((field->access_flags & ACC_PRIVATE) && 
      frame->method.holder_class != field->holder_class)
    goto illegal_access;

  if (is_cat2)
  {
    u8 val = obj->content.fields[field->index];
    val <<= 32;
    val |= 0xFF & obj->content.fields[field->index + 1];
    push_operand2(frame, val);
    return;
  }

  push_operand(frame, obj->content.fields[field->index]);
  return;

// TODO throw
illegal_access:
  terminateJVM(ctx);
  fprintf(stderr, "IllegalAccessError");
  exit(1);
null_ptr: 
  terminateJVM(ctx);
  fprintf(stderr, "NullPointerException");
  exit(1);
}

void handle_putfield(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  RuntimeField* field = resolve_field(ctx, cp_idx, false);

  bool is_cat2 = field->descriptor[0] == 'J' || field->descriptor[0] == 'D';
  u8 val = is_cat2 ? pop_operand2(frame) : pop_operand(frame);

  u4 obj_ref = pop_operand(frame);
  if (!obj_ref) goto null_ptr;
  Object* obj = &ctx->objects.entries[obj_ref];

  // se protegido, e resolução aconteceu, classe de obj 
  // deve ser subclasse da classe atual ou classe atual
  if ((field->access_flags & ACC_PROTECTED) && 
      !extends(obj->clazz, frame->method.holder_class))
    goto illegal_access;

  // mesma coisa se campo privado e classe não é atual 
  if ((field->access_flags & ACC_PRIVATE) && 
      frame->method.holder_class != field->holder_class)
    goto illegal_access;

  if (is_cat2)
  {
    obj->content.fields[field->index] = (u4)(val >> 32); // high_bytes
    obj->content.fields[field->index + 1] = (u4)val;     // low_bytes
    return;
  }

  obj->content.fields[field->index] = (u4)val;
  return;

// TODO throw
illegal_access:
  terminateJVM(ctx);
  fprintf(stderr, "IllegalAccessError");
  exit(1);
null_ptr: 
  terminateJVM(ctx);
  fprintf(stderr, "NullPointerException");
  exit(1);
}
