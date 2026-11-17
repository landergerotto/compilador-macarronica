	.text
	.file	"LLVMDialectModule"
	.globl	soma_f32                        # -- Begin function soma_f32
	.p2align	4, 0x90
	.type	soma_f32,@function
soma_f32:                               # @soma_f32
# %bb.0:
	movl	$7, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
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
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	soma_f32, .Lfunc_end0-soma_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
