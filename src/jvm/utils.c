#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
      printf("%lld\n", (long long)(int64_t)pop_operand2(frame));
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

void extract_class_dir(const char* path, char* dest, size_t dest_size)
{
  long long len = (long long)strlen(path);

  for (long long i = len - 1; i >= 0; i--)
  {
    if (path[i] == '/' || path[i] == '\\')
    {
      size_t dir_len = dir_len = i + 1; // inclui separador

      if (dir_len >= dest_size) dir_len = dest_size - 1;

      strncpy(dest, path, dir_len);
      dest[dir_len] = '\0';
      return;
    }
  }

  snprintf(dest, dest_size, ".%c", '/');
}
