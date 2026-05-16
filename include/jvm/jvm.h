#ifndef JVM_INCLUDED
#define JVM_INCLUDED

#include "common/classfile.h"

#define JVM_STACK_SIZE 1000

typedef struct {
  u4 pc; 
  u4 *locals;
  
  u4 *operand_stack;
  int stack_ptr;

  cp_info* constant_pool;
} Frame;

typedef struct {
  Frame frames[JVM_STACK_SIZE];
  int frame_ptr;
} JVM_Thread;

typedef struct {
  u2 class_index;
  u4 *fields;
} Object;

typedef struct {
  u1 type;            
  u4 length; 
  u4 *data;
} JVM_Array;

typedef struct {
  void** entries; // array de ponteiros genericos
  u4 count;
  u4 capacity;
} Heap;

typedef struct {
  ClassFile** method_area;
  int classes_count;

  Heap heap;
  JVM_Thread t;
} JVM_Context;

#endif
