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
  %8 = phi i64 [ %21, %11 ], [ 0, %1 ]
  %9 = phi <8 x i64> [ %20, %11 ], [ zeroinitializer, %1 ]
  %10 = icmp slt i64 %8, 1000000
  br i1 %10, label %11, label %22

11:                                               ; preds = %7
  %12 = sub i64 1000000, %8
  %13 = trunc i64 %12 to i32
  %14 = insertelement <8 x i32> undef, i32 %13, i32 0
  %15 = shufflevector <8 x i32> %14, <8 x i32> undef, <8 x i32> zeroinitializer
  %16 = icmp sgt <8 x i32> %15, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %17 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %18 = getelementptr i64, ptr %17, i64 %8
  %19 = call <8 x i64> @llvm.masked.load.v8i64.p0(ptr %18, i32 8, <8 x i1> %16, <8 x i64> zeroinitializer)
  %20 = add <8 x i64> %9, %19
  %21 = add i64 %8, 8
  br label %7

22:                                               ; preds = %7
  %23 = call i64 @llvm.vector.reduce.add.v8i64(<8 x i64> %9)
  ret i64 %23
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v8i64(<8 x i64>) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i64> @llvm.masked.load.v8i64.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i64>) #1

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
