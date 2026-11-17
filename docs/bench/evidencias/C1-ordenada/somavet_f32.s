	.text
	.file	"LLVMDialectModule"
	.globl	somavet_f32                     # -- Begin function somavet_f32
	.p2align	4, 0x90
	.type	somavet_f32,@function
somavet_f32:                            # @somavet_f32
# %bb.0:                                # %vector.ph
	vxorps	%xmm0, %xmm0, %xmm0
	movl	$24, %eax
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vaddss	-96(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-92(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-88(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-84(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-80(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-76(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-72(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-68(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	-64(%rdi,%rax,4), %xmm0, %xmm0
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
	vaddss	4(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	8(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	12(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	16(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	20(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	24(%rdi,%rax,4), %xmm0, %xmm0
	vaddss	28(%rdi,%rax,4), %xmm0, %xmm0
	addq	$32, %rax
	cmpq	$1000024, %rax                  # imm = 0xF4258
	jne	.LBB0_1
# %bb.2:                                # %middle.block
	retq
.Lfunc_end0:
	.size	somavet_f32, .Lfunc_end0-somavet_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
