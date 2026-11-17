; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

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
  %14 = phi i64 [ %36, %17 ], [ 0, %2 ]
  %15 = phi <8 x i64> [ %35, %17 ], [ zeroinitializer, %2 ]
  %16 = icmp slt i64 %14, 1000000
  br i1 %16, label %17, label %37

17:                                               ; preds = %13
  %18 = sub i64 1000000, %14
  %19 = trunc i64 %18 to i32
  %20 = insertelement <8 x i32> undef, i32 %19, i32 0
  %21 = shufflevector <8 x i32> %20, <8 x i32> undef, <8 x i32> zeroinitializer
  %22 = icmp sgt <8 x i32> %21, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %23 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, 1
  %24 = getelementptr i64, ptr %23, i64 %14
  %25 = call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %24, i32 8, <8 x i1> %22, <8 x i64> zeroinitializer)
  %26 = sub i64 1000000, %14
  %27 = trunc i64 %26 to i32
  %28 = insertelement <8 x i32> undef, i32 %27, i32 0
  %29 = shufflevector <8 x i32> %28, <8 x i32> undef, <8 x i32> zeroinitializer
  %30 = icmp sgt <8 x i32> %29, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %31 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, 1
  %32 = getelementptr i64, ptr %31, i64 %14
  %33 = call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %32, i32 8, <8 x i1> %30, <8 x i64> zeroinitializer)
  %34 = mul <8 x i64> %25, %33
  %35 = add <8 x i64> %15, %34
  %36 = add i64 %14, 8
  br label %13

37:                                               ; preds = %13
  %38 = call i64 @llvm.vector.reduce.add.v8i64(<8 x i64> %15)
  ret i64 %38
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v8i64(<8 x i64>) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i64> @llvm.masked.load.v8i64.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i64>) #1

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
