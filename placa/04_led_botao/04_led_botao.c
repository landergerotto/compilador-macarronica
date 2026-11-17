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
  int64_t mac_v_14 = {0};
  (void)mac_v_14;
  /* linha 2 */
  /* linha 3 */
  /* linha 4 */
  int64_t mac_t_2 = INT64_C(2);
  int64_t mac_t_3 = INT64_C(1);
  zl_pino_modo(mac_arg_i32(mac_t_2), mac_arg_i32(mac_t_3), 4);
  /* linha 5 */
  int64_t mac_t_6 = INT64_C(0);
  int64_t mac_t_7 = INT64_C(2);
  zl_pino_modo(mac_arg_i32(mac_t_6), mac_arg_i32(mac_t_7), 5);
  /* linha 6 */
  /* linha 6 */
  int64_t *mac_p_8 = &mac_v_14;
  int64_t mac_t_9 = INT64_C(1);
  *mac_p_8 = mac_t_9;
  while (1) {
  int64_t *mac_p_12 = &mac_v_14;
  int64_t mac_t_11 = *mac_p_12;
  int64_t mac_t_14 = INT64_C(6);
  int32_t mac_t_13 = mac_t_11 < mac_t_14;
  if (!mac_t_13) break;
  /* linha 7 */
  /* linha 8 */
  int64_t mac_t_17 = INT64_C(2);
  int32_t mac_t_18 = 1;
  zl_pino_escreve(mac_arg_i32(mac_t_17), mac_t_18, 8);
  /* linha 9 */
  const char * mac_t_21 = "\160\151\163\143\141\144\141\040";
  zl_imprime_texto(mac_t_21);
  /* linha 10 */
  int64_t *mac_p_25 = &mac_v_14;
  int64_t mac_t_24 = *mac_p_25;
  zl_imprime_int(mac_t_24);
  /* linha 11 */
  const char * mac_t_28 = "\072\040\114\105\104\040\141\172\165\154\040\141\143\145\163\157";
  zl_imprime_texto(mac_t_28);
  /* linha 12 */
  zl_imprime_nl();
  /* linha 13 */
  int64_t mac_t_33 = INT64_C(300);
  zl_espera_ms(mac_arg_i32(mac_t_33), 13);
  /* linha 14 */
  int64_t mac_t_36 = INT64_C(2);
  int32_t mac_t_37 = 0;
  zl_pino_escreve(mac_arg_i32(mac_t_36), mac_t_37, 14);
  /* linha 15 */
  int64_t mac_t_40 = INT64_C(300);
  zl_espera_ms(mac_arg_i32(mac_t_40), 15);
  /* linha 6 */
  int64_t *mac_p_41 = &mac_v_14;
  int64_t *mac_p_44 = &mac_v_14;
  int64_t mac_t_43 = *mac_p_44;
  int64_t mac_t_46 = INT64_C(1);
  int64_t mac_t_45 = mac_i64((uint64_t)mac_t_43 + (uint64_t)mac_t_46);
  *mac_p_41 = mac_t_45;
  }
  /* linha 17 */
  const char * mac_t_49 = "\142\157\164\141\157\040\102\117\117\124\040\163\157\154\164\157\040\050\107\120\111\117\060\040\143\157\155\040\160\165\154\154\055\165\160\051\072\040";
  zl_imprime_texto(mac_t_49);
  /* linha 18 */
  int64_t mac_t_54 = INT64_C(0);
  int32_t mac_t_53 = zl_pino_le(mac_arg_i32(mac_t_54), 18);
  zl_imprime_bool(mac_t_53);
  /* linha 19 */
  zl_imprime_nl();
  /* linha 20 */
  int64_t mac_t_57 = INT64_C(0);
  return mac_status(mac_t_57);
  return 0;
}

