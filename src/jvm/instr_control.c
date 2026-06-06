#include "common/bytecode.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void handle_return(JVM_Context *ctx, u1 opc)
{
  switch (opc) 
  {
    case opc_return:
      return pop_frame(&ctx->t);

    // empilha um u4 no frame do método chamador
    case opc_ireturn:
    case opc_freturn:
    case opc_areturn:
    {
      u4 ret_val = pop_operand(current_frame(ctx));
      pop_frame(&ctx->t);
      push_operand(current_frame(ctx), ret_val);
      
      return;
    }

    // empilham u8 
    case opc_lreturn:
    case opc_dreturn:
    {
      u8 ret_val = pop_operand2(current_frame(ctx));
      pop_frame(&ctx->t);
      push_operand2(current_frame(ctx), ret_val);
    }

    default:
    return;
  }
}


void handle_ifcond(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  u1* ifcond_pc = frame->pc - 1; // inicio do if
  u2 offset = fetch_u2(&frame->pc);

  int32_t value = (int32_t)pop_operand(frame);
  int cond = 0;

  switch (opc)
  {
    case (opc_ifeq):
      cond = value == 0;
      break;

    case (opc_ifne):
      cond = value != 0;
      break;

    case (opc_iflt):
      cond = value < 0;
      break;

    case (opc_ifle):
      cond = value <= 0;
      break;

    case (opc_ifgt):
      cond = value > 0;
      break;

    case (opc_ifge):
      cond = value >= 0;
      break;

    default:
      break;
  }

  if (cond)
  {
    frame->pc = ifcond_pc + offset;
  }
}
