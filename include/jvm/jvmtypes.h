#ifndef JVM_INCLUDED
#define JVM_INCLUDED

#include <stdbool.h>
#include "common/classfile.h"

#define JVM_STACK_SIZE 1000
#define JVM_HEAP_CAPACITY 100
#define JVM_MAX_CLASSES 100
#define JVM_STRING_TABLE_SIZE 64

typedef struct LoadedClass LoadedClass;

/**
 * @brief estrutura que representa o método 
 * em execução de um frame, que encapsula 
 * a referência à classe a que pertence (constant-pool), 
 * flags de acesso, tabela de exceção e ponteiro para o 
 * bytecode, o que facilita a execução do frame 
 */
typedef struct {
  LoadedClass* holder_class;
  u2 access_flags;

  const char* name;
  const char* descriptor;

  u2 max_stack;
  u2 max_locals;
  u4 code_length;
  u1* code;

  u2 exception_table_length;
  exception_info* exception_info;
} Method;

/**
 * @brief Estrutura que representa um Frame da JVM
 */
typedef struct {
  u1* pc;                   /**< PC da instrução atual */
  u4 *locals;              /**< Vetor de variáveis locais */
  
  u4 *operand_stack;       /**< pilha de operandos */
  int stack_ptr;           /**< índice para topo da pilha */

  Method method;           /**< Método atual */
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
  u1 dimensions; /**< Dimensões do array */
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
} ObjectTable;

/**
 * @brief Estrutura que representa uma tabela das classes carregadas
 * e seus campos estáticos
 */
struct LoadedClass {
  ClassFile* cf;        /*> ClassFile parseado */
  u4* static_fields;    /*> Campos estáticos (fields com ACC_STATIC) */
  bool is_initialized;  /*> Indica se classe foi inicializada 
                          (campos estáticos inicializados )*/
  LoadedClass* super;   /*> Ponteiro para superclasse na área de métodos */
};

/**
 * @brief Estrutura que representa uma tabela
 * com as strings constantes do programa
 * Contém array de ponteiros para strings que existem em 
 * constant_pool de alguma ClassFile
 */
typedef struct {
  char* strings[JVM_STRING_TABLE_SIZE]; /**< Array de ptr para string */
  u4 count;
  u4 capacity;
} StringTable;

/**
 * @brief Estrutura que representa contexto de execução da JVM 
 */
typedef struct {
  char base_dir[512]; 

  LoadedClass method_area[JVM_MAX_CLASSES]; 
  /**< Informação das classes e métodos */
  int classes_count; /**< Número de .class carregados */

  ObjectTable objects;    /**< Área de memória */
  StringTable strings; /**< Área de strings constantes */
  JVM_Thread t; /**< Thread a ser executada (multithread não suportada) */
} JVM_Context;

#endif
