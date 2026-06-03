#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include "common/classfile.h"
#include "common/bytecode.h" // IWYU pragma: keep

// 0
void handle_nop(JVM_Context* ctx, u1 opc) 
{
  // Opcode não implementado, execução deve ser
  // encerrada
  if (opc != opc_nop) {
    jvm_error_uninmplemented_opc(ctx, opc);
  }
}

void handle_tconst(JVM_Context *ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  float f;
  switch (opc)
  {
    // 1
    case opc_aconst_null:
      push_operand(frame, 0);
      break;

    // 2-8
    case opc_iconst_m1:
    case opc_iconst_0:
    case opc_iconst_1:
    case opc_iconst_2:
    case opc_iconst_3:
    case opc_iconst_4:
    case opc_iconst_5:
      push_operand(frame, (int32_t)(-1 + opc-opc_iconst_m1));
      break;

    // 9, 10
    case opc_lconst_0:
    case opc_lconst_1:
      push_operand(frame, (int32_t)0);
      push_operand(frame, (int32_t)(opc - opc_lconst_0));
      break;

    // 11, 12, 13
    case opc_fconst_0:
    case opc_fconst_1:
    case opc_fconst_2:
    {
      if (opc == opc_fconst_0) f = 0.0f;
      else if (opc == opc_fconst_1) f = 1.0f;
      else f = 2.0f;

      u4 value;
      memcpy(&value, &f, sizeof(float));
      push_operand(frame, value);
      break;
    }

    // 14, 15
    case opc_dconst_0:
    case opc_dconst_1:
    {
      double d = opc == opc_dconst_0 ? 0.0 : 0.1;
      u8 value;
      memcpy(&value, &d, sizeof(double));
      push_operand(frame, (u4)(value >> 8));
      push_operand(frame, (u4)(value & 0xFFFF));
      break;
    }

    default:
    break;
  }
}

// 16, 17
void handle_push(JVM_Context *ctx, u1 opc) 
{
  Frame* frame = current_frame(ctx);

  if (opc == opc_bipush)
    push_operand(frame, (int32_t)((int8_t)fetch_u1(frame->code, &frame->pc)));
  else if (opc == opc_sipush)
  {
    push_operand(frame, (int32_t)((int8_t)fetch_u2(frame->code, &frame->pc)));
  }
}

// 18, 19, 20 
void handle_ldc(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);

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
      push_operand(frame, entry->info.int_info.bytes);
      return;

    case CONSTANT_Float:
      push_operand(frame, entry->info.float_info.bytes);
      return;

    case CONSTANT_Long:
      push_operand(frame, entry->info.long_info.h_bytes);
      push_operand(frame, entry->info.long_info.l_bytes);
      return;

    case CONSTANT_Double:
      push_operand(frame, entry->info.double_info.h_bytes);
      push_operand(frame, entry->info.double_info.l_bytes);
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
          push_operand(frame, i);
          return;
        }
      }
      
      ctx->strings.strings[ctx->strings.count] = (char*)str;
      push_operand(frame, ctx->strings.count);
      ctx->strings.count++;
      break;
    }
    default:
      break;
  }
}


// 54-86
void handle_store(JVM_Context *ctx, u1 opc)
{
  Frame *f = current_frame(ctx);

  // Carrega o índice
  u1 idx = 0;
  if (opc < opc_istore_0)
  {
    idx = fetch_u1(f->code, &f->pc);
  }
  else if (opc < opc_iastore) 
  {
    // índice 0, 1, 2, 3, para istore, lstore, fstore, dstore, astore
    idx = ((opc - opc_istore_0) % 4);
  }

  // se double ou long
  if (opc == opc_dstore || opc == opc_lstore || 
    (IN_RANGE(opc, opc_dstore_0, opc_dstore_3)) ||
    (IN_RANGE(opc, opc_lstore_0, opc_lstore_3)))
  {
      u4 low = pop_operand(f);
      u4 high = pop_operand(f);
      
      f->locals[idx] = high;
      f->locals[idx+1] = low;
      return;
  }
 
  // tipos primitivos de 32 bits
  if (opc < opc_iastore)
  {
    f->locals[idx] = pop_operand(f);
    return;
  }

  // TODO
  // iastore, lastore, fastore, dastore, aastore, bastore, castore, sastore
}

// 178
void handle_getstatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
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

