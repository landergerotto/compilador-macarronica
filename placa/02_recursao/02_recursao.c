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


int64_t mac_fn_13(int64_t mac_v_16);
int64_t mac_fn_14(int64_t mac_v_17);
int32_t zl_principal(void);

int64_t mac_fn_13(int64_t mac_v_16)
{
  (void)mac_v_16;
  /* linha 2 */
  /* linha 3 */
  int64_t *mac_p_2 = &mac_v_16;
  int64_t mac_t_1 = *mac_p_2;
  int64_t mac_t_4 = INT64_C(2);
  int32_t mac_t_3 = mac_t_1 < mac_t_4;
  if (mac_t_3) {
  /* linha 4 */
  /* linha 5 */
  int64_t mac_t_5 = INT64_C(1);
  return mac_t_5;
  } else {
  /* linha 8 */
  /* linha 9 */
  int64_t *mac_p_8 = &mac_v_16;
  int64_t mac_t_7 = *mac_p_8;
  int64_t *mac_p_14 = &mac_v_16;
  int64_t mac_t_13 = *mac_p_14;
  int64_t mac_t_16 = INT64_C(1);
  int64_t mac_t_15 = mac_i64((uint64_t)mac_t_13 - (uint64_t)mac_t_16);
  int64_t mac_t_11 = mac_fn_13(mac_t_15);
  int64_t mac_t_9 = mac_i64((uint64_t)mac_t_7 * (uint64_t)mac_t_11);
  return mac_t_9;
  }
  return (int64_t){0};
}

int64_t mac_fn_14(int64_t mac_v_17)
{
  (void)mac_v_17;
  /* linha 14 */
  /* linha 15 */
  int64_t *mac_p_2 = &mac_v_17;
  int64_t mac_t_1 = *mac_p_2;
  int64_t mac_t_4 = INT64_C(2);
  int32_t mac_t_3 = mac_t_1 < mac_t_4;
  if (mac_t_3) {
  /* linha 16 */
  /* linha 17 */
  int64_t *mac_p_6 = &mac_v_17;
  int64_t mac_t_5 = *mac_p_6;
  return mac_t_5;
  } else {
  /* linha 20 */
  /* linha 21 */
  int64_t *mac_p_12 = &mac_v_17;
  int64_t mac_t_11 = *mac_p_12;
  int64_t mac_t_14 = INT64_C(1);
  int64_t mac_t_13 = mac_i64((uint64_t)mac_t_11 - (uint64_t)mac_t_14);
  int64_t mac_t_9 = mac_fn_14(mac_t_13);
  int64_t *mac_p_20 = &mac_v_17;
  int64_t mac_t_19 = *mac_p_20;
  int64_t mac_t_22 = INT64_C(2);
  int64_t mac_t_21 = mac_i64((uint64_t)mac_t_19 - (uint64_t)mac_t_22);
  int64_t mac_t_17 = mac_fn_14(mac_t_21);
  int64_t mac_t_15 = mac_i64((uint64_t)mac_t_9 + (uint64_t)mac_t_17);
  return mac_t_15;
  }
  return (int64_t){0};
}

int32_t zl_principal(void)
{
  int64_t mac_v_18 = {0};
  (void)mac_v_18;
  /* linha 26 */
  /* linha 27 */
  /* linha 28 */
  /* linha 28 */
  int64_t *mac_p_0 = &mac_v_18;
  int64_t mac_t_1 = INT64_C(0);
  *mac_p_0 = mac_t_1;
  while (1) {
  int64_t *mac_p_4 = &mac_v_18;
  int64_t mac_t_3 = *mac_p_4;
  int64_t mac_t_6 = INT64_C(21);
  int32_t mac_t_5 = mac_t_3 < mac_t_6;
  if (!mac_t_5) break;
  /* linha 29 */
  /* linha 30 */
  const char * mac_t_9 = "\146\141\164\157\162\151\141\154\050";
  zl_imprime_texto(mac_t_9);
  /* linha 31 */
  int64_t *mac_p_13 = &mac_v_18;
  int64_t mac_t_12 = *mac_p_13;
  zl_imprime_int(mac_t_12);
  /* linha 32 */
  const char * mac_t_16 = "\051\040\075\040";
  zl_imprime_texto(mac_t_16);
  /* linha 33 */
  int64_t *mac_p_22 = &mac_v_18;
  int64_t mac_t_21 = *mac_p_22;
  int64_t mac_t_20 = mac_fn_13(mac_t_21);
  zl_imprime_int(mac_t_20);
  /* linha 34 */
  zl_imprime_nl();
  /* linha 28 */
  int64_t *mac_p_25 = &mac_v_18;
  int64_t *mac_p_28 = &mac_v_18;
  int64_t mac_t_27 = *mac_p_28;
  int64_t mac_t_30 = INT64_C(5);
  int64_t mac_t_29 = mac_i64((uint64_t)mac_t_27 + (uint64_t)mac_t_30);
  *mac_p_25 = mac_t_29;
  }
  /* linha 36 */
  const char * mac_t_33 = "\146\151\142\157\156\141\143\143\151\050\062\060\051\040\075\040";
  zl_imprime_texto(mac_t_33);
  /* linha 37 */
  int64_t mac_t_38 = INT64_C(20);
  int64_t mac_t_37 = mac_fn_14(mac_t_38);
  zl_imprime_int(mac_t_37);
  /* linha 38 */
  zl_imprime_nl();
  /* linha 39 */
  int64_t mac_t_41 = INT64_C(0);
  return mac_status(mac_t_41);
  return 0;
}

