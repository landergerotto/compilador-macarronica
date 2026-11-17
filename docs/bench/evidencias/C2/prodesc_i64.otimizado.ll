; ModuleID = 'n1000000/C2/prodesc_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @prodesc_i64(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %2, %3
  %4 = phi <8 x i64> [ zeroinitializer, %2 ], [ %16, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %17, %3 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 1000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr i64, ptr %0, i64 %5
  %12 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %11, i32 8, <8 x i1> %10, <8 x i64> zeroinitializer)
  %13 = getelementptr i64, ptr %1, i64 %5
  %14 = tail call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %13, i32 8, <8 x i1> %10, <8 x i64> zeroinitializer)
  %15 = mul <8 x i64> %14, %12
  %16 = add <8 x i64> %15, %4
  %17 = add nuw nsw i64 %5, 8
  %18 = icmp ult i64 %5, 999992
  br i1 %18, label %3, label %19

19:                                               ; preds = %3
  %20 = tail call i64 @llvm.vector.reduce.add.v8i64(<8 x i64> %16)
  ret i64 %20
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
