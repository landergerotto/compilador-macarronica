; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

define void @soma_i64(ptr %0, ptr %1, ptr %2) {
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %0, 0
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, ptr %0, 1
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 0, 2
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1000000, 3, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, i64 1, 4, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %1, 0
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, ptr %1, 1
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 0, 2
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1000000, 3, 0
  %13 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, i64 1, 4, 0
  %14 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %2, 0
  %15 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %14, ptr %2, 1
  %16 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %15, i64 0, 2
  %17 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %16, i64 1000000, 3, 0
  %18 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %17, i64 1, 4, 0
  br label %19

19:                                               ; preds = %22, %3
  %20 = phi i64 [ %32, %22 ], [ 0, %3 ]
  %21 = icmp slt i64 %20, 1000000
  br i1 %21, label %22, label %33

22:                                               ; preds = %19
  %23 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, 1
  %24 = getelementptr i64, ptr %23, i64 %20
  %25 = load i64, ptr %24, align 4
  %26 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %13, 1
  %27 = getelementptr i64, ptr %26, i64 %20
  %28 = load i64, ptr %27, align 4
  %29 = add i64 %25, %28
  %30 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  %31 = getelementptr i64, ptr %30, i64 %20
  store i64 %29, ptr %31, align 4
  %32 = add i64 %20, 1
  br label %19

33:                                               ; preds = %19
  ret void
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
