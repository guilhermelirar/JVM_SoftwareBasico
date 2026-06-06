#include "common/bytecode.h"
#include "common/classfile.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
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
  int32_t offset = (int32_t)fetch_u2(&frame->pc);

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



void handle_goto_jsr_ret(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  u1* opcode_pc = frame->pc - 1; // inicio do opcode 

  switch (opc)
  {
    case (opc_goto):
    {
      int16_t offset = (int16_t)fetch_u2(&frame->pc);
      frame->pc = opcode_pc + offset; 
      return;
    }

    case (opc_goto_w):
    {
      int32_t offset = (int32_t)fetch_u4(&frame->pc);
      frame->pc = opcode_pc + offset; 
      return;
    }

    // salva pc absoluto na pilha e não endereço (ret terá que somar 
    // ao inicio do códiog para recuperar returnAddress)
    case (opc_jsr):
    {
      int16_t offset = (int16_t)fetch_u2(&frame->pc);
      push_operand(frame, (u4)(frame->pc - frame->method.code));
      frame->pc = opcode_pc + offset;
      return;
    }

    case (opc_jsr_w):
    {
      int32_t offset = (int32_t)fetch_u4(&frame->pc);
      // salva pc do retorno
      push_operand(frame, (u4)(frame->pc - frame->method.code));
      frame->pc = opcode_pc + offset;
      return;
    }

    // ret 
    case (opc_ret):
    {
      u1 idx = *frame->pc++; // indice para locals

      // returnAddress recuperado
      u1* returnAddress = frame->method.code + frame->locals[idx];
      frame->pc = returnAddress;
      return;
    }
  }
}
