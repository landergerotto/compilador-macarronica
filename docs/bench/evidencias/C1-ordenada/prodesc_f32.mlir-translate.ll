; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

define float @prodesc_f32(ptr %0, ptr %1) {
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
  %15 = phi float [ %25, %17 ], [ 0.000000e+00, %2 ]
  %16 = icmp slt i64 %14, 1000000
  br i1 %16, label %17, label %27

17:                                               ; preds = %13
  %18 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, 1
  %19 = getelementptr float, ptr %18, i64 %14
  %20 = load float, ptr %19, align 4
  %21 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, 1
  %22 = getelementptr float, ptr %21, i64 %14
  %23 = load float, ptr %22, align 4
  %24 = fmul float %20, %23
  %25 = fadd float %15, %24
  %26 = add i64 %14, 1
  br label %13

27:                                               ; preds = %13
  ret float %15
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
