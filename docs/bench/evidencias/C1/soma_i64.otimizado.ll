; ModuleID = 'n1000000/C1/soma_i64.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @soma_i64(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
vector.memcheck:
  %3 = ptrtoint ptr %2 to i64
  %4 = ptrtoint ptr %0 to i64
  %5 = ptrtoint ptr %1 to i64
  %6 = sub i64 %3, %4
  %diff.check = icmp ult i64 %6, 128
  %7 = sub i64 %3, %5
  %diff.check1 = icmp ult i64 %7, 128
  %conflict.rdx = or i1 %diff.check, %diff.check1
  br i1 %conflict.rdx, label %scalar.ph, label %vector.body

vector.body:                                      ; preds = %vector.memcheck, %vector.body
  %index = phi i64 [ %index.next.1, %vector.body ], [ 0, %vector.memcheck ]
  %8 = getelementptr i64, ptr %0, i64 %index
  %9 = getelementptr i64, ptr %8, i64 4
  %10 = getelementptr i64, ptr %8, i64 8
  %11 = getelementptr i64, ptr %8, i64 12
  %wide.load = load <4 x i64>, ptr %8, align 4
  %wide.load2 = load <4 x i64>, ptr %9, align 4
  %wide.load3 = load <4 x i64>, ptr %10, align 4
  %wide.load4 = load <4 x i64>, ptr %11, align 4
  %12 = getelementptr i64, ptr %1, i64 %index
  %13 = getelementptr i64, ptr %12, i64 4
  %14 = getelementptr i64, ptr %12, i64 8
  %15 = getelementptr i64, ptr %12, i64 12
  %wide.load5 = load <4 x i64>, ptr %12, align 4
  %wide.load6 = load <4 x i64>, ptr %13, align 4
  %wide.load7 = load <4 x i64>, ptr %14, align 4
  %wide.load8 = load <4 x i64>, ptr %15, align 4
  %16 = add <4 x i64> %wide.load5, %wide.load
  %17 = add <4 x i64> %wide.load6, %wide.load2
  %18 = add <4 x i64> %wide.load7, %wide.load3
  %19 = add <4 x i64> %wide.load8, %wide.load4
  %20 = getelementptr i64, ptr %2, i64 %index
  %21 = getelementptr i64, ptr %20, i64 4
  %22 = getelementptr i64, ptr %20, i64 8
  %23 = getelementptr i64, ptr %20, i64 12
  store <4 x i64> %16, ptr %20, align 4
  store <4 x i64> %17, ptr %21, align 4
  store <4 x i64> %18, ptr %22, align 4
  store <4 x i64> %19, ptr %23, align 4
  %index.next = or disjoint i64 %index, 16
  %24 = getelementptr i64, ptr %0, i64 %index.next
  %25 = getelementptr i64, ptr %24, i64 4
  %26 = getelementptr i64, ptr %24, i64 8
  %27 = getelementptr i64, ptr %24, i64 12
  %wide.load.1 = load <4 x i64>, ptr %24, align 4
  %wide.load2.1 = load <4 x i64>, ptr %25, align 4
  %wide.load3.1 = load <4 x i64>, ptr %26, align 4
  %wide.load4.1 = load <4 x i64>, ptr %27, align 4
  %28 = getelementptr i64, ptr %1, i64 %index.next
  %29 = getelementptr i64, ptr %28, i64 4
  %30 = getelementptr i64, ptr %28, i64 8
  %31 = getelementptr i64, ptr %28, i64 12
  %wide.load5.1 = load <4 x i64>, ptr %28, align 4
  %wide.load6.1 = load <4 x i64>, ptr %29, align 4
  %wide.load7.1 = load <4 x i64>, ptr %30, align 4
  %wide.load8.1 = load <4 x i64>, ptr %31, align 4
  %32 = add <4 x i64> %wide.load5.1, %wide.load.1
  %33 = add <4 x i64> %wide.load6.1, %wide.load2.1
  %34 = add <4 x i64> %wide.load7.1, %wide.load3.1
  %35 = add <4 x i64> %wide.load8.1, %wide.load4.1
  %36 = getelementptr i64, ptr %2, i64 %index.next
  %37 = getelementptr i64, ptr %36, i64 4
  %38 = getelementptr i64, ptr %36, i64 8
  %39 = getelementptr i64, ptr %36, i64 12
  store <4 x i64> %32, ptr %36, align 4
  store <4 x i64> %33, ptr %37, align 4
  store <4 x i64> %34, ptr %38, align 4
  store <4 x i64> %35, ptr %39, align 4
  %index.next.1 = add nuw nsw i64 %index, 32
  %40 = icmp eq i64 %index.next.1, 1000000
  br i1 %40, label %middle.block, label %vector.body, !llvm.loop !1

