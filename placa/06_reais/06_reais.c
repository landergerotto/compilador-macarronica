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
  float mac_v_14 = {0};
  (void)mac_v_14;
  float mac_v_15 = {0};
  (void)mac_v_15;
  float mac_v_16 = {0};
  (void)mac_v_16;
  double mac_v_17 = {0};
  (void)mac_v_17;
  double mac_v_18 = {0};
  (void)mac_v_18;
  double mac_v_19 = {0};
  (void)mac_v_19;
  double mac_v_20 = {0};
  (void)mac_v_20;
  double mac_v_21 = {0};
  (void)mac_v_21;
  /* linha 2 */
  /* linha 3 */
  /* linha 4 */
  /* linha 5 */
  float *mac_p_0 = &mac_v_14;
  int64_t mac_t_1 = INT64_C(16777217);
  float mac_t_2 = (float)mac_t_1;
  *mac_p_0 = mac_t_2;
  /* linha 6 */
  float *mac_p_6 = &mac_v_14;
  float mac_t_5 = *mac_p_6;
  zl_imprime_flut(mac_t_5);
  /* linha 7 */
  zl_imprime_nl();
  /* linha 8 */
  float *mac_p_9 = &mac_v_15;
  int64_t mac_t_10 = INT64_C(1);
  float mac_t_11 = (float)mac_t_10;
  *mac_p_9 = mac_t_11;
  /* linha 9 */
  float *mac_p_12 = &mac_v_16;
  int64_t mac_t_13 = INT64_C(3);
  float mac_t_14 = (float)mac_t_13;
  *mac_p_12 = mac_t_14;
  /* linha 10 */
  float *mac_p_19 = &mac_v_15;
  float mac_t_18 = *mac_p_19;
  float *mac_p_22 = &mac_v_16;
  float mac_t_21 = *mac_p_22;
  if (mac_t_21 == 0) zl_erro_execucao(ZL_ERRO_DIVISAO_ZERO, 10);
  float mac_t_20 = mac_t_18 / mac_t_21;
  zl_imprime_flut(mac_t_20);
  /* linha 11 */
  zl_imprime_nl();
  /* linha 12 */
  double *mac_p_25 = &mac_v_18;
  int64_t mac_t_26 = INT64_C(1);
  double mac_t_27 = (double)mac_t_26;
  *mac_p_25 = mac_t_27;
  /* linha 13 */
  double *mac_p_28 = &mac_v_19;
  int64_t mac_t_29 = INT64_C(3);
  double mac_t_30 = (double)mac_t_29;
  *mac_p_28 = mac_t_30;
  /* linha 14 */
  double *mac_p_31 = &mac_v_17;
  double *mac_p_34 = &mac_v_18;
  double mac_t_33 = *mac_p_34;
  double *mac_p_37 = &mac_v_19;
  double mac_t_36 = *mac_p_37;
  if (mac_t_36 == 0) zl_erro_execucao(ZL_ERRO_DIVISAO_ZERO, 14);
  double mac_t_35 = mac_t_33 / mac_t_36;
  *mac_p_31 = mac_t_35;
  /* linha 15 */
  double *mac_p_41 = &mac_v_17;
  double mac_t_40 = *mac_p_41;
  zl_imprime_duplocarpado(mac_t_40);
  /* linha 16 */
  zl_imprime_nl();
  /* linha 17 */
  double *mac_p_44 = &mac_v_20;
  int64_t mac_t_45 = INT64_C(2);
  double mac_t_46 = (double)mac_t_45;
  *mac_p_44 = mac_t_46;
  /* linha 18 */
  double *mac_p_52 = &mac_v_20;
  double mac_t_51 = *mac_p_52;
  double mac_t_50 = zl_raiz_duplocarpado(mac_t_51, 18);
  zl_imprime_duplocarpado(mac_t_50);
  /* linha 19 */
  zl_imprime_nl();
  /* linha 20 */
  double *mac_p_55 = &mac_v_21;
  int64_t mac_t_56 = INT64_C(10);
  double mac_t_57 = (double)mac_t_56;
  *mac_p_55 = mac_t_57;
  /* linha 21 */
  double *mac_p_62 = &mac_v_20;
  double mac_t_61 = *mac_p_62;
  double *mac_p_65 = &mac_v_21;
  double mac_t_64 = *mac_p_65;
  double mac_t_63 = zl_potencia_duplocarpado(mac_t_61, mac_t_64, 21);
  zl_imprime_duplocarpado(mac_t_63);
  /* linha 22 */
  zl_imprime_nl();
  /* linha 23 */
  double *mac_p_72 = &mac_v_17;
  double mac_t_71 = *mac_p_72;
  double *mac_p_75 = &mac_v_19;
  double mac_t_74 = *mac_p_75;
  double mac_t_73 = mac_t_71 * mac_t_74;
  zl_imprime_duplocarpado(mac_t_73);
  /* linha 24 */
  zl_imprime_nl();
  /* linha 25 */
  int64_t mac_t_81 = INT64_C(2);
  int64_t mac_t_83 = INT64_C(62);
  int64_t mac_t_82 = zl_potencia_int(mac_t_81, mac_t_83, 25);
  zl_imprime_int(mac_t_82);
  /* linha 26 */
  zl_imprime_nl();
  /* linha 27 */
  double *mac_p_91 = &mac_v_17;
  double mac_t_90 = *mac_p_91;
  double *mac_p_94 = &mac_v_19;
  double mac_t_93 = *mac_p_94;
  double mac_t_92 = mac_t_90 * mac_t_93;
  double *mac_p_97 = &mac_v_18;
  double mac_t_96 = *mac_p_97;
  int32_t mac_t_95 = mac_t_92 > mac_t_96;
  zl_imprime_bool(mac_t_95);
  /* linha 28 */
  zl_imprime_nl();
  /* linha 29 */
  int64_t mac_t_100 = INT64_C(0);
  return mac_status(mac_t_100);
  return 0;
}

