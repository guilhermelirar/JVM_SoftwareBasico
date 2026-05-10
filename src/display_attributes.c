#include "classfile.h"
#include "display.h"
#include "string.h"
#include <stdio.h>

static inline void print_SourceFile_attr(const ClassFile* cf, 
    u2 sourcefile_index, int indent, FILE* file)
{
  print_indent(indent, file);
  fprintf(file, "\"%s\"", 
  cp_get_utf8(cf, sourcefile_index));
}

static void print_ConstantValue_attr(const ClassFile *cf, attribute_info* attr,
    int indent, FILE* out)
{
  print_indent(indent, out);
  fprintf(out, "#%-15u // ", attr->info.constantvalue_index);
  print_cp_value(attr->info.constantvalue_index, cf, out);
}

static void print_Exceptions_attr(const ClassFile *cf, 
    Exceptions_attribute* e, int indent, FILE* file)
{
  print_indent(indent, file);
  fputs("throws: ", file);
  
  for (u2 i = 0; i < e->number_of_exceptions; i++) {
    print_cp_value(e->exception_index_table[i], cf, file);
    if (i < e->number_of_exceptions-1) fputs(", ", file);
  }
}

static void print_LineNumberTable_attr(LineNumberTable_attribute* attr,
    int indent, FILE* file)
{
  for (u2 i = 0; i < attr->line_number_table_length; i++) {
    print_indent(indent, file);
    fprintf(file, "line %3u: %-3u\n", 
      attr->line_number_table[i].line_number,
      attr->line_number_table[i].start_pc);
  }
}

static void print_LocalVariableTable_attr(const ClassFile* cf,
    LocalVariableTable_attribute* attr, int indent, FILE* file) 
{
  LocalVariableTable_entry* entry;
  for (u2 j = 0; j < attr->local_variable_table_length; j++)
  {
    entry = &attr->local_variable_table[j];
    print_indent(indent, file);
    fprintf(file, 
            "start: %u; len: %u; slot: %u; name: %s; signature: %s\n", 
            entry->start_pc, entry->length, entry->index, 
            cp_get_utf8(cf, entry->name_index),
            cp_get_utf8(cf, entry->descriptor_index)
    );
  }
}
    
void print_attributes(const ClassFile *cf, u2 count, 
    attribute_info *attributes, FILE *file, int indent) {
  
  for (int i = 0; i < count; i++) {
    const char *name = cp_get_utf8(cf, attributes[i].attribute_name_index);

    // identação
    print_indent(indent, file);
    fprintf(file, "[%d] %s: \n", i, name);
   
    // ConstantValue
    if (strcmp(name, "ConstantValue") == 0)
      print_ConstantValue_attr(cf, &attributes[i], indent + 4, file);

    // Code
    else if (strcmp(name, "Code") == 0)
      print_code(cf, attributes[i].info.code_attribute, file, indent + 4);    

    // Exceptions
    else if (strcmp(name, "Exceptions") == 0)
      print_Exceptions_attr(cf, attributes[i].info.exceptions_attribute, 
          indent + 4, file);

    // SourceFile
    else if (strcmp(name, "SourceFile") == 0)
      print_SourceFile_attr(cf, attributes[i].info.sourcefile_index, 
          indent + 4, file);

    // LineNumberTable
    else if (strcmp(name, "LineNumberTable") == 0) 
      print_LineNumberTable_attr(
          attributes[i].info.line_number_table_attribute, 
          indent + 4, file);

    // LocalVariableTable
    else if (strcmp(name, "LocalVariableTable") == 0)
      print_LocalVariableTable_attr(cf, 
          attributes[i].info.local_variable_table_attribute, 
          indent + 4, file);

    // Não implementado
    else { 
      print_indent(indent + 4, file);
      fprintf(file, "Unsupported (%u bytes long)", 
          attributes[i].attribute_length);
    }

    // Quebra linha a cada atributo
    fputc('\n', file);
  }
}
