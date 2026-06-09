#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "common/classfile.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"


static int find_field_by_name(ClassFile* cf,
    const char* name, const char* descriptor, int* offset)
{  
  int offset_v = 0;

  field_info* f;
  const char* f_name;
  const char* f_desc;
  for (int i = 0; i < cf->fields_count; i++)
  {
    f = &cf->fields[i];
    f_name = cp_get_utf8(cf->constant_pool, f->name_index);
    f_desc = cp_get_utf8(cf->constant_pool, f->descriptor_index);
      
    if (strcmp(name, f_name) == 0 && strcmp(f_desc, descriptor) == 0)
    {
      *offset = offset_v;
      return i;
    }
      
    if (f_desc[0] == 'J' || f_desc[0] == 'D')
      offset_v++;

    offset_v++;
  }
  
  return -1;
}

static int resolve_field(JVM_Context* ctx, 
    u2 cp_index, LoadedClass** base_class, LoadedClass** owner_class, 
    const char** descriptor, const char** name,
    u2 *access_flags)
{
  cp_info* cp = constant_pool(current_frame(ctx));
  CONSTANT_Fieldref_info* entry_info = &cp[cp_index].info.fieldref_info;

  // Assumindo CONSTANT_Fieldref
  const char* class_name = cp_class_name(cp, entry_info->class_index);

  CONSTANT_NameAndType_info* nt_info = &cp[entry_info->
    name_and_type_index].info.name_and_type_info;

  *name = cp_get_utf8(cp, nt_info->name_index);
  *descriptor = cp_get_utf8(cp, nt_info->descriptor_index);
  
  *base_class = get_class(ctx, class_name);
  *owner_class = NULL;
  LoadedClass* curr = *base_class;
  // TODO caso de erro

  int idx = 0, offset = 0;
  do {
    idx = find_field_by_name(curr->cf, *name, *descriptor, &offset);

    if (idx >= 0)
    {
      *owner_class = curr;
      *access_flags = curr->cf->fields[idx].access_flags;
      return offset; // indice em LoadedClass->fields;
    }

    curr = curr->super;
  } while (curr != NULL);

  return -1; // FIELD NÃO ENCONTRADO
}

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
  u2 nt_index = cp_entry->info.fieldref_info.name_and_type_index;
  const char* class = cp_class_name(constant_pool(frame), class_index);
  const char* nt = cp_nameandtype_name(constant_pool(frame), nt_index);

  // Ignorar classes java (ex: System)
  if (strcmp("java/lang/System", class) == 0 && strcmp("out", nt) == 0)
  {
    frame->operand_stack[++frame->stack_ptr] = JVM_HANDLE_SYSOUT;
    return;
  }

  LoadedClass* this = frame->method.holder_class;
  LoadedClass* base_class = NULL;
  LoadedClass* owner_class = NULL;
  const char* name = NULL;
  const char* descriptor = NULL;
  u2 access_flags = 0;

  int idx = resolve_field(ctx, 
      cp_idx, &base_class, &owner_class, &descriptor, &name, &access_flags);

  if (this != owner_class)
  {
    if ((access_flags & ACC_PRIVATE) || 
        ((access_flags & ACC_PROTECTED) && !extends(this, base_class)))
    {
      fprintf(stderr, "IllegalAccessException"); // TODO throw (?)
      terminateJVM(ctx);
      exit(1);
    }
  }

  // PC Rewind
  if (!base_class->is_initialized)
  {
    initialize_class(ctx, base_class);
    frame->pc = opc_pc;
    return;
  }

  if (descriptor[0] == 'J' || descriptor[0] == 'D')
  {
    u8 val = ((u8)owner_class->static_fields[idx] << 32) |
      owner_class->static_fields[idx + 1];

    push_operand2(frame, val);
    return;
  }

  push_operand(frame, owner_class->static_fields[idx]);  
}

// 179 
void handle_putstatic(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u1* opc_pc = frame->pc - 1;
  u2 cp_index = fetch_u2(&frame->pc); // indice para fields_count

  LoadedClass* this = frame->method.holder_class;
  LoadedClass* base_class = NULL;
  LoadedClass* owner_class = NULL;
  const char* name = NULL;
  const char* descriptor = NULL;
  u2 access_flags = 0;

  int idx = resolve_field(ctx, 
      cp_index, &base_class, &owner_class, 
      &descriptor, &name, &access_flags);

  if (this != owner_class)
  {
    if ((access_flags & ACC_PRIVATE) || 
        ((access_flags & ACC_PROTECTED) && !extends(this, base_class)))
    {
      fprintf(stderr, "IllegalAccessException"); // TODO throw (?)
      terminateJVM(ctx);
      exit(1);
    }
  }

  // PC Rewind
  if (!base_class->is_initialized)
  {
    initialize_class(ctx, base_class);
    frame->pc = opc_pc;
    return;
  }

  if (descriptor[0] == 'J' || descriptor[0] == 'D')
  {
    u8 value = pop_operand2(frame);
    owner_class->static_fields[idx] = (u4)(value >> 32);
    owner_class->static_fields[idx+1] = (u4)value;
    return;
  }

  owner_class->static_fields[idx] = pop_operand(frame); 
}
