; ModuleID = 'n1000000/C3/prodesc_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.ident_t = type { i32, i32, i32, i32, ptr }

@0 = private unnamed_addr constant [23 x i8] c";unknown;unknown;0;0;;\00", align 1
@1 = private unnamed_addr constant %struct.ident_t { i32 0, i32 2, i32 0, i32 22, ptr @0 }, align 8
@2 = private unnamed_addr constant %struct.ident_t { i32 0, i32 66, i32 0, i32 22, ptr @0 }, align 8
@3 = private unnamed_addr constant %struct.ident_t { i32 0, i32 18, i32 0, i32 22, ptr @0 }, align 8
@.gomp_critical_user_.reduction.var = common global [8 x i32] zeroinitializer, align 8

; Function Attrs: nounwind
define i64 @prodesc_i64(ptr %0, ptr %1) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded9 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %2 = alloca i64, align 8
  store i64 0, ptr %2, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep4 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 1
  store ptr %0, ptr %.fca.1.gep4, align 8
  %.fca.2.gep6 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep6, align 8
  %.fca.3.0.gep8 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep8, align 8
  %.fca.4.0.gep10 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep10, align 8
  store ptr %1, ptr %.reloaded9, align 8
  %.fca.1.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded9, i64 0, i32 1
  store ptr %1, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded9, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded9, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded9, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_.reloaded9 = getelementptr inbounds { ptr, ptr, ptr }, ptr %structArg, i64 0, i32 1
  store ptr %.reloaded9, ptr %gep_.reloaded9, align 8
  %gep_ = getelementptr inbounds { ptr, ptr, ptr }, ptr %structArg, i64 0, i32 2
  store ptr %2, ptr %gep_, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @prodesc_i64..omp_par, ptr nonnull %structArg)
  %3 = load i64, ptr %2, align 8
  ret i64 %3
}

