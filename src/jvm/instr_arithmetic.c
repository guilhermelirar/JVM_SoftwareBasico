#include "common/bytecode.h"
#include "common/classfile.h"
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

    case opc_drem:
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

  Frame* f = current_frame(ctx);

  switch (opc)
  {
    case opc_iinc:
    {
      bool widened = (*(f->pc - 2)) == opc_wide;
      
      u2 idx = widened ? fetch_u2(&f->pc) : *f->pc++;
      int16_t constant = widened ?
        (int16_t)fetch_u2(&f->pc) : (int8_t)*f->pc++;
        
      int local = (int)f->locals[idx];
      local += constant;
      f->locals[idx] = (u4)local;
      return;
    }

    case opc_ineg:
      push_operand(f, -(int32_t)pop_operand(f));
      return;

    case opc_lneg:
      push_operand2(f, -(int64_t)pop_operand2(f));
      return;

    case opc_fneg:
      push_operand(f, float_to_u4(-u4_to_float(pop_operand(f))));
      return;

    case opc_dneg:      
      push_operand2(f, double_to_u8(-u8_to_double(pop_operand2(f))));

    default:
      return;
  }
}

void handle_conversion(JVM_Context *ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  
  u4 cat1_v;
  u8 cat2_v;


  switch (opc)
  {
    case (opc_i2l): 
    {
      int32_t operand = (int32_t)pop_operand(frame);
      cat2_v = (u8)((int64_t)operand);
      push_operand2(frame, cat2_v);
      return;
    }

    case (opc_i2f): 
    {
      int32_t operand = (int32_t)pop_operand(frame);
      cat1_v = float_to_u4((float)operand);
      push_operand(frame, cat1_v);
      return;
    }

    case (opc_i2d): 
    {
      int32_t operand = (int32_t)pop_operand(frame);
      cat2_v = double_to_u8((double)operand); 
      push_operand2(frame, cat2_v);
      return;
    }

    case (opc_i2b):
    {
      cat1_v = (int32_t)((u1)pop_operand2(frame));
      push_operand(frame, cat1_v);
      return;
    }

    case (opc_i2c):
    {
      cat1_v = (u2)pop_operand(frame);
      push_operand(frame, cat1_v);
      return;
    }

    case (opc_i2s):
    {
      cat1_v = (int32_t)((u2)pop_operand(frame));
      push_operand(frame, cat1_v);
      return;
    }

    case (opc_l2i):
    {
      cat1_v = (int32_t)((int64_t)pop_operand2(frame));
      push_operand(frame, cat1_v);
      return;
    }

    case (opc_l2d):
    {
      double d = (double)((int64_t)pop_operand2(frame));
      push_operand2(frame, double_to_u8(d));
      return;
    }

    case (opc_l2f):
    {
      float f = (float)((int64_t)pop_operand2(frame));
      push_operand(frame, float_to_u4(f));
      return;
    } 

    case (opc_f2i):
    {
      float f = u4_to_float(pop_operand(frame));
      push_operand(frame, (int32_t)f);
      return;
    }

    case (opc_f2d):
    {
      float f = u4_to_float(pop_operand(frame));
      push_operand2(frame, double_to_u8((double)f));
      return;
    }

    case (opc_f2l):
    {
      float f = u4_to_float(pop_operand(frame));
      push_operand2(frame, (int64_t)f);
      return;
    }

    case (opc_d2i):
    {
      double d = u8_to_double(pop_operand2(frame));
      push_operand(frame, (int32_t)d);
      return;
    }

    case (opc_d2l):
    {
      double d = u8_to_double(pop_operand2(frame));
      push_operand2(frame, (int64_t)d);
      return;
    }

    case (opc_d2f):
    {
      double d = u8_to_double(pop_operand2(frame));
      push_operand(frame, float_to_u4((float)d));
      return;
    }
  }
  
}

void handle_logic(JVM_Context *ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);

  int32_t iv1, iv2;
  int64_t lv1, lv2;


  switch (opc) 
  {
    case (opc_ishl):
    case (opc_ishr):
    {
      iv2 = (int32_t)pop_operand(frame) & 0x1F;
      iv1 = (int32_t)pop_operand(frame);
      iv1 = (opc == opc_ishl) ? 
        iv1 << iv2 : iv1 >> iv2;
      push_operand(frame, (int32_t)iv1);
      return;
    }

    case (opc_lshl):
    case (opc_lshr):
    {
      iv2 = (int32_t)pop_operand(frame) & 0x1F;
      lv1 = (int64_t)pop_operand2(frame);
      lv1 = (opc == opc_lshl) ? lv1 << iv2 : lv1 >> iv2;
      push_operand2(frame, (int64_t)lv1);
      return;
    }

    case (opc_iushr):
    {
      iv2 = (int32_t)pop_operand(frame) & 0x1F;
      iv1 = (int32_t)pop_operand(frame);
      push_operand(frame, ((u4)iv1) >> iv2);
      return;
    }

    case (opc_lushr):
    {
      iv2 = (int32_t)pop_operand(frame) & 0x1F;
      lv1 = (int64_t)pop_operand2(frame);
      push_operand2(frame, ((u8)lv1) >> iv2);
      return;
    }

    case (opc_iand):
    {
      iv2 = (int32_t)pop_operand(frame);
      iv1 = (int32_t)pop_operand(frame);
      push_operand(frame, iv2 & iv1);
      return;
    }

    case (opc_land):
    {
      lv2 = (int64_t)pop_operand2(frame);
      lv1 = (int64_t)pop_operand2(frame);
      push_operand2(frame, lv2 & lv1);
      return;
    }

    case (opc_ior):
    {
      iv2 = pop_operand(frame);
      iv1 = pop_operand(frame);
      push_operand(frame, iv2 | iv1);
      return;
    }

    case (opc_lor):
    {
      lv2 = (int64_t)pop_operand2(frame);
      lv1 = (int64_t)pop_operand2(frame);
      push_operand2(frame, lv2 | lv1);
      return;
    }

    case (opc_ixor):
    {
      iv2 = (int32_t)pop_operand(frame);
      iv1 = (int32_t)pop_operand(frame);
      push_operand(frame, iv2 ^ iv1);
      return;
    }

    case (opc_lxor):
    {
      lv2 = (int64_t)pop_operand2(frame);
      lv1 = (int64_t)pop_operand2(frame);
      push_operand2(frame, lv2 ^ lv1);
      return;
    }
  }
}
