; ModuleID = '/tmp/macaronica-bench/completo/scripts/referencia_c.c'
source_filename = "/tmp/macaronica-bench/completo/scripts/referencia_c.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local float @prodesc_f32(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  br label %4

3:                                                ; preds = %4
  ret float %61

4:                                                ; preds = %4, %2
  %5 = phi i64 [ 0, %2 ], [ %62, %4 ]
  %6 = phi float [ 0.000000e+00, %2 ], [ %61, %4 ]
  %7 = getelementptr inbounds float, ptr %0, i64 %5
  %8 = load float, ptr %7, align 4, !tbaa !5
  %9 = getelementptr inbounds float, ptr %1, i64 %5
  %10 = load float, ptr %9, align 4, !tbaa !5
  %11 = fmul float %8, %10
  %12 = fadd float %6, %11
  %13 = or disjoint i64 %5, 1
  %14 = getelementptr inbounds float, ptr %0, i64 %13
  %15 = load float, ptr %14, align 4, !tbaa !5
  %16 = getelementptr inbounds float, ptr %1, i64 %13
  %17 = load float, ptr %16, align 4, !tbaa !5
  %18 = fmul float %15, %17
  %19 = fadd float %12, %18
  %20 = or disjoint i64 %5, 2
  %21 = getelementptr inbounds float, ptr %0, i64 %20
  %22 = load float, ptr %21, align 4, !tbaa !5
  %23 = getelementptr inbounds float, ptr %1, i64 %20
  %24 = load float, ptr %23, align 4, !tbaa !5
  %25 = fmul float %22, %24
  %26 = fadd float %19, %25
  %27 = or disjoint i64 %5, 3
  %28 = getelementptr inbounds float, ptr %0, i64 %27
  %29 = load float, ptr %28, align 4, !tbaa !5
  %30 = getelementptr inbounds float, ptr %1, i64 %27
  %31 = load float, ptr %30, align 4, !tbaa !5
  %32 = fmul float %29, %31
  %33 = fadd float %26, %32
  %34 = or disjoint i64 %5, 4
  %35 = getelementptr inbounds float, ptr %0, i64 %34
  %36 = load float, ptr %35, align 4, !tbaa !5
  %37 = getelementptr inbounds float, ptr %1, i64 %34
  %38 = load float, ptr %37, align 4, !tbaa !5
  %39 = fmul float %36, %38
  %40 = fadd float %33, %39
  %41 = or disjoint i64 %5, 5
  %42 = getelementptr inbounds float, ptr %0, i64 %41
  %43 = load float, ptr %42, align 4, !tbaa !5
  %44 = getelementptr inbounds float, ptr %1, i64 %41
  %45 = load float, ptr %44, align 4, !tbaa !5
  %46 = fmul float %43, %45
  %47 = fadd float %40, %46
  %48 = or disjoint i64 %5, 6
  %49 = getelementptr inbounds float, ptr %0, i64 %48
  %50 = load float, ptr %49, align 4, !tbaa !5
  %51 = getelementptr inbounds float, ptr %1, i64 %48
  %52 = load float, ptr %51, align 4, !tbaa !5
  %53 = fmul float %50, %52
  %54 = fadd float %47, %53
  %55 = or disjoint i64 %5, 7
  %56 = getelementptr inbounds float, ptr %0, i64 %55
  %57 = load float, ptr %56, align 4, !tbaa !5
  %58 = getelementptr inbounds float, ptr %1, i64 %55
  %59 = load float, ptr %58, align 4, !tbaa !5
  %60 = fmul float %57, %59
  %61 = fadd float %54, %60
  %62 = add nuw nsw i64 %5, 8
  %63 = icmp eq i64 %62, 1000000
  br i1 %63, label %3, label %4, !llvm.loop !9
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="skylake" "target-features"="+64bit,+adx,+aes,+avx,+avx2,+bmi,+bmi2,+clflushopt,+cmov,+crc32,+cx16,+cx8,+f16c,+fma,+fsgsbase,+fxsr,+invpcid,+lzcnt,+mmx,+movbe,+pclmul,+popcnt,+prfchw,+rdrnd,+rdseed,+sahf,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave,+xsavec,+xsaveopt,+xsaves,-amx-bf16,-amx-complex,-amx-fp16,-amx-int8,-amx-tile,-avx10.1-256,-avx10.1-512,-avx512bf16,-avx512bitalg,-avx512bw,-avx512cd,-avx512dq,-avx512er,-avx512f,-avx512fp16,-avx512ifma,-avx512pf,-avx512vbmi,-avx512vbmi2,-avx512vl,-avx512vnni,-avx512vp2intersect,-avx512vpopcntdq,-avxifma,-avxneconvert,-avxvnni,-avxvnniint16,-avxvnniint8,-cldemote,-clwb,-clzero,-cmpccxadd,-enqcmd,-fma4,-gfni,-hreset,-kl,-lwp,-movdir64b,-movdiri,-mwaitx,-pconfig,-pku,-prefetchi,-prefetchwt1,-ptwrite,-raoint,-rdpid,-rdpru,-rtm,-serialize,-sgx,-sha,-sha512,-shstk,-sm3,-sm4,-sse4a,-tbm,-tsxldtrk,-uintr,-usermsr,-vaes,-vpclmulqdq,-waitpkg,-wbnoinvd,-widekl,-xop" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Ubuntu clang version 18.1.8 (20ubuntu8)"}
!5 = !{!6, !6, i64 0}
!6 = !{!"float", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
