; ModuleID = 'n1000000/C1-escalar/prodesc_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @prodesc_i64(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi i64 [ 0, %2 ], [ %60, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %61, %3 ]
  %6 = getelementptr i64, ptr %0, i64 %5
  %7 = load i64, ptr %6, align 4
  %8 = getelementptr i64, ptr %1, i64 %5
  %9 = load i64, ptr %8, align 4
  %10 = mul i64 %9, %7
  %11 = add i64 %10, %4
  %12 = or disjoint i64 %5, 1
  %13 = getelementptr i64, ptr %0, i64 %12
  %14 = load i64, ptr %13, align 4
  %15 = getelementptr i64, ptr %1, i64 %12
  %16 = load i64, ptr %15, align 4
  %17 = mul i64 %16, %14
  %18 = add i64 %17, %11
  %19 = or disjoint i64 %5, 2
  %20 = getelementptr i64, ptr %0, i64 %19
  %21 = load i64, ptr %20, align 4
  %22 = getelementptr i64, ptr %1, i64 %19
  %23 = load i64, ptr %22, align 4
  %24 = mul i64 %23, %21
  %25 = add i64 %24, %18
  %26 = or disjoint i64 %5, 3
  %27 = getelementptr i64, ptr %0, i64 %26
  %28 = load i64, ptr %27, align 4
  %29 = getelementptr i64, ptr %1, i64 %26
  %30 = load i64, ptr %29, align 4
  %31 = mul i64 %30, %28
  %32 = add i64 %31, %25
  %33 = or disjoint i64 %5, 4
  %34 = getelementptr i64, ptr %0, i64 %33
  %35 = load i64, ptr %34, align 4
  %36 = getelementptr i64, ptr %1, i64 %33
  %37 = load i64, ptr %36, align 4
  %38 = mul i64 %37, %35
  %39 = add i64 %38, %32
  %40 = or disjoint i64 %5, 5
  %41 = getelementptr i64, ptr %0, i64 %40
  %42 = load i64, ptr %41, align 4
  %43 = getelementptr i64, ptr %1, i64 %40
  %44 = load i64, ptr %43, align 4
  %45 = mul i64 %44, %42
  %46 = add i64 %45, %39
  %47 = or disjoint i64 %5, 6
  %48 = getelementptr i64, ptr %0, i64 %47
  %49 = load i64, ptr %48, align 4
  %50 = getelementptr i64, ptr %1, i64 %47
  %51 = load i64, ptr %50, align 4
  %52 = mul i64 %51, %49
  %53 = add i64 %52, %46
  %54 = or disjoint i64 %5, 7
  %55 = getelementptr i64, ptr %0, i64 %54
  %56 = load i64, ptr %55, align 4
  %57 = getelementptr i64, ptr %1, i64 %54
  %58 = load i64, ptr %57, align 4
  %59 = mul i64 %58, %56
  %60 = add i64 %59, %53
  %61 = add nuw nsw i64 %5, 8
  %exitcond.not.7 = icmp eq i64 %61, 1000000
  br i1 %exitcond.not.7, label %62, label %3

62:                                               ; preds = %3
  ret i64 %60
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
