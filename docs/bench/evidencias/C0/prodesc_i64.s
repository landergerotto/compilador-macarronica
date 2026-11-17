	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_i64                     # -- Begin function prodesc_i64
	.p2align	4, 0x90
	.type	prodesc_i64,@function
prodesc_i64:                            # @prodesc_i64
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
	movq	%rax, %rcx
	movq	%rcx, -16(%rsp)                 # 8-byte Spill
	movq	%rax, -8(%rsp)                  # 8-byte Spill
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-16(%rsp), %rax                 # 8-byte Reload
	movq	-8(%rsp), %rcx                  # 8-byte Reload
	movq	%rcx, -48(%rsp)                 # 8-byte Spill
	movq	%rax, -40(%rsp)                 # 8-byte Spill
	cmpq	$1000000, %rax                  # imm = 0xF4240
	jge	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-40(%rsp), %rcx                 # 8-byte Reload
	movq	-48(%rsp), %rax                 # 8-byte Reload
	movq	-24(%rsp), %rsi                 # 8-byte Reload
	movq	-32(%rsp), %rdx                 # 8-byte Reload
	movq	(%rdx,%rcx,8), %rdx
	imulq	(%rsi,%rcx,8), %rdx
	addq	%rdx, %rax
	addq	$1, %rcx
	movq	%rcx, -16(%rsp)                 # 8-byte Spill
	movq	%rax, -8(%rsp)                  # 8-byte Spill
	jmp	.LBB0_1
.LBB0_3:
	movq	-48(%rsp), %rax                 # 8-byte Reload
	retq
.Lfunc_end0:
	.size	prodesc_i64, .Lfunc_end0-prodesc_i64
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
