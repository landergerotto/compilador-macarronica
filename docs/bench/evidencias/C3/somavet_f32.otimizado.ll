; ModuleID = 'n1000000/C3/somavet_f32.ll'
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
define float @somavet_f32(ptr %0) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %1 = alloca float, align 4
  store float 0.000000e+00, ptr %1, align 4
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 1
  store ptr %0, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 2
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 3, i64 0
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %.reloaded, i64 0, i32 4, i64 0
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_ = getelementptr inbounds { ptr, ptr }, ptr %structArg, i64 0, i32 1
  store ptr %1, ptr %gep_, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @somavet_f32..omp_par, ptr nonnull %structArg)
  %2 = load float, ptr %1, align 4
  %3 = fadd float %2, 0.000000e+00
  ret float %3
}

; Function Attrs: nounwind
define internal void @somavet_f32..omp_par(ptr noalias nocapture readnone %tid.addr, ptr noalias nocapture readnone %zero.addr, ptr nocapture readonly %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8
  %gep_ = getelementptr { ptr, ptr }, ptr %0, i64 0, i32 1
  %loadgep_ = load ptr, ptr %gep_, align 8
  %.elt1 = getelementptr inbounds { ptr, ptr, i64, [1 x i64], [1 x i64] }, ptr %loadgep_.reloaded, i64 0, i32 1
  %.unpack2 = load ptr, ptr %.elt1, align 8
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
  %invariant.gep = getelementptr float, ptr %.unpack2, i64 %2
  %omp_loop.cmp11.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp11.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %4 = add i64 %3, 1
  %5 = sub i64 %4, %2
  %xtraiter = and i64 %5, 7
  %6 = icmp ult i64 %reass.sub, 7
  br i1 %6, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %5, -8
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body, %omp_loop.body.preheader
  %omp_loop.iv12.unr = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.7, %omp_loop.body ]
  %.unr = phi float [ 0.000000e+00, %omp_loop.body.preheader ], [ %29, %omp_loop.body ]
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil
  %omp_loop.iv12.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv12.unr, %omp_loop.exit.loopexit.unr-lcssa ]
  %7 = phi float [ %9, %omp_loop.body.epil ], [ %.unr, %omp_loop.exit.loopexit.unr-lcssa ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.exit.loopexit.unr-lcssa ]
  %gep.epil = getelementptr float, ptr %invariant.gep, i64 %omp_loop.iv12.epil
  %8 = load float, ptr %gep.epil, align 4
  %9 = fadd float %8, %7
  store float %9, ptr %1, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv12.epil, 1
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
  %10 = load float, ptr %1, align 4
  %11 = atomicrmw fadd ptr %loadgep_, float %10 monotonic, align 4
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load float, ptr %loadgep_, align 4
  %red.private.value.0 = load float, ptr %1, align 4
  %12 = fadd float %red.value.0, %red.private.value.0
  store float %12, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @3, i32 %omp_global_thread_num7, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num8 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @2, i32 %omp_global_thread_num8)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv12 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.7, %omp_loop.body ]
  %13 = phi float [ 0.000000e+00, %omp_loop.body.preheader.new ], [ %29, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.7, %omp_loop.body ]
  %gep = getelementptr float, ptr %invariant.gep, i64 %omp_loop.iv12
  %14 = load float, ptr %gep, align 4
  %15 = fadd float %14, %13
  store float %15, ptr %1, align 4
  %omp_loop.next = or disjoint i64 %omp_loop.iv12, 1
  %gep.1 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next
  %16 = load float, ptr %gep.1, align 4
  %17 = fadd float %16, %15
  store float %17, ptr %1, align 4
  %omp_loop.next.1 = or disjoint i64 %omp_loop.iv12, 2
  %gep.2 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.1
  %18 = load float, ptr %gep.2, align 4
  %19 = fadd float %18, %17
  store float %19, ptr %1, align 4
  %omp_loop.next.2 = or disjoint i64 %omp_loop.iv12, 3
  %gep.3 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.2
  %20 = load float, ptr %gep.3, align 4
  %21 = fadd float %20, %19
  store float %21, ptr %1, align 4
  %omp_loop.next.3 = or disjoint i64 %omp_loop.iv12, 4
  %gep.4 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.3
  %22 = load float, ptr %gep.4, align 4
  %23 = fadd float %22, %21
  store float %23, ptr %1, align 4
  %omp_loop.next.4 = or disjoint i64 %omp_loop.iv12, 5
  %gep.5 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.4
  %24 = load float, ptr %gep.5, align 4
  %25 = fadd float %24, %23
  store float %25, ptr %1, align 4
  %omp_loop.next.5 = or disjoint i64 %omp_loop.iv12, 6
  %gep.6 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.5
  %26 = load float, ptr %gep.6, align 4
  %27 = fadd float %26, %25
  store float %27, ptr %1, align 4
  %omp_loop.next.6 = or disjoint i64 %omp_loop.iv12, 7
  %gep.7 = getelementptr float, ptr %invariant.gep, i64 %omp_loop.next.6
  %28 = load float, ptr %gep.7, align 4
  %29 = fadd float %28, %27
  store float %29, ptr %1, align 4
  %omp_loop.next.7 = add nuw i64 %omp_loop.iv12, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
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
