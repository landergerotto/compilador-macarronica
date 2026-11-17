	.text
	.file	"LLVMDialectModule"
	.globl	somavet_i64                     # -- Begin function somavet_i64
	.p2align	4, 0x90
	.type	somavet_i64,@function
somavet_i64:                            # @somavet_i64
# %bb.0:                                # %entry
	pushq	%r14
	pushq	%rbx
	subq	$72, %rsp
	movq	%rdi, %rbx
	movq	$0, 8(%rsp)
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movq	%rbx, 32(%rsp)
	movq	%rbx, 40(%rsp)
	movq	$0, 48(%rsp)
	movq	$1000000, 56(%rsp)              # imm = 0xF4240
	movq	$1, 64(%rsp)
	leaq	32(%rsp), %rax
	movq	%rax, 16(%rsp)
	leaq	8(%rsp), %rax
	movq	%rax, 24(%rsp)
	leaq	somavet_i64..omp_par(%rip), %rdx
	leaq	16(%rsp), %rcx
	movq	%r14, %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	callq	__kmpc_fork_call@PLT
	movq	8(%rsp), %rax
	addq	$72, %rsp
	popq	%rbx
	popq	%r14
	retq
.Lfunc_end0:
	.size	somavet_i64, .Lfunc_end0-somavet_i64
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function somavet_i64..omp_par
	.type	somavet_i64..omp_par,@function
somavet_i64..omp_par:                   # @somavet_i64..omp_par
# %bb.0:                                # %omp.par.entry
	pushq	%r15
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
	movq	(%rdx), %rax
	movq	8(%rdx), %r15
	movq	8(%rax), %r12
	movq	$0, 8(%rsp)
	movq	$0, 24(%rsp)
	movq	$999999, 16(%rsp)               # imm = 0xF423F
	movq	$1, 40(%rsp)
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movl	%eax, %ebx
	subq	$8, %rsp
	leaq	48(%rsp), %rax
	leaq	60(%rsp), %rcx
	leaq	32(%rsp), %r8
	leaq	24(%rsp), %r9
	movq	%r14, %rdi
	movl	%ebx, %esi
	movl	$34, %edx
	pushq	$0
	pushq	$1
	pushq	%rax
	callq	__kmpc_for_static_init_8u@PLT
	addq	$32, %rsp
	movq	24(%rsp), %rcx
	movq	16(%rsp), %rdx
	movq	%rdx, %rax
	subq	%rcx, %rax
	cmpq	$-1, %rax
	je	.LBB1_11
# %bb.1:                                # %omp_loop.body.preheader
	leaq	1(%rdx), %rax
	movq	%rax, %rsi
	subq	%rcx, %rsi
	cmpq	$16, %rsi
	jb	.LBB1_2
# %bb.3:                                # %vector.memcheck
	leaq	(%r12,%rdx,8), %rdx
	addq	$8, %rdx
	leaq	8(%rsp), %rdi
	cmpq	%rdx, %rdi
	jae	.LBB1_5
# %bb.4:                                # %vector.memcheck
	leaq	(%r12,%rcx,8), %rdx
	leaq	16(%rsp), %rdi
	cmpq	%rdi, %rdx
	jae	.LBB1_5
.LBB1_2:
	xorl	%edx, %edx
	xorl	%esi, %esi
.LBB1_9:                                # %omp_loop.body.preheader22
	addq	%rcx, %rdx
	.p2align	4, 0x90
.LBB1_10:                               # %omp_loop.body
                                        # =>This Inner Loop Header: Depth=1
	addq	(%r12,%rdx,8), %rsi
	movq	%rsi, 8(%rsp)
	incq	%rdx
	cmpq	%rdx, %rax
	jne	.LBB1_10
