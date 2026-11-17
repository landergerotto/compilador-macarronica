	.text
	.file	"LLVMDialectModule"
	.globl	soma_i64                        # -- Begin function soma_i64
	.p2align	4, 0x90
	.type	soma_i64,@function
soma_i64:                               # @soma_i64
# %bb.0:                                # %vector.memcheck
	movq	%rdx, %rax
	subq	%rdi, %rax
	cmpq	$128, %rax
	jb	.LBB0_4
# %bb.1:                                # %vector.memcheck
	movq	%rdx, %rax
	subq	%rsi, %rax
	cmpq	$128, %rax
	jb	.LBB0_4
# %bb.2:                                # %vector.body.preheader
	movl	$28, %eax
	.p2align	4, 0x90
.LBB0_3:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovdqu	-224(%rsi,%rax,8), %ymm0
	vmovdqu	-192(%rsi,%rax,8), %ymm1
	vmovdqu	-160(%rsi,%rax,8), %ymm2
	vmovdqu	-128(%rsi,%rax,8), %ymm3
	vpaddq	-224(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-192(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-160(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	-128(%rdi,%rax,8), %ymm3, %ymm3
	vmovdqu	%ymm0, -224(%rdx,%rax,8)
	vmovdqu	%ymm1, -192(%rdx,%rax,8)
	vmovdqu	%ymm2, -160(%rdx,%rax,8)
	vmovdqu	%ymm3, -128(%rdx,%rax,8)
	vmovdqu	-96(%rsi,%rax,8), %ymm0
	vmovdqu	-64(%rsi,%rax,8), %ymm1
	vmovdqu	-32(%rsi,%rax,8), %ymm2
	vmovdqu	(%rsi,%rax,8), %ymm3
	vpaddq	-96(%rdi,%rax,8), %ymm0, %ymm0
	vpaddq	-64(%rdi,%rax,8), %ymm1, %ymm1
	vpaddq	-32(%rdi,%rax,8), %ymm2, %ymm2
	vpaddq	(%rdi,%rax,8), %ymm3, %ymm3
	vmovdqu	%ymm0, -96(%rdx,%rax,8)
	vmovdqu	%ymm1, -64(%rdx,%rax,8)
	vmovdqu	%ymm2, -32(%rdx,%rax,8)
	vmovdqu	%ymm3, (%rdx,%rax,8)
	addq	$32, %rax
	cmpq	$1000028, %rax                  # imm = 0xF425C
	jne	.LBB0_3
	jmp	.LBB0_6
.LBB0_4:                                # %scalar.ph.preheader
	movl	$7, %eax
	.p2align	4, 0x90
.LBB0_5:                                # %scalar.ph
                                        # =>This Inner Loop Header: Depth=1
	movq	-56(%rsi,%rax,8), %rcx
	addq	-56(%rdi,%rax,8), %rcx
	movq	%rcx, -56(%rdx,%rax,8)
	movq	-48(%rsi,%rax,8), %rcx
	addq	-48(%rdi,%rax,8), %rcx
	movq	%rcx, -48(%rdx,%rax,8)
	movq	-40(%rsi,%rax,8), %rcx
	addq	-40(%rdi,%rax,8), %rcx
	movq	%rcx, -40(%rdx,%rax,8)
	movq	-32(%rsi,%rax,8), %rcx
	addq	-32(%rdi,%rax,8), %rcx
	movq	%rcx, -32(%rdx,%rax,8)
	movq	-24(%rsi,%rax,8), %rcx
	addq	-24(%rdi,%rax,8), %rcx
	movq	%rcx, -24(%rdx,%rax,8)
	movq	-16(%rsi,%rax,8), %rcx
	addq	-16(%rdi,%rax,8), %rcx
	movq	%rcx, -16(%rdx,%rax,8)
	movq	-8(%rsi,%rax,8), %rcx
	addq	-8(%rdi,%rax,8), %rcx
	movq	%rcx, -8(%rdx,%rax,8)
	movq	(%rsi,%rax,8), %rcx
	addq	(%rdi,%rax,8), %rcx
	movq	%rcx, (%rdx,%rax,8)
	addq	$8, %rax
	cmpq	$1000007, %rax                  # imm = 0xF4247
	jne	.LBB0_5
.LBB0_6:                                # %middle.block
	vzeroupper
	retq
.Lfunc_end0:
	.size	soma_i64, .Lfunc_end0-soma_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
