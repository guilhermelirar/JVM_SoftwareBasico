#ifndef INTERPRETER
#define INTERPRETER


#define T_BOOLEAN 4 
#define T_BOOLEAN	4
#define T_CHAR	5
#define T_FLOAT	6
#define T_DOUBLE	7
#define T_BYTE	8
#define T_SHORT	9
#define T_INT	10
#define T_LONG	11

#include "jvmtypes.h"

#define JVM_HANDLE_SYSOUT 0x01

typedef void (*instruction_handler)(JVM_Context* ctx, u1 opc);

extern const instruction_handler DISPATCH_TABLE[256];

#define IN_RANGE(x, min, max) ((x) >= (min) && (x) <= (max))

/**
 * @brief Função que retorna 2 bytes do bytecode Java,
 * apontado pela posição PC e avança o PC em 2.
 * @param code ponteiro para o array de bytecode Java
 * @param pc ponteiro para o u4 que representa o contador de programa,
 * será incrementado após a obtenção da word
 * @return word na posição do PC atual 
 */
static inline u2 fetch_u2(u1 **pc) {
  u1 *current_pc = *pc; 
  u2 high = (u2)(*current_pc++);
  u2 low = (u2)(*current_pc++);
  *pc = current_pc;
  return (high << 8) | low;
}

/**
 * @brief Função que retorna 2 words do bytecode Java,
 * apontadas pela posição PC e avança o PC em 4.
 * @param code ponteiro para o array de bytecode Java
 * @param pc ponteiro para o u4 que representa o contador de programa,
 * será incrementado após a obtenção das words
 * @return palavra de 16 bits na posição do PC atual 
 */
static inline u4 fetch_u4(u1 **pc) {
  u4 h = fetch_u2(pc);
  u4 l = fetch_u2(pc);
  return (h << 16) | l;
}


/**
 * @brief Função que implementa NOP
 * Termina a execução imediatamente caso o opcode 
 * não seja opc_nop ou opc_wide (lógica de wide 
 * é tratada internamente pelos handlers dos opcodes
 * que podem seguir wide)
 *
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_nop(JVM_Context* ctx, u1 opc);  // 0 


/**
 * @brief Função que implementa instruções tconst_<i> (1-15)
 * (aconst_null, iconst_<i>, lconst_<i>, fconst_<i>, dconst_<i>)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_tconst(JVM_Context* ctx, u1 opc);  // 0 


/**
 * @brief Função que implementa bipush (16) e sipush (17) 
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_push(JVM_Context* ctx, u1 opc); // 16, 17

/**
 * @brief Função que implementa ldc (18), ldc_w (19) e ldc2_w (20) 
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_ldc(JVM_Context* ctx, u1 opc); // 18, 19, 20

/**
 * @brief Função que implementa opcodes load (21-53)
 * que carregam valores das variáveis locais na pilha 
 * de operandos (iload-saload)
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_load(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes store (54-86)
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_store(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes astore (arrays)
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_astore(JVM_Context* ctx, u1 opc);


/**
 * @brief Função que implementa opcode de tableswitch 
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_tableswitch(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode de lookupswitch 
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_lookupswitch(JVM_Context* ctx, u1 opc);

/**
 * @brief função que implementa os opcodes de manipulação da pilha
 * sem retorno (87 a 96)
 * @param ctx contexto de execução Java 
 * @param opc opcode
 */ 
void handle_stack(JVM_Context* ctx, u1 opc);


/**
 * @brief Função que implementa opcodes de operações aritméticas
 * como add, sub, mul, div, rem, neg (95-119)
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_arithmetic(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de operações bitwise
 * (ishl-lxor)
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_logic(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de conversão de tipo
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_conversion(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode de comparação
 * de long (0x94)
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_lcmp(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de comparação de valores 
 * em ponto flutuantee e double (0x95-0x97 fcmp, dcmp) 
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_fdcmp(JVM_Context* ctx, u1 opc);



/**
 * @brief Função que implementa opcodes de operações de saltos 
 * condicionais if<conc> 0x99-0x9E
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_ifcond(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de comparação de valores 
 * da pilha de operandos e salto condicional ao resultado 
 * (if_cmpeq - if_acmpne)
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_ifcmp(JVM_Context* ctx, u1 opc);



/**
 * @brief Função que implementa opcode goto, jsr e ret (167-169)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */
void handle_goto_jsr_ret(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa execução dos opcodes de retorno (172-177)
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
 * @brief Função que implementa putstatic (179)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */                                  
void handle_putstatic(JVM_Context* ctx, u1 opc); // 179

/**
 * @brief Função que implementa opcode get para 
 * acessar variáveis de instância
 * @param ctx contexto de execução jvm 
 * @param opc opcode (180) */
void handle_getfield(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode putfield para 
 * mudança de campos de isntância
 * @param ctx contexto de execução jvm 
 * @param opc opcode (181) */
void handle_putfield(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa invokevirtual (182)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_invokevirtual(JVM_Context* ctx, u1 opc); // 182

/**
 * @brief Função que implementa instrução invokespecial, 
 * que lida com invocalçao de métodos de instância, 
 * lidando com superclasses, métodos privados e métodos 
 * de inicialização de instância
 */
void handle_invokespecial(JVM_Context* ctx, u1 opc); //183

/**
 * @brief Função que implementa invokestatic (184)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_invokestatic(JVM_Context* ctx, u1 opc); // 184

/**
 * @brief Função que implementa new (instanciação de objetos)
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_new(JVM_Context* ctx, u1 opc); // 187

/**
 * @brief Função que implementa newarray 
 * (instanciação de arrays de tipos primitivos)
 * @param ctx contexto de execução da JVM
 * @param opc opcode (188) */    
void handle_newarray(JVM_Context* ctx, u1 opc); // 188

/**
 * @brief Função que implementa arraylength  
 * (tamanho da array retornado na pilha como inteiro)
 * @param ctx contexto de execução da JVM
 * @param opc opcode (190, 0xBE) */    
void handle_arraylength(JVM_Context* ctx, u1 opc); // 190

#endif

