#ifndef INTERPRETER
#define INTERPRETER

#include "jvmtypes.h"

#define JVM_HANDLE_SYSOUT 0x01

typedef void (*instruction_handler)(JVM_Context* ctx, u1 opc);

extern const instruction_handler DISPATCH_TABLE[256];

/**
 * @brief Função que retorna um byte do bytecode Java,
 * apontado pela posição PC e avança o PC em 1.
 * @param code ponteiro para o array de bytecode Java
 * @param pc ponteiro para o u4 que representa o contador de programa,
 * será incrementado após a obtenção do byte
 * @return byte na posição do PC atual 
 */
static inline u1 fetch_u1(u1 *code, u4 *pc) {
  return code[(*pc)++];
}

/**
 * @brief Função que retorna 2 bytes do bytecode Java,
 * apontado pela posição PC e avança o PC em 2.
 * @param code ponteiro para o array de bytecode Java
 * @param pc ponteiro para o u4 que representa o contador de programa,
 * será incrementado após a obtenção da word
 * @return word na posição do PC atual 
 */
static inline u2 fetch_u2(u1 *code, u4 *pc) {
  u2 v = ((u2)code[*pc] << 8) |
         ((u2)code[*pc+1]);
  *pc += 2;
  return v;
}

/**
 * @brief Função que retorna 2 words do bytecode Java,
 * apontadas pela posição PC e avança o PC em 4.
 * @param code ponteiro para o array de bytecode Java
 * @param pc ponteiro para o u4 que representa o contador de programa,
 * será incrementado após a obtenção das words
 * @return palavra de 16 bits na posição do PC atual 
 */
static inline u4 fetch_u4(u1* code, u4 *pc) {
  u4 v = ((u4)code[*pc]     << 24) |
         ((u4)code[*pc + 1] << 16) |
         ((u4)code[*pc + 2] << 8)  |
          (u4)code[*pc + 3];
  *pc += 4;
  return v;
}

/**
 * @brief Função para o loop de execução da JVM (fetch decode e execute)
 *
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void jvm_run(JVM_Context* ctx);



/**
 * @brief Função que implementa NOP 
 *
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_nop(JVM_Context* ctx, u1 opc);  // 0 

/**
 * @brief Função que implementa ldc (18), ldc_w (19) e ldc2_w (20) 
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_ldc(JVM_Context* ctx, u1 opc); // 18, 19, 20

/**
 * @brief Função que implementa opcodes aload 
 * @param opc opcode
 */
void handle_aload(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa execução dos opcodes de retorno (169, 172-177)
 *
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_return(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa getstatic (178)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */                                  
void handle_getstatic(JVM_Context* ctx, u1 opc); // 178

/**
 * @brief Função que implementa invokevirtual (182)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_invokevirtual(JVM_Context* ctx, u1 opc); // 182

/**
 * @brief Função que implementa invokestatic (184)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_invokestatic(JVM_Context* ctx, u1 opc); // 184

#endif
