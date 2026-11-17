; ModuleID = 'n1000000/C1-escalar/soma_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @soma_f32(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %61, %4 ]
  %6 = getelementptr float, ptr %0, i64 %5
  %7 = load float, ptr %6, align 4
  %8 = getelementptr float, ptr %1, i64 %5
  %9 = load float, ptr %8, align 4
  %10 = fadd float %7, %9
  %11 = getelementptr float, ptr %2, i64 %5
  store float %10, ptr %11, align 4
  %12 = or disjoint i64 %5, 1
  %13 = getelementptr float, ptr %0, i64 %12
  %14 = load float, ptr %13, align 4
  %15 = getelementptr float, ptr %1, i64 %12
  %16 = load float, ptr %15, align 4
  %17 = fadd float %14, %16
  %18 = getelementptr float, ptr %2, i64 %12
  store float %17, ptr %18, align 4
  %19 = or disjoint i64 %5, 2
  %20 = getelementptr float, ptr %0, i64 %19
  %21 = load float, ptr %20, align 4
  %22 = getelementptr float, ptr %1, i64 %19
  %23 = load float, ptr %22, align 4
  %24 = fadd float %21, %23
  %25 = getelementptr float, ptr %2, i64 %19
  store float %24, ptr %25, align 4
  %26 = or disjoint i64 %5, 3
  %27 = getelementptr float, ptr %0, i64 %26
  %28 = load float, ptr %27, align 4
  %29 = getelementptr float, ptr %1, i64 %26
  %30 = load float, ptr %29, align 4
  %31 = fadd float %28, %30
  %32 = getelementptr float, ptr %2, i64 %26
  store float %31, ptr %32, align 4
  %33 = or disjoint i64 %5, 4
  %34 = getelementptr float, ptr %0, i64 %33
  %35 = load float, ptr %34, align 4
  %36 = getelementptr float, ptr %1, i64 %33
  %37 = load float, ptr %36, align 4
  %38 = fadd float %35, %37
  %39 = getelementptr float, ptr %2, i64 %33
  store float %38, ptr %39, align 4
  %40 = or disjoint i64 %5, 5
  %41 = getelementptr float, ptr %0, i64 %40
  %42 = load float, ptr %41, align 4
  %43 = getelementptr float, ptr %1, i64 %40
  %44 = load float, ptr %43, align 4
  %45 = fadd float %42, %44
  %46 = getelementptr float, ptr %2, i64 %40
  store float %45, ptr %46, align 4
  %47 = or disjoint i64 %5, 6
  %48 = getelementptr float, ptr %0, i64 %47
  %49 = load float, ptr %48, align 4
  %50 = getelementptr float, ptr %1, i64 %47
  %51 = load float, ptr %50, align 4
  %52 = fadd float %49, %51
  %53 = getelementptr float, ptr %2, i64 %47
  store float %52, ptr %53, align 4
  %54 = or disjoint i64 %5, 7
  %55 = getelementptr float, ptr %0, i64 %54
  %56 = load float, ptr %55, align 4
  %57 = getelementptr float, ptr %1, i64 %54
  %58 = load float, ptr %57, align 4
  %59 = fadd float %56, %58
  %60 = getelementptr float, ptr %2, i64 %54
  store float %59, ptr %60, align 4
  %61 = add nuw nsw i64 %5, 8
  %exitcond.not.7 = icmp eq i64 %61, 1000000
  br i1 %exitcond.not.7, label %62, label %4

62:                                               ; preds = %4
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
