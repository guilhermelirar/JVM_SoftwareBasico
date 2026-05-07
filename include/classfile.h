#ifndef CLASSFILE
#define CLASSFILE

#include "stdint.h"
#include <inttypes.h>

#define MAGIC 0xCAFEBABE

// constant pool info
#define CONSTANT_Class 7
#define CONSTANT_Fieldref 9
#define CONSTANT_Methodref 10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String 8
#define CONSTANT_Integer 3
#define CONSTANT_Float 4
#define CONSTANT_Long 5
#define CONSTANT_Double 6
#define CONSTANT_NameAndType 12
#define CONSTANT_Utf8 1
#define CONSTANT_MethodHandle 15
#define CONSTANT_MethodType 16
#define CONSTANT_InvokeDynamic 18

// access_flags
#define ACC_PUBLIC 0x0001
#define ACC_PRIVATE 0x0002
#define ACC_PROTECTED 0x0004
#define ACC_STATIC 0x0008
#define ACC_FINAL 0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_SUPER 0x0020
#define ACC_VOLATILE 0x0040
#define ACC_TRANSIENT 0x0080
#define ACC_INTERFACE 0x0200
#define ACC_ABSTRACT 0x0400
#define ACC_SYNTHETIC 0x1000
#define ACC_ANNOTATION 0x2000
#define ACC_ENUM 0x4000

typedef uint8_t  u1;  
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

// Forward declarations necessárias
typedef struct attribute_info attribute_info;
typedef struct Code_attribute Code_attribute;
typedef struct Exceptions_attribute Exceptions_attribute;

// Access_flags permitidas para cada contexto
typedef enum {
  ACCESS_CLASS,
  ACCESS_FIELD,
  ACCESS_METHOD
} access_context_t;

typedef struct {
  u2 name_index; 
} CONSTANT_Class_info;

typedef struct {
  u2 class_index;
  u2 name_and_type_index;
} CONSTANT_Fieldref_info;

typedef CONSTANT_Fieldref_info CONSTANT_Methodref_info;
typedef CONSTANT_Fieldref_info CONSTANT_InterfaceMethodref_info;

typedef struct {
  u2 name_index;
  u2 descriptor_index;
} CONSTANT_NameAndType_info;

typedef struct {
  u2 length;
  u1 *bytes;
} CONSTANT_Utf8_info;

typedef struct {
  u2 string_index;
} CONSTANT_String_info;

typedef struct {
  u4 bytes;
} CONSTANT_Integer_info;

typedef CONSTANT_Integer_info CONSTANT_Float_info;

typedef struct {
  u4 h_bytes;
  u4 l_bytes;
} CONSTANT_Long_info;

typedef CONSTANT_Long_info CONSTANT_Double_info;

typedef struct {
  u1 tag;
  union {
    CONSTANT_Class_info class_info;
    CONSTANT_Fieldref_info fieldref_info;
    CONSTANT_Methodref_info methodref_info;
    CONSTANT_InterfaceMethodref_info interface_methodref_info;
    CONSTANT_Utf8_info utf8_info;
    CONSTANT_String_info string_info;
    CONSTANT_NameAndType_info name_and_type_info;
    CONSTANT_Integer_info int_info;
    CONSTANT_Float_info float_info;
    CONSTANT_Double_info double_info;
    CONSTANT_Long_info long_info;
  } info;
} cp_info;

struct attribute_info {
  u2 attribute_name_index;
  u4 attribute_length;
  union {
    u2 constantvalue_index;
    Code_attribute* code_attribute;
    Exceptions_attribute* exceptions_attribute;
    u2 sourcefile_index;
  } info;
};

// CODE attribute
typedef struct {
  u2 start_pc;
  u2 end_pc;
  u2 handler_pc;
  u2 catch_type; 
} exception_info;


struct Code_attribute {
  u2 max_stack;
  u2 max_locals;
  u4 code_length;
  u1* code;
  u2 exception_table_length;
  exception_info* exception_table;
  u2 attributes_count;
  attribute_info* attributes;
};

struct Exceptions_attribute {
  u2 number_of_exceptions;
  u2 *exception_index_table;
};

typedef struct {
  u2 access_flags;
  u2 name_index;
  u2 descriptor_index;
  u2 attributes_count;
  attribute_info *attributes;
} field_info;

typedef struct {
  u2 access_flags;
  u2 name_index;
  u2 descriptor_index;
  u2 attributes_count;
  attribute_info* attributes;
} method_info;

typedef struct {
  u4 magic;
  u2 minor_version;
  u2 major_version;
  u2 constant_pool_count;
  cp_info *constant_pool;
  u2 access_flags;
  u2 this_class;
  u2 super_class;
  u2 interfaces_count;
  u2 *interfaces;
  u2 fields_count;
  field_info *fields;
  u2 methods_count;
  method_info* methods;
  u2 attributes_count;
  attribute_info* attributes;
} ClassFile;





// Obter strings
const char* cp_class_name(const ClassFile *cf, u2 class_index);
const char* cp_nameandtype_name(const ClassFile *cf, u2 nt_index);
const char* cp_get_utf8(const ClassFile *cf, u2 utf8_index);

#endif
