	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_i64                     # -- Begin function prodesc_i64
	.p2align	4, 0x90
	.type	prodesc_i64,@function
prodesc_i64:                            # @prodesc_i64
# %bb.0:                                # %vector.ph
	vpxor	%xmm0, %xmm0, %xmm0
	movl	$12, %eax
	vpxor	%xmm1, %xmm1, %xmm1
	vpxor	%xmm2, %xmm2, %xmm2
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovdqu	-96(%rdi,%rax,8), %ymm6
	vmovdqu	-64(%rdi,%rax,8), %ymm7
	vmovdqu	-32(%rdi,%rax,8), %ymm8
	vmovdqu	(%rdi,%rax,8), %ymm4
	vmovdqu	-96(%rsi,%rax,8), %ymm9
	vmovdqu	-64(%rsi,%rax,8), %ymm10
	vmovdqu	-32(%rsi,%rax,8), %ymm11
	vmovdqu	(%rsi,%rax,8), %ymm5
	vpsrlq	$32, %ymm9, %ymm12
	vpmuludq	%ymm6, %ymm12, %ymm12
	vpsrlq	$32, %ymm6, %ymm13
	vpmuludq	%ymm13, %ymm9, %ymm13
	vpaddq	%ymm12, %ymm13, %ymm12
	vpsllq	$32, %ymm12, %ymm12
	vpmuludq	%ymm6, %ymm9, %ymm6
	vpaddq	%ymm0, %ymm6, %ymm0
	vpaddq	%ymm0, %ymm12, %ymm0
	vpsrlq	$32, %ymm10, %ymm6
	vpmuludq	%ymm7, %ymm6, %ymm6
	vpsrlq	$32, %ymm7, %ymm9
	vpmuludq	%ymm9, %ymm10, %ymm9
	vpaddq	%ymm6, %ymm9, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm7, %ymm10, %ymm7
	vpaddq	%ymm1, %ymm7, %ymm1
	vpaddq	%ymm6, %ymm1, %ymm1
	vpsrlq	$32, %ymm11, %ymm6
	vpmuludq	%ymm6, %ymm8, %ymm6
	vpsrlq	$32, %ymm8, %ymm7
	vpmuludq	%ymm7, %ymm11, %ymm7
	vpaddq	%ymm6, %ymm7, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm8, %ymm11, %ymm7
	vpaddq	%ymm2, %ymm7, %ymm2
	vpaddq	%ymm6, %ymm2, %ymm2
	vpsrlq	$32, %ymm5, %ymm6
	vpmuludq	%ymm4, %ymm6, %ymm6
	vpsrlq	$32, %ymm4, %ymm7
	vpmuludq	%ymm7, %ymm5, %ymm7
	vpaddq	%ymm6, %ymm7, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm4, %ymm5, %ymm4
	vpaddq	%ymm3, %ymm4, %ymm3
	vpaddq	%ymm6, %ymm3, %ymm3
	addq	$16, %rax
	cmpq	$1000012, %rax                  # imm = 0xF424C
	jne	.LBB0_1
# %bb.2:                                # %middle.block
	vpaddq	%ymm0, %ymm1, %ymm0
	vpaddq	%ymm0, %ymm2, %ymm0
	vpaddq	%ymm0, %ymm3, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, %rax
	vzeroupper
	retq
.Lfunc_end0:
	.size	prodesc_i64, .Lfunc_end0-prodesc_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
