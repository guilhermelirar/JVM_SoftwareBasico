#include <string.h>
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include "common/classfile.h"
#include "common/bytecode.h" // IWYU pragma: keep

void handle_return(JVM_Context *ctx)
{
  pop_frame(&ctx->t);
}

// 0
void handle_nop(JVM_Context* ctx) {
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u1 opc = frame->code[frame->pc-1];
  
  // Opcode não implementado, execução deve ser
  // encerrada
  if (opc != opc_nop) {
    while (ctx->t.frame_ptr >= 0)
    {
      pop_frame(&ctx->t);
    }
  }
}

// 18, 19, 20 
void handle_ldc(JVM_Context* ctx)
{
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u1 opc = frame->code[frame->pc-1];

  u2 cp_idx;
  if (opc == opc_ldc) {
    cp_idx = fetch_u1(frame->code, &frame->pc); // lê 1 byte e avança
  } else { // ldc_w ou ldc2_w
    cp_idx = fetch_u2(frame->code, &frame->pc); 
  }

  cp_info* entry = &frame->constant_pool[cp_idx];

  switch (entry->tag) 
  {
    case CONSTANT_Integer:
      frame->operand_stack[++frame->stack_ptr] = entry->info.int_info.bytes;
      return;

    case CONSTANT_Float:
      frame->operand_stack[++frame->stack_ptr] = entry->info.int_info.bytes;
      return;

    case CONSTANT_String: 
    {
      const char* str = cp_get_utf8(frame->constant_pool, 
          entry->info.string_info.string_index);
      
      // ver se está na tabela de strings do ctx ou inserir
      for (u4 i = 0; i < ctx->strings.count; i++)
      {
        if (str == ctx->strings.strings[i])
        {
          frame->operand_stack[++frame->stack_ptr] = i;
          return;
        }
      }
      
      ctx->strings.strings[ctx->strings.count] = (char*)str;
      frame->operand_stack[++frame->stack_ptr] = ctx->strings.count;
      ctx->strings.count++;
      break;
    }
    default:
      break;
  }
}

// 178
void handle_getstatic(JVM_Context *ctx)
{
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);

  // encontrando cpinfo
  cp_info* cp_entry = &frame->constant_pool[cp_idx];
  
  if (cp_entry->tag != CONSTANT_Fieldref) return;

  u2 class_index = cp_entry->info.fieldref_info.class_index;
  u2 nt_index = cp_entry->info.fieldref_info.name_and_type_index;
  const char* class = cp_class_name(frame->constant_pool, class_index);
  const char* nt = cp_nameandtype_name(frame->constant_pool, nt_index);

  // Ignorar classes java (ex: System)
  if (strcmp("java/lang/System", class) == 0 && strcmp("out", nt) == 0)
  {
    frame->operand_stack[++frame->stack_ptr] = JVM_HANDLE_SYSOUT;
    return;
  }

  // TODO busca na área de métodos
  // ... busca na área de métodos
}

void handle_invokevirtual(JVM_Context *ctx) {
  Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);
  cp_info* entry = &frame->constant_pool[cp_idx];

  // classe do método
  u2 class_idx = entry->info.methodref_info.class_index;
  const char* class_name = cp_class_name(frame->constant_pool, class_idx);

  // name_and_type
  u2 nt_idx = entry->info.methodref_info.name_and_type_index;
  cp_info* nt_entry = &frame->constant_pool[nt_idx];

  // descritor
  const char* method_name = 
    cp_get_utf8(frame->constant_pool, nt_entry->info.name_and_type_info.name_index);
  const char* descriptor = 
    cp_get_utf8(frame->constant_pool, nt_entry->info.name_and_type_info.descriptor_index);

  if (strcmp(class_name, "java/io/PrintStream") == 0 && 
      strcmp(method_name, "println") == 0)
    handle_sysout(frame, ctx, descriptor[1]);
}

const instruction_handler DISPATCH_TABLE[256] = {
#include "jvm/dispatch_table.def"
};

void jvm_run(JVM_Context* ctx)
{
  while (ctx->t.frame_ptr >= 0)
  {
    Frame* frame = ctx->t.frames[ctx->t.frame_ptr];
    
    u1 opcode = fetch_u1(frame->code, &frame->pc);
    
    DISPATCH_TABLE[opcode](ctx);
    if (ctx->t.frame_ptr < 0) break;
  }
}
