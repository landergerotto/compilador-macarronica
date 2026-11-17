; ModuleID = 'n1000000/C2/prodesc_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @prodesc_f32(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi <8 x float> [ zeroinitializer, %2 ], [ %52, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %53, %3 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 1000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr float, ptr %0, i64 %5
  %12 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %11, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %13 = getelementptr float, ptr %1, i64 %5
  %14 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %13, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %15 = fmul <8 x float> %12, %14
  %16 = fadd <8 x float> %4, %15
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
  %27 = fmul <8 x float> %24, %26
  %28 = fadd <8 x float> %16, %27
  %29 = or disjoint i64 %5, 16
  %30 = trunc i64 %29 to i32
  %31 = sub nuw nsw i32 1000000, %30
  %32 = insertelement <8 x i32> poison, i32 %31, i64 0
  %33 = shufflevector <8 x i32> %32, <8 x i32> poison, <8 x i32> zeroinitializer
  %34 = icmp sgt <8 x i32> %33, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %35 = getelementptr float, ptr %0, i64 %29
  %36 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %35, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %37 = getelementptr float, ptr %1, i64 %29
  %38 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %37, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %39 = fmul <8 x float> %36, %38
  %40 = fadd <8 x float> %28, %39
  %41 = or disjoint i64 %5, 24
  %42 = trunc i64 %41 to i32
  %43 = sub nuw nsw i32 1000000, %42
  %44 = insertelement <8 x i32> poison, i32 %43, i64 0
  %45 = shufflevector <8 x i32> %44, <8 x i32> poison, <8 x i32> zeroinitializer
  %46 = icmp sgt <8 x i32> %45, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %47 = getelementptr float, ptr %0, i64 %41
  %48 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %47, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %49 = getelementptr float, ptr %1, i64 %41
  %50 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %49, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %51 = fmul <8 x float> %48, %50
  %52 = fadd <8 x float> %40, %51
  %53 = add nuw nsw i64 %5, 32
  %54 = icmp ult i64 %41, 999992
  br i1 %54, label %3, label %55

55:                                               ; preds = %3
  %56 = tail call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %52)
  ret float %56
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x float> @llvm.masked.load.v8f32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x float>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #1 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