; Function Attrs: nounwind
define internal void @prodesc_i64..omp_par(ptr noalias nocapture readnone %tid.addr, ptr noalias nocapture readnone %zero.addr, ptr nocapture readonly %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8
  %gep_.reloaded9 = getelementptr { ptr, ptr, ptr }, ptr %0, i64 0, i32 1
  %loadgep_.reloaded9 = load ptr, ptr %gep_.reloaded9, align 8
  %gep_ = getelementptr { ptr, ptr, ptr }, ptr %0, i64 0, i32 2
  %loadgep_ = load ptr, ptr %gep_, align 8
  %.elt1 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded, i64 0, i32 1
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.elt12 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded9, i64 0, i32 1
  %.unpack13 = load ptr, ptr %.elt12, align 8
  %1 = alloca i64, align 8
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %red.array = alloca [1 x ptr], align 8
  store i64 0, ptr %1, align 8
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num5 = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @1, i32 %omp_global_thread_num5, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %2 = load i64, ptr %p.lowerbound, align 8
  %3 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %3, %2
  %omp_loop.cmp22.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp22.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %4 = add i64 %3, 1
  %5 = sub i64 %4, %2
  %min.iters.check = icmp ult i64 %5, 16
  br i1 %min.iters.check, label %omp_loop.body.preheader43, label %vector.memcheck

vector.memcheck:                                  ; preds = %omp_loop.body.preheader
  %scevgep = getelementptr inbounds i8, ptr %1, i64 8
  %6 = shl i64 %2, 3
  %scevgep24 = getelementptr i8, ptr %.unpack2, i64 %6
  %7 = shl i64 %3, 3
  %8 = add i64 %7, 8
  %scevgep25 = getelementptr i8, ptr %.unpack2, i64 %8
  %scevgep26 = getelementptr i8, ptr %.unpack13, i64 %6
  %scevgep27 = getelementptr i8, ptr %.unpack13, i64 %8
  %bound0 = icmp ult ptr %1, %scevgep25
  %bound1 = icmp ult ptr %scevgep24, %scevgep
  %found.conflict = and i1 %bound0, %bound1
  %bound028 = icmp ult ptr %1, %scevgep27
  %bound129 = icmp ult ptr %scevgep26, %scevgep
  %found.conflict30 = and i1 %bound028, %bound129
  %conflict.rdx = or i1 %found.conflict, %found.conflict30
  br i1 %conflict.rdx, label %omp_loop.body.preheader43, label %vector.ph

vector.ph:                                        ; preds = %vector.memcheck
  %n.vec = and i64 %5, -16
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %22, %vector.body ]
  %vec.phi31 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %23, %vector.body ]
  %vec.phi32 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %24, %vector.body ]
  %vec.phi33 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %25, %vector.body ]
  %9 = add i64 %index, %2
  %10 = getelementptr i64, ptr %.unpack2, i64 %9
  %11 = getelementptr i64, ptr %10, i64 4
  %12 = getelementptr i64, ptr %10, i64 8
  %13 = getelementptr i64, ptr %10, i64 12
  %wide.load = load <4 x i64>, ptr %10, align 4, !alias.scope !1
  %wide.load34 = load <4 x i64>, ptr %11, align 4, !alias.scope !1
  %wide.load35 = load <4 x i64>, ptr %12, align 4, !alias.scope !1
  %wide.load36 = load <4 x i64>, ptr %13, align 4, !alias.scope !1
  %14 = getelementptr i64, ptr %.unpack13, i64 %9
  %15 = getelementptr i64, ptr %14, i64 4
  %16 = getelementptr i64, ptr %14, i64 8
  %17 = getelementptr i64, ptr %14, i64 12
  %wide.load37 = load <4 x i64>, ptr %14, align 4, !alias.scope !4
  %wide.load38 = load <4 x i64>, ptr %15, align 4, !alias.scope !4
  %wide.load39 = load <4 x i64>, ptr %16, align 4, !alias.scope !4
  %wide.load40 = load <4 x i64>, ptr %17, align 4, !alias.scope !4
  %18 = mul <4 x i64> %wide.load37, %wide.load
  %19 = mul <4 x i64> %wide.load38, %wide.load34
  %20 = mul <4 x i64> %wide.load39, %wide.load35
  %21 = mul <4 x i64> %wide.load40, %wide.load36
  %22 = add <4 x i64> %vec.phi, %18
  %23 = add <4 x i64> %vec.phi31, %19
  %24 = add <4 x i64> %vec.phi32, %20
  %25 = add <4 x i64> %vec.phi33, %21
  %index.next = add nuw i64 %index, 16
  %26 = icmp eq i64 %index.next, %n.vec
  br i1 %26, label %middle.block, label %vector.body, !llvm.loop !6

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i64> %23, %22
  %bin.rdx41 = add <4 x i64> %24, %bin.rdx
  %bin.rdx42 = add <4 x i64> %25, %bin.rdx41
  %27 = call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %bin.rdx42)
  store i64 %27, ptr %1, align 8
  %cmp.n = icmp eq i64 %5, %n.vec
  br i1 %cmp.n, label %omp_loop.exit, label %omp_loop.body.preheader43

omp_loop.body.preheader43:                        ; preds = %vector.memcheck, %omp_loop.body.preheader, %middle.block
  %omp_loop.iv23.ph = phi i64 [ 0, %vector.memcheck ], [ 0, %omp_loop.body.preheader ], [ %n.vec, %middle.block ]
  %.ph = phi i64 [ 0, %vector.memcheck ], [ 0, %omp_loop.body.preheader ], [ %27, %middle.block ]
  br label %omp_loop.body

