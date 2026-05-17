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

/** 
 * @brief Enum para indicar como interpretar um campo access_flags
 */
typedef enum {
  ACCESS_CLASS,   /**< interpretar como flags de classe */
  ACCESS_FIELD,   /**< interpretar como flags de field */
  ACCESS_METHOD   /**< interpretar como flags de método */
} access_context_t;

/** 
 * @brief Estrutura que armazena campos específicos de 
 * uma CONSTANT_Class em cp_info
 */
typedef struct {
  u2 name_index; /**< índice para uma CONSTANT_Utf8 em constant_pool */ 
} CONSTANT_Class_info;

/**
 * @brief Estrutura que armazena campos específicos de CONSTANT_Fieldref
 * dentro de cp_info.
 */
typedef struct {
  u2 class_index;         /**< índice para CONSTANT_Class em constant_pool */  
  u2 name_and_type_index; /**< índice para CONSTANT_NameAndType */
} CONSTANT_Fieldref_info;

typedef CONSTANT_Fieldref_info CONSTANT_Methodref_info;
typedef CONSTANT_Fieldref_info CONSTANT_InterfaceMethodref_info;

/**
 * @brief Estrutura que armazena campos específicos de CONSTANT_NameAndType
 * em uma cp_info
 */
typedef struct {
  u2 name_index;        /**< Índice para uma CONSTANT_Utf8 */
  u2 descriptor_index;  /**< Índice para um CONSTANT_Utf8*/
} CONSTANT_NameAndType_info;

/**
 * @brief Estrutura que armazena campos específicos de CONSTANT_Utf8
 */
typedef struct {
  u2 length;  /**< Tamanho da string utf-8 em bytes */
  u1 *bytes;  /**< [length + 1] guarda os bytes da string e terminador "\0" */
} CONSTANT_Utf8_info;

/**
 * @brief estrutura com campo específico de CONSTANT_Utf8
 */
typedef struct {
  u2 string_index; /**< Índice para CONSTANT_Utf8 */
} CONSTANT_String_info;

/**
 * @brief estrutura com campo específicos de CONSTANT_Integer
 */
typedef struct {
  u4 bytes; /**< Representa 32 bits do inteiro com sinal */
} CONSTANT_Integer_info;

typedef CONSTANT_Integer_info CONSTANT_Float_info;

/**
 * @brief estrutura com campos específicos de CONSTANT_Long
 */
typedef struct {
  u4 h_bytes; /**< 32 bits mais significativos do long sinalizado */
  u4 l_bytes; /**< 32 bits menos significativos do long sinalizado */
} CONSTANT_Long_info;

typedef CONSTANT_Long_info CONSTANT_Double_info;

/**
 * @brief estrutura que representa o pool de constantes da JVM
 */
typedef struct {
  u1 tag;     /**< Identificador de tipo da constante */
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
  } info;   /**< Armazena campos específicos do tipo definido em tag */
} cp_info;

// ATRIBUTOS
// Forward declarations necessárias
typedef struct attribute_info attribute_info;
typedef struct Code_attribute Code_attribute;
typedef struct Exceptions_attribute Exceptions_attribute;
typedef struct LineNumberTable_attribute LineNumberTable_attribute;
typedef struct LocalVariableTable_attribute LocalVariableTable_attribute;

/**
 * @brief Estrutura que representa um atributo. 
 * São usados nesta implementação os atributos: 
 * Code, ConstantValue, Exceptions, SourceFile, LineNumberTable,
 * LocalVariableTable
 */
struct attribute_info {
  u2 attribute_name_index;  /**< Índice para uma utf-8 no em constant_pool */
  u4 attribute_length; /**< Tamanho em bytes (excluindo 6 primeiros bytes) */
  union {
    u2 constantvalue_index;
    Code_attribute* code_attribute;
    Exceptions_attribute* exceptions_attribute;
    u2 sourcefile_index;
    LineNumberTable_attribute* line_number_table_attribute;
    LocalVariableTable_attribute* local_variable_table_attribute;
  } info; /**< Campos específicos de acordo com o tipo */
};

// CODE attribute
/**
 * @brief Estrutura que armazena campos de informação de exceções 
 * em um atributo Code.
 */
typedef struct {
  u2 start_pc; /**<  */
  u2 end_pc; /**<  */
  u2 handler_pc; /**< índice para code indicando bytecode inicial do manipulador */
  u2 catch_type; /**< se não nulo, índice para CONSTANT_Class 
                   (classe de exceções a ser capturada). Se nulo, cláusula finally de um 
                   comando try */ 
} exception_info;

/**
 * @brief Estrutura com campos de um atributo Code
 */
struct Code_attribute {
  u2 max_stack; /**< Tamanho máximo da pilha de operandos durante execução */
  u2 max_locals;  /**< Número de variáveis locais */
  u4 code_length; /**< Número de bytes do array code */
  u1* code; /**< bytecodes JVM que implementam o código */
  u2 exception_table_length; /**< número de entradas na tabela exception table */
  exception_info* exception_table; /**< manipuladores de exceções */
  u2 attributes_count;
  attribute_info* attributes;
};

/**
 * @brief Estrutura para atributo Exceptions.
 * Indica quais classes de exceção um método pode lançar 
 */
struct Exceptions_attribute {
  u2 number_of_exceptions;
  u2 *exception_index_table; /**< Índice válido para CONSTANT_Class */
};

/**
 * @brief Estrutura auxiliar de LineNumberTable_attribute.
 */
typedef struct {
  u2 start_pc;    /**< PC do início da linha */
  u2 line_number; /**< Linha do código fonte .java que começa com start_pc */
} line_number_table_line;

