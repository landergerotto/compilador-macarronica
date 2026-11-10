; Macaronica: LLVM IR textual portatil


declare void @zl_imprime_int(i32) nounwind
declare void @zl_imprime_flut(float) nounwind
declare void @zl_imprime_duplocarpado(double) nounwind
declare void @zl_imprime_texto(ptr) nounwind
declare void @zl_imprime_bool(i32) nounwind
declare void @zl_imprime_nl() nounwind
declare i32 @zl_le_int(i32) nounwind
declare float @zl_le_flut(i32) nounwind
declare double @zl_le_duplocarpado(i32) nounwind
declare i32 @zl_texto_igual(ptr, ptr) nounwind
declare void @zl_erro_execucao(i32, i32) noreturn nounwind
declare i32 @zl_potencia_int(i32, i32, i32) nounwind
declare float @zl_potencia_flut(float, float, i32) nounwind
declare double @zl_potencia_duplocarpado(double, double, i32) nounwind
declare float @zl_raiz_flut(float, i32) nounwind
declare double @zl_raiz_duplocarpado(double, i32) nounwind

define i32 @zl_principal() nounwind {
entry:
  ; linha 2
  ; linha 3
  ret i32 0
}

