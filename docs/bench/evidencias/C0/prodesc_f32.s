	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_f32                     # -- Begin function prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
	.cfi_startproc
# %bb.0:
	movl	$1, %edx
	movl	$1000000, %ecx                  # imm = 0xF4240
	movq	%rdi, %rax
	movq	%rax, -32(%rsp)                 # 8-byte Spill
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rsi, %rcx
	movq	%rcx, -24(%rsp)                 # 8-byte Spill
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	vxorps	%xmm0, %xmm0, %xmm0
	movq	%rax, -16(%rsp)                 # 8-byte Spill
	vmovss	%xmm0, -4(%rsp)                 # 4-byte Spill
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rsp), %rax                 # 8-byte Reload
	vmovss	-4(%rsp), %xmm0                 # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	vmovss	%xmm0, -44(%rsp)                # 4-byte Spill
	movq	%rax, -40(%rsp)                 # 8-byte Spill
	cmpq	$1000000, %rax                  # imm = 0xF4240
	jge	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rsp), %rax                 # 8-byte Reload
	vmovss	-44(%rsp), %xmm0                # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	movq	-24(%rsp), %rcx                 # 8-byte Reload
	movq	-32(%rsp), %rdx                 # 8-byte Reload
	vmovss	(%rdx,%rax,4), %xmm1            # xmm1 = mem[0],zero,zero,zero
	vmulss	(%rcx,%rax,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	addq	$1, %rax
	movq	%rax, -16(%rsp)                 # 8-byte Spill
	vmovss	%xmm0, -4(%rsp)                 # 4-byte Spill
	jmp	.LBB0_1
.LBB0_3:
	vmovss	-44(%rsp), %xmm0                # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
