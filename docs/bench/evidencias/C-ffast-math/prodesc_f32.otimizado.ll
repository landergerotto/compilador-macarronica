; ModuleID = '/tmp/macaronica-bench/completo/scripts/referencia_c.c'
source_filename = "/tmp/macaronica-bench/completo/scripts/referencia_c.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local nofpclass(nan inf) float @prodesc_f32(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi i64 [ 0, %2 ], [ %58, %3 ]
  %5 = phi <8 x float> [ zeroinitializer, %2 ], [ %54, %3 ]
  %6 = phi <8 x float> [ zeroinitializer, %2 ], [ %55, %3 ]
  %7 = phi <8 x float> [ zeroinitializer, %2 ], [ %56, %3 ]
  %8 = phi <8 x float> [ zeroinitializer, %2 ], [ %57, %3 ]
  %9 = getelementptr inbounds float, ptr %0, i64 %4
  %10 = getelementptr inbounds float, ptr %9, i64 8
  %11 = getelementptr inbounds float, ptr %9, i64 16
  %12 = getelementptr inbounds float, ptr %9, i64 24
  %13 = load <8 x float>, ptr %9, align 4, !tbaa !5
  %14 = load <8 x float>, ptr %10, align 4, !tbaa !5
  %15 = load <8 x float>, ptr %11, align 4, !tbaa !5
  %16 = load <8 x float>, ptr %12, align 4, !tbaa !5
  %17 = getelementptr inbounds float, ptr %1, i64 %4
  %18 = getelementptr inbounds float, ptr %17, i64 8
  %19 = getelementptr inbounds float, ptr %17, i64 16
  %20 = getelementptr inbounds float, ptr %17, i64 24
  %21 = load <8 x float>, ptr %17, align 4, !tbaa !5
  %22 = load <8 x float>, ptr %18, align 4, !tbaa !5
  %23 = load <8 x float>, ptr %19, align 4, !tbaa !5
  %24 = load <8 x float>, ptr %20, align 4, !tbaa !5
  %25 = fmul fast <8 x float> %21, %13
  %26 = fmul fast <8 x float> %22, %14
  %27 = fmul fast <8 x float> %23, %15
  %28 = fmul fast <8 x float> %24, %16
  %29 = fadd fast <8 x float> %25, %5
  %30 = fadd fast <8 x float> %26, %6
  %31 = fadd fast <8 x float> %27, %7
  %32 = fadd fast <8 x float> %28, %8
  %33 = or disjoint i64 %4, 32
  %34 = getelementptr inbounds float, ptr %0, i64 %33
  %35 = getelementptr inbounds float, ptr %34, i64 8
  %36 = getelementptr inbounds float, ptr %34, i64 16
  %37 = getelementptr inbounds float, ptr %34, i64 24
  %38 = load <8 x float>, ptr %34, align 4, !tbaa !5
  %39 = load <8 x float>, ptr %35, align 4, !tbaa !5
  %40 = load <8 x float>, ptr %36, align 4, !tbaa !5
  %41 = load <8 x float>, ptr %37, align 4, !tbaa !5
  %42 = getelementptr inbounds float, ptr %1, i64 %33
  %43 = getelementptr inbounds float, ptr %42, i64 8
  %44 = getelementptr inbounds float, ptr %42, i64 16
  %45 = getelementptr inbounds float, ptr %42, i64 24
  %46 = load <8 x float>, ptr %42, align 4, !tbaa !5
  %47 = load <8 x float>, ptr %43, align 4, !tbaa !5
  %48 = load <8 x float>, ptr %44, align 4, !tbaa !5
  %49 = load <8 x float>, ptr %45, align 4, !tbaa !5
  %50 = fmul fast <8 x float> %46, %38
  %51 = fmul fast <8 x float> %47, %39
  %52 = fmul fast <8 x float> %48, %40
  %53 = fmul fast <8 x float> %49, %41
  %54 = fadd fast <8 x float> %50, %29
  %55 = fadd fast <8 x float> %51, %30
  %56 = fadd fast <8 x float> %52, %31
  %57 = fadd fast <8 x float> %53, %32
  %58 = add nuw nsw i64 %4, 64
  %59 = icmp eq i64 %58, 1000000
  br i1 %59, label %60, label %3, !llvm.loop !9

60:                                               ; preds = %3
  %61 = fadd fast <8 x float> %55, %54
  %62 = fadd fast <8 x float> %56, %61
  %63 = fadd fast <8 x float> %57, %62
  %64 = tail call fast float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %63)
  ret float %64
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #1

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: read) uwtable "approx-func-fp-math"="true" "denormal-fp-math"="preserve-sign,preserve-sign" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="skylake" "target-features"="+64bit,+adx,+aes,+avx,+avx2,+bmi,+bmi2,+clflushopt,+cmov,+crc32,+cx16,+cx8,+f16c,+fma,+fsgsbase,+fxsr,+invpcid,+lzcnt,+mmx,+movbe,+pclmul,+popcnt,+prfchw,+rdrnd,+rdseed,+sahf,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave,+xsavec,+xsaveopt,+xsaves,-amx-bf16,-amx-complex,-amx-fp16,-amx-int8,-amx-tile,-avx10.1-256,-avx10.1-512,-avx512bf16,-avx512bitalg,-avx512bw,-avx512cd,-avx512dq,-avx512er,-avx512f,-avx512fp16,-avx512ifma,-avx512pf,-avx512vbmi,-avx512vbmi2,-avx512vl,-avx512vnni,-avx512vp2intersect,-avx512vpopcntdq,-avxifma,-avxneconvert,-avxvnni,-avxvnniint16,-avxvnniint8,-cldemote,-clwb,-clzero,-cmpccxadd,-enqcmd,-fma4,-gfni,-hreset,-kl,-lwp,-movdir64b,-movdiri,-mwaitx,-pconfig,-pku,-prefetchi,-prefetchwt1,-ptwrite,-raoint,-rdpid,-rdpru,-rtm,-serialize,-sgx,-sha,-sha512,-shstk,-sm3,-sm4,-sse4a,-tbm,-tsxldtrk,-uintr,-usermsr,-vaes,-vpclmulqdq,-waitpkg,-wbnoinvd,-widekl,-xop" "unsafe-fp-math"="true" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

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
!9 = distinct !{!9, !10, !11, !12}
!10 = !{!"llvm.loop.mustprogress"}
!11 = !{!"llvm.loop.isvectorized", i32 1}
!12 = !{!"llvm.loop.unroll.runtime.disable"}
