	.text
	.file	"LLVMDialectModule"
	.globl	somavet_i64                     # -- Begin function somavet_i64
	.p2align	4, 0x90
	.type	somavet_i64,@function
somavet_i64:                            # @somavet_i64
# %bb.0:                                # %vector.ph
	vpxor	%xmm0, %xmm0, %xmm0
	movl	$76, %eax
	vpxor	%xmm1, %xmm1, %xmm1
	vpxor	%xmm2, %xmm2, %xmm2
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vpaddq	-608(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-576(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-544(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	-512(%rdi,%rax,8), %ymm3, %ymm3
	vpaddq	-480(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-448(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-416(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	-384(%rdi,%rax,8), %ymm3, %ymm3
	vpaddq	-352(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-320(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-288(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	-256(%rdi,%rax,8), %ymm3, %ymm3
	vpaddq	-224(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-192(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-160(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	-128(%rdi,%rax,8), %ymm3, %ymm3
	vpaddq	-96(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-64(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-32(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	(%rdi,%rax,8), %ymm3, %ymm3
	addq	$80, %rax
	cmpq	$1000076, %rax                  # imm = 0xF428C
	jne	.LBB0_1
# %bb.2:                                # %middle.block
	vpaddq	%ymm0, %ymm1, %ymm0
	vpaddq	%ymm2, %ymm3, %ymm1
	vpaddq	%ymm0, %ymm1, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, %rax
	vzeroupper
	retq
.Lfunc_end0:
	.size	somavet_i64, .Lfunc_end0-somavet_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
