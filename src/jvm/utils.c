#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jvm/utils.h"
#include "common/bytecode.h"
#include "jvm/jvm.h"


void handle_sysout(Frame* frame, JVM_Context* ctx, char descriptor,
    const char* method) 
{
  char* end = !strcmp("println", method) ? "\n" : "";

  switch (descriptor) {
    case 'L':
    {
      u4 str_ref = pop_operand(frame);
      printf("%s%s", str_ref ? ctx->strings.strings[str_ref] : "null", end);
      break;
    }
      
    case 'S':
    case 'B':
    case 'I':
      printf("%d%s", (int)pop_operand(frame), end);
      break;

    case 'J':
      printf("%lld%s", (long long)(int64_t)pop_operand2(frame), end);
      break;

    case 'D':
      printf("%f%s", u8_to_double(pop_operand2(frame)), end);
      break;

    case 'F':
      printf("%f%s", u4_to_float(pop_operand(frame)), end);
      break;

    case 'C': 
      printf("%c%s", (char)pop_operand(frame), end);
      break;

    default:
      printf("%s", end);
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

const char* extract_class_name_from_path(const char *path)
{
  long long len = (long long)strlen(path);

  for (long long i = len - 1; i >= 0; i--)
  {
    if (path[i] == '/' || path[i] == '\\')
      return &path[i+1];
  }

  return path;
}
