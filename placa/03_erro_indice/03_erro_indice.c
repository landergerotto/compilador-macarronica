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
  int64_t mac_v_14[3] = {0};
  (void)mac_v_14;
  int64_t mac_v_15 = {0};
  (void)mac_v_15;
  /* linha 2 */
  /* linha 3 */
  /* linha 4 */
  /* linha 5 */
  /* linha 5 */
  int64_t *mac_p_0 = &mac_v_15;
  int64_t mac_t_1 = INT64_C(0);
  *mac_p_0 = mac_t_1;
  while (1) {
  int64_t *mac_p_4 = &mac_v_15;
  int64_t mac_t_3 = *mac_p_4;
  int64_t mac_t_6 = INT64_C(4);
  int32_t mac_t_5 = mac_t_3 < mac_t_6;
  if (!mac_t_5) break;
  /* linha 6 */
  /* linha 7 */
  int64_t *mac_p_9 = &mac_v_15;
  int64_t mac_t_8 = *mac_p_9;
  if (mac_t_8 < 0 || mac_t_8 >= 3) zl_erro_execucao(3, 7);
  int64_t *mac_p_7 = &mac_v_14[mac_t_8];
  int64_t *mac_p_12 = &mac_v_15;
  int64_t mac_t_11 = *mac_p_12;
  int64_t mac_t_14 = INT64_C(10);
  int64_t mac_t_13 = mac_i64((uint64_t)mac_t_11 * (uint64_t)mac_t_14);
  *mac_p_7 = mac_t_13;
  /* linha 8 */
  int64_t *mac_p_20 = &mac_v_15;
  int64_t mac_t_19 = *mac_p_20;
  if (mac_t_19 < 0 || mac_t_19 >= 3) zl_erro_execucao(3, 8);
  int64_t *mac_p_18 = &mac_v_14[mac_t_19];
  int64_t mac_t_17 = *mac_p_18;
  zl_imprime_int(mac_t_17);
  /* linha 9 */
  zl_imprime_nl();
  /* linha 5 */
  int64_t *mac_p_23 = &mac_v_15;
  int64_t *mac_p_26 = &mac_v_15;
  int64_t mac_t_25 = *mac_p_26;
  int64_t mac_t_28 = INT64_C(1);
  int64_t mac_t_27 = mac_i64((uint64_t)mac_t_25 + (uint64_t)mac_t_28);
  *mac_p_23 = mac_t_27;
  }
  /* linha 11 */
  const char * mac_t_31 = "\156\141\157\040\143\150\145\147\141\040\141\161\165\151";
  zl_imprime_texto(mac_t_31);
  /* linha 12 */
  zl_imprime_nl();
  /* linha 13 */
  int64_t mac_t_34 = INT64_C(0);
  return mac_status(mac_t_34);
  return 0;
}

