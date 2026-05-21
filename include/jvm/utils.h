#ifndef INCLUDE_JVM_UTILS
#define INCLUDE_JVM_UTILS

#include "jvm/jvmtypes.h"

/**
 * @brief Função utilitário para simular impressão pelo System.out
 * usando funções nativas C (printf).
 * @param frame Frame de execução 
 * @param ctx Contexto de execução JVM 
 * @param descriptor caracter inicail do descritor para indicar o 
 * tipo a do argumento a ser impresso, se for L, assume ser java/lang/String */
void handle_sysout(Frame* frame, JVM_Context* ctx, char descriptor);

#endif
