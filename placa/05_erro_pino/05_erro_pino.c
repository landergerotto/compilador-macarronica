/* Macaronica: C99 gerado automaticamente da AST anotada. */
#include <stdint.h>
#include <limits.h>
#include "zl_runtime.h"

/* zl_erro_execucao nao retorna (como o noreturn da IR). Sem isso,
 * com otimizacao o gcc trata o acesso logo depois da checagem de
 * indice como alcancavel e pode usar o comportamento indefinido
 * dele para tirar a checagem do laco (-Waggressive-loop-optimizations). */
#if defined(__GNUC__)
void zl_erro_execucao(int32_t codigo, int32_t linha) __attribute__((noreturn));
#endif

/* inteira = int64_t. A conta inteira e feita em uint64_t (modulo
 * 2^64) e volta a int64_t sem conversao fora da faixa, que em C
 * dependeria do compilador. */
static inline int64_t mac_i64(uint64_t value)
{
  return value <= (uint64_t)INT64_MAX ? (int64_t)value :
      -1 - (int64_t)(UINT64_MAX - value);
}

/* Status de zl_principal: 32 bits baixos, como trunc i64 to i32. */
static inline int32_t mac_status(int64_t value)
{
  uint32_t low = (uint32_t)((uint64_t)value & UINT32_MAX);
  return low <= (uint32_t)INT32_MAX ? (int32_t)low :
      -1 - (int32_t)(UINT32_MAX - low);
}

/* Argumento int32_t dos servicos da placa: satura fora da faixa. */
static inline int32_t mac_arg_i32(int64_t value)
{
  return value > INT32_MAX ? INT32_MAX :
      value < INT32_MIN ? INT32_MIN : (int32_t)value;
}


int32_t zl_principal(void);

int32_t zl_principal(void)
{
  /* linha 2 */
  /* linha 3 */
  int64_t mac_t_2 = INT64_C(2);
  int64_t mac_t_3 = INT64_C(1);
  zl_pino_modo(mac_arg_i32(mac_t_2), mac_arg_i32(mac_t_3), 3);
  /* linha 4 */
  int64_t mac_t_6 = INT64_C(2);
  int32_t mac_t_7 = 1;
  zl_pino_escreve(mac_arg_i32(mac_t_6), mac_t_7, 4);
  /* linha 5 */
  const char * mac_t_10 = "\107\120\111\117\062\040\143\157\156\146\151\147\165\162\141\144\157\040\143\157\155\157\040\163\141\151\144\141";
  zl_imprime_texto(mac_t_10);
  /* linha 6 */
  zl_imprime_nl();
  /* linha 7 */
  int64_t mac_t_15 = INT64_C(4);
  int32_t mac_t_16 = 1;
  zl_pino_escreve(mac_arg_i32(mac_t_15), mac_t_16, 7);
  /* linha 8 */
  const char * mac_t_19 = "\156\141\157\040\143\150\145\147\141\040\141\161\165\151";
  zl_imprime_texto(mac_t_19);
  /* linha 9 */
  zl_imprime_nl();
  /* linha 10 */
  int64_t mac_t_22 = INT64_C(0);
  return mac_status(mac_t_22);
  return 0;
}

