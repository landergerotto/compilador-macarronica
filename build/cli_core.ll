; Macaronica: LLVM IR textual portatil

%struct.Par = type { i32, ptr }
; global global
@mac.global.14 = global i32 zeroinitializer
@.str.39 = private unnamed_addr constant [5 x i8] c"\4F\6C\61\0A\00"
@.str.43 = private unnamed_addr constant [5 x i8] c"\4F\6C\61\0A\00"

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

define double @mac.soma(double %arg.0, double %arg.1) nounwind {
entry:
  ; a
  %v.15.addr = alloca double
  ; b
  %v.16.addr = alloca double
  store double %arg.0, ptr %v.15.addr
  store double %arg.1, ptr %v.16.addr
  ; linha 4
  ; linha 5
  %t.0 = load double, ptr %v.15.addr
  %t.1 = load double, ptr %v.16.addr
  %t.2 = fadd double %t.0, %t.1
  ret double %t.2
}

define i32 @zl_principal() nounwind {
entry:
  ; a
  %v.17.addr = alloca i32
  ; f
  %v.18.addr = alloca float
  ; d
  %v.19.addr = alloca double
  ; texto
  %v.20.addr = alloca ptr
  ; b
  %v.21.addr = alloca i1
  ; par
  %v.22.addr = alloca %struct.Par
  store i32 zeroinitializer, ptr %v.17.addr
  store float zeroinitializer, ptr %v.18.addr
  store double zeroinitializer, ptr %v.19.addr
  store ptr zeroinitializer, ptr %v.20.addr
  store i1 zeroinitializer, ptr %v.21.addr
  store %struct.Par zeroinitializer, ptr %v.22.addr
  ; linha 8
  ; linha 9
  ; linha 10
  ; linha 11
  ; linha 12
  ; linha 13
  ; linha 14
  ; linha 15
  store i32 -2147483648, ptr %v.17.addr
  ; linha 16
  %t.1 = icmp eq i32 2, 0
  br i1 %t.1, label %erro.0, label %continua.0
erro.0:
  call void @zl_erro_execucao(i32 1, i32 16)
  unreachable
continua.0:
  %t.2 = icmp eq i32 8, -2147483648
  %t.3 = icmp eq i32 2, -1
  %t.4 = and i1 %t.2, %t.3
  br i1 %t.4, label %erro.1, label %continua.1
erro.1:
  call void @zl_erro_execucao(i32 2, i32 16)
  unreachable
continua.1:
  %t.0 = sdiv i32 8, 2
  store i32 %t.0, ptr %v.17.addr
  ; linha 17
  store float 0x3FB99999A0000000, ptr %v.18.addr
  ; linha 18
  %t.5 = load i32, ptr %v.17.addr
  %t.6 = sitofp i32 %t.5 to double
  %t.7 = load float, ptr %v.18.addr
  %t.8 = fpext float %t.7 to double
  %t.9 = call double @mac.soma(double %t.6, double %t.8)
  store double %t.9, ptr %v.19.addr
  ; linha 19
  %t.10 = getelementptr [5 x i8], ptr @.str.39, i32 0, i32 0
  store ptr %t.10, ptr %v.20.addr
  ; linha 20
  %t.11 = load ptr, ptr %v.20.addr
  %t.12 = getelementptr [5 x i8], ptr @.str.43, i32 0, i32 0
  %t.14 = call i32 @zl_texto_igual(ptr %t.11, ptr %t.12)
  %t.13 = icmp ne i32 %t.14, 0
  %t.15 = xor i1 %t.13, false
  store i1 %t.15, ptr %v.21.addr
  ; linha 21
  %t.16 = load i32, ptr %v.17.addr
  call void @zl_imprime_int(i32 %t.16)
  ; linha 22
  %t.17 = load float, ptr %v.18.addr
  %t.19 = fcmp oeq float 0x4000000000000000, 0.000000e+00
  br i1 %t.19, label %erro.2, label %continua.2
erro.2:
  call void @zl_erro_execucao(i32 1, i32 22)
  unreachable
continua.2:
  %t.18 = fdiv float %t.17, 0x4000000000000000
  call void @zl_imprime_flut(float %t.18)
  ; linha 23
  %t.20 = load double, ptr %v.19.addr
  call void @zl_imprime_duplocarpado(double %t.20)
  ; linha 24
  %t.21 = load ptr, ptr %v.20.addr
  call void @zl_imprime_texto(ptr %t.21)
  ; linha 25
  %t.22 = load i1, ptr %v.21.addr
  %t.23 = xor i1 %t.22, true
  %t.24 = zext i1 %t.23 to i32
  call void @zl_imprime_bool(i32 %t.24)
  ; linha 26
  call void @zl_imprime_nl()
  ; linha 27
  ret i32 0
}

