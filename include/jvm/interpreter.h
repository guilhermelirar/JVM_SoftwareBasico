#ifndef INTERPRETER
#define INTERPRETER

#define T_BOOLEAN 4 /*> Valor do tipo primitivo booleano para array Java */ 
#define T_CHAR	5   /*> Valor do tipo primitivo caractere para array Java */ 
#define T_FLOAT	6   /*> Valor do tipo primitivo float para array Java */ 
#define T_DOUBLE	7 /*> Valor do tipo primitivo double para array Java */ 
#define T_BYTE	8  /*> Valor do tipo primitivo byte para array Java */ 
#define T_SHORT	9  /*> Valor do tipo primitivo short para array Java */
#define T_INT	10   /*> Valor do tipo primitivo int para array Java */
#define T_LONG	11 /*> Valor do tipo primitivo long para array Java */

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
 * @brief Função que implementa opcode nop ou captura opcodes não implementados
 * @details Funcionamento:
 * Não realiza nenhuma ação em caso de opc_nop (0x00) ou opc_wide, pois 
 * opc_wide é tratado em handlers específicos de instruções afetadas pelo 
 * wide. Resulta em erro de execução em caso de opcode não suportado.
 * @param ctx contexto de execução da JVM
 * @param opc opcode 
 * @return void não altera pilha de operandos, pode terminar a execução
 */
void handle_nop(JVM_Context* ctx, u1 opc);  // 0 


/**
 * @brief Função que implementa instruções tconst_<i> (1-15)
 * (aconst_null, iconst_<i>, lconst_<i>, fconst_<i>, dconst_<i>)
 * @details Funcionamento:
 * Obtém frame atual e empilha um valor (constante) dado pelo opcode
 * com o tipo adequado. Assume que o opcode passado é realmente um 
 * dos opcodes listados
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 * @return void empilha um valor da pilha (cat 1 ou 2)
 */
void handle_tconst(JVM_Context* ctx, u1 opc);  // 0 


/**
 * @brief Função que implementa bipush (16) e sipush (17) 
 * @details Funcionamento:
 * Em caso de bipush, empilha o próximo byte do código atual 
 * com extensão de sinal. E em caso de sipush, empilha os 
 * próximos dois bytes, concatenados com extensão de sinal.
 * A extensão de sinal converte ambos os valores em int32_t
 * @param ctx contexto de execução da JVM
 * @param opc opcode (16, 17)
 * @return void empilha até um valor na pilha de operandos do frame atual
 */
void handle_push(JVM_Context* ctx, u1 opc); // 16, 17

/**
 * @brief Função que implementa ldc (18), ldc_w (19) e ldc2_w (20) 
 * @param ctx contexto de execução da JVM
 * @details Funcionamento: 
 * Obtém no bytecode um ínice para a constant pool, e acessa a constant 
 * pool para resolver o valor e empilha o resultado
 * @param opc opcode (18, 19, 20)
 * @return void avança o pc em até 2 unidades, e empilha um resultado (cat1-2)
 */
void handle_ldc(JVM_Context* ctx, u1 opc); // 18, 19, 20

/**
 * @brief Função que implementa opcodes load (21-53)
 * que carregam valores das variáveis locais na pilha 
 * de operandos (iload-saload)
 * @details Funcionamento:
 * Verifica se o opcode foi precidido de wide, para obter o número 
 * de bytes necessários paraa reconstruir cp_idx, um índice para o 
 * o vetor de variáveis locais do método. Depois obtém o valor salvo 
 * no índice, de acordo com o tipo e tamanho (cat1 ou cat2) e empilha 
 * o resultado (push_operand e push_operand2).
 * @param ctx contexto de execução Java
 * @param opc opcode (21-53)
 * @return void altera pilha de operandos empilhando valores
 */
