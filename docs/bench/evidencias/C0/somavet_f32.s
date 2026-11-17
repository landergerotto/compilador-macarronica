	.text
	.file	"LLVMDialectModule"
	.globl	somavet_f32                     # -- Begin function somavet_f32
	.p2align	4, 0x90
	.type	somavet_f32,@function
somavet_f32:                            # @somavet_f32
	.cfi_startproc
# %bb.0:
	movl	$1, %eax
	movl	$1000000, %eax                  # imm = 0xF4240
	movq	%rdi, %rax
	movq	%rax, -24(%rsp)                 # 8-byte Spill
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	vxorps	%xmm0, %xmm0, %xmm0
	movq	%rax, -16(%rsp)                 # 8-byte Spill
	vmovss	%xmm0, -4(%rsp)                 # 4-byte Spill
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rsp), %rax                 # 8-byte Reload
	vmovss	-4(%rsp), %xmm0                 # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	vmovss	%xmm0, -36(%rsp)                # 4-byte Spill
	movq	%rax, -32(%rsp)                 # 8-byte Spill
	cmpq	$1000000, %rax                  # imm = 0xF4240
	jge	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rsp), %rax                 # 8-byte Reload
	vmovss	-36(%rsp), %xmm0                # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	movq	-24(%rsp), %rcx                 # 8-byte Reload
	vaddss	(%rcx,%rax,4), %xmm0, %xmm0
	addq	$1, %rax
	movq	%rax, -16(%rsp)                 # 8-byte Spill
	vmovss	%xmm0, -4(%rsp)                 # 4-byte Spill
	jmp	.LBB0_1
.LBB0_3:
	vmovss	-36(%rsp), %xmm0                # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	retq
.Lfunc_end0:
	.size	somavet_f32, .Lfunc_end0-somavet_f32
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
