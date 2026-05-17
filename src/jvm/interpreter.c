#include "jvm/interpreter.h"
#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include "common/bytecode.h"
#include <string.h>

static inline u1 fetch_u1(u1 *code, u4 *pc) {
  return code[(*pc)++];
}

static inline u2 fetch_u2(u1 *code, u4 *pc) {
  u2 v = ((u2)code[*pc] << 8) |
         ((u2)code[*pc+1]);
  *pc += 2;
  return v;
}

static inline u4 read_u4(u1* code, u4 *pc) {
  u4 v = ((u4)code[*pc]     << 24) |
         ((u4)code[*pc + 1] << 16) |
         ((u4)code[*pc + 2] << 8)  |
          (u4)code[*pc + 3];
  *pc += 4;
  return v;
}
// 0
void handle_nop(JVM_Context* ctx) {
  // Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
};

// 178
void handle_getstatic(JVM_Context *ctx)
{
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);

  // encontrando cpinfo
  cp_info* cp_entry = &frame->constant_pool[cp_idx];
  
  // TODO lidar melhor
  if (cp_entry->tag != CONSTANT_Fieldref) return;
  u2 class_index = cp_entry->info.fieldref_info.class_index;
  u2 nt_index = cp_entry->info.fieldref_info.name_and_type_index;
}

const instruction_handler DISPATCH_TABLE[256] = {
#include "jvm/dispatch_table.def"
};

