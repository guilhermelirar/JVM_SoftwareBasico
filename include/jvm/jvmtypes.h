#ifndef JVM_INCLUDED
#define JVM_INCLUDED

#include <stdbool.h>
#include "common/classfile.h"

#define JVM_STACK_SIZE 1000
#define JVM_HEAP_CAPACITY 100
#define JVM_MAX_CLASSES 100
#define JVM_STRING_TABLE_SIZE 64

#define JAVA_SYSTEM_OUT_IDX 0xFFFF

typedef struct LoadedClass LoadedClass;

/**
 * @brief estrutura que representa o método 
 * em execução de um frame, que encapsula 
 * a referência à classe a que pertence (constant-pool), 
 * flags de acesso, tabela de exceção e ponteiro para o 
 * bytecode, o que facilita a execução do frame 
 */
typedef struct {
  LoadedClass* holder_class; /*< Ponteiro para classe carregada que contém */
  const char* name; /*< Nome do método */
  const char* descriptor;
  u4 args_size; /*< Número de slots para os argumentos do método */

  // Ponteiros para estruturas da ClassFile
  method_info* info;
  Code_attribute* code_attr;
} RuntimeMethod;

/*@brief estrutura que representa um Field resolvido por 
 * meio de uma constant pool em tempo de execução, 
 * com acesso direto às informações.
 * */
typedef struct {
  LoadedClass* holder_class; /*< Classe que possui o field */
  u2 access_flags;
  const char* name;
  const char* descriptor;
  u2 index;                  /*< Índice em LoadedClass.static_fields 
                                 ou Object.fields */
  u2 attributes_count;       /**< Quantidade de atributos */
  attribute_info *attributes;
} RuntimeField;

typedef enum {
    CP_UNRESOLVED = 0,
    CP_RESOLVED_CLASS,
    JAVA_LANG_SYSTEM,
    CP_RESOLVED_FIELD,
    CP_RESOLVED_METHOD
} RuntimeCPTag;

/**
 * @brief Estrutura que representa uma entrada resolvida da constant pool 
 * Para melhorar o funcionamento desta JVM, referências á classe, interface, 
 * método e campos são resolvidas (valores finais a partir dos índices), 
 * com índices indiretos á constant pool sendo substituídos por valores finais
 * */
typedef struct {
  RuntimeCPTag tag;
  union {
    LoadedClass* clazz;
    RuntimeMethod method;
    RuntimeField field;
  } info;
} Resolved_cp_info;

/**
 * @brief Estrutura que representa um Frame da JVM
 */
typedef struct {
  u1* pc;                 /**< PC da instrução atual, ponteiro para bytecode */
  u4 *locals;             /**< Vetor de variáveis locais */
  
  u4 *operand_stack;      /**< pilha de operandos */
  int stack_ptr;          /**< índice para topo da pilha */

  RuntimeMethod method;           /**< Método atual */
} Frame;

/**
 @brief Estrutura que representa uma Thread da JVM 
 */
typedef struct {
  Frame* frames[JVM_STACK_SIZE]; /**< Pilha de frames */ 
  int frame_ptr;  /**<Índice para topo da pilha de frames */
} JVM_Thread;

/**
 * @brief distingue instancia de classse 
 * de objeto array
 */
typedef enum {
    OBJ_INSTANCE,
    OBJ_ARRAY
} ObjectType;

/**
 * @brief Representação de um array de tipos primitivos.
 */
typedef struct {
  u1 type;   /**< Código do tipo primitivo (T-Type)  */
  u1 dimensions; /**< Dimensões do array */
  u4 length; /**< Quantidade de elementos no array. */
  u4 *data;   /**< Ponteiro para os dados brutos */
} Array;

/**
 * @brief representação de um Objeto nesta JVM 
 */
typedef struct {
  ObjectType type;
  LoadedClass* clazz; /**< Classe desta instância */
  union {
    Array arr;
    u4 *fields;     /**< Conteúdo dos campos da instância */
  } content;
} Object;

/**
 * @brief Estrutura onde instâncias são armazenadas nesta JVM
 */
typedef struct {
  Object *entries; /**< Array de objetos */
  u4 count;  /**< Número de itens armazenados */
  u4 capacity; /**< Capacidade máxima de referências suportada */
} ObjectTable;

/**
 * @brief Estrutura que representa uma tabela das classes carregadas
 * e seus campos estáticos
 */
struct LoadedClass {
  const char* name;     /*> Nome da classe para fácil acesso */
  ClassFile* cf;        /*> ClassFile parseado */
  u4* static_fields;    /*> Campos estáticos (fields com ACC_STATIC) */
  u4 static_fields_size; /*> Número de slots*/
  bool is_initialized;  /*> Indica se classe foi inicializada 
                          (campos estáticos inicializados )*/
  u4 instance_size;     /*> Número de slots de fields de instância */
  Resolved_cp_info* cp;
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
