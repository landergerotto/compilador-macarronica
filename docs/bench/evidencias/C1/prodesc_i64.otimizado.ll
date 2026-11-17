; ModuleID = 'n1000000/C1/prodesc_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @prodesc_i64(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #0 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %14, %vector.body ]
  %vec.phi1 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %15, %vector.body ]
  %vec.phi2 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %16, %vector.body ]
  %vec.phi3 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %17, %vector.body ]
  %2 = getelementptr i64, ptr %0, i64 %index
  %3 = getelementptr i64, ptr %2, i64 4
  %4 = getelementptr i64, ptr %2, i64 8
  %5 = getelementptr i64, ptr %2, i64 12
  %wide.load = load <4 x i64>, ptr %2, align 4
  %wide.load4 = load <4 x i64>, ptr %3, align 4
  %wide.load5 = load <4 x i64>, ptr %4, align 4
  %wide.load6 = load <4 x i64>, ptr %5, align 4
  %6 = getelementptr i64, ptr %1, i64 %index
  %7 = getelementptr i64, ptr %6, i64 4
  %8 = getelementptr i64, ptr %6, i64 8
  %9 = getelementptr i64, ptr %6, i64 12
  %wide.load7 = load <4 x i64>, ptr %6, align 4
  %wide.load8 = load <4 x i64>, ptr %7, align 4
  %wide.load9 = load <4 x i64>, ptr %8, align 4
  %wide.load10 = load <4 x i64>, ptr %9, align 4
  %10 = mul <4 x i64> %wide.load7, %wide.load
  %11 = mul <4 x i64> %wide.load8, %wide.load4
  %12 = mul <4 x i64> %wide.load9, %wide.load5
  %13 = mul <4 x i64> %wide.load10, %wide.load6
  %14 = add <4 x i64> %10, %vec.phi
  %15 = add <4 x i64> %11, %vec.phi1
  %16 = add <4 x i64> %12, %vec.phi2
  %17 = add <4 x i64> %13, %vec.phi3
  %index.next = add nuw i64 %index, 16
  %18 = icmp eq i64 %index.next, 1000000
  br i1 %18, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i64> %15, %14
  %bin.rdx11 = add <4 x i64> %16, %bin.rdx
  %bin.rdx12 = add <4 x i64> %17, %bin.rdx11
  %19 = tail call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %bin.rdx12)
  ret i64 %19
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v4i64(<4 x i64>) #1

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
