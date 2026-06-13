#include "common/bytecode.h"
#include "common/classfile.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>

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
  int32_t offset = (int32_t)(int16_t)fetch_u2(&frame->pc);

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
      push_operand(frame, (u4)(frame->pc - frame->method.code_attr->code));
      frame->pc = opcode_pc + offset;
      return;
    }

    case (opc_jsr_w):
    {
      int32_t offset = (int32_t)fetch_u4(&frame->pc);
      // salva pc do retorno
      push_operand(frame, (u4)(frame->pc - frame->method.code_attr->code));
      frame->pc = opcode_pc + offset;
      return;
    }

    // ret 
    case (opc_ret):
    {
      bool widened = (*(frame->pc - 2)) == opc_wide;

      // indice para locals
      u2 idx = widened ? fetch_u2(&frame->pc) : *frame->pc++; 

      // returnAddress recuperado
      u1* returnAddress = frame->method.code_attr->code + frame->locals[idx];
      frame->pc = returnAddress;
      return;
    }
  }
}

void handle_ifcmp(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  u1* ifcond_pc = frame->pc - 1; // inicio do if
  int32_t offset = (int32_t)fetch_u2(&frame->pc);

  int32_t v2 = (int32_t)pop_operand(frame);
  int32_t v1 = (int32_t)pop_operand(frame);
  int cond = 0;

  switch (opc)
  {
    case (opc_if_icmpeq):
    case (opc_if_acmpeq):
      cond = v1 == v2;
      break;

    case (opc_if_acmpne):
    case (opc_if_icmpne):
      cond = v1 != v2;
      break;

    case (opc_if_icmplt):
      cond = v1 < v2;
      break;

    case (opc_if_icmple):
      cond = v1 <= v2;
      break;

    case (opc_if_icmpgt):
      cond = v1 > v2;
      break;

    case (opc_if_icmpge):
      cond = v1 >= v2;
      break;

    default:
      break;
  }

  if (cond)
  {
    frame->pc = ifcond_pc + offset;
  }
}


void handle_fdcmp(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);

  double v1, v2;

  if (opc == opc_fcmpl || opc == opc_fcmpg)
  {
    v2 = u4_to_float(pop_operand(frame));
    v1 = u4_to_float(pop_operand(frame));
  }
  else // double
  {
    v2 = u8_to_double(pop_operand2(frame));
    v1 = u8_to_double(pop_operand2(frame));
  }

  if (v1 > v2)
    push_operand(frame, 1);

  else if (v1 == v2)
    push_operand(frame, 0);

  else if (v1 < v2)
    push_operand(frame, (u4)-1);
}

void handle_lcmp(JVM_Context* ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  int64_t v2 = (int64_t)pop_operand2(frame);
  int64_t v1 = (int64_t)pop_operand2(frame);

  if (v1 > v2)
    push_operand(frame, 1);

  else if (v1 == v2)
    push_operand(frame, 0);

  else if (v1 < v2)
    push_operand(frame, (u4)-1);
}

void handle_tableswitch(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* f = current_frame(ctx);
  
  u1* base_pc = f->pc - 1;

  // offset
  while ((f->pc - f->method.code_attr->code) % 4 != 0)
    f->pc++;

  int32_t default_offset = (int32_t)fetch_u4(&f->pc);
  int32_t low = (int32_t)fetch_u4(&f->pc);
  int32_t high = (int32_t)fetch_u4(&f->pc);

  int32_t index = (int32_t)pop_operand(f);

  u1* table_start = f->pc;
  int32_t offset;
  if (index < low || index > high)
  {
    offset = default_offset;
  } 
  else 
  {
     u1* target_offset_ptr = table_start + ((index - low) * 4);
     offset = (int32_t)fetch_u4(&target_offset_ptr);
  }

  f->pc = base_pc + offset;
}

void handle_lookupswitch(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* f = current_frame(ctx);
  
  u1* base_pc = f->pc - 1;

  // offset
  while ((f->pc - f->method.code_attr->code) % 4 != 0)
    f->pc++;

  int32_t default_offset = (int32_t)fetch_u4(&f->pc);
  int32_t npairs = (int32_t)fetch_u4(&f->pc);

  int32_t key = (int32_t)pop_operand(f);
  int32_t offset;

  for (int i = 0; i < npairs; i++)
  {
    int32_t pair_key = (int32_t)fetch_u4(&f->pc);
    offset = (int32_t)fetch_u4(&f->pc);
    
    if (key == pair_key) 
      goto branch;
  }

  offset = default_offset;

branch:
  f->pc = base_pc + offset;
}
