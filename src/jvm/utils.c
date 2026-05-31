#include <stdio.h>
#include <stdlib.h>
#include "jvm/utils.h"
#include "common/bytecode.h"
#include "jvm/jvm.h"


void handle_sysout(Frame* frame, JVM_Context* ctx, char descriptor) 
{
  switch (descriptor) {
    case 'L':
        printf("%s\n", ctx->strings.strings[pop_operand(frame)]);
      break;
      
    case 'S':
    case 'B':
    case 'I':
      printf("%d\n", (int)pop_operand(frame));
      break;

    case 'J':
      printf("%ld\n", (long)pop_operand2(frame));
      break;

    case 'D':
      printf("%f\n", u8_to_double(pop_operand2(frame)));
      break;

    case 'F':
      printf("%f\n", u4_to_float(pop_operand(frame)));
      break;
  } 

  pop_operand(frame); // consumindo object_ref
}

void jvm_error_uninmplemented_opc(JVM_Context *ctx, u1 opc) 
{
  terminateJVM(ctx);
  fprintf(stderr, "ERROR: unimplemented opcode \"0x%X\" (%s)." 
      "\nAborting...\n", opc, opcode_table[opc].name);
  exit(1);
}
