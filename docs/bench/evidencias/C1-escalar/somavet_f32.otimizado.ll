; ModuleID = 'n1000000/C1-escalar/somavet_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @somavet_f32(ptr nocapture readonly %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi float [ 0.000000e+00, %1 ], [ %67, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %68, %2 ]
  %5 = getelementptr float, ptr %0, i64 %4
  %6 = load float, ptr %5, align 4
  %7 = fadd float %3, %6
  %8 = or disjoint i64 %4, 1
  %9 = getelementptr float, ptr %0, i64 %8
  %10 = load float, ptr %9, align 4
  %11 = fadd float %7, %10
  %12 = or disjoint i64 %4, 2
  %13 = getelementptr float, ptr %0, i64 %12
  %14 = load float, ptr %13, align 4
  %15 = fadd float %11, %14
  %16 = or disjoint i64 %4, 3
  %17 = getelementptr float, ptr %0, i64 %16
  %18 = load float, ptr %17, align 4
  %19 = fadd float %15, %18
  %20 = or disjoint i64 %4, 4
  %21 = getelementptr float, ptr %0, i64 %20
  %22 = load float, ptr %21, align 4
  %23 = fadd float %19, %22
  %24 = or disjoint i64 %4, 5
  %25 = getelementptr float, ptr %0, i64 %24
  %26 = load float, ptr %25, align 4
  %27 = fadd float %23, %26
  %28 = or disjoint i64 %4, 6
  %29 = getelementptr float, ptr %0, i64 %28
  %30 = load float, ptr %29, align 4
  %31 = fadd float %27, %30
  %32 = or disjoint i64 %4, 7
  %33 = getelementptr float, ptr %0, i64 %32
  %34 = load float, ptr %33, align 4
  %35 = fadd float %31, %34
  %36 = or disjoint i64 %4, 8
  %37 = getelementptr float, ptr %0, i64 %36
  %38 = load float, ptr %37, align 4
  %39 = fadd float %35, %38
  %40 = or disjoint i64 %4, 9
  %41 = getelementptr float, ptr %0, i64 %40
  %42 = load float, ptr %41, align 4
  %43 = fadd float %39, %42
  %44 = or disjoint i64 %4, 10
  %45 = getelementptr float, ptr %0, i64 %44
  %46 = load float, ptr %45, align 4
  %47 = fadd float %43, %46
  %48 = or disjoint i64 %4, 11
  %49 = getelementptr float, ptr %0, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = fadd float %47, %50
  %52 = or disjoint i64 %4, 12
  %53 = getelementptr float, ptr %0, i64 %52
  %54 = load float, ptr %53, align 4
  %55 = fadd float %51, %54
  %56 = or disjoint i64 %4, 13
  %57 = getelementptr float, ptr %0, i64 %56
  %58 = load float, ptr %57, align 4
  %59 = fadd float %55, %58
  %60 = or disjoint i64 %4, 14
  %61 = getelementptr float, ptr %0, i64 %60
  %62 = load float, ptr %61, align 4
  %63 = fadd float %59, %62
  %64 = or disjoint i64 %4, 15
  %65 = getelementptr float, ptr %0, i64 %64
  %66 = load float, ptr %65, align 4
  %67 = fadd float %63, %66
  %68 = add nuw nsw i64 %4, 16
  %exitcond.not.15 = icmp eq i64 %68, 1000000
  br i1 %exitcond.not.15, label %69, label %2

69:                                               ; preds = %2
  ret float %67
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
