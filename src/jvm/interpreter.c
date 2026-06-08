#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include "common/classfile.h"
#include "common/bytecode.h" // IWYU pragma: keep

// 0
void handle_nop(JVM_Context* ctx, u1 opc) 
{
  // Opcode não implementado, execução deve ser
  // encerrada
  if (opc != opc_nop) {
    jvm_error_uninmplemented_opc(ctx, opc);
  }
}

void handle_tconst(JVM_Context *ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  float f;
  switch (opc)
  {
    // 1
    case opc_aconst_null:
      push_operand(frame, 0);
      break;

    // 2-8
    case opc_iconst_m1:
    case opc_iconst_0:
    case opc_iconst_1:
    case opc_iconst_2:
    case opc_iconst_3:
    case opc_iconst_4:
    case opc_iconst_5:
      push_operand(frame, (int32_t)(-1 + opc-opc_iconst_m1));
      break;

    // 9, 10
    case opc_lconst_0:
    case opc_lconst_1:
      push_operand(frame, (int32_t)0);
      push_operand(frame, (int32_t)(opc - opc_lconst_0));
      break;

    // 11, 12, 13
    case opc_fconst_0:
    case opc_fconst_1:
    case opc_fconst_2:
    {
      if (opc == opc_fconst_0) f = 0.0f;
      else if (opc == opc_fconst_1) f = 1.0f;
      else f = 2.0f;

      u4 value;
      memcpy(&value, &f, sizeof(float));
      push_operand(frame, value);
      break;
    }

    // 14, 15
    case opc_dconst_0:
    case opc_dconst_1:
    {
      double d = opc == opc_dconst_0 ? 0.0 : 0.1;
      u8 value;
      memcpy(&value, &d, sizeof(double));
      push_operand(frame, (u4)(value >> 8));
      push_operand(frame, (u4)(value & 0xFFFF));
      break;
    }

    default:
    break;
  }
}

// 16, 17
void handle_push(JVM_Context *ctx, u1 opc) 
{
  Frame* frame = current_frame(ctx);

  if (opc == opc_bipush)
    push_operand(frame, (int32_t)((int8_t)*frame->pc++));
  else if (opc == opc_sipush)
  {
    push_operand(frame, (int32_t)((int16_t)fetch_u2(&frame->pc)));
  }
}

// 18, 19, 20 
void handle_ldc(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);

  u2 cp_idx;
  if (opc == opc_ldc) {
    cp_idx = *frame->pc++; // lê 1 byte e avança
  } else { // ldc_w ou ldc2_w
    cp_idx = fetch_u2(&frame->pc); 
  }

  cp_info* entry = &constant_pool(frame)[cp_idx];

  switch (entry->tag) 
  {
    case CONSTANT_Integer:
      push_operand(frame, entry->info.int_info.bytes);
      return;

    case CONSTANT_Float:
      push_operand(frame, entry->info.float_info.bytes);
      return;

    case CONSTANT_Long:
      push_operand(frame, entry->info.long_info.h_bytes);
      push_operand(frame, entry->info.long_info.l_bytes);
      return;

    case CONSTANT_Double:
      push_operand(frame, entry->info.double_info.h_bytes);
      push_operand(frame, entry->info.double_info.l_bytes);
      return;

    case CONSTANT_String: 
    {
      const char* str = cp_get_utf8(constant_pool(frame), 
          entry->info.string_info.string_index);
      
      // ver se está na tabela de strings do ctx ou inserir
      for (u4 i = 0; i < ctx->strings.count; i++)
      {
        if (str == ctx->strings.strings[i])
        {
          push_operand(frame, i);
          return;
        }
      }
      
      ctx->strings.strings[ctx->strings.count] = (char*)str;
      push_operand(frame, ctx->strings.count);
      ctx->strings.count++;
      break;
    }
    default:
      break;
  }
}

// 178
void handle_getstatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u2 cp_idx = fetch_u2(&frame->pc);

  // encontrando cpinfo
  cp_info* cp_entry = &constant_pool(frame)[cp_idx];
  
  if (cp_entry->tag != CONSTANT_Fieldref) return;

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

  // TODO busca na área de métodos
  // ... busca na área de métodos
}

// 179 
void handle_putstatic(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);

  u2 index = fetch_u2(&frame->pc); // indice para field
  cp_info* entry = &constant_pool(frame)[index];

  if (entry->tag != CONSTANT_Fieldref) return; // TODO erro?

  u2 field_class = entry->info.fieldref_info.class_index; 
  u2 nt_index = entry->info.fieldref_info.name_and_type_index;
  cp_info* nt_entry = &constant_pool(frame)[nt_index];
  u2 descriptor_index = nt_entry->info.name_and_type_info.descriptor_index;
  char* dsc = cp_get_utf8(constant_pool(frame), descriptor_index);

  bool is_cat2 = (dsc[0] == 'J' || dsc[0] == 'D');

  if (is_cat2)
  {
    u8 valor = pop_operand2(frame);
  }
  else 
  {
    u4 valor = pop_operand(frame);
  }


}
