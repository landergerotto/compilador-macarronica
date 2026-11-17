; ModuleID = 'n1000000/C3/soma_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.ident_t = type { i32, i32, i32, i32, ptr }

@0 = private unnamed_addr constant [23 x i8] c";unknown;unknown;0;0;;\00", align 1
@1 = private unnamed_addr constant %struct.ident_t { i32 0, i32 2, i32 0, i32 22, ptr @0 }, align 8
@2 = private unnamed_addr constant %struct.ident_t { i32 0, i32 66, i32 0, i32 22, ptr @0 }, align 8

; Function Attrs: nounwind
define void @soma_f32(ptr %0, ptr %1, ptr %2) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded5 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded6 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep14 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 1
  store ptr %0, ptr %.fca.1.gep14, align 8
  %.fca.2.gep16 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep16, align 8
  %.fca.3.0.gep18 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep18, align 8
  %.fca.4.0.gep20 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep20, align 8
  store ptr %1, ptr %.reloaded5, align 8
  %.fca.1.gep4 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded5, i64 0, i32 1
  store ptr %1, ptr %.fca.1.gep4, align 8
  %.fca.2.gep6 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded5, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep6, align 8
  %.fca.3.0.gep8 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded5, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep8, align 8
  %.fca.4.0.gep10 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded5, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep10, align 8
  store ptr %2, ptr %.reloaded6, align 8
  %.fca.1.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded6, i64 0, i32 1
  store ptr %2, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded6, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded6, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded6, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_.reloaded5 = getelementptr inbounds { ptr, ptr, ptr }, ptr %structArg, i64 0, i32 1
  store ptr %.reloaded5, ptr %gep_.reloaded5, align 8
  %gep_.reloaded6 = getelementptr inbounds { ptr, ptr, ptr }, ptr %structArg, i64 0, i32 2
  store ptr %.reloaded6, ptr %gep_.reloaded6, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @soma_f32..omp_par, ptr nonnull %structArg)
  ret void
}

