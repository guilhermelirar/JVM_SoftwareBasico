#ifndef LOADER
#define LOADER

#include "classfile.h"
#include "stdio.h"
#include "stdlib.h"
#include "reader.h"

#define SUCCESS 0
#define ERR_JAVA_INVALID_TAG -1 
#define ERR_CONSTANT_POOL_READ -2

// Função principal

/**
 * @brief Aloca memória para um ClassFile e faz o parsing de uma 
 * informação em bytes guardada num Reader.
 * @param r estrutura de leitura com bytes do .class
 * @return ponteiro para ClassFile válida, ou NULL em caso de arquivo inválido
 */
ClassFile* read_class(Reader *r);

// --- LIBERAÇÃO DE MEMÓRIA ---
/**
 * @brief Desaloca memória para ClassFile, chamando outras funções 
 * de liberação de campos.
 * @param classfile ponteiro para ClassFile a ser liberada
 */
void free_classfile(ClassFile* classfile);

/**
 * @brief Desaloca memória para pool de constantes da classe 
 * e todas as suas entradas
 * @param cf ponteiro para ClassFile da constant pool a ser liberada
 */
void free_constant_pool(ClassFile* cf);

/**
 * @brief Libera memória dos campos de uma classfile e seus atributos
 * @param cf ponteiro para ClassFile com fields a serem liberados
 */
void free_fields(ClassFile* cf);

/**
 * @brief Desaloca memória para os métodos de uma ClassFile e seus atributos.
 * @param cf ponteiro para ClassFile com métodos a serem liberados
 */
void free_methods(ClassFile* cf);

/**
 * @brief Desaloca memória para um array de attribute_info
 * @param cf ponteiro para ClassFile que contém os outros 
 *  parâmetros em sua estrutura
 * @param attributes ponteiro para array de attributes a ser liberado 
 * @param attributes_count número de entradas em attributes
 */
void free_attributes(ClassFile *cf,
    attribute_info* attributes,
    u2 attributes_count);

// --- LEITURA ---
/**
 * @brief Lê e inicializa o pool de constantes de uma ClassFile
 * @param file estrutura de leitura com bytes do .class na posição 
 * de início da constant pool
 * @param cf ClassFile com array constant_pool previamente alocado
 * @return SUCCESS em caso de sucesso ou ERR_CONSTANT_POOL_READ,
 * ERR_JAVA_INVALID_TAG em caso de erros
 */
int read_constant_pool(Reader *file, ClassFile *cf);

/**
 * @brief Lê e inicializa a tabela de interfaces de uma ClassFile
 * @param file estrutura de leitura com bytes do .class na posição 
 * de início das interfaces
 * @param cf ClassFile com array interfaces previamente alocado
 * @return SUCCESS em caso de sucesso
 */
int read_interfaces(Reader* file, ClassFile* cf);

/**
 * @brief inicializa o array de métodos de uma ClassFile
 * @param file estrutura de leitura com posição no início dos métodos
 * @param cf classfile com array methods previamente alocado para inicialização
 */
void read_methods(Reader* file, ClassFile* cf);

// --- LEITURA DE ATRIBUTOS ---
/**
 * @brief inicializa atributo Code 
 * @param r estrutura de leitura com posição no início das informações 
 * específicas de atributo
 * @param cf ClassFile onde este Code está contido
 * @param code_attr Code_attribute previamente alocado, a ser inicializado
 */
void read_code_attribute(
  Reader *r,
  ClassFile* cf,
  Code_attribute* code_attr
);

/**
 * @brief chama funções de inicialização específicas de acordo com o nome, 
 * ou ignora atributos não reconhecidos.
 * Atributos ConstantValue, Code, Exceptions, Sourcefile, LineNumberTable,
 * LocalVariableTable são reconhecidos. Os outros são ignorados e possuem
 * apenas o cabeçalho iniciado.
 * @param r estrutura de leitura com posição no início das informações 
 * específicas de atributo
 * @param cf ClassFile onde o atributo está contido
 * @param attr attribute_info previamente alocado
 */
void read_attribute_info(Reader* r, 
    ClassFile* cf, 
    attribute_info* attr);

/**
 * @brief inicializa cabeçalhos dos atributos (name_index e length)
 * e chama a read_attribute_info para o conteúdo
 * @param file estrutura de leitura na posição antes do 
 * início do vetor de atributos
 * @param cf ClassFile onde o vetor de atributos está presente
 * @param attributes_count tamanho do vetor attributes
 * @param attributes array previamente alocado de attributes a serem 
 * inicializados
 */
void read_attributes(Reader* file, ClassFile* cf, 
    u2 attributes_count, attribute_info* attributes); 

/**
 * @brief lê fields da classe
 * @param file estrutura de leitura
 * @param cf ClassFile dos fields a serem inicializados
 */
void read_fields(Reader* file, ClassFile* cf);

/**
 * @brief aloca e retorna uma utf8 de tamanho length presente no arquivo 
 * @param file estrutura de leitura com posição anterior aos bytes do utf8 
 * @param length quantidade de bytes a ser lida
 * @return ponteiro para uma string C (com terminador \0 em bytes[length])
 * alocada e inicializada por esta função
 */
u1* read_utf8(Reader* file, u2 length);

#endif
