	.text
	.file	"LLVMDialectModule"
	.globl	soma_i64                        # -- Begin function soma_i64
	.p2align	4, 0x90
	.type	soma_i64,@function
soma_i64:                               # @soma_i64
	.cfi_startproc
# %bb.0:
	movq	%rsi, -40(%rsp)                 # 8-byte Spill
	movq	%rdi, %rax
	movq	-40(%rsp), %rdi                 # 8-byte Reload
	movl	$1, %esi
	movl	$1000000, %ecx                  # imm = 0xF4240
	movq	%rax, %r8
	movq	%r8, -32(%rsp)                  # 8-byte Spill
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
                                        # kill: def $r8 killed $rsi
                                        # kill: def $r8 killed $rcx
	movq	%rdi, %r8
	movq	%r8, -24(%rsp)                  # 8-byte Spill
                                        # kill: def $rdi killed $rax
	movq	%rdx, %rcx
	movq	%rcx, -16(%rsp)                 # 8-byte Spill
	xorl	%eax, %eax
                                        # kill: def $rax killed $eax
	movq	%rax, -8(%rsp)                  # 8-byte Spill
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	movq	-8(%rsp), %rax                  # 8-byte Reload
	movq	%rax, -48(%rsp)                 # 8-byte Spill
	cmpq	$1000000, %rax                  # imm = 0xF4240
	jge	.LBB0_3
# %bb.2:                                #   in Loop: Header=BB0_1 Depth=1
	movq	-48(%rsp), %rax                 # 8-byte Reload
	movq	-16(%rsp), %rcx                 # 8-byte Reload
	movq	-24(%rsp), %rsi                 # 8-byte Reload
	movq	-32(%rsp), %rdx                 # 8-byte Reload
	movq	(%rdx,%rax,8), %rdx
	addq	(%rsi,%rax,8), %rdx
	movq	%rdx, (%rcx,%rax,8)
	addq	$1, %rax
	movq	%rax, -8(%rsp)                  # 8-byte Spill
	jmp	.LBB0_1
.LBB0_3:
	retq
.Lfunc_end0:
	.size	soma_i64, .Lfunc_end0-soma_i64
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.addrsig
