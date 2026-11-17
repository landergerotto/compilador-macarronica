; ModuleID = 'n1000000/C2/soma_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @soma_f32(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %29, %4 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 1000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr float, ptr %0, i64 %5
  %12 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %11, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %13 = getelementptr float, ptr %1, i64 %5
  %14 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %13, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %15 = fadd <8 x float> %12, %14
  %16 = getelementptr float, ptr %2, i64 %5
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %15, ptr %16, i32 4, <8 x i1> %10)
  %17 = or disjoint i64 %5, 8
  %18 = trunc i64 %17 to i32
  %19 = sub nuw nsw i32 1000000, %18
  %20 = insertelement <8 x i32> poison, i32 %19, i64 0
  %21 = shufflevector <8 x i32> %20, <8 x i32> poison, <8 x i32> zeroinitializer
  %22 = icmp sgt <8 x i32> %21, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %23 = getelementptr float, ptr %0, i64 %17
  %24 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %23, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %25 = getelementptr float, ptr %1, i64 %17
  %26 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %25, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %27 = fadd <8 x float> %24, %26
  %28 = getelementptr float, ptr %2, i64 %17
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %27, ptr %28, i32 4, <8 x i1> %22)
  %29 = add nuw nsw i64 %5, 16
  %30 = icmp ult i64 %17, 999992
  br i1 %30, label %4, label %31

31:                                               ; preds = %4
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x float> @llvm.masked.load.v8f32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x float>) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: write)
declare void @llvm.masked.store.v8f32.p0(<8 x float>, ptr nocapture, i32 immarg, <8 x i1>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: write) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
