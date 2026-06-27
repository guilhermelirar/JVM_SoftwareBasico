#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u4 new_object(JVM_Context* ctx, LoadedClass* clazz)
{
  if (ctx->objects.count == ctx->objects.capacity)
    return 0; // NULL
  u4 idx = ctx->objects.count++;
  ctx->objects.entries[idx].type = OBJ_INSTANCE;
  ctx->objects.entries[idx].clazz = clazz; 
  ctx->objects.entries[idx].content.fields = 
    (u4*)calloc(clazz->instance_size, sizeof(u4));

  return idx;
}

static u4 new_array(JVM_Context *ctx, 
    LoadedClass* clazz, u1 type, int32_t count, u4 dimensions)
{

  u4 arr_size = count;
  if ((dimensions == 1) 
    && ((type == T_DOUBLE) || (type == T_LONG)))
    arr_size *= 2;
 
  u4 ref = ctx->objects.count++;
  Object* obj = &ctx->objects.entries[ref];
  obj->type = OBJ_ARRAY;
  obj->content.arr.type = 0;
  obj->clazz = clazz;
  obj->content.arr.dimensions = dimensions;
  obj->content.arr.length = count;
  obj->content.arr.data = (u4*)calloc(arr_size, sizeof(u4));

  return ref;
}

void handle_new(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u1* opc_pc = frame->pc - 1; 

  u2 cp_idx = fetch_u2(&frame->pc);
  LoadedClass* clazz = resolve_class(ctx, cp_idx);

  // instanciação de classe que não pode ser instanciada
  if (clazz->cf->access_flags & ACC_ABSTRACT ||
      clazz->cf->access_flags & ACC_INTERFACE)
  {
    // TODO exceção 
    fprintf(stderr, "Illegal instantiation");
    terminateJVM(ctx);
    exit(1);
  }

  // inicializa classe antes de executar
  if (clazz != NULL && !clazz->is_initialized)
  {
    initialize_class(ctx, clazz);
    frame->pc = opc_pc;
    return;
  }

  u4 objectref = new_object(ctx, clazz);

  push_operand(frame, objectref);
}

void handle_newarray(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u1 type = *(frame->pc++);
 
  // obtém número de elementos e inicializa número de slots
  int32_t count = (int32_t)pop_operand(frame);
  if (count < 0)
  {
    fprintf(stderr, "NegativeArraySizeException"); // TODO
    terminateJVM(ctx);
    exit(1);
  }

  // inicializando objeto
  u4 arrayref = new_array(ctx, NULL, type, count, 1);
  push_operand(frame, arrayref);
}


void handle_arraylength(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u4 arrayref = pop_operand(frame);

  if (arrayref == 0 || ctx->objects.entries[arrayref].type != OBJ_ARRAY)
  {
    // TODO throw
    fprintf(stderr, "NullPointerException");
    terminateJVM(ctx);
    exit(1);
  }

  push_operand(frame, ctx->objects.entries[arrayref].content.arr.length);
}

static u1 char_to_ttype(char c) {
  switch (c) {
    case 'Z': return T_BOOLEAN; 
    case 'C': return T_CHAR;  
    case 'F': return T_FLOAT; 
    case 'D': return T_DOUBLE;  
    case 'B': return T_BYTE;  
    case 'S': return T_SHORT;  
    case 'I': return T_INT; 
    case 'J': return T_LONG;
    default:  return 0; 
  }
}

static void new_ref_array(JVM_Context* ctx, LoadedClass* ref_class)
{
  Frame* frame = current_frame(ctx);

  u4 dimensions = 1;
  const char* name = ref_class->name;
  while (*name == '[')
  {
    dimensions++;
    name++;
  }
  
  LoadedClass* clazz = NULL;
  if (*name == 'L') {
    name++; 
    size_t len = strchr(name, ';') - name; 
    char* clean_name = malloc(len + 1);
    
    strncpy(clean_name, name, len);
    clean_name[len] = '\0';
    clazz = get_class(ctx, clean_name);
    free(clean_name);
  } 
 
  u1 type = char_to_ttype(*name);
  int32_t count = (int32_t)pop_operand(frame);

  u4 ref = new_array(ctx, clazz, type, count, dimensions);
  push_operand(frame, ref);
}

void handle_anewarray(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  LoadedClass* ref_class = resolve_class(ctx, cp_idx);

  if (ref_class->name[0] == '[')
  {
    return new_ref_array(ctx, ref_class);
  }

  u4 ref = ctx->objects.count++;
  Object* obj = &ctx->objects.entries[ref];
  obj->type = OBJ_ARRAY;
  obj->content.arr.type = 0;
  obj->clazz = ref_class;
  obj->content.arr.dimensions = 1;

  int32_t len = pop_operand(frame);
  obj->content.arr.length = len;
  obj->content.arr.data = (u4*)calloc(len, sizeof(u4));
}
