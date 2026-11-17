	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2, 0x0                          # -- Begin function prodesc_f32
.LCPI0_0:
	.long	0x80000000                      # float -0
	.section	.rodata.cst32,"aM",@progbits,32
	.p2align	5, 0x0
.LCPI0_1:
	.long	0x00000000                      # float 0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.long	0x80000000                      # float -0
	.text
	.globl	prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
# %bb.0:                                # %vector.ph
	vbroadcastss	.LCPI0_0(%rip), %ymm0   # ymm0 = [-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0]
	vmovaps	.LCPI0_1(%rip), %ymm1           # ymm1 = [0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0]
	movl	$56, %eax
	vmovaps	%ymm0, %ymm2
	vmovaps	%ymm0, %ymm3
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovups	-224(%rdi,%rax,4), %ymm4
	vmovups	-192(%rdi,%rax,4), %ymm5
	vmovups	-160(%rdi,%rax,4), %ymm6
	vmovups	-128(%rdi,%rax,4), %ymm7
	vmulps	-224(%rsi,%rax,4), %ymm4, %ymm4
	vaddps	%ymm4, %ymm1, %ymm1
	vmulps	-192(%rsi,%rax,4), %ymm5, %ymm4
	vaddps	%ymm4, %ymm0, %ymm0
	vmulps	-160(%rsi,%rax,4), %ymm6, %ymm4
	vaddps	%ymm4, %ymm2, %ymm2
	vmulps	-128(%rsi,%rax,4), %ymm7, %ymm4
	vaddps	%ymm4, %ymm3, %ymm3
	vmovups	-96(%rdi,%rax,4), %ymm4
	vmovups	-64(%rdi,%rax,4), %ymm5
	vmovups	-32(%rdi,%rax,4), %ymm6
	vmovups	(%rdi,%rax,4), %ymm7
	vmulps	-96(%rsi,%rax,4), %ymm4, %ymm4
	vaddps	%ymm4, %ymm1, %ymm1
	vmulps	-64(%rsi,%rax,4), %ymm5, %ymm4
	vaddps	%ymm4, %ymm0, %ymm0
	vmulps	-32(%rsi,%rax,4), %ymm6, %ymm4
	vaddps	%ymm4, %ymm2, %ymm2
	vmulps	(%rsi,%rax,4), %ymm7, %ymm4
	vaddps	%ymm4, %ymm3, %ymm3
	addq	$64, %rax
	cmpq	$1000056, %rax                  # imm = 0xF4278
	jne	.LBB0_1
# %bb.2:                                # %middle.block
	vaddps	%ymm1, %ymm0, %ymm0
	vaddps	%ymm0, %ymm2, %ymm0
	vaddps	%ymm0, %ymm3, %ymm0
	vextractf128	$1, %ymm0, %xmm1
	vaddps	%xmm1, %xmm0, %xmm0
	vshufpd	$1, %xmm0, %xmm0, %xmm1         # xmm1 = xmm0[1,0]
	vaddps	%xmm1, %xmm0, %xmm0
	vmovshdup	%xmm0, %xmm1            # xmm1 = xmm0[1,1,3,3]
	vaddss	%xmm1, %xmm0, %xmm0
	vzeroupper
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
