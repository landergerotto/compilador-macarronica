	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_f32                     # -- Begin function prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
# %bb.0:                                # %vector.ph
	vxorps	%xmm0, %xmm0, %xmm0
	movl	$24, %eax
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovups	-96(%rdi,%rax,4), %ymm1
	vmovups	-64(%rdi,%rax,4), %ymm2
	vmovups	-32(%rdi,%rax,4), %ymm3
	vmovups	(%rdi,%rax,4), %ymm4
	vmulps	-96(%rsi,%rax,4), %ymm1, %ymm5
	vmulps	-64(%rsi,%rax,4), %ymm2, %ymm6
	vmulps	-32(%rsi,%rax,4), %ymm3, %ymm2
	vmulps	(%rsi,%rax,4), %ymm4, %ymm1
	vaddss	%xmm5, %xmm0, %xmm0
	vmovshdup	%xmm5, %xmm3            # xmm3 = xmm5[1,1,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufpd	$1, %xmm5, %xmm5, %xmm3         # xmm3 = xmm5[1,0]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufps	$255, %xmm5, %xmm5, %xmm3       # xmm3 = xmm5[3,3,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vextractf128	$1, %ymm5, %xmm3
	vaddss	%xmm3, %xmm0, %xmm0
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vaddss	%xmm4, %xmm0, %xmm0
	vshufpd	$1, %xmm3, %xmm3, %xmm4         # xmm4 = xmm3[1,0]
	vaddss	%xmm4, %xmm0, %xmm0
	vshufps	$255, %xmm3, %xmm3, %xmm3       # xmm3 = xmm3[3,3,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vaddss	%xmm6, %xmm0, %xmm0
	vmovshdup	%xmm6, %xmm3            # xmm3 = xmm6[1,1,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufpd	$1, %xmm6, %xmm6, %xmm3         # xmm3 = xmm6[1,0]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufps	$255, %xmm6, %xmm6, %xmm3       # xmm3 = xmm6[3,3,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vextractf128	$1, %ymm6, %xmm3
	vaddss	%xmm3, %xmm0, %xmm0
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vaddss	%xmm4, %xmm0, %xmm0
	vshufpd	$1, %xmm3, %xmm3, %xmm4         # xmm4 = xmm3[1,0]
	vaddss	%xmm4, %xmm0, %xmm0
	vshufps	$255, %xmm3, %xmm3, %xmm3       # xmm3 = xmm3[3,3,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vaddss	%xmm2, %xmm0, %xmm0
	vmovshdup	%xmm2, %xmm3            # xmm3 = xmm2[1,1,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufpd	$1, %xmm2, %xmm2, %xmm3         # xmm3 = xmm2[1,0]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufps	$255, %xmm2, %xmm2, %xmm3       # xmm3 = xmm2[3,3,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vextractf128	$1, %ymm2, %xmm2
	vaddss	%xmm2, %xmm0, %xmm0
	vmovshdup	%xmm2, %xmm3            # xmm3 = xmm2[1,1,3,3]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufpd	$1, %xmm2, %xmm2, %xmm3         # xmm3 = xmm2[1,0]
	vaddss	%xmm3, %xmm0, %xmm0
	vshufps	$255, %xmm2, %xmm2, %xmm2       # xmm2 = xmm2[3,3,3,3]
	vaddss	%xmm2, %xmm0, %xmm0
	vaddss	%xmm1, %xmm0, %xmm0
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
	addq	$32, %rax
	cmpq	$1000024, %rax                  # imm = 0xF4258
	jne	.LBB0_1
# %bb.2:                                # %middle.block
	vzeroupper
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
