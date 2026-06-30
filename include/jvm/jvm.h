#ifndef JVM
#define JVM

#include "common/bytecode.h"
#include "common/classfile.h"
#include "jvm/interpreter.h"
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

static inline Object* get_object(JVM_Context* ctx, u4 objectref)
{
  if (objectref >= JVM_HEAP_CAPACITY) return NULL;
  return &ctx->objects.entries[objectref];
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


static inline Frame* current_frame(JVM_Context* ctx)
{
  return ctx->t.frames[ctx->t.frame_ptr];
}

static inline u1* current_pc(JVM_Context* ctx)
{
  return ctx->t.frames[ctx->t.frame_ptr]->pc;
}

static inline cp_info* constant_pool(Frame* f)
{
  return f->method.holder_class->cf->constant_pool;
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
 * @param method ponteiro para RuntimeMethod cujo conteúdo será copiado
 * para o RuntimeMethod do novo frame
 * @return ponteiro para o novo Frame
 */
Frame* new_frame(RuntimeMethod* method);

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
 * @brief inicializa os campos estáticos de uma classe carregada, 
 * e empilha na pilha JVM os métodos de inicialização <clinit> da 
 * hierarquia em ordem tal que as superclasses são inicializadas 
 * primeiro que a classe base (loaded)
 * @param ctx Contexto de execução JVM 
 * @param loaded ponteiro para classe carregada na área de métodos
 */
void initialize_class(JVM_Context* ctx, LoadedClass* loaded);

/**
 * @brief carrega classe a partir do nome  e do contexto de execução JVM, 
 * iniciando o campo cf do LoadedClass e colocando na área de métodos 
 * @param ctx contexto de execução jvm
 * @param name nome classe (sem .class) em relação ao base_dir
 * @return LoadedClass* ponteiro para a classe carregada
 */
LoadedClass* load_class(JVM_Context* ctx, const char* name);

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
 * @brief retorna a classe com nome desejado, se já estiver na área 
 * de métodos, ou carrega e retorna depois de colcoar na área de métodos
 * @param ctx contexto de execução JVM
 * @param name nome da classe (chama find_class_by_name e load_class)
 * @return LoadedClass* ponteiro para a classe carregada, ou NULL se não 
 * encontrar
 */
LoadedClass* get_class(JVM_Context* ctx, const char* name);

/**
 * @brief procura uma classe na área de métodos 
 * cujo nome (CONSTANT_Class apontado por this_class)
 * seja igual ao argumento recebido, e a retorna caso encontrada
 * @param ctx Contexto de execução JVM 
 * @param name nome simples da classe
 */
LoadedClass* find_class_by_name(JVM_Context* ctx, const char* name);

/**
 * @brief Empilha frame com método main na thread do contexto 
 * @param ctx contexto de execução Java
 * @param entry_class_name Classe de entrada do interpretador 
 * (deve implementar ou herdar public static void main(String[] args))
 */
void stack_main_frame(JVM_Context* ctx, const char* entry_class_name);

/**
 * @brief resolve um método a partir de um nome, descritor, 
 * e uma classe carregada, procurando entre os métodos implementados
 * ou herdados
 * @param base_class_pp ponteiro para um ponteiro que armazena a classe base 
 * será usado como retorno, conterá a classe que implementa o método 
 * encontrado, que pode ser uma superclasse 
 * @param method_name nome do método 
 * @param method_descriptor descritor do método
 * @return method_info* ponteiro para membro ClassFile 
 * que contém metadados do método, pode ser NULL caso o método não 
 * tenha sido encontrado em nenhuma classe a partir da classe base
 */
method_info* lookup_method(LoadedClass** base_class_pp, 
    const char* method_name, const char* method_descriptor);

/**
 * @brief Função para o loop de execução da JVM (fetch decode e execute)
 * chama run_method a partir do método inicial main (índice 0)
 *
 * @param ctx contexto de execução da JVM
 */
void jvm_run(JVM_Context* ctx, const char *entry_class_name);

/**
 * @brief Inicializa uma estrutura RuntimeMethod passado como ponteiro 
 * @param holder_class classe que implementa o método 
 * @param m_info ponteiro para estrutura method_info da constant_pool 
 * da classe que implementa
 * @param runtime_m ponteiro para a estrutura RuntimeMethod que será
 * inicializada a partir dos dois parâmetros anteriores
 */
void init_RuntimeMethod(LoadedClass* holder_class, method_info* m_info, 
    RuntimeMethod* runtime_m);

/**
 * @brief retorna ponteiro para estrutura de método resolvida 
 * a partir de um contexto JVM e do índice da constant_pool da 
 * classe em execução do frame atual 
 * @param ctx Contexto JVM 
 * @param cp_index índice válido para um CONSTANT_Methodref 
 * na constant_pool da classe atual */
RuntimeMethod* resolve_method(JVM_Context* ctx, u2 cp_idx);

RuntimeMethod* resolve_interface_method(JVM_Context* ctx, u2 cp_idx);

/**
 * @brief retorna ponteiro para estrutura de field resolvida 
 * a partir de um contexto JVM e do índice da constant_pool da 
 * classe em execução do frame atual 
 * @param ctx Contexto JVM 
 * @param cp_index índice válido para um CONSTANT_Fieldref
 * na constant_pool da classe atual 
 * @param is_static deve ser true se field esperado é estático, e falso 
 * caso contrário, para cálculo correto do índice
 */
RuntimeField* resolve_field(JVM_Context* ctx, u2 cp_idx, bool is_static);

/**
 * @brief retorna ponteiro para estrutura LoadedClass resolvida 
 * a partir de um contexto JVM e do índice da constant_pool da 
 * classe em execução do frame atual 
 * @param ctx Contexto JVM 
 * @param cp_index índice válido para um CONSTANT_Fieldref
 * na constant_pool da classe atual */
LoadedClass* resolve_class(JVM_Context* ctx, u2 cp_idx);


/**
 * @brief Resolve uma setring a partir de um CONSTANT_String, 
 * carrega na tabela de Strings e retorna o índice na tabela 
 * de strings (StringTable)
 * @param ctx Contexto de execução JVM
 * @param clazz classe carregada em que a constant_pool contém o CONSTANT_String
 * válido no índice dado por cp_idx
 * @param cp_idx índice válido para um CONSTANT_String na constant_pool do 
 * classfile parseado da classe carregada 
 * @return u4 índice na StringTable para a string carregada 
 */
u4 load_string(JVM_Context *ctx, LoadedClass* clazz, u4 cp_idx);


/**
 * @brief função que retorna verdadeiro se a classe_a herda 
 * da classe_b (verdadeiro se as duas são iguais também).
 * Verifica se é possível obter class_b ao acessar continuamente 
 * o ponteiro super a partir de class_a e sua superclasse
 * @param class_a classe que pode herdade de b 
 * @param class_b classe que pode ser tal que class_a herda de class_b
 * @return bool verdadeiro se classes são iguais ou se class_a herda 
 * de class_b
 */
bool extends(LoadedClass* class_a, LoadedClass* class_b);


/**
 * @brief função que recebe um ponteiro de RuntimeMethod 
 * e cria um novo frame com o método carregado e empilha 
 * na thread principal do contexto passado por parâmetro
 * @param ctx Contexto de execução jvm 
 * @param target_method ponteiro para RuntimeMethod que contém o 
 * código que será executado no novo frame
 */
void invoke_method(JVM_Context* ctx, RuntimeMethod* target_method); 

/**
 * @brief Empilha um frame na thread ou retorna sem empilhar 
 * em caso de Stack Overflow
 * @param t Thread de execução para o frame ser empilhado
 * @param f Frame inicializado a ser empilhado
 */
#define STACK_OVERFLOR_ERROR -1
// TODO sistema de erros mais robusto
static inline int push_frame(JVM_Context* ctx, Frame* f)
{
  ctx->t.frame_ptr++;
  if (ctx->t.frame_ptr >= JVM_STACK_SIZE)
  {
    fprintf(stderr, "StackOverflowError\n");
    ctx->t.frame_ptr--;
    terminateJVM(ctx);
    exit(1);
  }
  ctx->t.frames[ctx->t.frame_ptr] = f;
  return 0;
}

/**
 * @brief simula carregamento de uma classe java, mas sem o comportamento real.
 * A exemplo de java/lang/Object, e exceções. Usada para simular a existência 
 * de classes da biblioteca do java no contexto em que apenas o nome da classe 
 * é necessário. 
 * @param ctx contexto de execução da JVM 
 * @param name nome da classe
 * @return LoadedClass* em que apenas o nome da classe é iniciado, a classe 
 * é marcada como inicializada e todos os outros campos são zerados 
 * (constant pool e classfile)
 */
LoadedClass* load_mock_class(JVM_Context* ctx, const char* name);

/**
 * @brief instancia um novo objeto da classe passada como parâmetro 
 * e retorna a referência obtida
 * @param ctx contexto de execução JVM 
 * @param clazz classe do objeto em questão
 * @return u4 referência do objeto (índice na tabela de objetos)
 */
u4 new_object(JVM_Context* ctx, LoadedClass* clazz);


/**
 * @brief lança uma exceção nativa
 * Cria um objeto de exceção e empilha sua referência na pilha 
 * de operandos do frame atual, e depois chama handle_athrow
 * para lidar com a exceção. Usado para exceções que não são 
 * lançadas pelo usuário, e sim por quebra de regras dentro 
 * da lógica de cada handler
 * @param ctx Contexto de execução JVM
 * @param name nome da instrução, deve ser uma instrução nativa (mock) ou 
 * de uma classe existente
 */
static inline void throw_native(JVM_Context* ctx, const char* name)
{
  Frame* f = current_frame(ctx);
  push_operand(f, new_object(ctx, get_class(ctx, name)));
  handle_athrow(ctx, opc_athrow);
}

/**
 * @brief retorna verdadeiro se classe é nativa java 
 * Usado para criação de objetos em que não é necessário 
 * inicializar nenhum campo além de nome. 
 * Exemplo: java/lang/Object, java/lang/String, java/lang/<exceptions>
 * @param nome da classe
 */
bool is_native_java_class(const char* name);

/**
 * @brief termina a execução após um erro fatal e exibe stack trace
 * Exibe a mensagem formatada seguida da stack trace, comparando 
 * o pc de cada frame com os atributos de LineNumberTable para obter 
 * o número da linha. Encerra a execução da jvm e libera toda a memória 
 * alocada.
 * @param ctx contexto de execução jvm
 * @param format string formatada a ser exibida, deve ser 
 * seguida dos valores passados ao formato
 */
void fatal_error(JVM_Context* ctx, const char* format, ...);
#endif
