; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

define i64 @somavet_i64(ptr %0) {
  %2 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %0, 0
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %2, ptr %0, 1
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, i64 0, 2
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 1000000, 3, 0
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1, 4, 0
  br label %7

7:                                                ; preds = %11, %1
  %8 = phi i64 [ %16, %11 ], [ 0, %1 ]
  %9 = phi i64 [ %15, %11 ], [ 0, %1 ]
  %10 = icmp slt i64 %8, 1000000
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %13 = getelementptr i64, ptr %12, i64 %8
  %14 = load i64, ptr %13, align 4
  %15 = add i64 %9, %14
  %16 = add i64 %8, 1
  br label %7

17:                                               ; preds = %7
  ret i64 %9
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
