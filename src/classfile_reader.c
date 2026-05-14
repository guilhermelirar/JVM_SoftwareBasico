#include "common/classfile_reader.h"
#include "common/reader.h"
#include "common/classfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_constant_pool(Reader *file, ClassFile *cf) {
  // assume: cursor logo após constant_pool_count
  cp_info* entry;
  for (int i = 1; i < cf->constant_pool_count; i++) {
    entry = &cf->constant_pool[i];
    entry->tag = read_u1(file);
    
    switch (entry->tag) {
      case CONSTANT_Class:
        entry->info.class_info.name_index = read_u2(file);  
        break;
        
      case CONSTANT_Fieldref:
        entry->info.fieldref_info.class_index = read_u2(file);
        entry->info.fieldref_info.name_and_type_index = read_u2(file);
        break;

      case CONSTANT_Methodref:
        entry->info.methodref_info.class_index = read_u2(file);
        entry->info.methodref_info.name_and_type_index = read_u2(file);
        break;
      
      case CONSTANT_InterfaceMethodref:
        entry->info.interface_methodref_info.class_index = read_u2(file);
        entry->info.interface_methodref_info.name_and_type_index = read_u2(file);
        break;
      
      case CONSTANT_NameAndType:
        entry->info.name_and_type_info.name_index = read_u2(file);
        entry->info.name_and_type_info.descriptor_index = read_u2(file);
        break;

      case CONSTANT_Utf8:
        entry->info.utf8_info.length = read_u2(file);
        entry->info.utf8_info.bytes = read_utf8(file, 
            entry->info.utf8_info.length);
        if (entry->info.utf8_info.bytes == NULL) return ERR_CONSTANT_POOL_READ;
        break; 
     
      case CONSTANT_String:
        entry->info.string_info.string_index = read_u2(file);
        break;

      case CONSTANT_Integer:
        entry->info.int_info.bytes = read_u4(file);
        break;

      case CONSTANT_Float:
        entry->info.float_info.bytes = read_u4(file);
        break;

      case CONSTANT_Long:
        entry->info.long_info.h_bytes = read_u4(file);
        entry->info.long_info.l_bytes = read_u4(file);
        i++;
        break;

      case CONSTANT_Double: 
        entry->info.double_info.h_bytes = read_u4(file);
        entry->info.double_info.l_bytes = read_u4(file);
        i++;
        break;

      default:
        fprintf(stderr, "Error: constant tag \"%d\" not recognized\n", 
            entry->tag);
        return ERR_JAVA_INVALID_TAG;
        break; 
    }
  }

  return SUCCESS;
}

u1* read_utf8(Reader *r, u2 length) {
  u1 *bytes = (u1*) malloc(length + 1); // malloc #3
    
  if (bytes == NULL) {
    return NULL;
  }

  for (u2 i = 0; i < length; i++) {
    bytes[i] = read_u1(r);
  }

  bytes[length] = '\0';

  return bytes;
}

int read_interfaces(Reader* file, ClassFile* cf) {
  for (int i = 0; i < cf->interfaces_count; i++) {
    u2 idx = read_u2(file);

    // Não é indice válido para constant_pool
    if (idx < 1 || idx >= cf->constant_pool_count || 
        cf->constant_pool[idx].tag != CONSTANT_Class) 
      return -1;
    cf->interfaces[i] = idx;
  }
  return SUCCESS;
}



void read_fields(Reader *file, ClassFile *cf) {
  field_info* field;
  for (int i = 0; i < cf->fields_count; i++) {
    field = &cf->fields[i];
    field->access_flags = read_u2(file);
    field->name_index = read_u2(file);
    field->descriptor_index = read_u2(file);
    field->attributes_count = read_u2(file);
    field->attributes = (attribute_info*)malloc(
        sizeof(attribute_info) * field->attributes_count); // malloc #6
    // TODO checar caso de alocação não funcionar
    read_attributes(file, cf, field->attributes_count, field->attributes);
  }
}

