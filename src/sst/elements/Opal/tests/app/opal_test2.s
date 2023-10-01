	.file	"opal_test2.c"
	.text
	.section	.rodata
.LC0:
	.string	"Inside Ariel"
	.text
	.globl	ariel_enable
	.type	ariel_enable, @function
ariel_enable:
.LFB15:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	ariel_enable, .-ariel_enable
	.section	.rodata
	.align 8
.LC1:
	.string	"Allocating arrays of size %d elements.\n"
.LC2:
	.string	"Done allocating arrays."
	.align 8
.LC3:
	.string	"Perfoming the fast_c compute loop..."
.LC5:
	.string	"Sum of arrays is: %f\n"
.LC6:
	.string	"Freeing arrays..."
.LC7:
	.string	"Done."
	.text
	.globl	main
	.type	main, @function
main:
.LFB16:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$80, %rsp
	movl	%edi, -68(%rbp)
	movq	%rsi, -80(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$3000, -60(%rbp)
	call	ariel_enable
	movl	$3000, %esi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$24000, %edi
	call	malloc@PLT
	movq	%rax, -48(%rbp)
	movl	$24000, %edi
	call	malloc@PLT
	movq	%rax, -40(%rbp)
	leaq	.LC2(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$0, -64(%rbp)
	jmp	.L3
.L4:
	movl	-64(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-48(%rbp), %rax
	addq	%rdx, %rax
	pxor	%xmm0, %xmm0
	cvtsi2sdl	-64(%rbp), %xmm0
	movsd	%xmm0, (%rax)
	movl	$3000, %eax
	subl	-64(%rbp), %eax
	movl	%eax, %edx
	movl	-64(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rcx
	movq	-40(%rbp), %rax
	addq	%rcx, %rax
	pxor	%xmm0, %xmm0
	cvtsi2sdl	%edx, %xmm0
	movsd	%xmm0, (%rax)
	addl	$1, -64(%rbp)
.L3:
	cmpl	$2999, -64(%rbp)
	jle	.L4
	leaq	.LC3(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movq	-40(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -32(%rbp)
	movl	-64(%rbp), %eax
	movl	%eax, -16(%rbp)
	leaq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$2, %edx
	movq	%rax, %rsi
	leaq	main._omp_fn.0(%rip), %rax
	movq	%rax, %rdi
	call	GOMP_parallel@PLT
	movl	-16(%rbp), %eax
	movl	%eax, -64(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, -56(%rbp)
	movl	$0, -64(%rbp)
	jmp	.L5
.L6:
	movl	-64(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movsd	-56(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, -56(%rbp)
	addl	$1, -64(%rbp)
.L5:
	cmpl	$2999, -64(%rbp)
	jle	.L6
	movq	-56(%rbp), %rax
	movq	%rax, %xmm0
	leaq	.LC5(%rip), %rax
	movq	%rax, %rdi
	movl	$1, %eax
	call	printf@PLT
	leaq	.LC6(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movq	-48(%rbp), %rax
	movq	%rax, %rdi
	call	free@PLT
	movq	-40(%rbp), %rax
	movq	%rax, %rdi
	call	free@PLT
	leaq	.LC7(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$0, %eax
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L8
	call	__stack_chk_fail@PLT
.L8:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	main, .-main
	.type	main._omp_fn.0, @function
main._omp_fn.0:
.LFB17:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -8(%rbp)
	movq	-24(%rbp), %rax
	movl	$0, 16(%rax)
	nop
.L11:
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	cmpl	$2999, %eax
	jg	.L13
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movapd	%xmm0, %xmm1
	addsd	%xmm0, %xmm1
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-16(%rbp), %rax
	addq	%rdx, %rax
	movsd	.LC8(%rip), %xmm0
	addsd	%xmm1, %xmm0
	movsd	%xmm0, (%rax)
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	leal	1(%rax), %edx
	movq	-24(%rbp), %rax
	movl	%edx, 16(%rax)
	jmp	.L11
.L13:
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	main._omp_fn.0, .-main._omp_fn.0
	.section	.rodata
	.align 8
.LC8:
	.long	0
	.long	1073217536
	.ident	"GCC: (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
