#include "common/bytecode.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include <stdint.h>

void handle_return(JVM_Context *ctx, u1 opc)
{
  switch (opc) 
  {
    case opc_return:
      return pop_frame(&ctx->t);

    case opc_ireturn:
    {
      u4 ret_val = pop_operand(current_frame(ctx));
      pop_frame(&ctx->t);
      push_operand(current_frame(ctx), (u4)ret_val);
      
      return;
    }

  }
}
