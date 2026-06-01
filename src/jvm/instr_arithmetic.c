#include "common/bytecode.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include <math.h>
#include <stdint.h>


static void handle_add(Frame *f, u1 opc)
{
  switch (opc)
  {
    case opc_iadd:
    {
      push_operand(f, 
          (u4)((int32_t)pop_operand(f) + (int32_t)pop_operand(f)));
      return;
    } 

    case opc_fadd:
    {
      float v2 = u4_to_float(pop_operand(f));
      float v1 = u4_to_float(pop_operand(f));
      push_operand(f, float_to_u4(v1 + v2));
      return;
    }

    case opc_ladd:
    {
      int64_t v2 = (int64_t)pop_operand2(f);
      int64_t v1 = (int64_t)pop_operand2(f);
      push_operand2(f, (u8)(v1 + v2));
      return;
    }

    case opc_dadd:
    {
      double d2 = u8_to_double(pop_operand2(f));
      double d1 = u8_to_double(pop_operand2(f));
      push_operand2(f, double_to_u8(d1 + d2));
    }

    default:
    return;
  }
}

static void handle_mul(Frame *f, u1 opc)
{
  switch (opc)
  {
    case opc_imul:
    {
      int32_t v2 = (int32_t)pop_operand(f); 
      int32_t v1 = (int32_t)pop_operand(f);
      push_operand(f, (u4)(v1 * v2));
      return;
    } 

    case opc_fmul:
    {
      float v2 = u4_to_float(pop_operand(f));
      float v1 = u4_to_float(pop_operand(f));
      push_operand(f, float_to_u4(v1 * v2));
      return;
    }

    case opc_lmul:
    {
      int64_t v2 = (int64_t)pop_operand2(f);
      int64_t v1 = (int64_t)pop_operand2(f);
      push_operand2(f, (u8)(v1 * v2));
      return;
    }

    case opc_dmul:
    {
      double d2 = u8_to_double(pop_operand2(f));
      double d1 = u8_to_double(pop_operand2(f));
      push_operand2(f, double_to_u8(d1 * d2));
    }

    default:
    return;
  }
}


static void handle_sub(Frame *f, u1 opc)
{
  switch (opc)
  {
    case opc_isub:
    {
      int32_t v2 = (int32_t)pop_operand(f); 
      int32_t v1 = (int32_t)pop_operand(f);
      push_operand(f, (u8)(v1 - v2));
      return;
    } 

    case opc_fsub:
    {
      float v2 = u4_to_float(pop_operand(f));
      float v1 = u4_to_float(pop_operand(f));
      push_operand(f, float_to_u4(v1 - v2));
      return;
    }

    case opc_lsub:
    {
      int64_t v2 = (int64_t)pop_operand2(f);
      int64_t v1 = (int64_t)pop_operand2(f);
      push_operand2(f, (u8)(v1 - v2));
      return;
    }

    case opc_dsub:
    {
      double d2 = u8_to_double(pop_operand2(f));
      double d1 = u8_to_double(pop_operand2(f));
      push_operand2(f, double_to_u8(d1 - d2));
    }

    default:
    return;
  }
}

static void handle_div(Frame *f, u1 opc)
{
  switch (opc)
  {
    case opc_idiv:
    {
      int32_t v2 = (int32_t)pop_operand(f); 
      int32_t v1 = (int32_t)pop_operand(f);
      push_operand(f, (u4)(v1 / v2));
      return;
    } 

    case opc_fdiv:
    {
      float v2 = u4_to_float(pop_operand(f));
      float v1 = u4_to_float(pop_operand(f));
      push_operand(f, float_to_u4(v1 / v2));
      return;
    }

    case opc_ldiv:
    {
      int64_t v2 = (int64_t)pop_operand2(f);
      int64_t v1 = (int64_t)pop_operand2(f);
      push_operand2(f, (u8)(v1 / v2));
      return;
    }

    case opc_ddiv:
    {
      double d2 = u8_to_double(pop_operand2(f));
      double d1 = u8_to_double(pop_operand2(f)); 
      push_operand2(f, double_to_u8(d1 / d2));
    }

    default:
    return;
  }
}

static void handle_rem(Frame *f, u1 opc)
{
  switch (opc)
  {
    case opc_irem:
    {
      u4 v2 = pop_operand(f); 
      u4 v1 = pop_operand(f);
      push_operand(f, (u4)((int32_t)v1 % (int32_t)v2));
      return;
    } 

    case opc_frem:
    {
      float v2 = u4_to_float(pop_operand(f));
      float v1 = u4_to_float(pop_operand(f));
      push_operand(f, float_to_u4(fmodf(v1, v2)));
      return;
    }

    case opc_lrem:
    {
      u8 v2 = pop_operand2(f);
      u8 v1 = pop_operand2(f);
      push_operand2(f, (u8)((int64_t)v1 % (int64_t)v2));
      return;
    }

    case opc_ddiv:
    {
      double d2 = u8_to_double(pop_operand2(f));
      double d1 = u8_to_double(pop_operand2(f));
      push_operand2(f, double_to_u8(fmod(d1, d2)));
    }

    default:
    return;
  }
}

void handle_arithmetic(JVM_Context *ctx, u1 opc)
{
  if (IN_RANGE(opc, opc_iadd, opc_dadd))
    return handle_add(current_frame(ctx), opc);

  if (IN_RANGE(opc, opc_isub, opc_dsub))
    return handle_sub(current_frame(ctx), opc);

  if (IN_RANGE(opc, opc_imul, opc_dmul))
    return handle_mul(current_frame(ctx), opc);

  if (IN_RANGE(opc, opc_idiv, opc_ddiv))
    return handle_div(current_frame(ctx), opc);

  if (IN_RANGE(opc, opc_irem, opc_drem))
    return handle_rem(current_frame(ctx), opc);

  if (opc == opc_iinc)
  {
    u1 idx = fetch_u1(current_frame(ctx)->code, &current_frame(ctx)->pc);
    int8_t constant = (int8_t)fetch_u1(current_frame(ctx)->code, 
        &current_frame(ctx)->pc);

    int local = (int)current_frame(ctx)->locals[idx];
    local += constant;
    current_frame(ctx)->locals[idx] = (u4)local;
  }
}
