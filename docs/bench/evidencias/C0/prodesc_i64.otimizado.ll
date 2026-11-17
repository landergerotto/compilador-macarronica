; ModuleID = 'n1000000/C0/prodesc_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define i64 @prodesc_i64(ptr %0, ptr %1) {
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %0, 0
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, ptr %0, 1
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 0, 2
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1000000, 3, 0
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1, 4, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %1, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, ptr %1, 1
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, i64 0, 2
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 1000000, 3, 0
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1, 4, 0
  br label %13

13:                                               ; preds = %17, %2
  %14 = phi i64 [ %26, %17 ], [ 0, %2 ]
  %15 = phi i64 [ %25, %17 ], [ 0, %2 ]
  %16 = icmp slt i64 %14, 1000000
  br i1 %16, label %17, label %27

17:                                               ; preds = %13
  %18 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, 1
  %19 = getelementptr i64, ptr %18, i64 %14
  %20 = load i64, ptr %19, align 4
  %21 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, 1
  %22 = getelementptr i64, ptr %21, i64 %14
  %23 = load i64, ptr %22, align 4
  %24 = mul i64 %20, %23
  %25 = add i64 %15, %24
  %26 = add i64 %14, 1
  br label %13

27:                                               ; preds = %13
  ret i64 %15
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
