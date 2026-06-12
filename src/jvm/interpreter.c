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
      double d = opc == opc_dconst_0 ? 0.0 : 1.0;
      u8 value = double_to_u8(d);
      push_operand2(frame, value);
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


void handle_stack(JVM_Context* ctx, u1 opc)
{
  Frame *frame = current_frame(ctx);

  switch (opc)
  {
    case opc_pop:
      pop_operand(frame);
      return;

    case opc_pop2:
      pop_operand2(frame);
      return;

    case opc_dup:
    {
      u4 value = pop_operand(frame);
      push_operand(frame, value);
      return push_operand(frame, value);
    }

    case opc_dup_x1:
    {
      u4 v1, v2;
      v1 = pop_operand(frame);
      v2 = pop_operand(frame);
      push_operand(frame, v1);
      push_operand(frame, v2);
      return push_operand(frame, v1);
    }
      
    case opc_dup_x2:
    {
      u4 v1 = pop_operand(frame);
      u8 v2 = pop_operand2(frame);
      push_operand(frame, v1);
      push_operand2(frame, v2);
      return push_operand(frame, v1);
    }

    case opc_dup2:
    {
      u8 value = pop_operand2(frame);
      push_operand2(frame, value);
      return push_operand2(frame, value);
    }

    case opc_dup2_x1:
    {
      u8 v1 = pop_operand2(frame);
      u4 v2 = pop_operand(frame);
      push_operand2(frame, v1);
      push_operand(frame, v2);
      return push_operand2(frame, v2);
    }

    case opc_dup2_x2:
    {
      u8 v1, v2;
      v1 = pop_operand2(frame);
      v2 = pop_operand2(frame);
      push_operand2(frame, v1);
      push_operand2(frame, v1);
      return push_operand2(frame, v2);
    }

    case opc_swap:
    {
      u4 v1, v2;
      v1 = pop_operand(frame);
      v2 = pop_operand(frame);
      push_operand(frame, v1);
      return push_operand(frame, v2);
    }

    default:
    return;
  }
}
