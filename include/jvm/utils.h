#ifndef INCLUDE_JVM_UTILS
#define INCLUDE_JVM_UTILS

#include "jvm/jvmtypes.h"
#include <string.h>

/**
 * @brief copia 32 bits para uma variável do tipo float e retorna,
 * para viabilizar operações de float a partir de u4
 * @param bits 32 bits do float 
 * @return float com os mesmos bits
 */
static inline float u4_to_float(u4 bits) {
  float f;
  memcpy(&f, &bits, sizeof(f));
  return f;
}

/**
 * @brief copia 32 bits de um float para uma variável do tipo u4 e retorna,
 * para viabilizar valores de slot jvm (u4) após operações com float
 * @param f float 
 * @return u4 com bits do float
 */
static inline u4 float_to_u4(float f) {
  u4 bits;
  memcpy(&bits, &f, sizeof(bits));
  return bits;
}

/**
 * @brief copia 64 bits para uma variável do tipo double e retorna,
 * para viabilizar operações de float a partir de u4
 * @param bits 64 bits do double
 * @return douvke com os mesmos bits
 */
static inline double u8_to_double(u8 bits) {
  double d;
  memcpy(&d, &bits, sizeof(d));
  return d;
}

/**
 * @brief copia 64 bits de um double para uma variável do tipo u8 e retorna,
 * para viabilizar valores de slot jvm (u4) após operações com double 
 * @param d double
 * @return u8 com bits do double
 */
static inline u8 double_to_u8(double d) {
  u8 bits;
  memcpy(&bits, &d, sizeof(bits));
  return bits;
}



/**
 * @brief Função utilitário para simular impressão pelo System.out
 * usando funções nativas C (printf).
 * @param frame Frame de execução 
 * @param ctx Contexto de execução JVM 
 * @param descriptor caracter inicail do descritor para indicar o 
 * tipo a do argumento a ser impresso, se for L, assume ser java/lang/String */
void handle_sysout(Frame* frame, JVM_Context* ctx, char descriptor);

/**
 * @brief Função que exibe mensagem de erro de opcode não implementado 
 * e aborta a execução do programa.
 * @param ctx Contexto de execução 
 * @param opc opcode não reconhecido ou não implementado 
 */
void jvm_error_uninmplemented_opc(JVM_Context* ctx, u1 opc);

/**
 * @brief retorna a partir de um caminho, o diretório base para 
 * resolução dos caminhos das .class 
 * @param path caminho completo de um .class 
 */
void extract_class_dir(const char* path, char* dest, size_t dest_size);

const char* extract_class_name_from_path(const char *path, 
    char* dest, size_t dest_size);

#endif
