#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdio.h>

typedef uint8_t  u1;  
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

/**
 * @brief Estrutura de leitura de buffer 
 */
typedef struct {
  u1 *buf; /**< Array contendo os bytes de leitura */
  u4 size; /**< Tamanho de buf */
  u4 pos; /**< Posição a ser lida */
} Reader;

/**
 * @brief retorna byte lido do Reader e 
 * avança posição de leitura em 1
 * @param r ponteiro para o Reader 
 * @return byte lido
 */
static inline u1 read_u1(Reader *r) {
  return r->buf[r->pos++];
}

/**
 * @brief retorna word (16 bits) lido do Reader e 
 * avança posição de leitura em 2
 * @param r ponteiro para o Reader 
 * @return u2 lido
 */
static inline u2 read_u2(Reader *r) {
  u2 v = ((u2)r->buf[r->pos] << 8) |
         ((u2)r->buf[r->pos + 1]);
  r->pos += 2;
  return v;
}

/**
 * @brief retorna word (16 bits) lido do Reader e 
 * avança posição de leitura em 4
 * @param r ponteiro para o Reader 
 * @return u4 lido
 */
static inline u4 read_u4(Reader *r) {
  u4 v = ((u4)r->buf[r->pos]     << 24) |
         ((u4)r->buf[r->pos + 1] << 16) |
         ((u4)r->buf[r->pos + 2] << 8)  |
          (u4)r->buf[r->pos + 3];
  r->pos += 4;
  return v;
}

/**
 * @brief carrega um arquivo binário, retornando buffer do conteúdo
 * e mudnado valor do parâmetro size com o tamanho do arquivo em bytes 
 * @param f descritor do arquivo a ser lido 
 * @param size ponteiro para u4 que armazenará tamanho obtido do arquivo 
 * @return ponteiro para buffer de bytes do arquivo
 */
u1 *load_file(FILE *f, u4 *size); 

#endif
