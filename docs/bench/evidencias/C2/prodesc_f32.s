	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst32,"aM",@progbits,32
	.p2align	5, 0x0                          # -- Begin function prodesc_f32
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
	.globl	prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
# %bb.0:
	addq	$96, %rdi
	addq	$96, %rsi
	vxorps	%xmm1, %xmm1, %xmm1
	movl	$999976, %eax                   # imm = 0xF4228
	movq	$-8, %rcx
	vmovdqa	.LCPI0_0(%rip), %ymm0           # ymm0 = [0,1,2,3,4,5,6,7]
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	leal	24(%rax), %edx
	vmovd	%edx, %xmm2
	vpbroadcastd	%xmm2, %ymm2
	vpcmpgtd	%ymm0, %ymm2, %ymm2
	vmaskmovps	-64(%rdi,%rcx,4), %ymm2, %ymm3
	vmaskmovps	-64(%rsi,%rcx,4), %ymm2, %ymm2
	vmulps	%ymm2, %ymm3, %ymm2
	vaddps	%ymm2, %ymm1, %ymm1
	leal	16(%rax), %edx
	vmovd	%edx, %xmm2
	vpbroadcastd	%xmm2, %ymm2
	vpcmpgtd	%ymm0, %ymm2, %ymm2
	vmaskmovps	-32(%rdi,%rcx,4), %ymm2, %ymm3
	vmaskmovps	-32(%rsi,%rcx,4), %ymm2, %ymm2
	vmulps	%ymm2, %ymm3, %ymm2
	vaddps	%ymm2, %ymm1, %ymm1
	leal	8(%rax), %edx
	vmovd	%edx, %xmm2
	vpbroadcastd	%xmm2, %ymm2
	vpcmpgtd	%ymm0, %ymm2, %ymm2
	vmaskmovps	(%rdi,%rcx,4), %ymm2, %ymm3
	vmaskmovps	(%rsi,%rcx,4), %ymm2, %ymm2
	vmulps	%ymm2, %ymm3, %ymm2
	vmovd	%eax, %xmm3
	vpbroadcastd	%xmm3, %ymm3
	vpcmpgtd	%ymm0, %ymm3, %ymm3
	vmaskmovps	32(%rdi,%rcx,4), %ymm3, %ymm4
	vmaskmovps	32(%rsi,%rcx,4), %ymm3, %ymm3
	vaddps	%ymm2, %ymm1, %ymm1
	vmulps	%ymm3, %ymm4, %ymm2
	vaddps	%ymm2, %ymm1, %ymm1
	addl	$-32, %eax
	addq	$32, %rcx
	cmpq	$999992, %rcx                   # imm = 0xF4238
	jb	.LBB0_1
# %bb.2:
	vpxor	%xmm0, %xmm0, %xmm0
	vaddss	%xmm0, %xmm1, %xmm0
	vmovshdup	%xmm1, %xmm2            # xmm2 = xmm1[1,1,3,3]
	vaddss	%xmm2, %xmm0, %xmm0
	vshufpd	$1, %xmm1, %xmm1, %xmm2         # xmm2 = xmm1[1,0]
	vaddss	%xmm2, %xmm0, %xmm0
	vshufps	$255, %xmm1, %xmm1, %xmm2       # xmm2 = xmm1[3,3,3,3]
	vaddss	%xmm2, %xmm0, %xmm0
	vextractf128	$1, %ymm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovshdup	%xmm1, %xmm2            # xmm2 = xmm1[1,1,3,3]
	vaddss	%xmm2, %xmm0, %xmm0
	vshufpd	$1, %xmm1, %xmm1, %xmm2         # xmm2 = xmm1[1,0]
	vaddss	%xmm2, %xmm0, %xmm0
	vshufps	$255, %xmm1, %xmm1, %xmm1       # xmm1 = xmm1[3,3,3,3]
	vaddss	%xmm1, %xmm0, %xmm0
	vzeroupper
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
