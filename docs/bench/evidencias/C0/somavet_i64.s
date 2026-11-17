	.text
	.file	"LLVMDialectModule"
	.globl	somavet_i64                     # -- Begin function somavet_i64
	.p2align	4, 0x90
	.type	somavet_i64,@function
somavet_i64:                            # @somavet_i64
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
	movq	%rax, %rcx
	movq	%rcx, -16(%rsp)                 # 8-byte Spill
	movq	%rax, -8(%rsp)                  # 8-byte Spill
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rsp), %rax                 # 8-byte Reload
	movq	-8(%rsp), %rcx                  # 8-byte Reload
	movq	%rcx, -40(%rsp)                 # 8-byte Spill
	movq	%rax, -32(%rsp)                 # 8-byte Spill
	cmpq	$1000000, %rax                  # imm = 0xF4240
	jge	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-32(%rsp), %rcx                 # 8-byte Reload
	movq	-24(%rsp), %rdx                 # 8-byte Reload
	movq	-40(%rsp), %rax                 # 8-byte Reload
	addq	(%rdx,%rcx,8), %rax
	addq	$1, %rcx
	movq	%rcx, -16(%rsp)                 # 8-byte Spill
	movq	%rax, -8(%rsp)                  # 8-byte Spill
	jmp	.LBB0_1
.LBB0_3:
	movq	-40(%rsp), %rax                 # 8-byte Reload
	retq
.Lfunc_end0:
	.size	somavet_i64, .Lfunc_end0-somavet_i64
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
