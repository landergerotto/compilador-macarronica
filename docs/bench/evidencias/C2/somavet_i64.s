	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function somavet_i64
.LCPI0_0:
	.long	0                               # 0x0
	.long	1                               # 0x1
	.long	2                               # 0x2
	.long	3                               # 0x3
.LCPI0_1:
	.long	4                               # 0x4
	.long	5                               # 0x5
	.long	6                               # 0x6
	.long	7                               # 0x7
	.text
	.globl	somavet_i64
	.p2align	4, 0x90
	.type	somavet_i64,@function
somavet_i64:                            # @somavet_i64
# %bb.0:
	addq	$192, %rdi
	vpxor	%xmm2, %xmm2, %xmm2
	movl	$999976, %eax                   # imm = 0xF4228
	movq	$-8, %rcx
	vmovdqa	.LCPI0_0(%rip), %xmm0           # xmm0 = [0,1,2,3]
	vmovdqa	.LCPI0_1(%rip), %xmm1           # xmm1 = [4,5,6,7]
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	leal	24(%rax), %edx
	vmovd	%edx, %xmm4
	vpbroadcastd	%xmm4, %xmm4
	vpcmpgtd	%xmm0, %xmm4, %xmm5
	vpmovsxdq	%xmm5, %ymm5
	vpcmpgtd	%xmm1, %xmm4, %xmm4
	vpmovsxdq	%xmm4, %ymm4
	vpmaskmovq	-96(%rdi,%rcx,8), %ymm4, %ymm4
	vpaddq	%ymm3, %ymm4, %ymm3
	vpmaskmovq	-128(%rdi,%rcx,8), %ymm5, %ymm4
	vpaddq	%ymm2, %ymm4, %ymm2
	leal	16(%rax), %edx
	vmovd	%edx, %xmm4
	vpbroadcastd	%xmm4, %xmm4
	vpcmpgtd	%xmm1, %xmm4, %xmm5
	vpmovsxdq	%xmm5, %ymm5
	vpcmpgtd	%xmm0, %xmm4, %xmm4
	vpmovsxdq	%xmm4, %ymm4
	vpmaskmovq	-64(%rdi,%rcx,8), %ymm4, %ymm4
	vpmaskmovq	-32(%rdi,%rcx,8), %ymm5, %ymm5
	leal	8(%rax), %edx
	vmovd	%edx, %xmm6
	vpbroadcastd	%xmm6, %xmm6
	vpcmpgtd	%xmm0, %xmm6, %xmm7
	vpmovsxdq	%xmm7, %ymm7
	vpcmpgtd	%xmm1, %xmm6, %xmm6
	vpmovsxdq	%xmm6, %ymm6
	vpmaskmovq	32(%rdi,%rcx,8), %ymm6, %ymm6
	vpaddq	%ymm5, %ymm6, %ymm5
	vpmaskmovq	(%rdi,%rcx,8), %ymm7, %ymm6
	vpaddq	%ymm3, %ymm5, %ymm3
	vpaddq	%ymm4, %ymm6, %ymm4
	vpaddq	%ymm2, %ymm4, %ymm2
	vmovd	%eax, %xmm4
	vpbroadcastd	%xmm4, %xmm4
	vpcmpgtd	%xmm1, %xmm4, %xmm5
	vpmovsxdq	%xmm5, %ymm5
	vpcmpgtd	%xmm0, %xmm4, %xmm4
	vpmovsxdq	%xmm4, %ymm4
	vpmaskmovq	64(%rdi,%rcx,8), %ymm4, %ymm4
	vpaddq	%ymm2, %ymm4, %ymm2
	vpmaskmovq	96(%rdi,%rcx,8), %ymm5, %ymm4
	vpaddq	%ymm3, %ymm4, %ymm3
	addl	$-32, %eax
	addq	$32, %rcx
	cmpq	$999992, %rcx                   # imm = 0xF4238
	jb	.LBB0_1
# %bb.2:
	vpaddq	%ymm3, %ymm2, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, %rax
	vzeroupper
	retq
.Lfunc_end0:
	.size	somavet_i64, .Lfunc_end0-somavet_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
