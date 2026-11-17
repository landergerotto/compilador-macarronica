	.text
	.file	"LLVMDialectModule"
	.globl	soma_i64                        # -- Begin function soma_i64
	.p2align	4, 0x90
	.type	soma_i64,@function
soma_i64:                               # @soma_i64
# %bb.0:                                # %entry
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$152, %rsp
	movq	%rdx, %rbx
	movq	%rsi, %r14
	movq	%rdi, %r15
	leaq	.L__unnamed_1(%rip), %r12
	movq	%r12, %rdi
	callq	__kmpc_global_thread_num@PLT
	movq	%r15, 112(%rsp)
	movq	%r15, 120(%rsp)
	movq	$0, 128(%rsp)
	movq	$1000000, 136(%rsp)             # imm = 0xF4240
	movq	$1, 144(%rsp)
	movq	%r14, 72(%rsp)
	movq	%r14, 80(%rsp)
	movq	$0, 88(%rsp)
	movq	$1000000, 96(%rsp)              # imm = 0xF4240
	movq	$1, 104(%rsp)
	movq	%rbx, 32(%rsp)
	movq	%rbx, 40(%rsp)
	movq	$0, 48(%rsp)
	movq	$1000000, 56(%rsp)              # imm = 0xF4240
	movq	$1, 64(%rsp)
	leaq	112(%rsp), %rax
	movq	%rax, 8(%rsp)
	leaq	72(%rsp), %rax
	movq	%rax, 16(%rsp)
	leaq	32(%rsp), %rax
	movq	%rax, 24(%rsp)
	leaq	soma_i64..omp_par(%rip), %rdx
	leaq	8(%rsp), %rcx
	movq	%r12, %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	callq	__kmpc_fork_call@PLT
	addq	$152, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
.Lfunc_end0:
	.size	soma_i64, .Lfunc_end0-soma_i64
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function soma_i64..omp_par
	.type	soma_i64..omp_par,@function
soma_i64..omp_par:                      # @soma_i64..omp_par
# %bb.0:                                # %omp.par.entry
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$32, %rsp
	movq	(%rdx), %rax
	movq	8(%rdx), %rcx
	movq	16(%rdx), %rdx
	movq	8(%rax), %r15
	movq	8(%rcx), %r12
	movq	8(%rdx), %r13
	movq	$0, 8(%rsp)
	movq	$999999, (%rsp)                 # imm = 0xF423F
	movq	$1, 16(%rsp)
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movl	%eax, %ebx
	subq	$8, %rsp
	leaq	24(%rsp), %rax
	leaq	36(%rsp), %rcx
	leaq	16(%rsp), %r8
	leaq	8(%rsp), %r9
	movq	%r14, %rdi
	movl	%ebx, %esi
	movl	$34, %edx
	pushq	$0
	pushq	$1
	pushq	%rax
	callq	__kmpc_for_static_init_8u@PLT
	addq	$32, %rsp
	movq	8(%rsp), %rcx
	movq	(%rsp), %rax
	movq	%rax, %rdx
	subq	%rcx, %rdx
	cmpq	$-1, %rdx
	je	.LBB1_10
# %bb.1:                                # %omp_loop.body.preheader
	incq	%rax
	movq	%rax, %rsi
	subq	%rcx, %rsi
	cmpq	$16, %rsi
	jae	.LBB1_3
# %bb.2:
	xorl	%edx, %edx
.LBB1_8:                                # %omp_loop.body.preheader46
	addq	%rcx, %rdx
	.p2align	4, 0x90
.LBB1_9:                                # %omp_loop.body
                                        # =>This Inner Loop Header: Depth=1
	movq	(%r12,%rdx,8), %rcx
	addq	(%r15,%rdx,8), %rcx
	movq	%rcx, (%r13,%rdx,8)
	incq	%rdx
	cmpq	%rdx, %rax
	jne	.LBB1_9
.LBB1_10:                               # %omp_loop.exit
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	movl	%ebx, %esi
	vzeroupper
	callq	__kmpc_for_static_fini@PLT
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	leaq	.L__unnamed_2(%rip), %rdi
	movl	%eax, %esi
	callq	__kmpc_barrier@PLT
	addq	$32, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
.LBB1_3:                                # %vector.memcheck
	movq	%r13, %rdi
	subq	%r15, %rdi
	xorl	%edx, %edx
	cmpq	$128, %rdi
	jb	.LBB1_8
# %bb.4:                                # %vector.memcheck
	movq	%r13, %rdi
	subq	%r12, %rdi
	cmpq	$128, %rdi
	jb	.LBB1_8
# %bb.5:                                # %vector.ph
	movq	%rsi, %rdx
	andq	$-16, %rdx
	leaq	96(,%rcx,8), %rdi
	addq	%r13, %rdi
	leaq	(%r12,%rcx,8), %r8
	addq	$96, %r8
	leaq	(%r15,%rcx,8), %r9
	addq	$96, %r9
	xorl	%r10d, %r10d
	.p2align	4, 0x90
.LBB1_6:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovdqu	-96(%r8,%r10,8), %ymm0
	vmovdqu	-64(%r8,%r10,8), %ymm1
	vmovdqu	-32(%r8,%r10,8), %ymm2
	vmovdqu	(%r8,%r10,8), %ymm3
	vpaddq	-96(%r9,%r10,8), %ymm0, %ymm0
	vpaddq	-64(%r9,%r10,8), %ymm1, %ymm1
	vpaddq	-32(%r9,%r10,8), %ymm2, %ymm2
	vpaddq	(%r9,%r10,8), %ymm3, %ymm3
	vmovdqu	%ymm0, -96(%rdi,%r10,8)
	vmovdqu	%ymm1, -64(%rdi,%r10,8)
	vmovdqu	%ymm2, -32(%rdi,%r10,8)
	vmovdqu	%ymm3, (%rdi,%r10,8)
	addq	$16, %r10
	cmpq	%r10, %rdx
	jne	.LBB1_6
# %bb.7:                                # %middle.block
	cmpq	%rdx, %rsi
	jne	.LBB1_8
	jmp	.LBB1_10
.Lfunc_end1:
	.size	soma_i64..omp_par, .Lfunc_end1-soma_i64..omp_par
                                        # -- End function
	.type	.L__unnamed_3,@object           # @0
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__unnamed_3:
	.asciz	";unknown;unknown;0;0;;"
	.size	.L__unnamed_3, 23

	.type	.L__unnamed_1,@object           # @1
	.section	.data.rel.ro,"aw",@progbits
	.p2align	3, 0x0
.L__unnamed_1:
	.long	0                               # 0x0
	.long	2                               # 0x2
	.long	0                               # 0x0
	.long	22                              # 0x16
	.quad	.L__unnamed_3
	.size	.L__unnamed_1, 24

	.type	.L__unnamed_2,@object           # @2
	.p2align	3, 0x0
.L__unnamed_2:
	.long	0                               # 0x0
	.long	66                              # 0x42
	.long	0                               # 0x0
	.long	22                              # 0x16
	.quad	.L__unnamed_3
	.size	.L__unnamed_2, 24

	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym soma_i64..omp_par
