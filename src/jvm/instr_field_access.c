#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "common/classfile.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"


static bool extends(LoadedClass* class_a, LoadedClass* class_b)
{
  do {
    if (class_a == class_b) return true;
    class_a = class_a->super;
  } while (class_a != NULL);
  
  return false;
}

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
  RuntimeField* field = resolve_field(ctx, cp_idx);

  if (field->index == JAVA_SYSTEM_OUT_IDX)
  {
    push_operand(frame, JVM_HANDLE_SYSOUT);
    return;
  }

  if (frame->method.holder_class != clazz)
  {
    if ((field->access_flags & ACC_PRIVATE) || 
        ((field->access_flags & ACC_PROTECTED) && 
         !extends(frame->method.holder_class, clazz)))
    {
      fprintf(stderr, "IllegalAccessException"); // TODO throw (?)
      terminateJVM(ctx);
      exit(1);
    }
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
  RuntimeField* field = resolve_field(ctx, cp_idx);

  if (frame->method.holder_class != clazz)
  {
    if ((field->access_flags & ACC_PRIVATE) || 
        ((field->access_flags & ACC_PROTECTED) && 
         !extends(frame->method.holder_class, clazz)))
    {
      fprintf(stderr, "IllegalAccessException"); // TODO throw (?)
      terminateJVM(ctx);
      exit(1);
    }
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
    u8 value = pop_operand2(frame);
    field->holder_class->static_fields[field->index] = (u4)(value >> 32);
    field->holder_class->static_fields[field->index + 1] = (u4)value;
    return;
  }

  field->holder_class->static_fields[field->index] = pop_operand(frame); 
}
