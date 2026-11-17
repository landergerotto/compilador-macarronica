; ModuleID = 'n1000000/C1-ordenada/somavet_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @somavet_f32(ptr nocapture readonly %0) local_unnamed_addr #0 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi float [ 0.000000e+00, %vector.ph ], [ %8, %vector.body ]
  %1 = getelementptr float, ptr %0, i64 %index
  %2 = getelementptr float, ptr %1, i64 8
  %3 = getelementptr float, ptr %1, i64 16
  %4 = getelementptr float, ptr %1, i64 24
  %wide.load = load <8 x float>, ptr %1, align 4
  %wide.load1 = load <8 x float>, ptr %2, align 4
  %wide.load2 = load <8 x float>, ptr %3, align 4
  %wide.load3 = load <8 x float>, ptr %4, align 4
  %5 = tail call float @llvm.vector.reduce.fadd.v8f32(float %vec.phi, <8 x float> %wide.load)
  %6 = tail call float @llvm.vector.reduce.fadd.v8f32(float %5, <8 x float> %wide.load1)
  %7 = tail call float @llvm.vector.reduce.fadd.v8f32(float %6, <8 x float> %wide.load2)
  %8 = tail call float @llvm.vector.reduce.fadd.v8f32(float %7, <8 x float> %wide.load3)
  %index.next = add nuw i64 %index, 32
  %9 = icmp eq i64 %index.next, 1000000
  br i1 %9, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  ret float %8
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
