; ModuleID = 'n1000000/C1-reassoc/somavet_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @somavet_f32(ptr nocapture readonly %0) local_unnamed_addr #0 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <8 x float> [ <float 0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr float, ptr %0, i64 %index
  %2 = getelementptr float, ptr %1, i64 8
  %3 = getelementptr float, ptr %1, i64 16
  %4 = getelementptr float, ptr %1, i64 24
  %wide.load = load <8 x float>, ptr %1, align 4
  %wide.load4 = load <8 x float>, ptr %2, align 4
  %wide.load5 = load <8 x float>, ptr %3, align 4
  %wide.load6 = load <8 x float>, ptr %4, align 4
  %5 = fadd reassoc <8 x float> %vec.phi, %wide.load
  %6 = fadd reassoc <8 x float> %vec.phi1, %wide.load4
  %7 = fadd reassoc <8 x float> %vec.phi2, %wide.load5
  %8 = fadd reassoc <8 x float> %vec.phi3, %wide.load6
  %9 = getelementptr float, ptr %0, i64 %index
  %10 = getelementptr float, ptr %9, i64 32
  %11 = getelementptr float, ptr %9, i64 40
  %12 = getelementptr float, ptr %9, i64 48
  %13 = getelementptr float, ptr %9, i64 56
  %wide.load.1 = load <8 x float>, ptr %10, align 4
  %wide.load4.1 = load <8 x float>, ptr %11, align 4
  %wide.load5.1 = load <8 x float>, ptr %12, align 4
  %wide.load6.1 = load <8 x float>, ptr %13, align 4
  %14 = fadd reassoc <8 x float> %5, %wide.load.1
  %15 = fadd reassoc <8 x float> %6, %wide.load4.1
  %16 = fadd reassoc <8 x float> %7, %wide.load5.1
  %17 = fadd reassoc <8 x float> %8, %wide.load6.1
  %18 = getelementptr float, ptr %0, i64 %index
  %19 = getelementptr float, ptr %18, i64 64
  %20 = getelementptr float, ptr %18, i64 72
  %21 = getelementptr float, ptr %18, i64 80
  %22 = getelementptr float, ptr %18, i64 88
  %wide.load.2 = load <8 x float>, ptr %19, align 4
  %wide.load4.2 = load <8 x float>, ptr %20, align 4
  %wide.load5.2 = load <8 x float>, ptr %21, align 4
  %wide.load6.2 = load <8 x float>, ptr %22, align 4
  %23 = fadd reassoc <8 x float> %14, %wide.load.2
  %24 = fadd reassoc <8 x float> %15, %wide.load4.2
  %25 = fadd reassoc <8 x float> %16, %wide.load5.2
  %26 = fadd reassoc <8 x float> %17, %wide.load6.2
  %27 = getelementptr float, ptr %0, i64 %index
  %28 = getelementptr float, ptr %27, i64 96
  %29 = getelementptr float, ptr %27, i64 104
  %30 = getelementptr float, ptr %27, i64 112
  %31 = getelementptr float, ptr %27, i64 120
  %wide.load.3 = load <8 x float>, ptr %28, align 4
  %wide.load4.3 = load <8 x float>, ptr %29, align 4
  %wide.load5.3 = load <8 x float>, ptr %30, align 4
  %wide.load6.3 = load <8 x float>, ptr %31, align 4
  %32 = fadd reassoc <8 x float> %23, %wide.load.3
  %33 = fadd reassoc <8 x float> %24, %wide.load4.3
  %34 = fadd reassoc <8 x float> %25, %wide.load5.3
  %35 = fadd reassoc <8 x float> %26, %wide.load6.3
  %36 = getelementptr float, ptr %0, i64 %index
  %37 = getelementptr float, ptr %36, i64 128
  %38 = getelementptr float, ptr %36, i64 136
  %39 = getelementptr float, ptr %36, i64 144
  %40 = getelementptr float, ptr %36, i64 152
  %wide.load.4 = load <8 x float>, ptr %37, align 4
  %wide.load4.4 = load <8 x float>, ptr %38, align 4
  %wide.load5.4 = load <8 x float>, ptr %39, align 4
  %wide.load6.4 = load <8 x float>, ptr %40, align 4
  %41 = fadd reassoc <8 x float> %32, %wide.load.4
  %42 = fadd reassoc <8 x float> %33, %wide.load4.4
  %43 = fadd reassoc <8 x float> %34, %wide.load5.4
  %44 = fadd reassoc <8 x float> %35, %wide.load6.4
  %index.next.4 = add nuw nsw i64 %index, 160
  %45 = icmp eq i64 %index.next.4, 1000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd reassoc <8 x float> %42, %41
  %bin.rdx7 = fadd reassoc <8 x float> %43, %bin.rdx
  %bin.rdx8 = fadd reassoc <8 x float> %44, %bin.rdx7
  %46 = tail call reassoc float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %bin.rdx8)
  ret float %46
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
