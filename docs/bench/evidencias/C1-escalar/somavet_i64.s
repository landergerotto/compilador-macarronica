	.text
	.file	"LLVMDialectModule"
	.globl	somavet_i64                     # -- Begin function somavet_i64
	.p2align	4, 0x90
	.type	somavet_i64,@function
somavet_i64:                            # @somavet_i64
# %bb.0:
	movl	$15, %ecx
	xorl	%eax, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	addq	-120(%rdi,%rcx,8), %rax
	addq	-112(%rdi,%rcx,8), %rax
	addq	-104(%rdi,%rcx,8), %rax
	addq	-96(%rdi,%rcx,8), %rax
	addq	-88(%rdi,%rcx,8), %rax
	addq	-80(%rdi,%rcx,8), %rax
	addq	-72(%rdi,%rcx,8), %rax
	addq	-64(%rdi,%rcx,8), %rax
	addq	-56(%rdi,%rcx,8), %rax
	addq	-48(%rdi,%rcx,8), %rax
	addq	-40(%rdi,%rcx,8), %rax
	addq	-32(%rdi,%rcx,8), %rax
	addq	-24(%rdi,%rcx,8), %rax
	addq	-16(%rdi,%rcx,8), %rax
	addq	-8(%rdi,%rcx,8), %rax
	addq	(%rdi,%rcx,8), %rax
	addq	$16, %rcx
	cmpq	$1000015, %rcx                  # imm = 0xF424F
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	somavet_i64, .Lfunc_end0-somavet_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
