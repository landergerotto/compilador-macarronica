; ModuleID = 'n1000000/C3/prodesc_f32.ll'
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
define float @prodesc_f32(ptr %0, ptr %1) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded9 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %2 = alloca float, align 4
  store float 0.000000e+00, ptr %2, align 4
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
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @prodesc_f32..omp_par, ptr nonnull %structArg)
  %3 = load float, ptr %2, align 4
  %4 = fadd float %3, 0.000000e+00
  ret float %4
}

; Function Attrs: nounwind
define internal void @prodesc_f32..omp_par(ptr noalias nocapture readnone %tid.addr, ptr noalias nocapture readnone %zero.addr, ptr nocapture readonly %0) #0 {
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
  %1 = alloca float, align 4
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %red.array = alloca [1 x ptr], align 8
  store float 0.000000e+00, ptr %1, align 4
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
  %xtraiter = and i64 %5, 3
  %6 = icmp ult i64 %reass.sub, 3
  br i1 %6, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %5, -4
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body, %omp_loop.body.preheader
  %omp_loop.iv23.unr = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.3, %omp_loop.body ]
  %.unr = phi float [ 0.000000e+00, %omp_loop.body.preheader ], [ %46, %omp_loop.body ]
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil
  %omp_loop.iv23.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv23.unr, %omp_loop.exit.loopexit.unr-lcssa ]
  %7 = phi float [ %14, %omp_loop.body.epil ], [ %.unr, %omp_loop.exit.loopexit.unr-lcssa ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.exit.loopexit.unr-lcssa ]
  %8 = add i64 %omp_loop.iv23.epil, %2
  %9 = getelementptr float, ptr %.unpack2, i64 %8
  %10 = load float, ptr %9, align 4
  %11 = getelementptr float, ptr %.unpack13, i64 %8
  %12 = load float, ptr %11, align 4
  %13 = fmul float %10, %12
  %14 = fadd float %7, %13
  store float %14, ptr %1, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv23.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %omp_loop.exit, label %omp_loop.body.epil, !llvm.loop !1

omp_loop.exit:                                    ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil, %omp.par.entry
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
  %15 = load float, ptr %1, align 4
  %16 = atomicrmw fadd ptr %loadgep_, float %15 monotonic, align 4
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load float, ptr %loadgep_, align 4
  %red.private.value.0 = load float, ptr %1, align 4
  %17 = fadd float %red.value.0, %red.private.value.0
  store float %17, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @3, i32 %omp_global_thread_num7, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num8 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @2, i32 %omp_global_thread_num8)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv23 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.3, %omp_loop.body ]
  %18 = phi float [ 0.000000e+00, %omp_loop.body.preheader.new ], [ %46, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.3, %omp_loop.body ]
  %19 = add i64 %omp_loop.iv23, %2
  %20 = getelementptr float, ptr %.unpack2, i64 %19
  %21 = load float, ptr %20, align 4
  %22 = getelementptr float, ptr %.unpack13, i64 %19
  %23 = load float, ptr %22, align 4
  %24 = fmul float %21, %23
  %25 = fadd float %18, %24
  store float %25, ptr %1, align 4
  %omp_loop.next = or disjoint i64 %omp_loop.iv23, 1
  %26 = add i64 %omp_loop.next, %2
  %27 = getelementptr float, ptr %.unpack2, i64 %26
  %28 = load float, ptr %27, align 4
  %29 = getelementptr float, ptr %.unpack13, i64 %26
  %30 = load float, ptr %29, align 4
  %31 = fmul float %28, %30
  %32 = fadd float %25, %31
  store float %32, ptr %1, align 4
  %omp_loop.next.1 = or disjoint i64 %omp_loop.iv23, 2
  %33 = add i64 %omp_loop.next.1, %2
  %34 = getelementptr float, ptr %.unpack2, i64 %33
  %35 = load float, ptr %34, align 4
  %36 = getelementptr float, ptr %.unpack13, i64 %33
  %37 = load float, ptr %36, align 4
  %38 = fmul float %35, %37
  %39 = fadd float %32, %38
  store float %39, ptr %1, align 4
  %omp_loop.next.2 = or disjoint i64 %omp_loop.iv23, 3
  %40 = add i64 %omp_loop.next.2, %2
  %41 = getelementptr float, ptr %.unpack2, i64 %40
  %42 = load float, ptr %41, align 4
  %43 = getelementptr float, ptr %.unpack13, i64 %40
  %44 = load float, ptr %43, align 4
  %45 = fmul float %42, %44
  %46 = fadd float %39, %45
  store float %46, ptr %1, align 4
  %omp_loop.next.3 = add nuw i64 %omp_loop.iv23, 4
  %niter.next.3 = add i64 %niter, 4
  %niter.ncmp.3 = icmp eq i64 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
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
  %4 = load float, ptr %3, align 4
  %5 = load ptr, ptr %1, align 8
  %6 = load float, ptr %5, align 4
  %7 = fadd float %4, %6
  store float %7, ptr %3, align 4
  ret void
}

; Function Attrs: convergent nounwind
declare i32 @__kmpc_reduce(ptr, i32, i32, i64, ptr, ptr, ptr) local_unnamed_addr #1

; Function Attrs: convergent nounwind
declare void @__kmpc_end_reduce(ptr, i32, ptr) local_unnamed_addr #1

; Function Attrs: nounwind
declare !callback !3 void @__kmpc_fork_call(ptr, i32, ptr, ...) local_unnamed_addr #0

attributes #0 = { nounwind }
attributes #1 = { convergent nounwind }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2}
!2 = !{!"llvm.loop.unroll.disable"}
!3 = !{!4}
!4 = !{i64 2, i64 -1, i64 -1, i1 true}
