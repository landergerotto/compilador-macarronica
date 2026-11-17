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


int64_t mac_g_15[10] = {0};
int64_t mac_fn_13(int64_t mac_v_16);
int32_t zl_principal(void);

int64_t mac_fn_13(int64_t mac_v_16)
{
  (void)mac_v_16;
  int64_t mac_v_17 = {0};
  (void)mac_v_17;
  int64_t mac_v_18 = {0};
  (void)mac_v_18;
  /* linha 4 */
  /* linha 5 */
  /* linha 6 */
  int64_t *mac_p_0 = &mac_v_18;
  int64_t mac_t_1 = INT64_C(0);
  *mac_p_0 = mac_t_1;
  /* linha 7 */
  /* linha 7 */
  int64_t *mac_p_2 = &mac_v_17;
  int64_t mac_t_3 = INT64_C(0);
  *mac_p_2 = mac_t_3;
  while (1) {
  int64_t *mac_p_6 = &mac_v_17;
  int64_t mac_t_5 = *mac_p_6;
  int64_t *mac_p_9 = &mac_v_16;
  int64_t mac_t_8 = *mac_p_9;
  int32_t mac_t_7 = mac_t_5 < mac_t_8;
  if (!mac_t_7) break;
  /* linha 8 */
  /* linha 9 */
  int64_t *mac_p_10 = &mac_v_18;
  int64_t *mac_p_13 = &mac_v_18;
  int64_t mac_t_12 = *mac_p_13;
  int64_t *mac_p_18 = &mac_v_17;
  int64_t mac_t_17 = *mac_p_18;
  if (mac_t_17 < 0 || mac_t_17 >= 10) zl_erro_execucao(3, 9);
  int64_t *mac_p_16 = &mac_g_15[mac_t_17];
  int64_t mac_t_15 = *mac_p_16;
  int64_t mac_t_14 = mac_i64((uint64_t)mac_t_12 + (uint64_t)mac_t_15);
  *mac_p_10 = mac_t_14;
  /* linha 7 */
  int64_t *mac_p_19 = &mac_v_17;
  int64_t *mac_p_22 = &mac_v_17;
  int64_t mac_t_21 = *mac_p_22;
  int64_t mac_t_24 = INT64_C(1);
  int64_t mac_t_23 = mac_i64((uint64_t)mac_t_21 + (uint64_t)mac_t_24);
  *mac_p_19 = mac_t_23;
  }
  /* linha 11 */
  int64_t *mac_p_26 = &mac_v_18;
  int64_t mac_t_25 = *mac_p_26;
  return mac_t_25;
  return (int64_t){0};
}