.LBB1_11:                               # %omp_loop.exit
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
	leaq	8(%rsp), %rax
	movq	%rax, 32(%rsp)
	leaq	.L__unnamed_3(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movl	%eax, %ebx
	subq	$8, %rsp
	movq	.gomp_critical_user_.reduction.var@GOTPCREL(%rip), %rax
	leaq	.omp.reduction.func(%rip), %r9
	leaq	40(%rsp), %r8
	movl	$8, %ecx
	movq	%r14, %rdi
	movl	%ebx, %esi
	movl	$1, %edx
	pushq	%rax
	callq	__kmpc_reduce@PLT
	addq	$16, %rsp
	cmpl	$1, %eax
	je	.LBB1_14
# %bb.12:                               # %omp_loop.exit
	cmpl	$2, %eax
	jne	.LBB1_15
# %bb.13:                               # %reduce.switch.atomic
	movq	8(%rsp), %rax
	lock		addq	%rax, (%r15)
	jmp	.LBB1_15
.LBB1_14:                               # %reduce.switch.nonatomic
	movq	8(%rsp), %rax
	addq	%rax, (%r15)
	leaq	.L__unnamed_3(%rip), %rdi
	movq	.gomp_critical_user_.reduction.var@GOTPCREL(%rip), %rdx
	movl	%ebx, %esi
	callq	__kmpc_end_reduce@PLT
.LBB1_15:                               # %reduce.finalize
	leaq	.L__unnamed_1(%rip), %rdi
	callq	__kmpc_global_thread_num@PLT
	leaq	.L__unnamed_2(%rip), %rdi
	movl	%eax, %esi
	callq	__kmpc_barrier@PLT
	addq	$56, %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%r15
	retq
.LBB1_5:                                # %vector.ph
	movq	%rsi, %rdx
	andq	$-16, %rdx
	leaq	(%r12,%rcx,8), %rdi
	addq	$96, %rdi
	vpxor	%xmm0, %xmm0, %xmm0
	xorl	%r8d, %r8d
	vpxor	%xmm1, %xmm1, %xmm1
	vpxor	%xmm2, %xmm2, %xmm2
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB1_6:                                # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vpaddq	-96(%rdi,%r8,8), %ymm0, %ymm0
	vpaddq	-64(%rdi,%r8,8), %ymm1, %ymm1
	vpaddq	-32(%rdi,%r8,8), %ymm2, %ymm2
	vpaddq	(%rdi,%r8,8), %ymm3, %ymm3
	addq	$16, %r8
	cmpq	%r8, %rdx
	jne	.LBB1_6
# %bb.7:                                # %middle.block
	vpaddq	%ymm0, %ymm1, %ymm0
	vpaddq	%ymm2, %ymm3, %ymm1
	vpaddq	%ymm0, %ymm1, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, 8(%rsp)
	cmpq	%rdx, %rsi
	je	.LBB1_11
# %bb.8:
	vmovq	%xmm0, %rsi
	jmp	.LBB1_9
.Lfunc_end1:
	.size	somavet_i64..omp_par, .Lfunc_end1-somavet_i64..omp_par
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function .omp.reduction.func
	.type	.omp.reduction.func,@function
.omp.reduction.func:                    # @.omp.reduction.func
# %bb.0:
	movq	(%rdi), %rax
	movq	(%rsi), %rcx
	movq	(%rcx), %rcx
	addq	%rcx, (%rax)
	retq
.Lfunc_end2:
	.size	.omp.reduction.func, .Lfunc_end2-.omp.reduction.func
                                        # -- End function
	.type	.L__unnamed_4,@object           # @0
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__unnamed_4:
	.asciz	";unknown;unknown;0;0;;"
	.size	.L__unnamed_4, 23

	.type	.L__unnamed_1,@object           # @1
	.section	.data.rel.ro,"aw",@progbits
	.p2align	3, 0x0
.L__unnamed_1:
	.long	0                               # 0x0
	.long	2                               # 0x2
	.long	0                               # 0x0
	.long	22                              # 0x16
	.quad	.L__unnamed_4
	.size	.L__unnamed_1, 24

	.type	.L__unnamed_2,@object           # @2
	.p2align	3, 0x0
.L__unnamed_2:
	.long	0                               # 0x0
	.long	66                              # 0x42
	.long	0                               # 0x0
	.long	22                              # 0x16
	.quad	.L__unnamed_4
	.size	.L__unnamed_2, 24

	.type	.L__unnamed_3,@object           # @3
	.p2align	3, 0x0
.L__unnamed_3:
	.long	0                               # 0x0
	.long	18                              # 0x12
	.long	0                               # 0x0
	.long	22                              # 0x16
	.quad	.L__unnamed_4
	.size	.L__unnamed_3, 24

	.type	.gomp_critical_user_.reduction.var,@object # @.gomp_critical_user_.reduction.var
	.comm	.gomp_critical_user_.reduction.var,32,8
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym somavet_i64..omp_par
	.addrsig_sym .omp.reduction.func
	.addrsig_sym .gomp_critical_user_.reduction.var