/**
 * @brief Estrutura para atributo LineNumberTable.
 * Indica relação entre índice do bytecode e linha do código fonte.
 */
struct LineNumberTable_attribute {
  u2 line_number_table_length;
  line_number_table_line *line_number_table;
};

/**
 * @brief Estrutura auxiliar LocalVariableTable.
 * Entrada do array local_variable_table. 
 */
typedef struct {
  u2 start_pc; /**< PC de início em que a variável mantém o valor */
  u2 length;  /**< Tamanho do intervalo em que a variável mantém o valor */
  u2 name_index; /**< Índice para um CONSTANT_Utf8 com o nome da variável */ 
  u2 descriptor_index; /**< Índice para um CONSTANT_Utf8 com um descritor
                   de campo válido para o tipo da variável */
  u2 index; /**< Índice para a variável no array de variáveis locais do frame */
} LocalVariableTable_entry;

/**
 * @brief Estrutura para implementação do atributo LocalVariableTable
 */
struct LocalVariableTable_attribute {
  u2 local_variable_table_length;
  LocalVariableTable_entry* local_variable_table;
};

/**
 * @brief Estrutura que representa um campo (field) da classe.
 * * Contém informações sobre variáveis de instância ou variáveis estáticas.
 * Não inclui variáveis locais de métodos.
 */
typedef struct {
  u2 access_flags;      /**< Máscara de bits para flags de acesso */
  u2 name_index;        /**< Índice para CONSTANT_Utf8 com o nome do campo */
  u2 descriptor_index;  /**< Índice para CONSTANT_Utf8 com o tipo do campo */
  u2 attributes_count;  /**< Quantidade de atributos */
  attribute_info *attributes; /**< Array de estruturas de atributos do campo */
} field_info;

/**
 * @brief Estrutura que contém a definição de um método da classe.
 * * Armazena as permissões de acesso, nome, assinatura e os atributos associados, 
 * como o atributo "Code", que contém o bytecode real para execução.
 */
typedef struct {
  u2 access_flags; /**< Máscara de bits para permissões de acesso */
  u2 name_index; /**< Índice para CONSTANT_Utf8 contendo o nome do método */
  u2 descriptor_index; /**< Índice para CONSTANT_Utf8 com a assinatura de tipos */
  u2 attributes_count; /**< Quantidade de atributos do método */
  attribute_info* attributes; /**< Tabela de atributos */
} method_info;

/**
 * @brief Estrutura que representa um arquivo .class. 
 * * Usado para leitura e exibição, e na interpretação 
 * (armazenado na área de métodos)
 */
typedef struct {
  u4 magic;   /**< 0xCAFEBABE no início de um .class válido */
  u2 minor_version; /**< Indicador da versão do formato M.(m) */
  u2 major_version; /**< Indicador da versão do formato (M).m */
  u2 constant_pool_count; /**< Número de entradas na constant_pool + 1 */
  cp_info *constant_pool; /**< Tabela de estruturas, com índice 0 vazio */
  u2 access_flags; /**< Máscara de bits para permissão de acesso da classe */
  u2 this_class; /**< Índice para CONSTANT_Class representando classe atual */
  u2 super_class;/**< Índice para CONSTANT_Class representando classe mãe */
  u2 interfaces_count;/**< Número de interfaces implementadas pela classe */
  u2 *interfaces;/**< Array de índices de CONSTANT_Class representando interfaces*/
  u2 fields_count;/**< Número de varíáveis de classe ou instância*/
  field_info *fields;/**< Tabela com descrição de campo de classe e interface */
  u2 methods_count;/**< Número de métodos (não herdados) da classe*/
  method_info* methods;/**< Tabela com descrição completa de um método */
  u2 attributes_count;/**< Número de atributos desta classe */
  attribute_info* attributes;/**< Tabela de atributos desta classe*/
} ClassFile;

// Obter strings

/**
 * @brief Resolve e retorna uma utf_8 a partir do índice 
 * para uma CONSTANT_Class na constant_pool.
 * @param cf Ponteiro para a classe do constant_pool a ser usado na busca.
 * @param class_index índice válido para CONSTANT_Class na constant_pool.
 * @return Ponteiro para string C representando utf-8 de nome da classe. 
 *  Ou string "<invalid class>" se índice não aponta para CONSTANT_Class
 */
const char* cp_class_name(const ClassFile *cf, u2 class_index);

/**
 * @brief Resolve e retorna uma utf_8 a partir do índice 
 * para uma CONSTANT_NameAndType na constant_pool.
 * @param cf Ponteiro para a classe do constant_pool a ser usado na busca.
 * @param nt_index índice válido para CONSTANT_NameAndType na constant_pool.
 * @return Ponteiro para string C representando utf-8 do name_index 
 * do name_and_type_info. Ou string "<invalid nt>" se índice fornecido não 
 * aponta para CONSTANT_NameAndType
 */
const char* cp_nameandtype_name(const ClassFile *cf, u2 nt_index);

/**
 * @brief Resolve e retorna uma utf_8 a partir do índice 
 * para uma CONSTANT_Utf8 na constant_pool.
 * @param cf Ponteiro para a classe do constant_pool a ser usado na busca.
 * @param utf_index índice válido para CONSTANT_Utf8 na constant_pool.
 * @return Ponteiro para string C representando utf-8 em bytes. Ou
 * string "<invalid UTF-8>" caso índice não aponte para CONSTANT_Utf8
 */
const char* cp_get_utf8(const ClassFile *cf, u2 utf8_index);

#endif
