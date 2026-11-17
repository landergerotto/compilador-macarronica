	.text
	.file	"referencia_c.c"
	.globl	somavet_f32                     # -- Begin function somavet_f32
	.p2align	4, 0x90
	.type	somavet_f32,@function
somavet_f32:                            # @somavet_f32
	.cfi_startproc
# %bb.0:
	vxorps	%xmm0, %xmm0, %xmm0
	movl	$15, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	vaddss	-60(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-56(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-52(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-48(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-44(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-40(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-36(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-32(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-28(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-24(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-20(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-16(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-12(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-8(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-4(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	(%rdi,%rax,4), %xmm0, %xmm0
	addq	$16, %rax
	cmpq	$1000015, %rax                  # imm = 0xF424F
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	somavet_f32, .Lfunc_end0-somavet_f32
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 18.1.8 (20ubuntu8)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
