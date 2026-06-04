#ifndef JVM
#define JVM

#include "common/classfile.h"
#include "jvmtypes.h"
#include <stdio.h>
#include <stdlib.h>

/** 
 * @brief Coloca um valor de 32 bits (int, float, ou referência) 
 * na Operand Stack. 
 * @param f Frame com a operand_stack
 * @param value valor de 32 bits a ser empilhado 
 */
static inline void push_operand(Frame* f, u4 value) 
{
  f->operand_stack[++f->stack_ptr] = value;
}

/** 
 * @brief Empilha um valor de 64 bits
 * na Operand Stack. 
 * @param f Frame com a operand_stack
 * @param value valor de 32 bits a ser empilhado 
 */
static inline void push_operand2(Frame* f, u8 value) 
{
  f->operand_stack[++f->stack_ptr] = (u4)(value >> 32);
  f->operand_stack[++f->stack_ptr] = (u4)value;
}

/** 
 * @brief Desempilha um valor de 32 bits (int, float, ou referência) 
 * da Operand Stack. 
 * @param f Frame com a operand_stack
 */
static inline u4 pop_operand(Frame* f) 
{
  if (f->stack_ptr < 0) {
    // Proteção contra esvaziamento da pilha (Underflow)
    fprintf(stderr, "ERROR: Operand stack underflow!\n");
    exit(1); 
  }
  
  return f->operand_stack[f->stack_ptr--];
}

/** 
 * @brief Desempilha um valor de 64 bits 
 * da Operand Stack. 
 * @param f Frame com a operand_stack
 */
static inline u8 pop_operand2(Frame *f)
{
  if (f->stack_ptr < 1) {
    // Proteção contra esvaziamento da pilha (Underflow)
    fprintf(stderr, "ERROR: Operand stack underflow!\n");
    exit(1); 
  }

  u8 l = (u8) f->operand_stack[f->stack_ptr--];
  u8 h = (u8) f->operand_stack[f->stack_ptr--];
  return (h << 32) | l;
}

/**
 * @brief Libera memória de um frame 
 * @param f Frame a ser liberado  
 */
void free_frame(Frame* f);

/**
 * @brief Desempilha um frame da thread, liberando 
 * sua memória 
 * @param t Thread de execução
 */
static inline void pop_frame(JVM_Thread* t)
{
  if (t->frame_ptr < 0) return;
  Frame* f = t->frames[t->frame_ptr--];
  free_frame(f);
}

/**
 * @brief Empilha um frame na thread ou retorna sem empilhar 
 * em caso de Stack Overflow
 * @param t Thread de execução para o frame ser empilhado
 * @param f Frame inicializado a ser empilhado
 */
#define STACK_OVERFLOR_ERROR -1
// TODO sistema de erros mais robusto
static inline int push_frame(JVM_Thread* t, Frame* f)
{
  t->frame_ptr++;
  if (t->frame_ptr >= JVM_STACK_SIZE) {
    return STACK_OVERFLOR_ERROR;
  }
  t->frames[t->frame_ptr] = f;
  return 0;
}

static inline Frame* current_frame(JVM_Context* ctx)
{
  return ctx->t.frames[ctx->t.frame_ptr];
}

static inline u4 current_pc(JVM_Context* ctx)
{
  return ctx->t.frames[ctx->t.frame_ptr]->pc;
}

/**
 * @brief retorna method_info de um método de nome name e descritor fornecido
 * @param cf ClassFile que contém o método
 * @param name nome simples do método
 * @param descriptor descritor de tipos do método
 * @return method_info do método encontrado, ou NULL caso o método 
 * não tenha sido encontrado
 */
method_info* find_method(ClassFile *cf, const char* name, 
    const char* descriptor);

/**
 * @brief inicializa uma estrutura JVM_Context sem classes carregadas
 * @return JVM_Context pronto para loop de execução
 * ou NULL caso main_class seja inválida (ex: sem método main correto)
 */
JVM_Context* jvm_init();

/**
 * @brief aloca memória e inicializa um novo Frame
 * @param cf ClassFile onde o método está declarado
 * @param method informação do método a ser carregado
 * @return ponteiro para o novo Frame
 */
Frame* new_frame(ClassFile* cf, method_info* method);

/**
 @brief Termina execução da JVM liberando toda memória alocada
 @param ctx contexto de execução a ser desalocado
 */
void terminateJVM(JVM_Context* ctx);

/**
 * @brief Conta número de slots de 32 bits para os argumentos de um método
 * a partir de seu descritor
 * @param descriptor string do descritor do método
 * @return int número de slots de 32 bits ocupados pelos argumentos
 * (número de operações push e pop)
 */
int count_args_size(const char* descriptor);

/**
 * @brief Lê um arquivo .class a partir de um caminho e retorna o ClassFile
 * alocado e inicializado, ou erro.
 * @param path caminho do classfile relativo ao diretório de execução 
 * @return ClassFile ponteiro para ClassFile inicializado
 */
ClassFile* ClassFile_from_path(const char* path);

/**
 * TODO 
 */
void initialize_class(JVM_Context* ctx, LoadedClass* loaded);



/**
 * @brief carrega e inicializa uma classe (campos estáticos) a partir do nome 
 * e do contexto de execução JVM 
 * @param ctx contexto de execução jvm 
 * @param name nome classe em relação ao base_dir
 */
LoadedClass* load_class(JVM_Context* ctx, const char* name);

/**
 * @brief carrega e inicializa a classe main, preparando o contexto JVM 
 * para iniciar a execução (empilha main)
 * @param ctx contexto JVM_Context a ter o main empilhado na thread principal
 * @param path caminho da classe que contém o main, absoluto, ou relativo ao 
 * diretório de execução da jvm 
 */
void load_main_class(JVM_Context* ctx, const char* path);

/**
 * @brief executa o bytecode de um método, e todos os métodos que forem 
 * invocados em sua execução. Obtém o opcode por meio do código do frame 
 * em questão e através dele busca a função handler na DISPATCH_TABLE. Retorna 
 * se o topo da pilha for menor que o argumento frame_ptr 
 * @param ctx Contexto de execução da jvm já inicializado 
 * @param frame_ptr ponteiro para frame que quando for finalizado, a execução 
 * termina (se for 0, executa o programa inteiro)
 * */
void run_method(JVM_Context *ctx, int frame_ptr);

/**
 * @brief Encontra uma LoadedClass da área de métodos que tenha o método 
 * especificado pelos parâmetros, e seja uma superclasse imediata ou não 
 * de base_class
 * @param ctx Contexto de execução JVM inciado 
 * @param base_class classe base que deve estar na área de métodos de ctx 
 * @param method_name nome simples do método 
 * @param descriptor descritor do método
 * @return LoadedClass* ponteiro para classe carregada que possua o método, 
 * ou NULL caso não tenha sido encontrada
 * */
LoadedClass* find_superclass_with_method(JVM_Context* ctx, 
    LoadedClass* base_class, const char* method_name, const char* descriptor);

/**
 * @brief procura uma classe na área de métodos 
 * cujo nome (CONSTANT_Class apontado por this_class)
 * seja igual ao argumento recebido, e a retorna caso encontrada
 * @param ctx Contexto de execução JVM 
 * @param name nome simples da classe
 */
LoadedClass* find_class_by_name(JVM_Context* ctx, const char* name);

/**
 * @brief Função para o loop de execução da JVM (fetch decode e execute)
 * chama run_method a partir do método inicial main (índice 0)
 *
 * @param ctx contexto de execução da JVM
 */
void jvm_run(JVM_Context* ctx);

#endif
