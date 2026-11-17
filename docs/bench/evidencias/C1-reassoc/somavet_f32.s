	.text
	.file	"LLVMDialectModule"
	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2, 0x0                          # -- Begin function somavet_f32
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
	.globl	somavet_f32
	.p2align	4, 0x90
	.type	somavet_f32,@function
somavet_f32:                            # @somavet_f32
# %bb.0:                                # %vector.ph
	vbroadcastss	.LCPI0_0(%rip), %ymm0   # ymm0 = [-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0]
	vmovaps	.LCPI0_1(%rip), %ymm1           # ymm1 = [0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0,-0.0E+0]
	movl	$152, %eax
	vmovaps	%ymm0, %ymm2
	vmovaps	%ymm0, %ymm3
	.p2align	4, 0x90
.LBB0_1:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vaddps	-608(%rdi,%rax,4), %ymm1, %ymm1
	vaddps	-576(%rdi,%rax,4), %ymm0, %ymm0
	vaddps	-544(%rdi,%rax,4), %ymm2, %ymm2
	vaddps	-512(%rdi,%rax,4), %ymm3, %ymm3
	vaddps	-480(%rdi,%rax,4), %ymm1, %ymm1
	vaddps	-448(%rdi,%rax,4), %ymm0, %ymm0
	vaddps	-416(%rdi,%rax,4), %ymm2, %ymm2
	vaddps	-384(%rdi,%rax,4), %ymm3, %ymm3
	vaddps	-352(%rdi,%rax,4), %ymm1, %ymm1
	vaddps	-320(%rdi,%rax,4), %ymm0, %ymm0
	vaddps	-288(%rdi,%rax,4), %ymm2, %ymm2
	vaddps	-256(%rdi,%rax,4), %ymm3, %ymm3
	vaddps	-224(%rdi,%rax,4), %ymm1, %ymm1
	vaddps	-192(%rdi,%rax,4), %ymm0, %ymm0
	vaddps	-160(%rdi,%rax,4), %ymm2, %ymm2
	vaddps	-128(%rdi,%rax,4), %ymm3, %ymm3
	vaddps	-96(%rdi,%rax,4), %ymm1, %ymm1
	vaddps	-64(%rdi,%rax,4), %ymm0, %ymm0
	vaddps	-32(%rdi,%rax,4), %ymm2, %ymm2
	vaddps	(%rdi,%rax,4), %ymm3, %ymm3
	addq	$160, %rax
	cmpq	$1000152, %rax                  # imm = 0xF42D8
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
	.size	somavet_f32, .Lfunc_end0-somavet_f32
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