void handle_invokevirtual(JVM_Context *ctx, u1 opc) {
  (void)opc;
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
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.name_index);
  
  const char* descriptor = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.descriptor_index);

  if (strcmp(class_name, "java/io/PrintStream") == 0 && 
      strcmp(method_name, "println") == 0)
    handle_sysout(frame, ctx, descriptor[1]);
}

void handle_invokestatic(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(frame->code, &frame->pc);
  cp_info* entry = &frame->constant_pool[cp_idx];

  u2 class_idx = entry->info.methodref_info.class_index;
  const char* class_name = cp_class_name(frame->constant_pool, class_idx);

  // name_and_type
  u2 nt_idx = entry->info.methodref_info.name_and_type_index;
  cp_info* nt_entry = &frame->constant_pool[nt_idx];

  // descritor
  const char* method_name = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.name_index);
  const char* descriptor = 
    cp_get_utf8(frame->constant_pool, 
        nt_entry->info.name_and_type_info.descriptor_index);


  // procura classe 
  LoadedClass* class = find_class_by_name(ctx, class_name);
  if (class == NULL) goto err_no_such_method;

  // procura método na classe base, e nas superclasses
  method_info* m = find_method(class->cf, method_name, descriptor);
  if (m == NULL)
  {
    class = find_superclass_with_method(ctx, class, method_name, descriptor);
    if (class == NULL) goto err_no_such_method;
  }

  // garantido que existe
  m = find_method(class->cf, method_name, descriptor);

  if (m->access_flags & ACC_PRIVATE)
    goto err_no_such_method;

  int args = count_args_size(descriptor);
  push_frame(&ctx->t, new_frame(class->cf, m));
  Frame* new_f = current_frame(ctx);

  for (int i = args-1; i >= 0; i--) {
    new_f->locals[i] = frame->operand_stack[frame->stack_ptr--];
  }

  return;
err_no_such_method:
  fprintf(stderr, 
      "Fatal Error: NoSuchMethodError." 
      "Could not find method '%s%s' in hierarchy.\n", 
      method_name, descriptor);
  exit(1); 
}

void handle_load(JVM_Context* ctx, u1 opc) {
  Frame* frame = current_frame(ctx);

  u4 idx = 0;  // índice para a variável local

  // Opcodes com operandos
  if (IN_RANGE(opc, opc_iload, opc_aload))
    idx = fetch_u1(frame->code, &frame->pc);

  // Opcodes sem operandos
  else if (IN_RANGE(opc, opc_iload_0, opc_aload_3)) 
  {
    // índice 0, 1, 2, 3, para iload, lload, fload, dload, aload
    idx = ((opc - opc_iload_0) % 4);
  }

  // cat2
  if (opc == opc_lload || opc == opc_dload ||
      IN_RANGE(opc, opc_lload_0, opc_lload_3) || 
      IN_RANGE(opc, opc_dload_0, opc_dload_3)) 
  {

    u8 bits = ((u8)frame->locals[idx] << 32) | // high 
      (u8) frame->locals[idx+1]; // low
    
    push_operand2(frame, bits);
    return;
  }

  if (IN_RANGE(opc, opc_iload, opc_aload_3))
  {
    push_operand(frame, frame->locals[idx]);
    return;
  } 

  // ARRAY TODO checagem de null pointer, e indices validos
  if (IN_RANGE(opc, opc_iaload , opc_saload))
  {
    idx = pop_operand(frame);
    u4 arrayref = pop_operand(frame);

    JVM_Array* array;
    void* array_v = ctx->objects.entries[arrayref];
    // TODO null check

    array = (JVM_Array*)array_v;
    // TODO index on range check

    // TODO checagem de tipo
    if (opc == opc_baload) // byte
    {
      push_operand(frame, (int32_t)((int8_t)array->data[idx]));
      return;
    }

    if (opc == opc_caload) // char 
    {
      push_operand(frame, array->data[idx]);
      return;
    }

    if (opc == opc_saload) // short (signed)
    {
      push_operand(frame, (int32_t)((int16_t)array->data[idx]));
      return;
    }

    if (opc == opc_laload || opc == opc_daload) // cat2 
    {
      push_operand(frame, array->data[idx*2]);   // high
      push_operand(frame, array->data[idx*2+1]); // low
      return;
    }

    // if (float or int)
    push_operand(frame, array->data[idx]);
  }
}

