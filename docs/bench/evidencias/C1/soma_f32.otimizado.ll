; ModuleID = 'n1000000/C1/soma_f32.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @soma_f32(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
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
  %8 = getelementptr float, ptr %0, i64 %index
  %9 = getelementptr float, ptr %8, i64 8
  %10 = getelementptr float, ptr %8, i64 16
  %11 = getelementptr float, ptr %8, i64 24
  %wide.load = load <8 x float>, ptr %8, align 4
  %wide.load2 = load <8 x float>, ptr %9, align 4
  %wide.load3 = load <8 x float>, ptr %10, align 4
  %wide.load4 = load <8 x float>, ptr %11, align 4
  %12 = getelementptr float, ptr %1, i64 %index
  %13 = getelementptr float, ptr %12, i64 8
  %14 = getelementptr float, ptr %12, i64 16
  %15 = getelementptr float, ptr %12, i64 24
  %wide.load5 = load <8 x float>, ptr %12, align 4
  %wide.load6 = load <8 x float>, ptr %13, align 4
  %wide.load7 = load <8 x float>, ptr %14, align 4
  %wide.load8 = load <8 x float>, ptr %15, align 4
  %16 = fadd <8 x float> %wide.load, %wide.load5
  %17 = fadd <8 x float> %wide.load2, %wide.load6
  %18 = fadd <8 x float> %wide.load3, %wide.load7
  %19 = fadd <8 x float> %wide.load4, %wide.load8
  %20 = getelementptr float, ptr %2, i64 %index
  %21 = getelementptr float, ptr %20, i64 8
  %22 = getelementptr float, ptr %20, i64 16
  %23 = getelementptr float, ptr %20, i64 24
  store <8 x float> %16, ptr %20, align 4
  store <8 x float> %17, ptr %21, align 4
  store <8 x float> %18, ptr %22, align 4
  store <8 x float> %19, ptr %23, align 4
  %index.next = or disjoint i64 %index, 32
  %24 = getelementptr float, ptr %0, i64 %index.next
  %25 = getelementptr float, ptr %24, i64 8
  %26 = getelementptr float, ptr %24, i64 16
  %27 = getelementptr float, ptr %24, i64 24
  %wide.load.1 = load <8 x float>, ptr %24, align 4
  %wide.load2.1 = load <8 x float>, ptr %25, align 4
  %wide.load3.1 = load <8 x float>, ptr %26, align 4
  %wide.load4.1 = load <8 x float>, ptr %27, align 4
  %28 = getelementptr float, ptr %1, i64 %index.next
  %29 = getelementptr float, ptr %28, i64 8
  %30 = getelementptr float, ptr %28, i64 16
  %31 = getelementptr float, ptr %28, i64 24
  %wide.load5.1 = load <8 x float>, ptr %28, align 4
  %wide.load6.1 = load <8 x float>, ptr %29, align 4
  %wide.load7.1 = load <8 x float>, ptr %30, align 4
  %wide.load8.1 = load <8 x float>, ptr %31, align 4
  %32 = fadd <8 x float> %wide.load.1, %wide.load5.1
  %33 = fadd <8 x float> %wide.load2.1, %wide.load6.1
  %34 = fadd <8 x float> %wide.load3.1, %wide.load7.1
  %35 = fadd <8 x float> %wide.load4.1, %wide.load8.1
  %36 = getelementptr float, ptr %2, i64 %index.next
  %37 = getelementptr float, ptr %36, i64 8
  %38 = getelementptr float, ptr %36, i64 16
  %39 = getelementptr float, ptr %36, i64 24
  store <8 x float> %32, ptr %36, align 4
  store <8 x float> %33, ptr %37, align 4
  store <8 x float> %34, ptr %38, align 4
  store <8 x float> %35, ptr %39, align 4
  %index.next.1 = add nuw nsw i64 %index, 64
  %40 = icmp eq i64 %index.next.1, 1000000
  br i1 %40, label %middle.block, label %vector.body, !llvm.loop !1

scalar.ph:                                        ; preds = %vector.memcheck, %scalar.ph
  %41 = phi i64 [ %97, %scalar.ph ], [ 0, %vector.memcheck ]
  %42 = getelementptr float, ptr %0, i64 %41
  %43 = load float, ptr %42, align 4
  %44 = getelementptr float, ptr %1, i64 %41
  %45 = load float, ptr %44, align 4
  %46 = fadd float %43, %45
  %47 = getelementptr float, ptr %2, i64 %41
  store float %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr float, ptr %0, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = getelementptr float, ptr %1, i64 %48
  %52 = load float, ptr %51, align 4
  %53 = fadd float %50, %52
  %54 = getelementptr float, ptr %2, i64 %48
  store float %53, ptr %54, align 4
  %55 = or disjoint i64 %41, 2
  %56 = getelementptr float, ptr %0, i64 %55
  %57 = load float, ptr %56, align 4
  %58 = getelementptr float, ptr %1, i64 %55
  %59 = load float, ptr %58, align 4
  %60 = fadd float %57, %59
  %61 = getelementptr float, ptr %2, i64 %55
  store float %60, ptr %61, align 4
  %62 = or disjoint i64 %41, 3
  %63 = getelementptr float, ptr %0, i64 %62
  %64 = load float, ptr %63, align 4
  %65 = getelementptr float, ptr %1, i64 %62
  %66 = load float, ptr %65, align 4
  %67 = fadd float %64, %66
  %68 = getelementptr float, ptr %2, i64 %62
  store float %67, ptr %68, align 4
  %69 = or disjoint i64 %41, 4
  %70 = getelementptr float, ptr %0, i64 %69
  %71 = load float, ptr %70, align 4
  %72 = getelementptr float, ptr %1, i64 %69
  %73 = load float, ptr %72, align 4
  %74 = fadd float %71, %73
  %75 = getelementptr float, ptr %2, i64 %69
  store float %74, ptr %75, align 4
  %76 = or disjoint i64 %41, 5
  %77 = getelementptr float, ptr %0, i64 %76
  %78 = load float, ptr %77, align 4
  %79 = getelementptr float, ptr %1, i64 %76
  %80 = load float, ptr %79, align 4
  %81 = fadd float %78, %80
  %82 = getelementptr float, ptr %2, i64 %76
  store float %81, ptr %82, align 4
  %83 = or disjoint i64 %41, 6
  %84 = getelementptr float, ptr %0, i64 %83
  %85 = load float, ptr %84, align 4
  %86 = getelementptr float, ptr %1, i64 %83
  %87 = load float, ptr %86, align 4
  %88 = fadd float %85, %87
  %89 = getelementptr float, ptr %2, i64 %83
  store float %88, ptr %89, align 4
  %90 = or disjoint i64 %41, 7
  %91 = getelementptr float, ptr %0, i64 %90
  %92 = load float, ptr %91, align 4
  %93 = getelementptr float, ptr %1, i64 %90
  %94 = load float, ptr %93, align 4
  %95 = fadd float %92, %94
  %96 = getelementptr float, ptr %2, i64 %90
  store float %95, ptr %96, align 4
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
