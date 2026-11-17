	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_i64                     # -- Begin function prodesc_i64
	.p2align	4, 0x90
	.type	prodesc_i64,@function
prodesc_i64:                            # @prodesc_i64
# %bb.0:                                # %entry
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	subq	$112, %rsp
	movq	%rsi, %rbx
	movq	%rdi, %r14
	movq	$0, (%rsp)
	leaq	.L__unnamed_1(%rip), %r15
	movq	%r15, %rdi
	callq	__kmpc_global_thread_num@PLT
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
	leaq	72(%rsp), %rax
	movq	%rax, 8(%rsp)
	leaq	32(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	%rsp, %rax
	movq	%rax, 24(%rsp)
	leaq	prodesc_i64..omp_par(%rip), %rdx
	leaq	8(%rsp), %rcx
	movq	%r15, %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	callq	__kmpc_fork_call@PLT
	movq	(%rsp), %rax
	addq	$112, %rsp
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
.Lfunc_end0:
	.size	prodesc_i64, .Lfunc_end0-prodesc_i64
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function prodesc_i64..omp_par
	.type	prodesc_i64..omp_par,@function
prodesc_i64..omp_par:                   # @prodesc_i64..omp_par
# %bb.0:                                # %omp.par.entry
	pushq	%rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
	movq	(%rdx), %rax
	movq	8(%rdx), %rcx
	movq	16(%rdx), %r15
	movq	8(%rax), %r12
	movq	8(%rcx), %r13
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
	je	.LBB1_6
# %bb.1:                                # %omp_loop.body.preheader
	leaq	1(%rdx), %rax
	movq	%rax, %rsi
	subq	%rcx, %rsi
	cmpq	$16, %rsi
	jae	.LBB1_11
# %bb.2:
	xorl	%edx, %edx
.LBB1_3:                                # %omp_loop.body.preheader43
	xorl	%edi, %edi
.LBB1_4:                                # %omp_loop.body.preheader43
	addq	%rcx, %rdx
	.p2align	4, 0x90
.LBB1_5:                                # %omp_loop.body
                                        # =>This Inner Loop Header: Depth=1
	movq	(%r13,%rdx,8), %rcx
	imulq	(%r12,%rdx,8), %rcx
	addq	%rcx, %rdi
	movq	%rdi, 8(%rsp)
	incq	%rdx
	cmpq	%rdx, %rax
	jne	.LBB1_5
.LBB1_6:                                # %omp_loop.exit
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
	je	.LBB1_9
# %bb.7:                                # %omp_loop.exit
	cmpl	$2, %eax
	jne	.LBB1_10
# %bb.8:                                # %reduce.switch.atomic
	movq	8(%rsp), %rax
	lock		addq	%rax, (%r15)
	jmp	.LBB1_10
.LBB1_9:                                # %reduce.switch.nonatomic
	movq	8(%rsp), %rax
	addq	%rax, (%r15)
	leaq	.L__unnamed_3(%rip), %rdi
	movq	.gomp_critical_user_.reduction.var@GOTPCREL(%rip), %rdx
	movl	%ebx, %esi
	callq	__kmpc_end_reduce@PLT
.LBB1_10:                               # %reduce.finalize
	leaq	.L__unnamed_1(%rip), %rdi
	callq	__kmpc_global_thread_num@PLT
	leaq	.L__unnamed_2(%rip), %rdi
	movl	%eax, %esi
	callq	__kmpc_barrier@PLT
	addq	$56, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
.LBB1_11:                               # %vector.memcheck
	leaq	16(%rsp), %rdi
	leaq	(%r12,%rcx,8), %r8
	leaq	(%r12,%rdx,8), %r9
	addq	$8, %r9
	leaq	(,%rcx,8), %r10
	addq	%r13, %r10
	leaq	8(,%rdx,8), %rdx
	addq	%r13, %rdx
	leaq	8(%rsp), %r11
	cmpq	%r9, %r11
	setb	%bpl
	cmpq	%rdi, %r8
	setb	%r14b
	cmpq	%rdx, %r11
	setb	%r8b
	cmpq	%rdi, %r10
	setb	%r9b
	xorl	%edx, %edx
	testb	%r14b, %bpl
	jne	.LBB1_3
# %bb.12:                               # %vector.memcheck
	movl	$0, %edi
	andb	%r9b, %r8b
	jne	.LBB1_4
# %bb.13:                               # %vector.ph
	movq	%rsi, %rdx
	andq	$-16, %rdx
	leaq	96(,%rcx,8), %rdi
	addq	%r13, %rdi
	leaq	(%r12,%rcx,8), %r8
	addq	$96, %r8
	vpxor	%xmm0, %xmm0, %xmm0
	xorl	%r9d, %r9d
	vpxor	%xmm1, %xmm1, %xmm1
	vpxor	%xmm2, %xmm2, %xmm2
	vpxor	%xmm3, %xmm3, %xmm3
	.p2align	4, 0x90
.LBB1_14:                               # %vector.body
                                        # =>This Inner Loop Header: Depth=1
	vmovdqu	-96(%r8,%r9,8), %ymm6
	vmovdqu	-64(%r8,%r9,8), %ymm7
	vmovdqu	-32(%r8,%r9,8), %ymm8
	vmovdqu	(%r8,%r9,8), %ymm4
	vmovdqu	-96(%rdi,%r9,8), %ymm9
	vmovdqu	-64(%rdi,%r9,8), %ymm10
	vmovdqu	-32(%rdi,%r9,8), %ymm11
	vmovdqu	(%rdi,%r9,8), %ymm5
	vpsrlq	$32, %ymm9, %ymm12
	vpmuludq	%ymm6, %ymm12, %ymm12
	vpsrlq	$32, %ymm6, %ymm13
	vpmuludq	%ymm13, %ymm9, %ymm13
	vpaddq	%ymm12, %ymm13, %ymm12
	vpsllq	$32, %ymm12, %ymm12
	vpmuludq	%ymm6, %ymm9, %ymm6
	vpaddq	%ymm6, %ymm0, %ymm0
	vpaddq	%ymm0, %ymm12, %ymm0
	vpsrlq	$32, %ymm10, %ymm6
	vpmuludq	%ymm7, %ymm6, %ymm6
	vpsrlq	$32, %ymm7, %ymm9
	vpmuludq	%ymm9, %ymm10, %ymm9
	vpaddq	%ymm6, %ymm9, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm7, %ymm10, %ymm7
	vpaddq	%ymm7, %ymm1, %ymm1
	vpaddq	%ymm6, %ymm1, %ymm1
	vpsrlq	$32, %ymm11, %ymm6
	vpmuludq	%ymm6, %ymm8, %ymm6
	vpsrlq	$32, %ymm8, %ymm7
	vpmuludq	%ymm7, %ymm11, %ymm7
	vpaddq	%ymm6, %ymm7, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm8, %ymm11, %ymm7
	vpaddq	%ymm7, %ymm2, %ymm2
	vpaddq	%ymm6, %ymm2, %ymm2
	vpsrlq	$32, %ymm5, %ymm6
	vpmuludq	%ymm4, %ymm6, %ymm6
	vpsrlq	$32, %ymm4, %ymm7
	vpmuludq	%ymm7, %ymm5, %ymm7
	vpaddq	%ymm6, %ymm7, %ymm6
	vpsllq	$32, %ymm6, %ymm6
	vpmuludq	%ymm4, %ymm5, %ymm4
	vpaddq	%ymm4, %ymm3, %ymm3
	vpaddq	%ymm6, %ymm3, %ymm3
	addq	$16, %r9
	cmpq	%r9, %rdx
	jne	.LBB1_14
# %bb.15:                               # %middle.block
	vpaddq	%ymm0, %ymm1, %ymm0
	vpaddq	%ymm0, %ymm2, %ymm0
	vpaddq	%ymm0, %ymm3, %ymm0
	vextracti128	$1, %ymm0, %xmm1
	vpaddq	%xmm1, %xmm0, %xmm0
	vpshufd	$238, %xmm0, %xmm1              # xmm1 = xmm0[2,3,2,3]
	vpaddq	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, 8(%rsp)
	cmpq	%rdx, %rsi
	je	.LBB1_6
# %bb.16:
	vmovq	%xmm0, %rdi
	jmp	.LBB1_4
.Lfunc_end1:
	.size	prodesc_i64..omp_par, .Lfunc_end1-prodesc_i64..omp_par
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
	.addrsig_sym prodesc_i64..omp_par
	.addrsig_sym .omp.reduction.func
	.addrsig_sym .gomp_critical_user_.reduction.var
