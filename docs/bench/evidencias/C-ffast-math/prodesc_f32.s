	.text
	.file	"referencia_c.c"
	.globl	prodesc_f32                     # -- Begin function prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
	.cfi_startproc
# %bb.0:
	vxorps	%xmm0, %xmm0, %xmm0
	movl	$56, %eax
	vxorps	%xmm3, %xmm3, %xmm3
	vxorps	%xmm1, %xmm1, %xmm1
	vxorps	%xmm2, %xmm2, %xmm2
	.p2align	4, 0x90
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	vmovups	-224(%rsi,%rax,4), %ymm4
	vmovups	-192(%rsi,%rax,4), %ymm5
	vmovups	-160(%rsi,%rax,4), %ymm6
	vmovups	-128(%rsi,%rax,4), %ymm7
	vfmadd132ps	-224(%rdi,%rax,4), %ymm0, %ymm4 # ymm4 = (ymm4 * mem) + ymm0
	vfmadd132ps	-192(%rdi,%rax,4), %ymm3, %ymm5 # ymm5 = (ymm5 * mem) + ymm3
	vfmadd132ps	-160(%rdi,%rax,4), %ymm1, %ymm6 # ymm6 = (ymm6 * mem) + ymm1
	vfmadd132ps	-128(%rdi,%rax,4), %ymm2, %ymm7 # ymm7 = (ymm7 * mem) + ymm2
	vmovups	-96(%rsi,%rax,4), %ymm0
	vmovups	-64(%rsi,%rax,4), %ymm3
	vmovups	-32(%rsi,%rax,4), %ymm1
	vmovups	(%rsi,%rax,4), %ymm2
	vfmadd132ps	-96(%rdi,%rax,4), %ymm4, %ymm0 # ymm0 = (ymm0 * mem) + ymm4
	vfmadd132ps	-64(%rdi,%rax,4), %ymm5, %ymm3 # ymm3 = (ymm3 * mem) + ymm5
	vfmadd132ps	-32(%rdi,%rax,4), %ymm6, %ymm1 # ymm1 = (ymm1 * mem) + ymm6
	vfmadd132ps	(%rdi,%rax,4), %ymm7, %ymm2 # ymm2 = (ymm2 * mem) + ymm7
	addq	$64, %rax
	cmpq	$1000056, %rax                  # imm = 0xF4278
	jne	.LBB0_1
# %bb.2:
	vaddps	%ymm0, %ymm3, %ymm0
	vaddps	%ymm1, %ymm2, %ymm1
	vaddps	%ymm0, %ymm1, %ymm0
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
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 18.1.8 (20ubuntu8)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
