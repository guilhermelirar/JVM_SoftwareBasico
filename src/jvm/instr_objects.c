#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include "jvm/interpreter.h"
#include "jvm/jvm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void extend_object_table(JVM_Context* ctx)
{
  u4 new_capacity = ctx->objects.capacity * 2;
  if (new_capacity < 100) new_capacity = 100;

  Object* new_entries = (Object*)realloc(ctx->objects.entries, 
                                         new_capacity * sizeof(Object));

  if (new_entries == NULL)
  {
    fprintf(stderr, "[FATAL] OutOfMemoryError" 
        " (%d objects and could not get more memory)\n", ctx->objects.count);
    terminateJVM(ctx);
    exit(1);
  }

  ctx->objects.entries = new_entries;

  // zerando memória dos objetos novos
  u4 objetos_novos = new_capacity - ctx->objects.capacity;
  memset(&ctx->objects.entries[ctx->objects.capacity], 
      0, objetos_novos * sizeof(Object));

  ctx->objects.capacity = new_capacity;
}

u4 new_object(JVM_Context* ctx, LoadedClass* clazz)
{
  // requisita mais memória ao sistema operacional
  if (ctx->objects.count == ctx->objects.capacity)
  {
    extend_object_table(ctx);
  }

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
    fatal_error(ctx, "[FATAL] Illegal Instantiation of : %s", clazz->name);
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
    return throw_native(ctx, "java/lang/NegativeArraySizeException");

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
    return throw_native(ctx, "java/lang/NullPointerException");

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

static u4 new_ref_array(JVM_Context* ctx, int32_t count, u1 dimensions,  
      LoadedClass* ref_class)
{
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
  return new_array(ctx, clazz, type, count, dimensions);
}

void handle_anewarray(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);

  LoadedClass* ref_class = resolve_class(ctx, cp_idx);

  int32_t count = pop_operand(frame);
  u4 ref = 0;
  if (ref_class->name[0] == '[')
    ref =  new_ref_array(ctx, count, 1, ref_class);
  else 
    ref = new_array(ctx, ref_class, 0, count, 1); 

  push_operand(frame, ref);
}

void handle_multianewarray(JVM_Context *ctx, u1 opc)
{
  (void)opc;
  Frame* frame = current_frame(ctx);
  u2 cp_idx = fetch_u2(&frame->pc);
  u1 dimensions = *(frame->pc++);

  u4* dim_count = (u4*)calloc(dimensions, sizeof(u4));
  for(u1 i = dimensions; i > 0; i--)
  {
    dim_count[i-1] = (int32_t)pop_operand(frame);
  }

  LoadedClass* clazz = resolve_class(ctx, cp_idx);
  u4 arrayref = 0;
  if (clazz->name[0] == '[')
    arrayref =  new_ref_array(ctx, dim_count[0], 1, clazz);
  else 
    arrayref = new_array(ctx, clazz, char_to_ttype(clazz->name[0]), 
        dim_count[0], dimensions);  
  
  u4 parent_ref = arrayref;
  for(u1 i = 1; i < dimensions; i++)
  {
    Array* parent = &ctx->objects.entries[parent_ref].content.arr;
    for (u4 j = 0; j < parent->length; j++)
    {
      u4 child_ref = new_array(ctx, clazz, 
          0, dim_count[i], dimensions - i);

      parent->data[j] = child_ref; 
      parent_ref = child_ref;
    }
  }

  free(dim_count);
  push_operand(frame, arrayref);
}


void handle_instanceof(JVM_Context* ctx, u1 opc)
{
  Frame* frame = current_frame(ctx);
  (void)opc;

  u2 cp_idx = fetch_u2(&frame->pc);
  LoadedClass* clazz = resolve_class(ctx, cp_idx);

  u4 objectref = pop_operand(frame);
  if (objectref == 0)
    return push_operand(frame, 0);

  Object* obj = &ctx->objects.entries[objectref];
  if (clazz->name[0] == '[') // array
  {
    if (obj->type == OBJ_ARRAY && 
        obj->content.arr.type == (clazz->name)[strlen(clazz->name)-1])
      return push_operand(frame, 1);

    return push_operand(frame, 1);
  }

  u4 res = extends(obj->clazz, clazz) ? 1 : 0;
  push_operand(frame, res);
}
