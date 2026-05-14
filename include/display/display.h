#ifndef DISPLAY
#define DISPLAY

#include "common/classfile.h"
#include "common/reader.h"
#include <stdio.h>

typedef struct {
  u2 mask;
  const char* java_kw;
  const char* debug_kw;
} access_flag_desc;

typedef enum {
    ACCESS_FMT_DEBUG, 
    ACCESS_FMT_JAVA 
} access_format_t;

static inline void print_indent(int level, FILE *f) {
  for (int i = 0; i < level; i++) {
    fputc(' ', f);
  }
}

void print_field_descriptor(FILE* out, const char* str, int* pos, int dimensions);

void print_utf8(FILE* out, const char* str);

void print_operands(const ClassFile* cf, Reader *code_reader,
    u1 opc, FILE* out, int indent);

void print_code(const ClassFile *cf, 
    const Code_attribute* code, FILE* out, int indent);  

void print_method_definition(const ClassFile* cf, method_info* m, FILE* out);


void print_cp_value(u2 idx, const ClassFile *cf, FILE *file);

void print_class_constant_pool(const ClassFile *cf, FILE *file);

void print_access_flags(u2 bits, access_context_t ctx, 
    access_format_t fmt, FILE* file);

void print_interfaces(const ClassFile* cf, FILE* file);

void print_fields(const ClassFile* cf, FILE* file);

void print_attributes(const ClassFile *cf, 
    u2 count, attribute_info *attributes, 
    FILE *file, int indent);

void print_methods(const ClassFile* cf, FILE *file);

void print_class_member(const ClassFile* cf,
    u2 index,
    u2 name_index, u2 descriptor_index, u2 attributes_count,
    attribute_info* attributes,
    access_context_t access_ctx, u2 access_flags,
    FILE* file);

void printclass(const ClassFile* cf, FILE* file);

#endif