scalar.ph:                                        ; preds = %vector.memcheck, %scalar.ph
  %41 = phi i64 [ %97, %scalar.ph ], [ 0, %vector.memcheck ]
  %42 = getelementptr i64, ptr %0, i64 %41
  %43 = load i64, ptr %42, align 4
  %44 = getelementptr i64, ptr %1, i64 %41
  %45 = load i64, ptr %44, align 4
  %46 = add i64 %45, %43
  %47 = getelementptr i64, ptr %2, i64 %41
  store i64 %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr i64, ptr %0, i64 %48
  %50 = load i64, ptr %49, align 4
  %51 = getelementptr i64, ptr %1, i64 %48
  %52 = load i64, ptr %51, align 4
  %53 = add i64 %52, %50
  %54 = getelementptr i64, ptr %2, i64 %48
  store i64 %53, ptr %54, align 4
  %55 = or disjoint i64 %41, 2
  %56 = getelementptr i64, ptr %0, i64 %55
  %57 = load i64, ptr %56, align 4
  %58 = getelementptr i64, ptr %1, i64 %55
  %59 = load i64, ptr %58, align 4
  %60 = add i64 %59, %57
  %61 = getelementptr i64, ptr %2, i64 %55
  store i64 %60, ptr %61, align 4
  %62 = or disjoint i64 %41, 3
  %63 = getelementptr i64, ptr %0, i64 %62
  %64 = load i64, ptr %63, align 4
  %65 = getelementptr i64, ptr %1, i64 %62
  %66 = load i64, ptr %65, align 4
  %67 = add i64 %66, %64
  %68 = getelementptr i64, ptr %2, i64 %62
  store i64 %67, ptr %68, align 4
  %69 = or disjoint i64 %41, 4
  %70 = getelementptr i64, ptr %0, i64 %69
  %71 = load i64, ptr %70, align 4
  %72 = getelementptr i64, ptr %1, i64 %69
  %73 = load i64, ptr %72, align 4
  %74 = add i64 %73, %71
  %75 = getelementptr i64, ptr %2, i64 %69
  store i64 %74, ptr %75, align 4
  %76 = or disjoint i64 %41, 5
  %77 = getelementptr i64, ptr %0, i64 %76
  %78 = load i64, ptr %77, align 4
  %79 = getelementptr i64, ptr %1, i64 %76
  %80 = load i64, ptr %79, align 4
  %81 = add i64 %80, %78
  %82 = getelementptr i64, ptr %2, i64 %76
  store i64 %81, ptr %82, align 4
  %83 = or disjoint i64 %41, 6
  %84 = getelementptr i64, ptr %0, i64 %83
  %85 = load i64, ptr %84, align 4
  %86 = getelementptr i64, ptr %1, i64 %83
  %87 = load i64, ptr %86, align 4
  %88 = add i64 %87, %85
  %89 = getelementptr i64, ptr %2, i64 %83
  store i64 %88, ptr %89, align 4
  %90 = or disjoint i64 %41, 7
  %91 = getelementptr i64, ptr %0, i64 %90
  %92 = load i64, ptr %91, align 4
  %93 = getelementptr i64, ptr %1, i64 %90
  %94 = load i64, ptr %93, align 4
  %95 = add i64 %94, %92
  %96 = getelementptr i64, ptr %2, i64 %90
  store i64 %95, ptr %96, align 4
  %97 = add nuw nsw i64 %41, 8
  %exitcond.not.7 = icmp eq i64 %97, 1000000
  br i1 %exitcond.not.7, label %middle.block, label %scalar.ph, !llvm.loop !4

middle.block:                                     ; preds = %vector.body, %scalar.ph
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
!4 = distinct !{!4, !2}