omp_loop.exit:                                    ; preds = %omp_loop.body, %middle.block, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @1, i32 %omp_global_thread_num5)
  %omp_global_thread_num6 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @2, i32 %omp_global_thread_num6)
  store ptr %1, ptr %red.array, align 8
  %omp_global_thread_num7 = call i32 @__kmpc_global_thread_num(ptr nonnull @3)
  %reduce = call i32 @__kmpc_reduce(ptr nonnull @3, i32 %omp_global_thread_num7, i32 1, i64 8, ptr nonnull %red.array, ptr nonnull @.omp.reduction.func, ptr nonnull @.gomp_critical_user_.reduction.var)
  switch i32 %reduce, label %reduce.finalize [
    i32 1, label %reduce.switch.nonatomic
    i32 2, label %reduce.switch.atomic
  ]

reduce.switch.atomic:                             ; preds = %omp_loop.exit
  %28 = load i64, ptr %1, align 8
  %29 = atomicrmw add ptr %loadgep_, i64 %28 monotonic, align 8
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load i64, ptr %loadgep_, align 4
  %red.private.value.0 = load i64, ptr %1, align 8
  %30 = add i64 %red.private.value.0, %red.value.0
  store i64 %30, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @3, i32 %omp_global_thread_num7, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num8 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @2, i32 %omp_global_thread_num8)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body.preheader43, %omp_loop.body
  %omp_loop.iv23 = phi i64 [ %omp_loop.next, %omp_loop.body ], [ %omp_loop.iv23.ph, %omp_loop.body.preheader43 ]
  %31 = phi i64 [ %38, %omp_loop.body ], [ %.ph, %omp_loop.body.preheader43 ]
  %32 = add i64 %omp_loop.iv23, %2
  %33 = getelementptr i64, ptr %.unpack2, i64 %32
  %34 = load i64, ptr %33, align 4
  %35 = getelementptr i64, ptr %.unpack13, i64 %32
  %36 = load i64, ptr %35, align 4
  %37 = mul i64 %36, %34
  %38 = add i64 %31, %37
  store i64 %38, ptr %1, align 8
  %omp_loop.next = add nuw i64 %omp_loop.iv23, 1
  %exitcond.not = icmp eq i64 %omp_loop.iv23, %reass.sub
  br i1 %exitcond.not, label %omp_loop.exit, label %omp_loop.body, !llvm.loop !9
}

; Function Attrs: nounwind
declare i32 @__kmpc_global_thread_num(ptr) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @__kmpc_for_static_init_8u(ptr, i32, i32, ptr, ptr, ptr, ptr, i64, i64) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @__kmpc_for_static_fini(ptr, i32) local_unnamed_addr #0

; Function Attrs: convergent nounwind
declare void @__kmpc_barrier(ptr, i32) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define internal void @.omp.reduction.func(ptr nocapture readonly %0, ptr nocapture readonly %1) #2 {
  %3 = load ptr, ptr %0, align 8
  %4 = load i64, ptr %3, align 4
  %5 = load ptr, ptr %1, align 8
  %6 = load i64, ptr %5, align 4
  %7 = add i64 %6, %4
  store i64 %7, ptr %3, align 4
  ret void
}

; Function Attrs: convergent nounwind
declare i32 @__kmpc_reduce(ptr, i32, i32, i64, ptr, ptr, ptr) local_unnamed_addr #1

; Function Attrs: convergent nounwind
declare void @__kmpc_end_reduce(ptr, i32, ptr) local_unnamed_addr #1

; Function Attrs: nounwind
declare !callback !10 void @__kmpc_fork_call(ptr, i32, ptr, ...) local_unnamed_addr #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v4i64(<4 x i64>) #3

attributes #0 = { nounwind }
attributes #1 = { convergent nounwind }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none) }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{!2}
!2 = distinct !{!2, !3}
!3 = distinct !{!3, !"LVerDomain"}
!4 = !{!5}
!5 = distinct !{!5, !3}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.isvectorized", i32 1}
!8 = !{!"llvm.loop.unroll.runtime.disable"}
!9 = distinct !{!9, !7}
!10 = !{!11}
!11 = !{i64 2, i64 -1, i64 -1, i1 true}
