	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst32,"aM",@progbits,32
	.p2align	5, 0x0                          # -- Begin function soma_f32
.LCPI0_0:
	.long	0                               # 0x0
	.long	1                               # 0x1
	.long	2                               # 0x2
	.long	3                               # 0x3
	.long	4                               # 0x4
	.long	5                               # 0x5
	.long	6                               # 0x6
	.long	7                               # 0x7
	.text
	.globl	soma_f32
	.p2align	4, 0x90
	.type	soma_f32,@function
soma_f32:                               # @soma_f32
# %bb.0:
	addq	$32, %rdi
	addq	$32, %rsi
	addq	$32, %rdx
	movl	$999992, %eax                   # imm = 0xF4238
	movq	$-8, %rcx
	vmovdqa	.LCPI0_0(%rip), %ymm0           # ymm0 = [0,1,2,3,4,5,6,7]
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	leal	8(%rax), %r8d
	vmovd	%r8d, %xmm1
	vpbroadcastd	%xmm1, %ymm1
	vpcmpgtd	%ymm0, %ymm1, %ymm1
	vmaskmovps	(%rdi,%rcx,4), %ymm1, %ymm2
	vmaskmovps	(%rsi,%rcx,4), %ymm1, %ymm3
	vaddps	%ymm3, %ymm2, %ymm2
	vmaskmovps	%ymm2, %ymm1, (%rdx,%rcx,4)
	vmovd	%eax, %xmm1
	vpbroadcastd	%xmm1, %ymm1
	vpcmpgtd	%ymm0, %ymm1, %ymm1
	vmaskmovps	32(%rdi,%rcx,4), %ymm1, %ymm2
	vmaskmovps	32(%rsi,%rcx,4), %ymm1, %ymm3
	vaddps	%ymm3, %ymm2, %ymm2
	vmaskmovps	%ymm2, %ymm1, 32(%rdx,%rcx,4)
	addl	$-16, %eax
	addq	$16, %rcx
	cmpq	$999992, %rcx                   # imm = 0xF4238
	jb	.LBB0_1
# %bb.2:
	vzeroupper
	retq
.Lfunc_end0:
	.size	soma_f32, .Lfunc_end0-soma_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
