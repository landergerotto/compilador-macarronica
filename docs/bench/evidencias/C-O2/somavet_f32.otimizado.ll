; ModuleID = '/tmp/macaronica-bench/completo/scripts/referencia_c.c'
source_filename = "/tmp/macaronica-bench/completo/scripts/referencia_c.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local float @somavet_f32(ptr nocapture noundef readonly %0) local_unnamed_addr #0 {
  br label %3

2:                                                ; preds = %3
  ret float %68

3:                                                ; preds = %3, %1
  %4 = phi i64 [ 0, %1 ], [ %69, %3 ]
  %5 = phi float [ 0.000000e+00, %1 ], [ %68, %3 ]
  %6 = getelementptr inbounds float, ptr %0, i64 %4
  %7 = load float, ptr %6, align 4, !tbaa !5
  %8 = fadd float %5, %7
  %9 = or disjoint i64 %4, 1
  %10 = getelementptr inbounds float, ptr %0, i64 %9
  %11 = load float, ptr %10, align 4, !tbaa !5
  %12 = fadd float %8, %11
  %13 = or disjoint i64 %4, 2
  %14 = getelementptr inbounds float, ptr %0, i64 %13
  %15 = load float, ptr %14, align 4, !tbaa !5
  %16 = fadd float %12, %15
  %17 = or disjoint i64 %4, 3
  %18 = getelementptr inbounds float, ptr %0, i64 %17
  %19 = load float, ptr %18, align 4, !tbaa !5
  %20 = fadd float %16, %19
  %21 = or disjoint i64 %4, 4
  %22 = getelementptr inbounds float, ptr %0, i64 %21
  %23 = load float, ptr %22, align 4, !tbaa !5
  %24 = fadd float %20, %23
  %25 = or disjoint i64 %4, 5
  %26 = getelementptr inbounds float, ptr %0, i64 %25
  %27 = load float, ptr %26, align 4, !tbaa !5
  %28 = fadd float %24, %27
  %29 = or disjoint i64 %4, 6
  %30 = getelementptr inbounds float, ptr %0, i64 %29
  %31 = load float, ptr %30, align 4, !tbaa !5
  %32 = fadd float %28, %31
  %33 = or disjoint i64 %4, 7
  %34 = getelementptr inbounds float, ptr %0, i64 %33
  %35 = load float, ptr %34, align 4, !tbaa !5
  %36 = fadd float %32, %35
  %37 = or disjoint i64 %4, 8
  %38 = getelementptr inbounds float, ptr %0, i64 %37
  %39 = load float, ptr %38, align 4, !tbaa !5
  %40 = fadd float %36, %39
  %41 = or disjoint i64 %4, 9
  %42 = getelementptr inbounds float, ptr %0, i64 %41
  %43 = load float, ptr %42, align 4, !tbaa !5
  %44 = fadd float %40, %43
  %45 = or disjoint i64 %4, 10
  %46 = getelementptr inbounds float, ptr %0, i64 %45
  %47 = load float, ptr %46, align 4, !tbaa !5
  %48 = fadd float %44, %47
  %49 = or disjoint i64 %4, 11
  %50 = getelementptr inbounds float, ptr %0, i64 %49
  %51 = load float, ptr %50, align 4, !tbaa !5
  %52 = fadd float %48, %51
  %53 = or disjoint i64 %4, 12
  %54 = getelementptr inbounds float, ptr %0, i64 %53
  %55 = load float, ptr %54, align 4, !tbaa !5
  %56 = fadd float %52, %55
  %57 = or disjoint i64 %4, 13
  %58 = getelementptr inbounds float, ptr %0, i64 %57
  %59 = load float, ptr %58, align 4, !tbaa !5
  %60 = fadd float %56, %59
  %61 = or disjoint i64 %4, 14
  %62 = getelementptr inbounds float, ptr %0, i64 %61
  %63 = load float, ptr %62, align 4, !tbaa !5
  %64 = fadd float %60, %63
  %65 = or disjoint i64 %4, 15
  %66 = getelementptr inbounds float, ptr %0, i64 %65
  %67 = load float, ptr %66, align 4, !tbaa !5
  %68 = fadd float %64, %67
  %69 = add nuw nsw i64 %4, 16
  %70 = icmp eq i64 %69, 1000000
  br i1 %70, label %2, label %3, !llvm.loop !9
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
