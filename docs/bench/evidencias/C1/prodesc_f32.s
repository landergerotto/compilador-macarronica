	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_f32                     # -- Begin function prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
# %bb.0:
	vxorps	%xmm0, %xmm0, %xmm0
	movl	$7, %eax
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	vmovss	-28(%rdi,%rax,4), %xmm1         # xmm1 = mem[0],zero,zero,zero
	vmovss	-24(%rdi,%rax,4), %xmm2         # xmm2 = mem[0],zero,zero,zero
	vmulss	-28(%rsi,%rax,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmulss	-24(%rsi,%rax,4), %xmm2, %xmm1
	vmovss	-20(%rdi,%rax,4), %xmm2         # xmm2 = mem[0],zero,zero,zero
	vmulss	-20(%rsi,%rax,4), %xmm2, %xmm2
	vaddss	%xmm1, %xmm0, %xmm0
	vaddss	%xmm2, %xmm0, %xmm0
	vmovss	-16(%rdi,%rax,4), %xmm1         # xmm1 = mem[0],zero,zero,zero
	vmulss	-16(%rsi,%rax,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	-12(%rdi,%rax,4), %xmm1         # xmm1 = mem[0],zero,zero,zero
	vmulss	-12(%rsi,%rax,4), %xmm1, %xmm1
	vmovss	-8(%rdi,%rax,4), %xmm2          # xmm2 = mem[0],zero,zero,zero
	vmulss	-8(%rsi,%rax,4), %xmm2, %xmm2
	vaddss	%xmm1, %xmm0, %xmm0
	vaddss	%xmm2, %xmm0, %xmm0
	vmovss	-4(%rdi,%rax,4), %xmm1          # xmm1 = mem[0],zero,zero,zero
	vmulss	-4(%rsi,%rax,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	(%rdi,%rax,4), %xmm1            # xmm1 = mem[0],zero,zero,zero
	vmulss	(%rsi,%rax,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	addq	$8, %rax
	cmpq	$1000007, %rax                  # imm = 0xF4247
	jne	.LBB0_1
# %bb.2:
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
