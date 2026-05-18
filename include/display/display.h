#ifndef DISPLAY
#define DISPLAY

#include "common/classfile.h"
#include "common/reader.h"
#include <stdio.h>

/**
 * @brief Estrutura auxiliar que representa uma flag de acesso
 */
typedef struct {
  u2 mask;  /**< Máscara de bits da flag de acesso */
  const char* java_kw; /**< Nome simples da flag (public, static, ...)*/
  const char* debug_kw; /**< Nome de debug (ACC_PUBLIC, ACC_STATIC, ...)*/
} access_flag_desc;

/**
 * @brief Estrutura auxiliar para informar se flag deve ser exibida em 
 * estilo java (simples) ou debug
 */
typedef enum {
    ACCESS_FMT_DEBUG, 
    ACCESS_FMT_JAVA 
} access_format_t;

/**
 * @brief função para imprimir uma identação (caracteres vazios)
 * @param level número de caracteres de espaço vazio
 * @param file descritor do arquivo de texto destino
 */
static inline void print_indent(int level, FILE *f) {
  for (int i = 0; i < level; i++) {
    fputc(' ', f);
  }
}

/**
 * @brief Imprime descritor de campo
 * @param out descritor do arquivo de texto de saída
 * @param str string que representa o descritor em formato java .class
 * @param pos ponteiro para a posição de início da string. Será modificado para 
 * que a função chamadora saiba onde continuar a leitura
 * @param dimensions número de dimensões (ex 2, se [[I). Deve ser 0 na primeira
 * chamada. Valor deve ser > ) apenas em caso de chamada recursiva
 */
void print_field_descriptor(FILE* out, const char* str, 
    int* pos, int dimensions);

/**
 * @brief Imprime string utf-8 em arquivo de saída
 * @param out descritor do arquivo de texto de saída
 * @param str bytes do utf-8
 */
void print_utf8(FILE* out, const char* str);

/**
 * @brief Imprime os operandos de uma instrução JVM
 * @param cf ClassFile do método onde o opcode se encontra
 * @param code_reader ponteiro para estrutura de leitura do bytecode
 * @param opc valor do opcode
 * @param out descritor do arquivo de texto de saída
 * @param indent identação base
 */
void print_operands(const ClassFile* cf, Reader *code_reader,
    u1 opc, FILE* out, int indent);

/**
 * @brief imprime um atributo de código (disassembler), seus atributos internos,
 * instruções e operandos das instruções
 * @param cf ClassFile que contém o método do code 
 * @param code ponteiro para atributo code
 * @param out descritor do arquivo de saída 
 * @param ident nível de identação base
 */
void print_code(const ClassFile *cf, 
    const Code_attribute* code, FILE* out, int indent);  

/**
 * @brief exibe a definição de um método (ex: 
 *  public static void main(String[] args))
 * @param cf ClassFile que contém o método 
 * @param m ponteiro para estrutura de informação de método 
 * @param out descritor do arquivo de saída 
 */
void print_method_definition(const ClassFile* cf, method_info* m, FILE* out);

/**
 * @brief exibe o valor armazenado num índice da constant pool  
 * @param idx índice válido para constant pool
 * @param cf ClassFile da constant pool desejada
 * @param file descritor do arquivo de saída 
 */
void print_cp_value(u2 idx, const ClassFile *cf, FILE *file);

/**
 * @brief exibe relação índice - valor da constant pool de uma ClassFile,
 * pulando índices vazios (0 ou vazios por long ou double).
 * @param cf ClassFile da constant pool desejada
 * @param file descritor do arquivo de saída 
 */
void print_class_constant_pool(const ClassFile *cf, FILE *file);

/**
 * @brief imprime flag de acesso no formato fmt (debug ou Java), de acordo
 * com o contexto (método, classe, field)
 * @param u2 máscara de bits das flags de acesso
 * @param ctx contexto de interpretação das flags
 * @param fmt formato de exibição das flags
 * @param file descritor do arquivo de saída 
 */
void print_access_flags(u2 bits, access_context_t ctx, 
    access_format_t fmt, FILE* file);

/**
 * @brief imprime listas das interfaces e suas informações de uma ClassFile
 * @param cf ClassFile que implementa as interfaces
 * @param file descritor do arquivo de saída
 */
void print_interfaces(const ClassFile* cf, FILE* file);

/**
 * @brief imprime fields de uma classe e suas informações 
 * (chama print_class_member adequadamente) 
 * @param cf ClassFile que contém os fields
 * @param file descritor do arquivo de saída
 */
void print_fields(const ClassFile* cf, FILE* file);

/**
 * @brief imprime atributos (informações e atributos internos)
 * contidos em uma lista de referências, no contexto de uma ClassFile
 * @param cf ClassFile do contexto do atributo
 * @param count número de atrubutos a serem impressos
 * @param attributes lista de atributos a serem impressos
 * @param file descritor do arquivo de saída
 * @param indent nível de indentação
 */
void print_attributes(const ClassFile *cf, 
    u2 count, attribute_info *attributes, 
    FILE *file, int indent);

/**
 * @brief imprime a lista de métodos de uma classe e suas informações
 * (chama print_class_member adequadamente)
 * @param cf ClassFile que contém os métodso
 * @param file descritor do arquivo de saída
 */
void print_methods(const ClassFile* cf, FILE *file);

/**
 * @brief exibe informações de um membro de classe (field ou método)
 * @param cf ClassFile que contém o membro
 * @param index índice do contexto do membro de classe na lista de exibição 
 * a qual ele está inserido
 * @param name_index índice válido da constant pool para CONSTANT_Utf8 
 * que representa o nome do atributo
 * @param descriptor_index índice da constant pool 
 * para o CONSTANT_Utf8 que representa o descritor deste membro
 * @param attributes_count número de atributos deste membro
 * @param attributes lista dos atributos do campo
 * @param access_ctx contexto de exibição das flags de acesso 
 * @param access_flags máscara com flags de acesso
 * @param file descritor do arquivo de saída
 */
void print_class_member(const ClassFile* cf,
    u2 index,
    u2 name_index, u2 descriptor_index, u2 attributes_count,
    attribute_info* attributes,
    access_context_t access_ctx, u2 access_flags,
    FILE* file);

/**
 * @brief fazendo uso de todas as funções do módulo de display,
 * imprime as informações contidas numa estrutura ClassFile
 * @param cf ponteiro para objeto ClassFile a ser impresso 
 * @param file descritor do arquivo de saída
 */
void printclass(const ClassFile* cf, FILE* file);

#endif
