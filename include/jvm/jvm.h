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

/**
 * @brief retorna um objeto a partir de uma referência passada 
 * @param ctx Contexto de execução JVM 
 * @param objectref referência dos objetos 
 */
static inline Object* get_object(JVM_Context* ctx, u4 objectref)
{
  if (objectref >= ctx->objects.count) return NULL;
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
 * @details Funcionamento:
 * Aloca struct JVM_Context, inicializa capacidade da tabela de objetos 
 * para 100, inicializa contagem de objetos em 1 (índice 0 é reservado 
 * para representar nullptr). Inicializa contagem de classes em 0, strings 
 * em 1 (nullptr com índice 0 reservado). 
 * @return JVM_Context ponteiro para jvm context alocado ou NULL 
 * em caso de erro
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
 * @brief prepara a pilha para inicialziar uma classe 
 * @details Funniniciamento:
 * Inicializa os campos estáticos de uma classe carregada, 
 * e empilha na pilha JVM os métodos de inicialização <clinit> da 
 * hierarquia em ordem tal que as superclasses são inicializadas 
 * primeiro que a classe base (loaded). As classes são marcadas como 
 * is_initialized = true, logo no início, para evitar dependência cíclica 
 * de inicializações, o que poderia causar estouro da pilha. Este método 
 * não executa os inicializadores, e sim unicamente os empiha na pilha de 
 * frames, de forma que garantidamente, na próxima iteração do loop jvm_run, 
 * as classes serão inicializadas na ordem que é determinada pelas 
 * especificações da JVM
 * @param ctx Contexto de execução JVM 
 * @param loaded ponteiro para classe carregada na área de métodos
 */
void initialize_class(JVM_Context* ctx, LoadedClass* loaded);

/**
 * @brief carrega classe a partir do nome  e do contexto de execução JVM, 
 * iniciando o campo cf do LoadedClass e colocando na área de métodos
 * @details Funcionamento:
 * Procura uma classe a partir de seu nome simples, no diretório da classe 
 * que tem o main (não resolve pacotes, ou interpreta pastas como pacotes, 
 * ao coincidir caminhos relativos). Caso a classe seja uma classe nativa, como 
 * String, ou exceções comuns como NullPointerException, ArithmeticException, 
 * ArrayIndexOutOfBoundsEx., NegativeArraySizeEx., ClassCastEx., a classe é 
 * inicializada sem ser carregada propriamente, uma vez que a existência do 
 * LoadedClass é necessária para o Funcionamento desta máquina virtual, mas o 
 * comportamento foge do escopo deste projeto
 * @param ctx contexto de execução jvm
 * @param name nome classe (sem .class) em relação ao base_dir
 * @return LoadedClass* ponteiro para a classe carregada
 */
LoadedClass* load_class(JVM_Context* ctx, const char* name);

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
 * @details Funcionamento:
 * procura pelo nome (CONSTANT_Class apontado por this_class).
 * seja igual ao argumento recebido, e a retorna caso encontrada.
 * Caso não encontrou e não sej aum array, retorna NULL
 * @param ctx Contexto de execução JVM 
 * @param name nome simples da classe
 * @return LoadedClass* ponteiro para classe encontrada.  */
LoadedClass* find_class_by_name(JVM_Context* ctx, const char* name);

/**
 * @brief Empilha frame com método main na thread do contexto
 * @details Funcionamento:
 * Usa get_class para obter a classe passada como argumento, e tenta 
 * carregá-la. Após efetuar o carregamente de toda a hierarquia, checa 
 * pela existência do método main adequado, e em caso afirmativo, empilha 
 * o método main e inicializa as classes, de forma que após o retorno da 
 * função, a pilha de frames da thread única de ctx está de tal forma que 
 * a inicialziação da hierarquia e a execução do main acontecerçaõ na ordem 
 * determinada pela especificação da JVM
 * @param ctx contexto de execução Java
 * @param entry_class_name Classe de entrada do interpretador 
 * (deve implementar ou herdar public static void main(String[] args))
 */
void stack_main_frame(JVM_Context* ctx, const char* entry_class_name);

/**
 * @brief Função para o loop de execução da JVM (fetch decode e execute)
 * @details Funcionamento:
 * chama stack_main_frame para empilhar o método main, 
 * passando entry_class_name. Caso a classe seja válida, o laço começa 
 * com um método de inicialização, ou o método main, como frame no topo 
 * da pilha de frames. O opcode é obtido com *frame->pc++ e o handler é 
 * obtido por meio da dispatch table. Ao final da execução, apenas retorna, 
 * não tem a responsabilidade de liberar memória 
 * (terminateJVM deve ser chamado separadamente)
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
 * classe em execução do frame atual.
 * @details Funcionamento:
 * A partir do cp_idx, procura pelo método, navegando pelos 
 * índices e procurando a declaração deo método nas superclasses 
 * (caso aplicado). Pode desviar para fatal_error e encerrar a execução 
 * caso o método não exista, ou uma política de acesso seja violada.
 * Métodos das classes java não são inicialziados além do nome e descritor,
 * pois não são efetivamente arregados e são gerados apenas para garantir a 
 * consistência das funções deste programa
 * @param ctx Contexto JVM 
 * @param cp_index índice válido para um CONSTANT_Methodref 
 * na constant_pool da classe atual 
 * @return RuntimeMethod ponteiro estrutura RuntimeMethod inicializada com 
 * pelo menos o nome e descritor */
RuntimeMethod* resolve_method(JVM_Context* ctx, u2 cp_idx);

/**
 * @brief Retorna método de interface resolvido, análogo ao resolve_method, mas 
 * lidando com CONSTANT_InterfaceMethodRef
 * @param ctx Contexto JVM 
 * @param cp_index índice válido para um CONSTANT_Methodref 
 * na constant_pool da classe atual 
 * @return RuntimeMethod ponteiro estrutura RuntimeMethod inicializada com 
 * pelo menos o nome e descritor, e certamente não o código 
 */
RuntimeMethod* resolve_interface_method(JVM_Context* ctx, u2 cp_idx);

/**
 * @brief retorna ponteiro para estrutura de field resolvida 
 * a partir de um contexto JVM e do índice da constant_pool da 
 * classe em execução do frame atual 
 * @details Funcionamento:
 * Navega pelos índices da constant_pool para obter os valores relevantes finais
 * do campo, como suas flags de acesso, a classe que o contém (pode ser uma 
 * superclasse da classe que inicialmente foi obtida), inicia descritor e nome 
 * do campo, atributos e contagem de atributos. O índice real do ínicio do field
 * no vetor de campos da classe ou instância é calculado com base no tamanho 
 * (cat1 ou cat2) dos fields da mesma categoria (static ou de instancia) que 
 * o antecedem. Os índices são gerados de forma que o RuntimeField é válido 
 * para todas as classes numa mesma hierarquia, pois as primeiras classes 
 * (classe mãe) tem os fields de instância com os índices mais baixos.
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
 * @details Funcionamento:
 * Resolve os índices e campos na constant_pool dao ClassFile para obter 
 * uma LoadedClass* adequada, chamando get_class.
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
 * @details Funcionamento:
 * A exemplo de java/lang/Object, e exceções. Usada para simular a existência 
 * de classes da biblioteca do java no contexto em que apenas o nome da classe 
 * é necessário. Inicializa apenas o nome da classe, mas não campos
 * dependentes de classfile real. Funções que recebem algum LoadedClass devem 
 * tratar ou não este caso conforme necessidade
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
 * @details Funcionamento:
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
 * @details Funcionamento:
 * Exibe a mensagem formatada seguida da stack trace, comparando 
 * o pc de cada frame com os atributos de LineNumberTable para obter 
 * o número da linha. Encerra a execução da jvm e libera toda a memória 
 * alocada. Retorna código de erro (1) ao sistema operacional.
 * @param ctx contexto de execução jvm
 * @param format string formatada a ser exibida, deve ser 
 * seguida dos valores passados ao formato
 */
void fatal_error(JVM_Context* ctx, const char* format, ...);
#endif
