#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "common/bytecode.h"
#include <stdint.h>
#include <stdio.h>

// 54-86
void handle_store(JVM_Context *ctx, u1 opc)
{
  Frame *f = current_frame(ctx);
  bool widened = (f->pc - 1 != f->method.code_attr->code) && 
    (*(f->pc - 2)) == opc_wide;

  // Carrega o índice
  u2 idx = 0;
  if (opc < opc_istore_0)
  {
    // wide torna index u2
    idx = widened ? fetch_u2(&f->pc) : *f->pc++;
  }
  else if (opc < opc_iastore) 
  {
    // índice 0, 1, 2, 3, para istore, lstore, fstore, dstore, astore
    idx = ((opc - opc_istore_0) % 4);
  }

  // se double ou long
  if (opc == opc_dstore || opc == opc_lstore || 
    (IN_RANGE(opc, opc_dstore_0, opc_dstore_3)) ||
    (IN_RANGE(opc, opc_lstore_0, opc_lstore_3)))
  {
      u4 low = pop_operand(f);
      u4 high = pop_operand(f);
      
      f->locals[idx] = high;
      f->locals[idx+1] = low;
      return;
  }
 
  // tipos primitivos de 32 bits
  if (opc < opc_iastore)
  {
    f->locals[idx] = pop_operand(f);
    return;
  }

  if (IN_RANGE(opc, opc_astore_0, opc_astore_3))
  {
    f->locals[opc - opc_astore_0] = pop_operand(f);
  }
}

void handle_load(JVM_Context* ctx, u1 opc) {
  Frame* frame = current_frame(ctx);

  u2 idx = 0;  // índice para a variável local
  bool widened = (frame->pc - 1 != frame->method.code_attr->code) && 
    (*(frame->pc - 2)) == opc_wide;

  // Opcodes com operandos
  if (IN_RANGE(opc, opc_iload, opc_aload))
  {
    // wide torna index u2
    idx = widened ? fetch_u2(&frame->pc) : *frame->pc++;
  }
  // Opcodes sem operandos
  else if (IN_RANGE(opc, opc_iload_0, opc_aload_3)) 
  {
    // índice 0, 1, 2, 3, para iload, lload, fload, dload, aload
    idx = ((opc - opc_iload_0) % 4);
  }

  // cat2
  if (opc == opc_lload || opc == opc_dload ||
      IN_RANGE(opc, opc_lload_0, opc_lload_3) || 
      IN_RANGE(opc, opc_dload_0, opc_dload_3)) 
  {

    u8 bits = ((u8)frame->locals[idx] << 32) | // high 
      (u8) frame->locals[idx+1]; // low
    
    push_operand2(frame, bits);
    return;
  }

  if (IN_RANGE(opc, opc_iload, opc_aload_3))
  {
    push_operand(frame, frame->locals[idx]);
    return;
  } 

  // ARRAY TODO checagem de null pointer, e indices validos
  if (IN_RANGE(opc, opc_iaload , opc_saload))
  {
    idx = pop_operand(frame);
    u4 arrayref = pop_operand(frame);

    Object* array_obj = &ctx->objects.entries[arrayref];
    // TODO null check

    Array* array = &array_obj->content.arr;
    // TODO index on range check

    // TODO checagem de tipo
    if (opc == opc_baload) // byte
    {
      push_operand(frame, (int32_t)((int8_t)array->data[idx]));
      return;
    }

    if (opc == opc_caload) // char 
    {
      push_operand(frame, array->data[idx]);
      return;
    }

    if (opc == opc_saload) // short (signed)
    {
      push_operand(frame, (int32_t)((int16_t)array->data[idx]));
      return;
    }

    if (opc == opc_laload || opc == opc_daload) // cat2 
    {
      u8 bits = ((u8)array->data[idx*2] << 32) | (u8)array->data[idx*2+1];
      push_operand2(frame, bits);
      return;
    }

    // if (float or int)
    push_operand(frame, array->data[idx]);
  }
}

void handle_tastore(JVM_Context *ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  switch (opc)
  {
    case opc_iastore:
    case opc_fastore:
    case opc_bastore:
    case opc_aastore:
    case opc_sastore:
    case opc_castore:
    {
      u4 val = pop_operand(frame);
      int32_t index = (int32_t)pop_operand(frame);
      u4 arrayref = pop_operand(frame);

      if (!arrayref)
        goto nullptr_exc;
      
      Array* arr = &ctx->objects.entries[arrayref].content.arr;
      if (!IN_RANGE(index, 0, (int32_t)arr->length - 1))
        goto idx_oob;

      arr->data[index] = val;
      return;
    }

    case opc_dastore:
    case opc_lastore:
    {
      u8 val = pop_operand2(frame);
      int32_t index = (int32_t)pop_operand(frame);
      u4 arrayref = pop_operand(frame);
      
      if (!arrayref)
        goto nullptr_exc;
      
      Array* arr = &ctx->objects.entries[arrayref].content.arr;
      if (!IN_RANGE(index, 0, (int32_t)arr->length * 2 - 2))
        goto idx_oob;

      arr->data[index*2] = (u4)(val >> 32);
      arr->data[index*2 + 1] = (u4)val;
    }

    default:
    return;
  }

idx_oob: // TODO throw
  terminateJVM(ctx);
  fprintf(stderr, "ArrayIndexOutOfBounds");
  exit(1); 
nullptr_exc: // TODO throw
  terminateJVM(ctx);
  fprintf(stderr, "NullPointerException");
  exit(1);
}

