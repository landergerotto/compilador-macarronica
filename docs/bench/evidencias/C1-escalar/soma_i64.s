	.text
	.file	"LLVMDialectModule"
	.globl	soma_i64                        # -- Begin function soma_i64
	.p2align	4, 0x90
	.type	soma_i64,@function
soma_i64:                               # @soma_i64
# %bb.0:
	movl	$7, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
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
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	soma_i64, .Lfunc_end0-soma_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
