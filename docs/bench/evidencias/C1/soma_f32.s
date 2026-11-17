	.text
	.file	"LLVMDialectModule"
	.globl	soma_f32                        # -- Begin function soma_f32
	.p2align	4, 0x90
	.type	soma_f32,@function
soma_f32:                               # @soma_f32
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
	movl	$56, %eax
	.p2align	4, 0x90
.LBB0_3:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovups	-224(%rdi,%rax,4), %ymm0
	vmovups	-192(%rdi,%rax,4), %ymm1
	vmovups	-160(%rdi,%rax,4), %ymm2
	vmovups	-128(%rdi,%rax,4), %ymm3
	vaddps	-224(%rsi,%rax,4), %ymm0, %ymm0
	vaddps	-192(%rsi,%rax,4), %ymm1, %ymm1
	vaddps	-160(%rsi,%rax,4), %ymm2, %ymm2
	vaddps	-128(%rsi,%rax,4), %ymm3, %ymm3
	vmovups	%ymm0, -224(%rdx,%rax,4)
	vmovups	%ymm1, -192(%rdx,%rax,4)
	vmovups	%ymm2, -160(%rdx,%rax,4)
	vmovups	%ymm3, -128(%rdx,%rax,4)
	vmovups	-96(%rdi,%rax,4), %ymm0
	vmovups	-64(%rdi,%rax,4), %ymm1
	vmovups	-32(%rdi,%rax,4), %ymm2
	vmovups	(%rdi,%rax,4), %ymm3
	vaddps	-96(%rsi,%rax,4), %ymm0, %ymm0
	vaddps	-64(%rsi,%rax,4), %ymm1, %ymm1
	vaddps	-32(%rsi,%rax,4), %ymm2, %ymm2
	vaddps	(%rsi,%rax,4), %ymm3, %ymm3
	vmovups	%ymm0, -96(%rdx,%rax,4)
	vmovups	%ymm1, -64(%rdx,%rax,4)
	vmovups	%ymm2, -32(%rdx,%rax,4)
	vmovups	%ymm3, (%rdx,%rax,4)
	addq	$64, %rax
	cmpq	$1000056, %rax                  # imm = 0xF4278
	jne	.LBB0_3
	jmp	.LBB0_6
.LBB0_4:                                # %scalar.ph.preheader
	movl	$7, %eax
	.p2align	4, 0x90
.LBB0_5:                                # %scalar.ph
                                        # =>This Inner Loop Header: Depth=1
	vmovss	-28(%rdi,%rax,4), %xmm0         # xmm0 = mem[0],zero,zero,zero
	vaddss	-28(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -28(%rdx,%rax,4)
	vmovss	-24(%rdi,%rax,4), %xmm0         # xmm0 = mem[0],zero,zero,zero
	vaddss	-24(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -24(%rdx,%rax,4)
	vmovss	-20(%rdi,%rax,4), %xmm0         # xmm0 = mem[0],zero,zero,zero
	vaddss	-20(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -20(%rdx,%rax,4)
	vmovss	-16(%rdi,%rax,4), %xmm0         # xmm0 = mem[0],zero,zero,zero
	vaddss	-16(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -16(%rdx,%rax,4)
	vmovss	-12(%rdi,%rax,4), %xmm0         # xmm0 = mem[0],zero,zero,zero
	vaddss	-12(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -12(%rdx,%rax,4)
	vmovss	-8(%rdi,%rax,4), %xmm0          # xmm0 = mem[0],zero,zero,zero
	vaddss	-8(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -8(%rdx,%rax,4)
	vmovss	-4(%rdi,%rax,4), %xmm0          # xmm0 = mem[0],zero,zero,zero
	vaddss	-4(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, -4(%rdx,%rax,4)
	vmovss	(%rdi,%rax,4), %xmm0            # xmm0 = mem[0],zero,zero,zero
	vaddss	(%rsi,%rax,4), %xmm0, %xmm0
	vmovss	%xmm0, (%rdx,%rax,4)
	addq	$8, %rax
	cmpq	$1000007, %rax                  # imm = 0xF4247
	jne	.LBB0_5
.LBB0_6:                                # %middle.block
	vzeroupper
	retq
.Lfunc_end0:
	.size	soma_f32, .Lfunc_end0-soma_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
