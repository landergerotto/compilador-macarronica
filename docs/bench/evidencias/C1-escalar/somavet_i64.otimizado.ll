; ModuleID = 'n1000000/C1-escalar/somavet_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i64 @somavet_i64(ptr nocapture readonly %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i64 [ 0, %1 ], [ %67, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %68, %2 ]
  %5 = getelementptr i64, ptr %0, i64 %4
  %6 = load i64, ptr %5, align 4
  %7 = add i64 %6, %3
  %8 = or disjoint i64 %4, 1
  %9 = getelementptr i64, ptr %0, i64 %8
  %10 = load i64, ptr %9, align 4
  %11 = add i64 %10, %7
  %12 = or disjoint i64 %4, 2
  %13 = getelementptr i64, ptr %0, i64 %12
  %14 = load i64, ptr %13, align 4
  %15 = add i64 %14, %11
  %16 = or disjoint i64 %4, 3
  %17 = getelementptr i64, ptr %0, i64 %16
  %18 = load i64, ptr %17, align 4
  %19 = add i64 %18, %15
  %20 = or disjoint i64 %4, 4
  %21 = getelementptr i64, ptr %0, i64 %20
  %22 = load i64, ptr %21, align 4
  %23 = add i64 %22, %19
  %24 = or disjoint i64 %4, 5
  %25 = getelementptr i64, ptr %0, i64 %24
  %26 = load i64, ptr %25, align 4
  %27 = add i64 %26, %23
  %28 = or disjoint i64 %4, 6
  %29 = getelementptr i64, ptr %0, i64 %28
  %30 = load i64, ptr %29, align 4
  %31 = add i64 %30, %27
  %32 = or disjoint i64 %4, 7
  %33 = getelementptr i64, ptr %0, i64 %32
  %34 = load i64, ptr %33, align 4
  %35 = add i64 %34, %31
  %36 = or disjoint i64 %4, 8
  %37 = getelementptr i64, ptr %0, i64 %36
  %38 = load i64, ptr %37, align 4
  %39 = add i64 %38, %35
  %40 = or disjoint i64 %4, 9
  %41 = getelementptr i64, ptr %0, i64 %40
  %42 = load i64, ptr %41, align 4
  %43 = add i64 %42, %39
  %44 = or disjoint i64 %4, 10
  %45 = getelementptr i64, ptr %0, i64 %44
  %46 = load i64, ptr %45, align 4
  %47 = add i64 %46, %43
  %48 = or disjoint i64 %4, 11
  %49 = getelementptr i64, ptr %0, i64 %48
  %50 = load i64, ptr %49, align 4
  %51 = add i64 %50, %47
  %52 = or disjoint i64 %4, 12
  %53 = getelementptr i64, ptr %0, i64 %52
  %54 = load i64, ptr %53, align 4
  %55 = add i64 %54, %51
  %56 = or disjoint i64 %4, 13
  %57 = getelementptr i64, ptr %0, i64 %56
  %58 = load i64, ptr %57, align 4
  %59 = add i64 %58, %55
  %60 = or disjoint i64 %4, 14
  %61 = getelementptr i64, ptr %0, i64 %60
  %62 = load i64, ptr %61, align 4
  %63 = add i64 %62, %59
  %64 = or disjoint i64 %4, 15
  %65 = getelementptr i64, ptr %0, i64 %64
  %66 = load i64, ptr %65, align 4
  %67 = add i64 %66, %63
  %68 = add nuw nsw i64 %4, 16
  %exitcond.not.15 = icmp eq i64 %68, 1000000
  br i1 %exitcond.not.15, label %69, label %2

69:                                               ; preds = %2
  ret i64 %67
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
