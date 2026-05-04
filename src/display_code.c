#include "classfile.h"
#include "display.h"
#include "bytecode.h"


static void print_cp_operands(const ClassFile* cf, u1 operands, 
  Reader* code, FILE* out) {
  u2 idx = operands == 2 ? read_u2(code) :
  read_u1(code);

  fprintf(out, "#%d // ", idx);
  print_cp_value(idx, cf, out); 
}

static void print_local_literal_operands(u1 operands, Reader* code, FILE* out) {
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
  
  fprintf(out, "(%d to %d)\n", low_byte, high_byte);
  for (u4 i = 0; i <= high_byte - low_byte; i++) {
    print_indent(indent, out);
    fprintf(out, "%3d: %-3d\n", low_byte + i, base_pc + read_u4(code));
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

void print_operands(const ClassFile* cf, Reader *code_reader,
    u1 opc, FILE* out, int indent) {
  const opcode_info* opi = &opcode_table[opc];
  if (!opi->operands) return;

  if (opi->type == OP_CP) {
    print_cp_operands(cf, opi->operands, code_reader, out);
  }

  if (opi->type == OP_LOCAL || opi->type == OP_LITERAL) {
    print_local_literal_operands(opi->operands, code_reader, out);
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
      
      case opc_lookupswitch:
        print_lookupswitch_operands(code_reader, indent, out);
        break;
        
      default:
        break;

    }
  }
} 

void print_code(const ClassFile *cf, 
    const Code_attribute* code, FILE* out, int indent) {
  print_indent(indent, out);
  fprintf(out, "max_stack: %d, max_locals: %d\n", code->max_stack,
      code->max_locals);

  Reader code_reader = { code->code, code->code_length, 0 };
  
  // TODO operandos
  while (code_reader.pos < code_reader.size) {
    print_indent(indent, out);
    u4 pc = code_reader.pos;
    u1 opc = read_u1(&code_reader);
    fprintf(out, "%3d: %-15s ", pc, opcode_table[opc].name);

    print_operands(cf, &code_reader, opc, out, indent+5);
    fputc('\n', out);
  }


  return;
}
