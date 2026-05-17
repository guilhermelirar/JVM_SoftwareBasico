#ifndef JVM_INCLUDED
#define JVM_INCLUDED

#include "common/classfile.h"

#define JVM_STACK_SIZE 1000
#define JVM_HEAP_CAPACITY 100
#define JVM_MAX_CLASSES 100

/**
 * @brief Estrutura que representa um Frame da JVM
 */
typedef struct {
  u4 pc;                   /**< Índice da instrução atual */
  u4 *locals;              /**< Vetor de variáveis locais */
  
  u4 *operand_stack;       /**< pilha de operandos */
  int stack_ptr;           /**< índice para topo da pilha */

  u1* code;                /**< array do bytecide */
  cp_info* constant_pool;  /**< ponteiro para pool de constantes */
} Frame;

/**
 @brief Estrutura que representa uma Thread da JVM 
 */
typedef struct {
  Frame* frames[JVM_STACK_SIZE]; /**< Pilha de frames */ 
  int frame_ptr;  /**<Índice para topo da pilha de frames */
} JVM_Thread;

/**
 * @brief representação de um Objeto nesta JVM 
 */
typedef struct {
  u2 class_index; /**< Índice para classe desta instância */
  u4 *fields;     /**< Conteúdo dos campos da instância */
} Object;

/**
 * @brief Representação de um array de tipos primitivos.
 */
typedef struct {
  u1 type;   /**< Código do tipo primitivo (T-Type)  */
  u4 length; /**< Quantidade de elementos no array. */
  u4 *data;   /**< Ponteiro para os dados brutos */
} JVM_Array;

/**
 * @brief Estrutura onde instâncias são armazenadas nesta JVM
 */
typedef struct {
  void* entries[JVM_HEAP_CAPACITY]; /**< Array de referências para 
                                      os objetos ou array */
  u4 count;  /**< Número de itens armazenados */
  u4 capacity; /**< Capacidade máxima de referências suportada */
} Heap;

typedef struct {
  ClassFile* cf;
  u4* static_fields;
} LoadedClass;

/**
 * @brief Estrutura que representa contexto de execução da JVM 
 */
typedef struct {
  LoadedClass method_area[JVM_MAX_CLASSES]; 
  /**< Informação das classes e métodos */
  int classes_count; /**< Número de .class carregados */

  Heap heap;    /**< Área de memória */
  JVM_Thread t; /**< Thread a ser executada (multithread não suportada) */
} JVM_Context;

#endif
