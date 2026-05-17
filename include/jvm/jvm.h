#ifndef JVM
#define JVM

#include "common/classfile.h"
#include "jvmtypes.h"

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
 * @brief inicializa JVM_Context ao receber classe de entrada
 * @param main_class ClassFile inicializada que deve ser ponto de entrada 
 * da execução
 * @return JVM_Context pronto para loop de execução caso main_class seja válida,
 * ou NULL caso main_class seja inválida (ex: sem método main correto)
 */
JVM_Context* jvm_init(ClassFile* main_class);

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
 * @brief Libera memória de um frame 
 * @param f Frame a ser liberado  
 */
void free_frame(Frame* f);

#endif
