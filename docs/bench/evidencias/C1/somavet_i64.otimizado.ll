; ModuleID = 'n1000000/C1/somavet_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @somavet_i64(ptr nocapture readonly %0) local_unnamed_addr #0 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <4 x i64> [ zeroinitializer, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr i64, ptr %0, i64 %index
  %2 = getelementptr i64, ptr %1, i64 4
  %3 = getelementptr i64, ptr %1, i64 8
  %4 = getelementptr i64, ptr %1, i64 12
  %wide.load = load <4 x i64>, ptr %1, align 4
  %wide.load4 = load <4 x i64>, ptr %2, align 4
  %wide.load5 = load <4 x i64>, ptr %3, align 4
  %wide.load6 = load <4 x i64>, ptr %4, align 4
  %5 = add <4 x i64> %wide.load, %vec.phi
  %6 = add <4 x i64> %wide.load4, %vec.phi1
  %7 = add <4 x i64> %wide.load5, %vec.phi2
  %8 = add <4 x i64> %wide.load6, %vec.phi3
  %9 = getelementptr i64, ptr %0, i64 %index
  %10 = getelementptr i64, ptr %9, i64 16
  %11 = getelementptr i64, ptr %9, i64 20
  %12 = getelementptr i64, ptr %9, i64 24
  %13 = getelementptr i64, ptr %9, i64 28
  %wide.load.1 = load <4 x i64>, ptr %10, align 4
  %wide.load4.1 = load <4 x i64>, ptr %11, align 4
  %wide.load5.1 = load <4 x i64>, ptr %12, align 4
  %wide.load6.1 = load <4 x i64>, ptr %13, align 4
  %14 = add <4 x i64> %wide.load.1, %5
  %15 = add <4 x i64> %wide.load4.1, %6
  %16 = add <4 x i64> %wide.load5.1, %7
  %17 = add <4 x i64> %wide.load6.1, %8
  %18 = getelementptr i64, ptr %0, i64 %index
  %19 = getelementptr i64, ptr %18, i64 32
  %20 = getelementptr i64, ptr %18, i64 36
  %21 = getelementptr i64, ptr %18, i64 40
  %22 = getelementptr i64, ptr %18, i64 44
  %wide.load.2 = load <4 x i64>, ptr %19, align 4
  %wide.load4.2 = load <4 x i64>, ptr %20, align 4
  %wide.load5.2 = load <4 x i64>, ptr %21, align 4
  %wide.load6.2 = load <4 x i64>, ptr %22, align 4
  %23 = add <4 x i64> %wide.load.2, %14
  %24 = add <4 x i64> %wide.load4.2, %15
  %25 = add <4 x i64> %wide.load5.2, %16
  %26 = add <4 x i64> %wide.load6.2, %17
  %27 = getelementptr i64, ptr %0, i64 %index
  %28 = getelementptr i64, ptr %27, i64 48
  %29 = getelementptr i64, ptr %27, i64 52
  %30 = getelementptr i64, ptr %27, i64 56
  %31 = getelementptr i64, ptr %27, i64 60
  %wide.load.3 = load <4 x i64>, ptr %28, align 4
  %wide.load4.3 = load <4 x i64>, ptr %29, align 4
  %wide.load5.3 = load <4 x i64>, ptr %30, align 4
  %wide.load6.3 = load <4 x i64>, ptr %31, align 4
  %32 = add <4 x i64> %wide.load.3, %23
  %33 = add <4 x i64> %wide.load4.3, %24
  %34 = add <4 x i64> %wide.load5.3, %25
  %35 = add <4 x i64> %wide.load6.3, %26
  %36 = getelementptr i64, ptr %0, i64 %index
  %37 = getelementptr i64, ptr %36, i64 64
  %38 = getelementptr i64, ptr %36, i64 68
  %39 = getelementptr i64, ptr %36, i64 72
  %40 = getelementptr i64, ptr %36, i64 76
  %wide.load.4 = load <4 x i64>, ptr %37, align 4
  %wide.load4.4 = load <4 x i64>, ptr %38, align 4
  %wide.load5.4 = load <4 x i64>, ptr %39, align 4
  %wide.load6.4 = load <4 x i64>, ptr %40, align 4
  %41 = add <4 x i64> %wide.load.4, %32
  %42 = add <4 x i64> %wide.load4.4, %33
  %43 = add <4 x i64> %wide.load5.4, %34
  %44 = add <4 x i64> %wide.load6.4, %35
  %index.next.4 = add nuw nsw i64 %index, 80
  %45 = icmp eq i64 %index.next.4, 1000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !1

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i64> %42, %41
  %bin.rdx7 = add <4 x i64> %43, %bin.rdx
  %bin.rdx8 = add <4 x i64> %44, %bin.rdx7
  %46 = tail call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %bin.rdx8)
  ret i64 %46
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
