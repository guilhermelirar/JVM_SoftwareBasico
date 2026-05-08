#include "classfile.h"
#include "display.h"
#include "bytecode.h"
#include "reader.h"
#include <stdio.h>
#include <strings.h>


static void print_cp_operands(const ClassFile* cf, u1 operands, 
  Reader* code, FILE* out) {
  u2 idx = operands == 2 ? read_u2(code) :
  read_u1(code);

  fprintf(out, "#%-10u // ", idx);
  print_cp_value(idx, cf, out); 
}

static void print_local_operands(u1 operands, Reader* code, FILE* out) {
  if (operands == 2) {
    fprintf(out, "%u", read_u2(code));
  } else {
    fprintf(out, "%u", read_u1(code));
  }
}

static void print_literal_operands(u1 operands, Reader* code, FILE* out) {
  if (operands == 2) {
    fprintf(out, "%d", (int16_t)read_u2(code));
  } else {
    fprintf(out, "%d", (int8_t)read_u1(code));
  }
}

static void print_branch_operands(u1 operands, Reader* code, FILE* out) {
    u4 base_pc = code->pos - 1;

    int32_t offset;
    if (operands == 2) {
      offset = (int16_t)read_u2(code);
    } else if (operands == 4) {
      offset = (int32_t)read_u4(code); 
    } else {
      offset = (int8_t)read_u1(code);
    }

    fprintf(out, "%d", base_pc + offset);
}

static void print_tableswitch_operands(Reader* code, int indent, FILE* out) {
  u4 base_pc = code->pos - 1;
  while (code->pos % 4 != 0) 
    read_u1(code); // 0 a 3 bytes de padding
  
  u4 default_offset = read_u4(code);
  u4 low_byte = read_u4(code);
  u4 high_byte = read_u4(code);

  // tabela de salto 
  fputc('\n', out);
  print_indent(indent, out);
  
  fprintf(out, "(%u to %u)\n", low_byte, high_byte);
  for (u4 i = 0; i <= high_byte - low_byte; i++) {
    print_indent(indent, out);
    fprintf(out, "%3u: %-3d\n", low_byte + i, base_pc + read_u4(code));
  }

  print_indent(indent, out);
  fprintf(out, "default: %-3d", default_offset + base_pc);
}

static void print_lookupswitch_operands(Reader* code, int indent, FILE* out) {
  u4 base_pc = code->pos - 1;
  while (code->pos % 4 != 0) 
    read_u1(code); // 0 a 3 bytes de padding 
                          
  int32_t default_offset = (int32_t)read_u4(code);
  int32_t npairs = (int32_t)read_u4(code);
  fputc('\n', out);
  print_indent(indent, out);
  fprintf(out, "(%d pairs) \n", npairs);
  
  while (npairs--) {
    int32_t match = (int32_t)read_u4(code);
    int32_t offset = (int32_t)read_u4(code);
    
  print_indent(indent, out);
    fprintf(out, "%4d: %d\n", (int32_t)match, (int32_t)offset + base_pc);
  }

  print_indent(indent, out);
  fprintf(out, "default: %d", default_offset + base_pc);
}

static void print_invokeinterface_operands(Reader* code, 
    const ClassFile* cf, FILE* out) {
  u2 idx = read_u2(code);
  u1 count = read_u1(code);
  read_u1(code); // byte 0

  fprintf(out, "#%u, %-3u     //", idx, count);

  print_cp_value(idx, cf, out); 
}


static void print_wide_operands(Reader* code, FILE* out) {
  u1 opc = read_u1(code);

  // format 2 (iinc)
  if (opc == opc_iinc) {
      u2 index = read_u2(code);                   // indexbyte{1,2}
      int16_t increment = (int16_t)read_u2(code); // constbyte{1,2}

      fprintf(out, "iinc %u, %+d", index, increment);
      return;
  }

  // format 1 (iload, fload, aload, lload, dload, istore, 
  //           fstore, astore, lstore, dstore, ret)
  fprintf(out, "%s ", opcode_table[opc].name);
  print_local_operands(2, code, out);
}

void print_operands(const ClassFile* cf, Reader *code_reader,
    u1 opc, FILE* out, int indent) {
  const opcode_info* opi = &opcode_table[opc];
  if (!opi->operands) return;

  if (opi->type == OP_CP) {
    print_cp_operands(cf, opi->operands, code_reader, out);
  }

  if (opi->type == OP_LOCAL) {
    print_local_operands(opi->operands, code_reader, out);
  }

  if (opi->type == OP_LITERAL) {
    print_literal_operands(opi->operands, code_reader, out);
  }

  if (opi->type == OP_BRANCH) {
    print_branch_operands(opi->operands, code_reader, out);
  }

  if (opi->type == OP_SPECIAL) {
    switch (opc) {
      // tableswitch 
      case opc_tableswitch: 
        print_tableswitch_operands(code_reader, indent, out);
        break;
      
      // lookupswitch
      case opc_lookupswitch:
        print_lookupswitch_operands(code_reader, indent, out);
        break;
        
      // iinc
      case opc_iinc: {
        u1 index = read_u1(code_reader);
        int8_t increment = (int8_t)read_u1(code_reader);

        fprintf(out, "%u, %+d", index, increment);
        break;
      }

      // invokeinterface 
      case opc_invokeinterface:
        print_invokeinterface_operands(code_reader, cf, out);
        break;

      // wide
      case opc_wide:
        print_wide_operands(code_reader, out);
        break;

      case opc_invokedynamic:
        // indexbyte{1, 2}, 0, 0
        print_cp_operands(cf, 2, code_reader, out);
        read_u2(code_reader);
        break;

      case opc_multianewarray: {
        // indexbyte{1,2} dimensions
        u2 index = read_u2(code_reader);
        u1 dimensions = read_u1(code_reader);
        fprintf(out, "%u [%u]\t  //", index, dimensions);
        print_cp_value(index, cf, out);
      }


      default:
        break;

    }
  }
} 

void print_code(const ClassFile *cf, 
    const Code_attribute* code, FILE* out, int indent) {
  print_indent(indent, out);
  fprintf(out, "max_stack: %u, max_locals: %u\n", code->max_stack,
      code->max_locals);

  Reader code_reader = { code->code, code->code_length, 0 };
  
  while (code_reader.pos < code_reader.size) {
    print_indent(indent, out);
    u4 pc = code_reader.pos;
    u1 opc = read_u1(&code_reader);
    fprintf(out, "%3u: %-15s ", pc, opcode_table[opc].name);

    print_operands(cf, &code_reader, opc, out, indent+5);
    fputc('\n', out);
  }

  // Exceptions
  if (code->exception_table_length) {
    print_indent(indent-2, out);
    fputs("Exception table:\n", out);
    print_indent(indent, out);
    fputs("from    to   target type\n", out);
  }

  for (u2 i = 0; i < code->exception_table_length; i++) {
    print_indent(indent, out);

    // from to target
    fprintf(out, "%4u %5u %8u ", 
        code->exception_table[i].start_pc,
        code->exception_table[i].end_pc,
        code->exception_table[i].handler_pc);
   
    // type
    print_cp_value(code->exception_table[i].catch_type, 
        cf, out);
    fputc('\n', out);
  }

  // Exibindo atributos internos
  if (code->attributes_count > 0) {
    print_attributes(cf, code->attributes_count, 
        code->attributes, out, indent + 2);
  }

  return;
}
