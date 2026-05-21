#include <stdio.h>
#include "jvm/utils.h"
#include "jvm/interpreter.h"


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
