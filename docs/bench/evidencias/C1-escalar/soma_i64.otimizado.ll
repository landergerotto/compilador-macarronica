; ModuleID = 'n1000000/C1-escalar/soma_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @soma_i64(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %61, %4 ]
  %6 = getelementptr i64, ptr %0, i64 %5
  %7 = load i64, ptr %6, align 4
  %8 = getelementptr i64, ptr %1, i64 %5
  %9 = load i64, ptr %8, align 4
  %10 = add i64 %9, %7
  %11 = getelementptr i64, ptr %2, i64 %5
  store i64 %10, ptr %11, align 4
  %12 = or disjoint i64 %5, 1
  %13 = getelementptr i64, ptr %0, i64 %12
  %14 = load i64, ptr %13, align 4
  %15 = getelementptr i64, ptr %1, i64 %12
  %16 = load i64, ptr %15, align 4
  %17 = add i64 %16, %14
  %18 = getelementptr i64, ptr %2, i64 %12
  store i64 %17, ptr %18, align 4
  %19 = or disjoint i64 %5, 2
  %20 = getelementptr i64, ptr %0, i64 %19
  %21 = load i64, ptr %20, align 4
  %22 = getelementptr i64, ptr %1, i64 %19
  %23 = load i64, ptr %22, align 4
  %24 = add i64 %23, %21
  %25 = getelementptr i64, ptr %2, i64 %19
  store i64 %24, ptr %25, align 4
  %26 = or disjoint i64 %5, 3
  %27 = getelementptr i64, ptr %0, i64 %26
  %28 = load i64, ptr %27, align 4
  %29 = getelementptr i64, ptr %1, i64 %26
  %30 = load i64, ptr %29, align 4
  %31 = add i64 %30, %28
  %32 = getelementptr i64, ptr %2, i64 %26
  store i64 %31, ptr %32, align 4
  %33 = or disjoint i64 %5, 4
  %34 = getelementptr i64, ptr %0, i64 %33
  %35 = load i64, ptr %34, align 4
  %36 = getelementptr i64, ptr %1, i64 %33
  %37 = load i64, ptr %36, align 4
  %38 = add i64 %37, %35
  %39 = getelementptr i64, ptr %2, i64 %33
  store i64 %38, ptr %39, align 4
  %40 = or disjoint i64 %5, 5
  %41 = getelementptr i64, ptr %0, i64 %40
  %42 = load i64, ptr %41, align 4
  %43 = getelementptr i64, ptr %1, i64 %40
  %44 = load i64, ptr %43, align 4
  %45 = add i64 %44, %42
  %46 = getelementptr i64, ptr %2, i64 %40
  store i64 %45, ptr %46, align 4
  %47 = or disjoint i64 %5, 6
  %48 = getelementptr i64, ptr %0, i64 %47
  %49 = load i64, ptr %48, align 4
  %50 = getelementptr i64, ptr %1, i64 %47
  %51 = load i64, ptr %50, align 4
  %52 = add i64 %51, %49
  %53 = getelementptr i64, ptr %2, i64 %47
  store i64 %52, ptr %53, align 4
  %54 = or disjoint i64 %5, 7
  %55 = getelementptr i64, ptr %0, i64 %54
  %56 = load i64, ptr %55, align 4
  %57 = getelementptr i64, ptr %1, i64 %54
  %58 = load i64, ptr %57, align 4
  %59 = add i64 %58, %56
  %60 = getelementptr i64, ptr %2, i64 %54
  store i64 %59, ptr %60, align 4
  %61 = add nuw nsw i64 %5, 8
  %exitcond.not.7 = icmp eq i64 %61, 1000000
  br i1 %exitcond.not.7, label %62, label %4

62:                                               ; preds = %4
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
