; ModuleID = 'n1000000/C2/somavet_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @somavet_i64(ptr nocapture readonly %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi <8 x i64> [ zeroinitializer, %1 ], [ %39, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %40, %2 ]
  %5 = trunc i64 %4 to i32
  %6 = sub nuw nsw i32 1000000, %5
  %7 = insertelement <8 x i32> poison, i32 %6, i64 0
  %8 = shufflevector <8 x i32> %7, <8 x i32> poison, <8 x i32> zeroinitializer
  %9 = icmp sgt <8 x i32> %8, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %10 = getelementptr i64, ptr %0, i64 %4
  %11 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %10, i32 8, <8 x i1> %9, <8 x i64> zeroinitializer)
  %12 = add <8 x i64> %11, %3
  %13 = or disjoint i64 %4, 8
  %14 = trunc i64 %13 to i32
  %15 = sub nuw nsw i32 1000000, %14
  %16 = insertelement <8 x i32> poison, i32 %15, i64 0
  %17 = shufflevector <8 x i32> %16, <8 x i32> poison, <8 x i32> zeroinitializer
  %18 = icmp sgt <8 x i32> %17, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %19 = getelementptr i64, ptr %0, i64 %13
  %20 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %19, i32 8, <8 x i1> %18, <8 x i64> zeroinitializer)
  %21 = add <8 x i64> %20, %12
  %22 = or disjoint i64 %4, 16
  %23 = trunc i64 %22 to i32
  %24 = sub nuw nsw i32 1000000, %23
  %25 = insertelement <8 x i32> poison, i32 %24, i64 0
  %26 = shufflevector <8 x i32> %25, <8 x i32> poison, <8 x i32> zeroinitializer
  %27 = icmp sgt <8 x i32> %26, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %28 = getelementptr i64, ptr %0, i64 %22
  %29 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %28, i32 8, <8 x i1> %27, <8 x i64> zeroinitializer)
  %30 = add <8 x i64> %29, %21
  %31 = or disjoint i64 %4, 24
  %32 = trunc i64 %31 to i32
  %33 = sub nuw nsw i32 1000000, %32
  %34 = insertelement <8 x i32> poison, i32 %33, i64 0
  %35 = shufflevector <8 x i32> %34, <8 x i32> poison, <8 x i32> zeroinitializer
  %36 = icmp sgt <8 x i32> %35, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %37 = getelementptr i64, ptr %0, i64 %31
  %38 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %37, i32 8, <8 x i1> %36, <8 x i64> zeroinitializer)
  %39 = add <8 x i64> %38, %30
  %40 = add nuw nsw i64 %4, 32
  %41 = icmp ult i64 %31, 999992
  br i1 %41, label %2, label %42

42:                                               ; preds = %2
  %43 = tail call i64 @llvm.vector.reduce.add.v8i64(<8 x i64> %39)
  ret i64 %43
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v8i64(<8 x i64>) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i64> @llvm.masked.load.v8i64.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i64>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #1 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
