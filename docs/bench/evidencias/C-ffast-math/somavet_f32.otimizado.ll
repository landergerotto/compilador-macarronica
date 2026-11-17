; ModuleID = '/tmp/macaronica-bench/completo/scripts/referencia_c.c'
source_filename = "/tmp/macaronica-bench/completo/scripts/referencia_c.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local nofpclass(nan inf) float @somavet_f32(ptr nocapture noundef readonly %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i64 [ 0, %1 ], [ %72, %2 ]
  %4 = phi <8 x float> [ zeroinitializer, %1 ], [ %68, %2 ]
  %5 = phi <8 x float> [ zeroinitializer, %1 ], [ %69, %2 ]
  %6 = phi <8 x float> [ zeroinitializer, %1 ], [ %70, %2 ]
  %7 = phi <8 x float> [ zeroinitializer, %1 ], [ %71, %2 ]
  %8 = getelementptr inbounds float, ptr %0, i64 %3
  %9 = getelementptr inbounds float, ptr %8, i64 8
  %10 = getelementptr inbounds float, ptr %8, i64 16
  %11 = getelementptr inbounds float, ptr %8, i64 24
  %12 = load <8 x float>, ptr %8, align 4, !tbaa !5
  %13 = load <8 x float>, ptr %9, align 4, !tbaa !5
  %14 = load <8 x float>, ptr %10, align 4, !tbaa !5
  %15 = load <8 x float>, ptr %11, align 4, !tbaa !5
  %16 = fadd fast <8 x float> %12, %4
  %17 = fadd fast <8 x float> %13, %5
  %18 = fadd fast <8 x float> %14, %6
  %19 = fadd fast <8 x float> %15, %7
  %20 = getelementptr float, ptr %0, i64 %3
  %21 = getelementptr float, ptr %20, i64 32
  %22 = getelementptr float, ptr %20, i64 40
  %23 = getelementptr float, ptr %20, i64 48
  %24 = getelementptr float, ptr %20, i64 56
  %25 = load <8 x float>, ptr %21, align 4, !tbaa !5
  %26 = load <8 x float>, ptr %22, align 4, !tbaa !5
  %27 = load <8 x float>, ptr %23, align 4, !tbaa !5
  %28 = load <8 x float>, ptr %24, align 4, !tbaa !5
  %29 = fadd fast <8 x float> %25, %16
  %30 = fadd fast <8 x float> %26, %17
  %31 = fadd fast <8 x float> %27, %18
  %32 = fadd fast <8 x float> %28, %19
  %33 = getelementptr float, ptr %0, i64 %3
  %34 = getelementptr float, ptr %33, i64 64
  %35 = getelementptr float, ptr %33, i64 72
  %36 = getelementptr float, ptr %33, i64 80
  %37 = getelementptr float, ptr %33, i64 88
  %38 = load <8 x float>, ptr %34, align 4, !tbaa !5
  %39 = load <8 x float>, ptr %35, align 4, !tbaa !5
  %40 = load <8 x float>, ptr %36, align 4, !tbaa !5
  %41 = load <8 x float>, ptr %37, align 4, !tbaa !5
  %42 = fadd fast <8 x float> %38, %29
  %43 = fadd fast <8 x float> %39, %30
  %44 = fadd fast <8 x float> %40, %31
  %45 = fadd fast <8 x float> %41, %32
  %46 = getelementptr float, ptr %0, i64 %3
  %47 = getelementptr float, ptr %46, i64 96
  %48 = getelementptr float, ptr %46, i64 104
  %49 = getelementptr float, ptr %46, i64 112
  %50 = getelementptr float, ptr %46, i64 120
  %51 = load <8 x float>, ptr %47, align 4, !tbaa !5
  %52 = load <8 x float>, ptr %48, align 4, !tbaa !5
  %53 = load <8 x float>, ptr %49, align 4, !tbaa !5
  %54 = load <8 x float>, ptr %50, align 4, !tbaa !5
  %55 = fadd fast <8 x float> %51, %42
  %56 = fadd fast <8 x float> %52, %43
  %57 = fadd fast <8 x float> %53, %44
  %58 = fadd fast <8 x float> %54, %45
  %59 = getelementptr float, ptr %0, i64 %3
  %60 = getelementptr float, ptr %59, i64 128
  %61 = getelementptr float, ptr %59, i64 136
  %62 = getelementptr float, ptr %59, i64 144
  %63 = getelementptr float, ptr %59, i64 152
  %64 = load <8 x float>, ptr %60, align 4, !tbaa !5
  %65 = load <8 x float>, ptr %61, align 4, !tbaa !5
  %66 = load <8 x float>, ptr %62, align 4, !tbaa !5
  %67 = load <8 x float>, ptr %63, align 4, !tbaa !5
  %68 = fadd fast <8 x float> %64, %55
  %69 = fadd fast <8 x float> %65, %56
  %70 = fadd fast <8 x float> %66, %57
  %71 = fadd fast <8 x float> %67, %58
  %72 = add nuw nsw i64 %3, 160
  %73 = icmp eq i64 %72, 1000000
  br i1 %73, label %74, label %2, !llvm.loop !9

74:                                               ; preds = %2
  %75 = fadd fast <8 x float> %69, %68
  %76 = fadd fast <8 x float> %70, %75
  %77 = fadd fast <8 x float> %71, %76
  %78 = tail call fast float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %77)
  ret float %78
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
