#ifndef INTERPRETER
#define INTERPRETER

#include "jvmtypes.h"

#define JVM_HANDLE_SYSOUT 0x01

typedef void (*instruction_handler)(JVM_Context* ctx);

extern const instruction_handler DISPATCH_TABLE[256];

static inline u1 fetch_u1(u1 *code, u4 *pc) {
  return code[(*pc)++];
}

static inline u2 fetch_u2(u1 *code, u4 *pc) {
  u2 v = ((u2)code[*pc] << 8) |
         ((u2)code[*pc+1]);
  *pc += 2;
  return v;
}

static inline u4 fetch_u4(u1* code, u4 *pc) {
  u4 v = ((u4)code[*pc]     << 24) |
         ((u4)code[*pc + 1] << 16) |
         ((u4)code[*pc + 2] << 8)  |
          (u4)code[*pc + 3];
  *pc += 4;
  return v;
}

void main_loop(JVM_Context* ctx);

void handle_nop(JVM_Context* ctx);  // 0 
void handle_ldc(JVM_Context* ctx); // 18, 19, 20
void handle_getstatic(JVM_Context* ctx); // 178
void handle_invokevirtual(JVM_Context* ctx); // 182

#endif
