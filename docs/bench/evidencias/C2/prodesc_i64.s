	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function prodesc_i64
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
	.globl	prodesc_i64
	.p2align	4, 0x90
	.type	prodesc_i64,@function
prodesc_i64:                            # @prodesc_i64
# %bb.0:
	vpxor	%xmm0, %xmm0, %xmm0
	movq	$-8, %rax
	movl	$1000000, %ecx                  # imm = 0xF4240
	vmovdqa	.LCPI0_0(%rip), %xmm1           # xmm1 = [0,1,2,3]
	vmovdqa	.LCPI0_1(%rip), %xmm2           # xmm2 = [4,5,6,7]
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	vmovd	%ecx, %xmm4
	vpbroadcastd	%xmm4, %xmm4
	vpcmpgtd	%xmm1, %xmm4, %xmm5
	vpmovsxdq	%xmm5, %ymm5
	vpcmpgtd	%xmm2, %xmm4, %xmm4
	vpmovsxdq	%xmm4, %ymm4
	vpmaskmovq	96(%rdi,%rax,8), %ymm4, %ymm6
	vpmaskmovq	64(%rdi,%rax,8), %ymm5, %ymm7
	vpmaskmovq	96(%rsi,%rax,8), %ymm4, %ymm4
	vpmaskmovq	64(%rsi,%rax,8), %ymm5, %ymm5
	vpsrlq	$32, %ymm5, %ymm8
	vpmuludq	%ymm7, %ymm8, %ymm8
	vpsrlq	$32, %ymm7, %ymm9
	vpmuludq	%ymm5, %ymm9, %ymm9
	vpaddq	%ymm8, %ymm9, %ymm8
	vpsllq	$32, %ymm8, %ymm8
	vpmuludq	%ymm7, %ymm5, %ymm5
	vpaddq	%ymm0, %ymm5, %ymm0
	vpaddq	%ymm0, %ymm8, %ymm0
	vpsrlq	$32, %ymm4, %ymm5
	vpmuludq	%ymm6, %ymm5, %ymm5
	vpsrlq	$32, %ymm6, %ymm7
	vpmuludq	%ymm7, %ymm4, %ymm7
	vpaddq	%ymm5, %ymm7, %ymm5
	vpsllq	$32, %ymm5, %ymm5
	vpmuludq	%ymm6, %ymm4, %ymm4
	vpaddq	%ymm3, %ymm4, %ymm3
	vpaddq	%ymm5, %ymm3, %ymm3
	addq	$8, %rax
	addl	$-8, %ecx
	cmpq	$999992, %rax                   # imm = 0xF4238
	jb	.LBB0_1
# %bb.2:
	vpaddq	%ymm3, %ymm0, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, %rax
	vzeroupper
	retq
.Lfunc_end0:
	.size	prodesc_i64, .Lfunc_end0-prodesc_i64
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
