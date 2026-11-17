	.text
	.file	"LLVMDialectModule"
	.globl	prodesc_f32                     # -- Begin function prodesc_f32
	.p2align	4, 0x90
	.type	prodesc_f32,@function
prodesc_f32:                            # @prodesc_f32
# %bb.0:                                # %entry
	pushq	%r15
	pushq	%r14
	pushq	%rbx
	subq	$112, %rsp
	movq	%rsi, %rbx
	movq	%rdi, %r14
	movl	$0, 4(%rsp)
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
	leaq	4(%rsp), %rax
	movq	%rax, 24(%rsp)
	leaq	prodesc_f32..omp_par(%rip), %rdx
	leaq	8(%rsp), %rcx
	movq	%r15, %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	callq	__kmpc_fork_call@PLT
	vxorps	%xmm0, %xmm0, %xmm0
	vaddss	4(%rsp), %xmm0, %xmm0
	addq	$112, %rsp
	popq	%rbx
	popq	%r14
	popq	%r15
	retq
.Lfunc_end0:
	.size	prodesc_f32, .Lfunc_end0-prodesc_f32
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function prodesc_f32..omp_par
	.type	prodesc_f32..omp_par,@function
prodesc_f32..omp_par:                   # @prodesc_f32..omp_par
# %bb.0:                                # %omp.par.entry
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$48, %rsp
	movq	(%rdx), %rax
	movq	8(%rdx), %rcx
	movq	16(%rdx), %r15
	movq	8(%rax), %r12
	movq	8(%rcx), %r13
	movl	$0, 4(%rsp)
	movq	$0, 16(%rsp)
	movq	$999999, 8(%rsp)                # imm = 0xF423F
	movq	$1, 32(%rsp)
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movl	%eax, %ebx
	subq	$8, %rsp
	leaq	40(%rsp), %rax
	leaq	52(%rsp), %rcx
	leaq	24(%rsp), %r8
	leaq	16(%rsp), %r9
	movq	%r14, %rdi
	movl	%ebx, %esi
	movl	$34, %edx
	pushq	$0
	pushq	$1
	pushq	%rax
	callq	__kmpc_for_static_init_8u@PLT
	addq	$32, %rsp
	movq	16(%rsp), %rcx
	movq	8(%rsp), %rdx
	subq	%rcx, %rdx
	cmpq	$-1, %rdx
	je	.LBB1_6
# %bb.1:                                # %omp_loop.body.preheader
	leaq	1(%rdx), %rsi
	movl	%esi, %eax
	andl	$3, %eax
	cmpq	$3, %rdx
	jae	.LBB1_12
# %bb.2:
	vxorps	%xmm0, %xmm0, %xmm0
	xorl	%edx, %edx
	jmp	.LBB1_3
.LBB1_12:                               # %omp_loop.body.preheader.new
	andq	$-4, %rsi
	leaq	12(,%rcx,4), %rdi
	addq	%r13, %rdi
	leaq	(%r12,%rcx,4), %r8
	addq	$12, %r8
	vxorps	%xmm0, %xmm0, %xmm0
	xorl	%edx, %edx
	.p2align	4, 0x90
