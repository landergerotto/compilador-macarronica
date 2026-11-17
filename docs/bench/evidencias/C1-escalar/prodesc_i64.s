	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_i64                     # -- Begin function prodesc_i64
	.p2align	4, 0x90
	.type	prodesc_i64,@function
prodesc_i64:                            # @prodesc_i64
# %bb.0:
	movl	$7, %ecx
	xorl	%eax, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-56(%rsi,%rcx,8), %rdx
	movq	-48(%rsi,%rcx,8), %r8
	imulq	-56(%rdi,%rcx,8), %rdx
	addq	%rax, %rdx
	imulq	-48(%rdi,%rcx,8), %r8
	movq	-40(%rsi,%rcx,8), %rax
	imulq	-40(%rdi,%rcx,8), %rax
	addq	%r8, %rax
	addq	%rdx, %rax
	movq	-32(%rsi,%rcx,8), %rdx
	imulq	-32(%rdi,%rcx,8), %rdx
	movq	-24(%rsi,%rcx,8), %r8
	imulq	-24(%rdi,%rcx,8), %r8
	addq	%rdx, %r8
	movq	-16(%rsi,%rcx,8), %rdx
	imulq	-16(%rdi,%rcx,8), %rdx
	addq	%r8, %rdx
	addq	%rax, %rdx
	movq	-8(%rsi,%rcx,8), %r8
	imulq	-8(%rdi,%rcx,8), %r8
	movq	(%rsi,%rcx,8), %rax
	imulq	(%rdi,%rcx,8), %rax
	addq	%r8, %rax
	addq	%rdx, %rax
	addq	$8, %rcx
	cmpq	$1000007, %rcx                  # imm = 0xF4247
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	prodesc_i64, .Lfunc_end0-prodesc_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
