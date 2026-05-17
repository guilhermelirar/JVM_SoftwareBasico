#ifndef INTERPRETER
#define INTERPRETER

#include "jvmtypes.h"

typedef void (*instruction_handler)(JVM_Context* ctx);

extern const instruction_handler DISPATCH_TABLE[256];

void handle_nop(JVM_Context* ctx);
void handle_getstatic(JVM_Context* ctx);

#endif