.LBB1_13:                               # %omp_loop.body
                                        # =>This Inner Loop Header: Depth=1
	vmovss	-12(%r8,%rdx,4), %xmm1          # xmm1 = mem[0],zero,zero,zero
	vmulss	-12(%rdi,%rdx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	%xmm0, 4(%rsp)
	vmovss	-8(%r8,%rdx,4), %xmm1           # xmm1 = mem[0],zero,zero,zero
	vmulss	-8(%rdi,%rdx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	%xmm0, 4(%rsp)
	vmovss	-4(%r8,%rdx,4), %xmm1           # xmm1 = mem[0],zero,zero,zero
	vmulss	-4(%rdi,%rdx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	%xmm0, 4(%rsp)
	vmovss	(%r8,%rdx,4), %xmm1             # xmm1 = mem[0],zero,zero,zero
	vmulss	(%rdi,%rdx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	%xmm0, 4(%rsp)
	addq	$4, %rdx
	cmpq	%rdx, %rsi
	jne	.LBB1_13
.LBB1_3:                                # %omp_loop.exit.loopexit.unr-lcssa
	testq	%rax, %rax
	je	.LBB1_6
# %bb.4:                                # %omp_loop.body.epil.preheader
	addq	%rcx, %rdx
	leaq	(,%rdx,4), %rcx
	addq	%r13, %rcx
	leaq	(%r12,%rdx,4), %rdx
	xorl	%esi, %esi
	.p2align	4, 0x90
.LBB1_5:                                # %omp_loop.body.epil
                                        # =>This Inner Loop Header: Depth=1
	vmovss	(%rdx,%rsi,4), %xmm1            # xmm1 = mem[0],zero,zero,zero
	vmulss	(%rcx,%rsi,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	%xmm0, 4(%rsp)
	incq	%rsi
	cmpq	%rsi, %rax
	jne	.LBB1_5
.LBB1_6:                                # %omp_loop.exit
	leaq	.L__unnamed_1(%rip), %r14
	movq	%r14, %rdi
	movl	%ebx, %esi
	callq	__kmpc_for_static_fini@PLT
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	leaq	.L__unnamed_2(%rip), %rdi
	movl	%eax, %esi
	callq	__kmpc_barrier@PLT
	leaq	4(%rsp), %rax
	movq	%rax, 24(%rsp)
	leaq	.L__unnamed_3(%rip), %r14
	movq	%r14, %rdi
	callq	__kmpc_global_thread_num@PLT
	movl	%eax, %ebx
	subq	$8, %rsp
	movq	.gomp_critical_user_.reduction.var@GOTPCREL(%rip), %rax
	leaq	.omp.reduction.func(%rip), %r9
	leaq	32(%rsp), %r8
	movl	$8, %ecx
	movq	%r14, %rdi
	movl	%ebx, %esi
	movl	$1, %edx
	pushq	%rax
	callq	__kmpc_reduce@PLT
	addq	$16, %rsp
	cmpl	$1, %eax
	je	.LBB1_10
# %bb.7:                                # %omp_loop.exit
	cmpl	$2, %eax
	jne	.LBB1_11
# %bb.8:                                # %reduce.switch.atomic
	vmovss	4(%rsp), %xmm0                  # xmm0 = mem[0],zero,zero,zero
	vmovss	(%r15), %xmm1                   # xmm1 = mem[0],zero,zero,zero
	.p2align	4, 0x90
.LBB1_9:                                # %atomicrmw.start
                                        # =>This Inner Loop Header: Depth=1
	vaddss	%xmm0, %xmm1, %xmm2
	vmovd	%xmm2, %ecx
	vmovd	%xmm1, %eax
	lock		cmpxchgl	%ecx, (%r15)
	vmovd	%eax, %xmm1
	jne	.LBB1_9
	jmp	.LBB1_11
.LBB1_10:                               # %reduce.switch.nonatomic
	vmovss	(%r15), %xmm0                   # xmm0 = mem[0],zero,zero,zero
	vaddss	4(%rsp), %xmm0, %xmm0
	vmovss	%xmm0, (%r15)
	leaq	.L__unnamed_3(%rip), %rdi
	movq	.gomp_critical_user_.reduction.var@GOTPCREL(%rip), %rdx
	movl	%ebx, %esi
	callq	__kmpc_end_reduce@PLT
.LBB1_11:                               # %reduce.finalize
	leaq	.L__unnamed_1(%rip), %rdi
	callq	__kmpc_global_thread_num@PLT
	leaq	.L__unnamed_2(%rip), %rdi
	movl	%eax, %esi
	callq	__kmpc_barrier@PLT
	addq	$48, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	retq
.Lfunc_end1:
	.size	prodesc_f32..omp_par, .Lfunc_end1-prodesc_f32..omp_par
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function .omp.reduction.func
	.type	.omp.reduction.func,@function
.omp.reduction.func:                    # @.omp.reduction.func
# %bb.0:
	movq	(%rdi), %rax
	vmovss	(%rax), %xmm0                   # xmm0 = mem[0],zero,zero,zero
	movq	(%rsi), %rcx
	vaddss	(%rcx), %xmm0, %xmm0
	vmovss	%xmm0, (%rax)
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
	.addrsig_sym prodesc_f32..omp_par
	.addrsig_sym .omp.reduction.func
	.addrsig_sym .gomp_critical_user_.reduction.var
