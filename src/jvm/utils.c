#include <stdio.h>
#include <stdlib.h>
#include "jvm/utils.h"
#include "common/bytecode.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"


void handle_sysout(Frame* frame, JVM_Context* ctx, char descriptor) 
{
  u4 val = frame->operand_stack[frame->stack_ptr--];
  u4 object_ref = frame->operand_stack[frame->stack_ptr--];

  if (object_ref == JVM_HANDLE_SYSOUT)
  {
    switch (descriptor) {
      case 'L':
        printf("%s\n", ctx->strings.strings[val]);
        break;
      
      case 'I':
        printf("%d\n", (int)val);
        break;
    }
  }
}

void jvm_error_uninmplemented_opc(JVM_Context *ctx, u1 opc) 
{
  terminateJVM(ctx);
  fprintf(stderr, "ERROR: unimplemented opcode \"0x%u\" (%s)." 
      "\nAborting...\n", opc, opcode_table[opc].name);
  exit(1);
}
