	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function soma_i64
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
	.globl	soma_i64
	.p2align	4, 0x90
	.type	soma_i64,@function
soma_i64:                               # @soma_i64
# %bb.0:
	movq	$-8, %rax
	movl	$1000000, %ecx                  # imm = 0xF4240
	vmovdqa	.LCPI0_0(%rip), %xmm0           # xmm0 = [0,1,2,3]
	vmovdqa	.LCPI0_1(%rip), %xmm1           # xmm1 = [4,5,6,7]
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	vmovd	%ecx, %xmm2
	vpbroadcastd	%xmm2, %xmm2
	vpcmpgtd	%xmm0, %xmm2, %xmm3
	vpmovsxdq	%xmm3, %ymm3
	vpcmpgtd	%xmm1, %xmm2, %xmm2
	vpmovsxdq	%xmm2, %ymm2
	vpmaskmovq	96(%rdi,%rax,8), %ymm2, %ymm4
	vpmaskmovq	64(%rdi,%rax,8), %ymm3, %ymm5
	vpmaskmovq	96(%rsi,%rax,8), %ymm2, %ymm6
	vpaddq	%ymm4, %ymm6, %ymm4
	vpmaskmovq	64(%rsi,%rax,8), %ymm3, %ymm6
	vpmaskmovq	%ymm4, %ymm2, 96(%rdx,%rax,8)
	vpaddq	%ymm5, %ymm6, %ymm2
	vpmaskmovq	%ymm2, %ymm3, 64(%rdx,%rax,8)
	addq	$8, %rax
	addl	$-8, %ecx
	cmpq	$999992, %rax                   # imm = 0xF4238
	jb	.LBB0_1
# %bb.2:
	vzeroupper
	retq
.Lfunc_end0:
	.size	soma_i64, .Lfunc_end0-soma_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
