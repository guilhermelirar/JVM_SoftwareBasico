#include "common/classfile.h"
#include "common/classfile_reader.h"
#include "common/reader.h"
#include <string.h>
#include <stdlib.h>

void read_code_attribute(
  Reader *r,
  ClassFile *cf,
  Code_attribute* code_attr
) {
  // malloc #10, 11, 12
  code_attr->max_stack = read_u2(r);
  code_attr->max_locals = read_u2(r);
  code_attr->code_length = read_u4(r);
  code_attr->code = (u1*)calloc(code_attr->code_length, sizeof(u1));
  if (!code_attr) return; // TODO melhorar
  
  for (u4 i = 0; i < code_attr->code_length; i++) 
    code_attr->code[i] = read_u1(r);
  
  code_attr->exception_table_length = read_u2(r);
  if (code_attr->exception_table_length) {
    code_attr->exception_table = (exception_info*)calloc(
      code_attr->exception_table_length, sizeof(exception_info)
    );
    if (!code_attr->exception_table) return;
  } else code_attr->exception_table = NULL;

  // Leitura de tabela de exceções
  exception_info* ei = NULL;
  for (u2 i = 0; i < code_attr->exception_table_length; i++) {
    ei = &code_attr->exception_table[i];
    ei->start_pc = read_u2(r);
    ei->end_pc = read_u2(r);
    ei->handler_pc = read_u2(r);
    ei->catch_type = read_u2(r);
  }
  
  code_attr->attributes_count = read_u2(r);
  code_attr->attributes = (attribute_info*)calloc(
      code_attr->attributes_count, 
      sizeof(attribute_info)
      );
  if (!code_attr->attributes) return;

  read_attributes(r, cf, 
      code_attr->attributes_count, code_attr->attributes);
}

static void read_exceptions(Reader *r, Exceptions_attribute* e) {
  e->number_of_exceptions = read_u2(r);
  e->exception_index_table = calloc(e->number_of_exceptions, sizeof(u2));
  for (u2 i = 0; i < e->number_of_exceptions; i++) {
    e->exception_index_table[i] = read_u2(r);
  }
}

static void read_LocalVariableTable(Reader *r, attribute_info* attr) 
{
  // Alocar tabela
  LocalVariableTable_attribute* local_var_table_attr = 
    (LocalVariableTable_attribute*)
    malloc(sizeof(LocalVariableTable_attribute));

  if (local_var_table_attr == NULL) {
    for (u4 i = 0; i < attr->attribute_length; i++)
      read_u1(r);
    return;
  }

  attr->info.local_variable_table_attribute = local_var_table_attr;

  u2 length = read_u2(r);
  local_var_table_attr->local_variable_table_length = length;

  local_var_table_attr->local_variable_table = (LocalVariableTable_entry*)
    calloc(length, sizeof(LocalVariableTable_entry));

  if (local_var_table_attr->local_variable_table == NULL) {
    while (local_var_table_attr->local_variable_table_length--) 
    {
      read_u4(r); read_u4(r); // 8 bytes cada entrada
    }
    return;
  }

  // Lendo cada uma das entradas
  LocalVariableTable_entry* entry;
  for (u2 i = 0; i < local_var_table_attr->local_variable_table_length; i++) 
  {
    entry = &local_var_table_attr->local_variable_table[i];
    entry->start_pc = read_u2(r);
    entry->length = read_u2(r);
    entry->name_index = read_u2(r);
    entry->descriptor_index = read_u2(r);
    entry->index = read_u2(r);
  }
}

void read_attribute_info(Reader* r, ClassFile* cf, 
    attribute_info* attr) {
  const char* attr_name = cp_get_utf8(cf->constant_pool, 
      attr->attribute_name_index);
  
  // Caso constan value, indice para constant_pool
  if (strcmp(attr_name, "ConstantValue") == 0) {
    attr->info.constantvalue_index = read_u2(r); 
    return;
  }

  if (strcmp(attr_name, "Code") == 0) {
    attr->info.code_attribute = malloc(sizeof(Code_attribute)); // malloc #9
    read_code_attribute(r, cf, attr->info.code_attribute);
    return;
  } 

  if (strcmp(attr_name, "Exceptions") == 0) {
    // malloc exceptions_attribute
    attr->info.exceptions_attribute = malloc(sizeof(Exceptions_attribute));
    read_exceptions(r, attr->info.exceptions_attribute);
    return;
  }

  if (strcmp(attr_name, "SourceFile") == 0) {
    attr->info.sourcefile_index = read_u2(r);
    return;
  }

  if (strcmp(attr_name, "LineNumberTable") == 0) {
    // malloc LineNumberTable_attribute
    attr->info.line_number_table_attribute = 
      (LineNumberTable_attribute*)
      malloc(sizeof(LineNumberTable_attribute));
   
    u2 line_number_table_len = read_u2(r);
    attr->info.line_number_table_attribute->line_number_table_length =
      line_number_table_len;

    // malloc das linhas
    attr->info.line_number_table_attribute->line_number_table = 
      (line_number_table_line*)calloc(line_number_table_len, 
          sizeof(line_number_table_line));

    for (u2 j = 0; j < line_number_table_len; j++) {
      line_number_table_line *entry = &attr->info.
        line_number_table_attribute->line_number_table[j];
    
      entry->start_pc = read_u2(r);
      entry->line_number = read_u2(r);
    }
    return;
  }

  if (strcmp(attr_name, "LocalVariableTable") == 0) {
    return read_LocalVariableTable(r, attr);
  }

  // Ignorando silenciosamente atributos não reconhecidos
  for (u4 i = 0; i < attr->attribute_length; i++) {
    read_u1(r);
  }
}

void read_attributes(Reader* r, ClassFile* cf,
    u2 attributes_count, attribute_info* attributes)
{
  for (u2 i = 0; i < attributes_count; i++) {
    attributes[i].attribute_name_index = read_u2(r);
    attributes[i].attribute_length = read_u4(r);

    read_attribute_info(r, cf, &attributes[i]);
  }
}

void free_attributes(ClassFile* cf, attribute_info* attributes, 
    u2 attributes_count) {
  if (!attributes) return;

  for (int i = 0; i < attributes_count; i++) {
    const char* name = cp_get_utf8(cf->constant_pool, 
        attributes[i].attribute_name_index);

    if (strcmp(name, "Code") == 0) {
      if (attributes[i].info.code_attribute) {
        free(attributes[i].info.code_attribute->code);
        free(attributes[i].info.code_attribute->exception_table);

        if (attributes[i].info.code_attribute->attributes) {
            free_attributes(cf, 
                            attributes[i].info
                              .code_attribute->attributes, 
                            
                            attributes[i].info
                              .code_attribute->attributes_count);
        }

        free(attributes[i].info.code_attribute);
      }
    }

    if (strcmp(name, "Exceptions") == 0) {
      Exceptions_attribute* exc = attributes[i].info.exceptions_attribute;
      if (exc) {
        free(exc->exception_index_table);
        free(exc);
      }
    }

    if (strcmp(name, "LineNumberTable") == 0) {
      free(attributes[i].info.
          line_number_table_attribute->line_number_table);
      free(attributes[i].info.line_number_table_attribute);
    }

    if (strcmp(name, "LocalVariableTable") == 0) {
      free(attributes[i].info.
          local_variable_table_attribute->local_variable_table);
      free(attributes[i].info.local_variable_table_attribute);
    }
  }

  free(attributes);
}