void read_methods(Reader *file, ClassFile *cf) {
  method_info* m = NULL;
  for (int i = 0; i < cf->methods_count; i++) {
    m = &cf->methods[i];
    m->access_flags = read_u2(file);
    m->name_index = read_u2(file); // TODO tratar out of range ou nao
    m->descriptor_index = read_u2(file);
    m->attributes_count = read_u2(file);
    m->attributes = (attribute_info*)malloc(
        sizeof(attribute_info) * m->attributes_count); // malloc #8
    read_attributes(file, cf, m->attributes_count, m->attributes);
  }
}

ClassFile *read_class(Reader *reader) {
  ClassFile* cf = NULL;

  if (reader == NULL) {
    goto cleanup;
  }
  
  cf = (ClassFile*)calloc(1, sizeof(ClassFile)); // alloc #1
  
  cf->magic = read_u4(reader);
  
  if (cf->magic != (u4)MAGIC) {
    printf("File doesn't contain magic.\n"
        "  Expected: %X\n  Got: %X\n", MAGIC, cf->magic);
    goto cleanup;
  } 

  cf->minor_version = read_u2(reader);
  cf->major_version = read_u2(reader);
  cf->constant_pool_count = read_u2(reader);
 
  cf->constant_pool = (cp_info*)calloc(cf->constant_pool_count,
      sizeof(cp_info)); // malloc #2
  if (!cf->constant_pool) goto cleanup;

  if (read_constant_pool(reader, cf) != SUCCESS) goto cleanup; // malloc #3

  cf->access_flags = read_u2(reader);

  cf->this_class = read_u2(reader); // Idx para CONSTANT_Class em constant_pool
  cf->super_class = read_u2(reader); // ''

  // Interfaces
  cf->interfaces_count = read_u2(reader);
  cf->interfaces = (u2*)malloc(sizeof(u2) * cf->interfaces_count); // malloc #4
  if (!cf->interfaces) goto cleanup;
  read_interfaces(reader, cf);

  // Fields
  cf->fields_count = read_u2(reader);
  cf->fields = (field_info*)malloc(
      sizeof(field_info) *  cf->fields_count
      ); // malloc #5
  if (!cf->interfaces) goto cleanup;
  read_fields(reader, cf);

  // Methods
  cf->methods_count = read_u2(reader);
  cf->methods = (method_info*)malloc(
      sizeof(method_info) * cf->methods_count
      ); // malloc #7
  read_methods(reader, cf);


  // Attributes
  cf->attributes_count = read_u2(reader);
  cf->attributes = (attribute_info*)calloc(cf->attributes_count, 
      sizeof(attribute_info)); // calloc attributes
  read_attributes(reader, cf, cf->attributes_count, cf->attributes);

  return cf;

cleanup:
  if (cf) free_classfile(cf);
  return NULL;
}


void free_constant_pool(ClassFile *cf) {
  if (!cf->constant_pool) return; 
  cp_info *entry; 

  // Libera memória de strings
  for (int i = 1; i < cf->constant_pool_count; i++) {
    entry = &cf->constant_pool[i];
    if (entry && (entry->tag == CONSTANT_Utf8) 
        && entry->info.utf8_info.bytes) 
      free(entry->info.utf8_info.bytes); // free #3
  }
    
  free(cf->constant_pool); // free #2
  cf->constant_pool = NULL;
}



void free_methods(ClassFile* cf) {
  if (!cf->methods) return;
  for (int i = 0; i < cf->methods_count; i++) {
    free_attributes(cf, cf->methods[i].attributes,
                    cf->methods[i].attributes_count);
  }
  free(cf->methods); // free #5
  return;
}


void free_fields(ClassFile* cf) {
  if (!cf->fields) return;

  for (int i = 0; i < cf->fields_count; i++) {
    free_attributes(cf, cf->fields[i].attributes,
                    cf->fields[i].attributes_count);
  }

  free(cf->fields); // free #8
  return;
}

void free_classfile(ClassFile* cf) {
  if (cf == NULL) return;
  if (cf->interfaces) free(cf->interfaces);      // free #4
  if (cf->fields) free_fields(cf);               // ->free #5,6,7
  if (cf->methods) free_methods(cf);             // ->free #9,8,7

  if (cf->attributes) {
    free_attributes(cf, cf->attributes, cf->attributes_count);
  }

  if (cf->constant_pool) free_constant_pool(cf); // ->free #3,2
  free(cf); // free #1
}