int32_t zl_principal(void)
{
  int64_t mac_v_19 = {0};
  (void)mac_v_19;
  int64_t mac_v_20[10] = {0};
  (void)mac_v_20;
  /* linha 15 */
  /* linha 16 */
  /* linha 17 */
  /* linha 18 */
  /* linha 18 */
  int64_t *mac_p_0 = &mac_v_19;
  int64_t mac_t_1 = INT64_C(0);
  *mac_p_0 = mac_t_1;
  while (1) {
  int64_t *mac_p_4 = &mac_v_19;
  int64_t mac_t_3 = *mac_p_4;
  int64_t mac_t_6 = INT64_C(10);
  int32_t mac_t_5 = mac_t_3 < mac_t_6;
  if (!mac_t_5) break;
  /* linha 19 */
  /* linha 20 */
  int64_t *mac_p_9 = &mac_v_19;
  int64_t mac_t_8 = *mac_p_9;
  if (mac_t_8 < 0 || mac_t_8 >= 10) zl_erro_execucao(3, 20);
  int64_t *mac_p_7 = &mac_g_15[mac_t_8];
  int64_t *mac_p_12 = &mac_v_19;
  int64_t mac_t_11 = *mac_p_12;
  int64_t *mac_p_15 = &mac_v_19;
  int64_t mac_t_14 = *mac_p_15;
  int64_t mac_t_13 = mac_i64((uint64_t)mac_t_11 * (uint64_t)mac_t_14);
  *mac_p_7 = mac_t_13;
  /* linha 21 */
  int64_t *mac_p_18 = &mac_v_19;
  int64_t mac_t_17 = *mac_p_18;
  if (mac_t_17 < 0 || mac_t_17 >= 10) zl_erro_execucao(3, 21);
  int64_t *mac_p_16 = &mac_v_20[mac_t_17];
  int64_t mac_t_20 = INT64_C(2);
  int64_t *mac_p_23 = &mac_v_19;
  int64_t mac_t_22 = *mac_p_23;
  int64_t mac_t_21 = mac_i64((uint64_t)mac_t_20 * (uint64_t)mac_t_22);
  *mac_p_16 = mac_t_21;
  /* linha 18 */
  int64_t *mac_p_24 = &mac_v_19;
  int64_t *mac_p_27 = &mac_v_19;
  int64_t mac_t_26 = *mac_p_27;
  int64_t mac_t_29 = INT64_C(1);
  int64_t mac_t_28 = mac_i64((uint64_t)mac_t_26 + (uint64_t)mac_t_29);
  *mac_p_24 = mac_t_28;
  }
  /* linha 23 */
  /* linha 23 */
  int64_t *mac_p_30 = &mac_v_19;
  int64_t mac_t_31 = INT64_C(0);
  *mac_p_30 = mac_t_31;
  while (1) {
  int64_t *mac_p_34 = &mac_v_19;
  int64_t mac_t_33 = *mac_p_34;
  int64_t mac_t_36 = INT64_C(10);
  int32_t mac_t_35 = mac_t_33 < mac_t_36;
  if (!mac_t_35) break;
  /* linha 24 */
  /* linha 25 */
  int64_t *mac_p_42 = &mac_v_19;
  int64_t mac_t_41 = *mac_p_42;
  if (mac_t_41 < 0 || mac_t_41 >= 10) zl_erro_execucao(3, 25);
  int64_t *mac_p_40 = &mac_g_15[mac_t_41];
  int64_t mac_t_39 = *mac_p_40;
  zl_imprime_int(mac_t_39);
  /* linha 26 */
  const char * mac_t_45 = "\040";
  zl_imprime_texto(mac_t_45);
  /* linha 23 */
  int64_t *mac_p_46 = &mac_v_19;
  int64_t *mac_p_49 = &mac_v_19;
  int64_t mac_t_48 = *mac_p_49;
  int64_t mac_t_51 = INT64_C(1);
  int64_t mac_t_50 = mac_i64((uint64_t)mac_t_48 + (uint64_t)mac_t_51);
  *mac_p_46 = mac_t_50;
  }
  /* linha 28 */
  zl_imprime_nl();
  /* linha 29 */
  const char * mac_t_56 = "\163\157\155\141\040\144\157\163\040\161\165\141\144\162\141\144\157\163\072\040";
  zl_imprime_texto(mac_t_56);
  /* linha 30 */
  int64_t mac_t_61 = INT64_C(10);
  int64_t mac_t_60 = mac_fn_13(mac_t_61);
  zl_imprime_int(mac_t_60);
  /* linha 31 */
  zl_imprime_nl();
  /* linha 32 */
  const char * mac_t_66 = "\165\154\164\151\155\157\040\144\157\142\162\157\072\040";
  zl_imprime_texto(mac_t_66);
  /* linha 33 */
  int64_t mac_t_71 = INT64_C(9);
  if (mac_t_71 < 0 || mac_t_71 >= 10) zl_erro_execucao(3, 33);
  int64_t *mac_p_70 = &mac_v_20[mac_t_71];
  int64_t mac_t_69 = *mac_p_70;
  zl_imprime_int(mac_t_69);
  /* linha 34 */
  zl_imprime_nl();
  /* linha 35 */
  int64_t mac_t_74 = INT64_C(0);
  return mac_status(mac_t_74);
  return 0;
}