void handle_load(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes store (54-86)
 * @details Funcionamento:
 * Verifica se o opcode foi precedido por wide, para reconstruir 
 * com tamanho adequado um índice para o vetor de variáveis locais. 
 * Em seguida, de acordo com o tipo dado pelo opcode, desempilha um 
 * valor e o guarda no vetor de variáveis locais. (usa pop_operand
 * ou push_operand2)
 * @param ctx contextode execução Java
 * @param opc opcode
 * @return void altera a pilha (desempilhando) e escreve no vetor de 
 * variáveis locais do método corrente
 */
void handle_store(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes <t>astore (arrays)
 * @details Funcionamento: 
 * De acordo com o opcode, desempilha um valor, um índice 
 * e a referência para um array, e caso o array seja válido, 
 * armazena o valor na posição dada pelo índice. Pode desviar 
 * o fluxo para handle_athrow ao lançar uma exceção nativa  
 * ArrayIndexOutOfBounds ou NullPointer
 * @param ctx contexto de execução Java
 * @param opc opcode
 * @return void altera a pilha de operandos (pop_operand) e altera a 
 * tabela de objetos ao alterar conteúdos de Array
 */
void handle_tastore(JVM_Context* ctx, u1 opc);


/**
 * @brief Função que implementa opcode de tableswitch 
 * @detail Funcionamento:
 * carrega do bytecode um offset, um low, um high, e um índice, 
 * e de acordo com esses valores, obtém o offset de salto
 * @param ctx contexto de execução Java
 * @param opc opcode
 * @return void altera o pc do frame atual caso haja desvio dado 
 * pela tabela de desvios
 */
void handle_tableswitch(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode de lookupswitch 
 * @details Funcionamento:
 * obtem o offset padrão, o número de pares, a chava, e calcula o offset 
 * de acordo com a chave, procurando pelo bytecode até a posição dada pela 
 * chave, e desvia para o valor (offset) caso encontra a chave, ou para o 
 * offset no caso de não encontrar
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
 * @details Funcionamento
 * Por meio de um switch_case, efetua operações com dois valores da 
 * pilha de operandos e guarda o resultado na pilha de operandos do 
 * frame atual
 * @param ctx contexto de execução Java
 * @param opc opcode
 */
void handle_arithmetic(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de operações bitwise
 * (ishl-lxor)
 * @details Funcionamento:
 * Por meio de um switch case efetua uma operação bitwise com valores na 
 * pilha de operandos e empilha o resultado na mesma pilha de operandos
 * @param ctx contexto de execução Java
 * @param opc opcode (ishl-lxor)
 */
void handle_logic(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de conversão de tipo (i2l-d2f)
 * @param ctx contexto de execução JVM
 * @details Funcionamento:
 * internamente um switch_case que de acordo com o opcode, obtem o operando 
 * com cast adequado, pela pilha de operandos, e converte o operando para 
 * um novo tipo, empilhando-o na pilha de operandos de volta
 * @param opc opcode
 */
void handle_conversion(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode de comparação
 * de long (0x94)
 * @details Funcionamento:
 * obtem dois valores com pop_operand2 e cast para long, e compara os valores
 * para decidir se empilha 1 (v1 > v2), 0 (v1 == v2), -1 (v1 < v2) na pilha 
 * de operandos do frame
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_lcmp(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de comparação de valores 
 * em ponto flutuantee e double (0x95-0x97 fcmp, dcmp) 
 * @details Funcionamento:
 * Compara dois valores em ponto flutuante obtidos com pop_operand ou 
 * pop_operand2, com cast adequado, podendo empilhar 1 (v1>v2),
 * 0 (v1 == v2) ou -1 (caso de v1 < v2) na pilha de operandos
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_fdcmp(JVM_Context* ctx, u1 opc);



/**
 * @brief Função que implementa opcodes de operações de saltos 
 * condicionais if<conc> 0x99-0x9E
 * @details Funcionamento:
 * obtem um offset de salto (int32_t) com (cast)fetch_u2, e a partir de um 
 * valor (int32_t) desempilhado com pop_operand, salta caso a condição 
 * determinada pelo opcode seja antendida.
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_ifcond(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcodes de comparação de valores 
 * da pilha de operandos e salto condicional ao resultado 
 * (if_cmpeq - if_acmpne)
 * @details Funcionamento:
* obtem um offset com fetch_u2, e dois valores a partir da pilha. 
 * Compara os dois valores para decidir se o offset será somado ao pc 
 * da instrução para obter um novo pc.
 * @param ctx contexto de execução JVM
 * @param opc opcode
 */
void handle_ifcmp(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa opcode goto, jsr e ret (167-169)
 * @details Funcionamento:
 * Aplica desvio de acordo com o opcode. Em caso de goto, obtem um 
 * offset com fetch_u2, atualiza o pc (da instrução) somando ao offset, 
 * e retorna. Caso jsr, realiza o mesmo procedimento, mas antes de mudar o pc, 
 * salva o pc atual (próx instrução) na pilha de operandos. Caso opc_ret, faz 
 * o procedimento inverso do anterior, obtendo o pc de retorno por meio do 
 * vetor de variáveis locais (em caso de wide, 2 bytes, com fetch_u2), e 
 * atualiza o pc. goto_w e jsr_w também são implementados de forma análoga, 
 * porém com offset de 32 bits e não 16 bits
 * @param ctx contexto de execução da JVM
 * @param opc opcode (167-169)
 */
void handle_goto_jsr_ret(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa execução dos opcodes de retorno (172-177)
 * @details Funcionamento:
 * de acordo com o opcode, pode desempilhar ou não da pilha de operandos para 
 * obter um valor de retorno. E passa o valor de retorno (se aplicado) ao 
 * frame chamador, após desempilhar o frame atual, liberando toda sua memória
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
 * @details Funcionamento:
 * Obtém com fetch_u2 índice na constant pool para o método e o 
 * resolve com resolve_method. Caso o método seja print ou println de 
 * java/io/Printstream, a saída será simulada de forma adequada, e a função 
 * retorna. Caso contrário, uma referência é desempilhada, e em caso da 
 * referência ser 0, a execução é desviada para throw_native -> handle_athrow.
 * Após checagem de acesso com base na classe do objeto, uma nova resolução 
 * de método é efetuada a partir da classe do objeto para encontrar um possível
 * método sobrescrito. Após a segunda resolução, o RuntimeMethod final é usado 
 * para instanciar o novo frame com invoke_method
 * @param ctx contexto de execução da JVM
 * @param opc opcode (182)
 */    
void handle_invokevirtual(JVM_Context* ctx, u1 opc); // 182

/**
 * @brief Função que implementa instrução invokespecial 
 * @details obtém o índice para constant pool com fetch_u2, 
 * e usa resolve_method para encontrar o RuntimeMethod adequado.
 * Ao receber o método, checa se a classe é nativa, e em caso afirmativo,
 * desempilha um cat1 (referência de objeto) da pilha de operandos, pois se 
 * trata de <init> de classes não implementadas, mas cuja existência da 
 * estrutura LoadedClass é necessária para o funcionamento deste programa.
 * Caso não seja nativa (não java/...), invoca o método ou resolve o método 
 * com base na classe super da classe atual (caso em que invokespecial é uma 
 * chamada de super.<método>), ambas usando invoke_method
 * @param ctx contexto de execução da JVM
 * @param opc opcode (184)
 */
void handle_invokespecial(JVM_Context* ctx, u1 opc); //183

/**
 * @brief Função que implementa invokestatic (184)
 * @param ctx contexto de execução da JVM
 * @param opc opcode (184)
 * @details Funcionamento:
 * Obtém o índice da constant pool com fetch_u2, e resolve o método 
 * com resolve_method. A função anterior pode desviar para um erro fatal 
 * em caso de erro de acesso ou método não encontrado. Caso a classe 
 * do método não tenha sido inicializada, usa-se initialize_class e retorna 
 * o pc para o início da instrução invokestatic. Na próxima execução (ou caso 
 * a classe já esteja inicializada), usa invoke_method para empilhar o frame 
 * com o método invocado
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_invokestatic(JVM_Context* ctx, u1 opc); // 184

/**
 * @brief Função que implementa invokeinterface (185)
 * Para executar métodos de instância resultantes da 
 * implementação de interfaces
 * @details Funcionamento:
 * Obtem com fetch_u2 o índice da constant pool que aponta para um método 
 * de interface. Depois da resolução com resolve_method, uma referência é 
 * desempilhada, e uma segunda resolução é efetuada, a partir da classe 
 * do objeto da referência, para encontrar a implementação do método. 
 * Invoca o método em com invoke_method caso a resolução seja bem sucedida,
 * ou termina o programa com fatal_error, devido a "AbstractMethodError"
 * @param ctx contexto de execução da JVM
 * @param opc opcode (185)
 */    
void handle_invokeinterface(JVM_Context* ctx, u1 opc); // 184

/**
 * @brief Função que implementa new (instanciação de objetos)
 * @details Funcionamento: 
 * obtem o índice para uma classe na constant pool e resolve a classe 
 * com resolve_class. Se a clase for abstrata, levanta um erro fatal, 
 * caso a classe não seja inicializada, marca para inicialização, retorna 
 * o pc para o início da instrução e retorna da função. Quando a classe é
 * instanciada, chama new_object e empilha o valor de retorno (referencia u4)
 * na pilha de operandos do método corrente
 * @param ctx contexto de execução da JVM
 * @param opc opcode
 */    
void handle_new(JVM_Context* ctx, u1 opc); // 187

/**
 * @brief Função que implementa newarray 
 * (instanciação de arrays de tipos primitivos)
 * @details Funcionamento:
 * obtém o número de elementos do array da pilha de operandos, 
 * e o tipo do elemento por meio do bytecode. Em caso de count negativo, 
 * desvia o programa para throw_native (NegativeArraySizeException). 
 * Caso contrário, cria um novo Object com a configuração OBJ_ARRAY e tamanho 
 * inicializado e valores zerados
 * @param ctx contexto de execução da JVM
 * @param opc opcode (188) */    
void handle_newarray(JVM_Context* ctx, u1 opc); // 188

/**
 * @brief Função que implementa newarray 
 * (instanciação de arrays de referência)
 * @details Funcionamento:
 * análogo ao newarray, mas resolve uma classe por meio de um cp_idx
 * obtido com fetch_u2. Obtem a contagem de elementos e inicializa um array
 * de referências
 * @param ctx contexto de execução da JVM
 * @param opc opcode (189) */    
void handle_anewarray(JVM_Context* ctx, u1 opc); // 188

/**
 * @brief Função que implementa multianewarray 
 * (instanciação de arrays multidimensionais com 
 * cada dimensão já alocada)
 * @details Funcionamento:
 * A partir de um número de dimensões dados pelo bytecode, bem como 
 * o cp_idx com fetch_u2, obtem uma referencia de classe e cria um array 
 * com base nessa refereência, com um número de dimensões, cada uma com 
 * tamanhos inicializados. No final empilha a referencia com push_operand
 * @param ctx contexto de execução da JVM
 * @param opc opcode (189) */    
void handle_multianewarray(JVM_Context* ctx, u1 opc); // 189

/**
 * @brief Função que implementa arraylength  
 * (tamanho da array retornado na pilha como inteiro)
 * @details Funcionamento:
 * desempilha uma referência do array e procura o objeto. Se a referência 
 * for nula ou objeto inváido, desvia para throw_native (NullPointer). 
 * Se for válido, empilha o atributo length do campo Array do conteúdo do 
 * Object
 * @param ctx contexto de execução da JVM
 * @param opc opcode (190, 0xBE) */    
void handle_arraylength(JVM_Context* ctx, u1 opc); // 190

/**
 * @brief Função que implementa athrow.
 * @details Funcionamento:
 * Espera uma referência para objeto de exceção no topo da pilha.
 * Busca por um catch no método corrente, e caso encontre-o, a pilha 
 * de operandos é zerada para ter apenas a referência à exceção, e o 
 * pc é mudado para handler_pc. Em caso não encontrar, o frame é desempilhado
 * e o processo reinicia até encontrar o catch ou encerrar o programa.
 * Essa função pode ser chamada fora de jvm_run, para lançamento de exceções 
 * nativas, desde que a referência ao objeto tenha sido empilhada.
 * @param ctx contexto de execução da JVM
 * @param opc opcode (191) */   
void handle_athrow(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa checkcast
 * @details Funcionamento:
 * obtem uma classe a partir da resolução de um índice para constant pool, 
 * e verifica se a referencia do topo da pilha, sem desempilhar, é de uma 
 * classe que herda ou é igual a classe resolvida, ou se a referência é nula. 
 * Caso a referência não seja null, e a classe da referência não herda da 
 * classe resolvida, um ClassCastException é lançado
 * @param ctx contexto de execução da JVM
 * @param opc opcode (192) */   
void handle_checkcast(JVM_Context* ctx, u1 opc);

/**
 * @brief Função que implementa instanceof 
 * @param ctx contexto de execução da JVM
 * @param opc opcode (193) */   
void handle_instanceof(JVM_Context* ctx, u1 opc);

#endif

