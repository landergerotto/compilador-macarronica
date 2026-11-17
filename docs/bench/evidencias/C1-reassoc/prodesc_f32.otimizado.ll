; ModuleID = 'n1000000/C1-reassoc/prodesc_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @prodesc_f32(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #0 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.1, %vector.body ]
  %vec.phi = phi <8 x float> [ <float 0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %30, %vector.body ]
  %vec.phi1 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %31, %vector.body ]
  %vec.phi2 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %32, %vector.body ]
  %vec.phi3 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %33, %vector.body ]
  %2 = getelementptr float, ptr %0, i64 %index
  %3 = getelementptr float, ptr %2, i64 8
  %4 = getelementptr float, ptr %2, i64 16
  %5 = getelementptr float, ptr %2, i64 24
  %wide.load = load <8 x float>, ptr %2, align 4
  %wide.load4 = load <8 x float>, ptr %3, align 4
  %wide.load5 = load <8 x float>, ptr %4, align 4
  %wide.load6 = load <8 x float>, ptr %5, align 4
  %6 = getelementptr float, ptr %1, i64 %index
  %7 = getelementptr float, ptr %6, i64 8
  %8 = getelementptr float, ptr %6, i64 16
  %9 = getelementptr float, ptr %6, i64 24
  %wide.load7 = load <8 x float>, ptr %6, align 4
  %wide.load8 = load <8 x float>, ptr %7, align 4
  %wide.load9 = load <8 x float>, ptr %8, align 4
  %wide.load10 = load <8 x float>, ptr %9, align 4
  %10 = fmul <8 x float> %wide.load, %wide.load7
  %11 = fmul <8 x float> %wide.load4, %wide.load8
  %12 = fmul <8 x float> %wide.load5, %wide.load9
  %13 = fmul <8 x float> %wide.load6, %wide.load10
  %14 = fadd reassoc <8 x float> %vec.phi, %10
  %15 = fadd reassoc <8 x float> %vec.phi1, %11
  %16 = fadd reassoc <8 x float> %vec.phi2, %12
  %17 = fadd reassoc <8 x float> %vec.phi3, %13
  %index.next = or disjoint i64 %index, 32
  %18 = getelementptr float, ptr %0, i64 %index.next
  %19 = getelementptr float, ptr %18, i64 8
  %20 = getelementptr float, ptr %18, i64 16
  %21 = getelementptr float, ptr %18, i64 24
  %wide.load.1 = load <8 x float>, ptr %18, align 4
  %wide.load4.1 = load <8 x float>, ptr %19, align 4
  %wide.load5.1 = load <8 x float>, ptr %20, align 4
  %wide.load6.1 = load <8 x float>, ptr %21, align 4
  %22 = getelementptr float, ptr %1, i64 %index.next
  %23 = getelementptr float, ptr %22, i64 8
  %24 = getelementptr float, ptr %22, i64 16
  %25 = getelementptr float, ptr %22, i64 24
  %wide.load7.1 = load <8 x float>, ptr %22, align 4
  %wide.load8.1 = load <8 x float>, ptr %23, align 4
  %wide.load9.1 = load <8 x float>, ptr %24, align 4
  %wide.load10.1 = load <8 x float>, ptr %25, align 4
  %26 = fmul <8 x float> %wide.load.1, %wide.load7.1
  %27 = fmul <8 x float> %wide.load4.1, %wide.load8.1
  %28 = fmul <8 x float> %wide.load5.1, %wide.load9.1
  %29 = fmul <8 x float> %wide.load6.1, %wide.load10.1
  %30 = fadd reassoc <8 x float> %14, %26
  %31 = fadd reassoc <8 x float> %15, %27
  %32 = fadd reassoc <8 x float> %16, %28
  %33 = fadd reassoc <8 x float> %17, %29
  %index.next.1 = add nuw nsw i64 %index, 64
  %34 = icmp eq i64 %index.next.1, 1000000
  br i1 %34, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd reassoc <8 x float> %31, %30
  %bin.rdx11 = fadd reassoc <8 x float> %32, %bin.rdx
  %bin.rdx12 = fadd reassoc <8 x float> %33, %bin.rdx11
  %35 = tail call reassoc float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %bin.rdx12)
  ret float %35
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #1

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