; Function Attrs: nounwind
define internal void @soma_f32..omp_par(ptr noalias nocapture readnone %tid.addr, ptr noalias nocapture readnone %zero.addr, ptr nocapture readonly %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8
  %gep_.reloaded5 = getelementptr { ptr, ptr, ptr }, ptr %0, i64 0, i32 1
  %loadgep_.reloaded5 = load ptr, ptr %gep_.reloaded5, align 8
  %gep_.reloaded6 = getelementptr { ptr, ptr, ptr }, ptr %0, i64 0, i32 2
  %loadgep_.reloaded6 = load ptr, ptr %gep_.reloaded6, align 8
  %.elt1 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded, i64 0, i32 1
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.unpack236 = ptrtoint ptr %.unpack2 to i64
  %.elt12 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded5, i64 0, i32 1
  %.unpack13 = load ptr, ptr %.elt12, align 8
  %.unpack1337 = ptrtoint ptr %.unpack13 to i64
  %.elt23 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded6, i64 0, i32 1
  %.unpack24 = load ptr, ptr %.elt23, align 8
  %.unpack2435 = ptrtoint ptr %.unpack24 to i64
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num3 = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @1, i32 %omp_global_thread_num3, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %1 = load i64, ptr %p.lowerbound, align 8
  %2 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %2, %1
  %omp_loop.cmp33.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp33.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %3 = add i64 %2, 1
  %4 = sub i64 %3, %1
  %min.iters.check = icmp ult i64 %4, 32
  br i1 %min.iters.check, label %omp_loop.body.preheader46, label %vector.memcheck

vector.memcheck:                                  ; preds = %omp_loop.body.preheader
  %5 = sub i64 %.unpack2435, %.unpack236
  %diff.check = icmp ult i64 %5, 128
  %6 = sub i64 %.unpack2435, %.unpack1337
  %diff.check38 = icmp ult i64 %6, 128
  %conflict.rdx = or i1 %diff.check, %diff.check38
  br i1 %conflict.rdx, label %omp_loop.body.preheader46, label %vector.ph

vector.ph:                                        ; preds = %vector.memcheck
  %n.vec = and i64 %4, -32
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %7 = add i64 %index, %1
  %8 = getelementptr float, ptr %.unpack2, i64 %7
  %9 = getelementptr float, ptr %8, i64 8
  %10 = getelementptr float, ptr %8, i64 16
  %11 = getelementptr float, ptr %8, i64 24
  %wide.load = load <8 x float>, ptr %8, align 4
  %wide.load39 = load <8 x float>, ptr %9, align 4
  %wide.load40 = load <8 x float>, ptr %10, align 4
  %wide.load41 = load <8 x float>, ptr %11, align 4
  %12 = getelementptr float, ptr %.unpack13, i64 %7
  %13 = getelementptr float, ptr %12, i64 8
  %14 = getelementptr float, ptr %12, i64 16
  %15 = getelementptr float, ptr %12, i64 24
  %wide.load42 = load <8 x float>, ptr %12, align 4
  %wide.load43 = load <8 x float>, ptr %13, align 4
  %wide.load44 = load <8 x float>, ptr %14, align 4
  %wide.load45 = load <8 x float>, ptr %15, align 4
  %16 = fadd <8 x float> %wide.load, %wide.load42
  %17 = fadd <8 x float> %wide.load39, %wide.load43
  %18 = fadd <8 x float> %wide.load40, %wide.load44
  %19 = fadd <8 x float> %wide.load41, %wide.load45
  %20 = getelementptr float, ptr %.unpack24, i64 %7
  %21 = getelementptr float, ptr %20, i64 8
  %22 = getelementptr float, ptr %20, i64 16
  %23 = getelementptr float, ptr %20, i64 24
  store <8 x float> %16, ptr %20, align 4
  store <8 x float> %17, ptr %21, align 4
  store <8 x float> %18, ptr %22, align 4
  store <8 x float> %19, ptr %23, align 4
  %index.next = add nuw i64 %index, 32
  %24 = icmp eq i64 %index.next, %n.vec
  br i1 %24, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  %cmp.n = icmp eq i64 %4, %n.vec
  br i1 %cmp.n, label %omp_loop.exit, label %omp_loop.body.preheader46

omp_loop.body.preheader46:                        ; preds = %vector.memcheck, %omp_loop.body.preheader, %middle.block
  %omp_loop.iv34.ph = phi i64 [ 0, %vector.memcheck ], [ 0, %omp_loop.body.preheader ], [ %n.vec, %middle.block ]
  br label %omp_loop.body

omp_loop.exit:                                    ; preds = %omp_loop.body, %middle.block, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @1, i32 %omp_global_thread_num3)
  %omp_global_thread_num4 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @2, i32 %omp_global_thread_num4)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body.preheader46, %omp_loop.body
  %omp_loop.iv34 = phi i64 [ %omp_loop.next, %omp_loop.body ], [ %omp_loop.iv34.ph, %omp_loop.body.preheader46 ]
  %25 = add i64 %omp_loop.iv34, %1
  %26 = getelementptr float, ptr %.unpack2, i64 %25
  %27 = load float, ptr %26, align 4
  %28 = getelementptr float, ptr %.unpack13, i64 %25
  %29 = load float, ptr %28, align 4
  %30 = fadd float %27, %29
  %31 = getelementptr float, ptr %.unpack24, i64 %25
  store float %30, ptr %31, align 4
  %omp_loop.next = add nuw i64 %omp_loop.iv34, 1
  %exitcond.not = icmp eq i64 %omp_loop.iv34, %reass.sub
  br i1 %exitcond.not, label %omp_loop.exit, label %omp_loop.body, !llvm.loop !4
}

; Function Attrs: nounwind
declare i32 @__kmpc_global_thread_num(ptr) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @__kmpc_for_static_init_8u(ptr, i32, i32, ptr, ptr, ptr, ptr, i64, i64) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @__kmpc_for_static_fini(ptr, i32) local_unnamed_addr #0

; Function Attrs: convergent nounwind
declare void @__kmpc_barrier(ptr, i32) local_unnamed_addr #1

; Function Attrs: nounwind
declare !callback !5 void @__kmpc_fork_call(ptr, i32, ptr, ...) local_unnamed_addr #0

attributes #0 = { nounwind }
attributes #1 = { convergent nounwind }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
!4 = distinct !{!4, !2}
!5 = !{!6}
!6 = !{i64 2, i64 -1, i64 -1, i1 true}
