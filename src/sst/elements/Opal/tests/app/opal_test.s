	.file	"opal_test.c"
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
	movl	$0, %eax
	call	printf@PLT
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	ariel_enable, .-ariel_enable
	.section	.rodata
.LC1:
	.string	"ZERO BYTE MALLOC"
	.align 8
.LC2:
	.string	"Performing a mlm Malloc for size %llu\n"
	.text
	.globl	mlm_malloc
	.type	mlm_malloc, @function
mlm_malloc:
.LFB16:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$544, %rsp
	movq	%rdi, -536(%rbp)
	movl	%esi, -540(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	cmpq	$0, -536(%rbp)
	jne	.L3
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$-1, %edi
	call	exit@PLT
.L3:
	movq	-536(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC2(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movq	-536(%rbp), %rax
	movq	%rax, %rdi
	call	malloc@PLT
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L5
	call	__stack_chk_fail@PLT
.L5:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	mlm_malloc, .-mlm_malloc
	.section	.rodata
	.align 8
.LC3:
	.string	"Allocating arrays of size %d elements.\n"
	.align 8
.LC4:
	.string	"allocated address: a:%x b:%x c:%x\n"
.LC5:
	.string	"Done allocating arrays."
.LC8:
	.string	"Sum of arrays is: %f\n"
.LC9:
	.string	"Done."
	.text
	.globl	main
	.type	main, @function
main:
.LFB17:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$64, %rsp
	movl	%edi, -52(%rbp)
	movq	%rsi, -64(%rbp)
	movl	$2000, -36(%rbp)
	call	ariel_enable
	movl	$2000, %esi
	leaq	.LC3(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$1, %esi
	movl	$16000, %edi
	call	mlm_malloc
	movq	%rax, -24(%rbp)
	movl	$1, %esi
	movl	$16000, %edi
	call	mlm_malloc
	movq	%rax, -16(%rbp)
	movl	$16000, %edi
	call	malloc@PLT
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movq	-24(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC4(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	leaq	.LC5(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$0, -44(%rbp)
	jmp	.L7
.L8:
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-24(%rbp), %rax
	addq	%rdx, %rax
	pxor	%xmm0, %xmm0
	cvtsi2sdl	-44(%rbp), %xmm0
	movsd	%xmm0, (%rax)
	movl	$2000, %eax
	subl	-44(%rbp), %eax
	movl	%eax, %edx
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rcx
	movq	-16(%rbp), %rax
	addq	%rcx, %rax
	pxor	%xmm0, %xmm0
	cvtsi2sdl	%edx, %xmm0
	movsd	%xmm0, (%rax)
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	pxor	%xmm0, %xmm0
	movsd	%xmm0, (%rax)
	addl	$1, -44(%rbp)
.L7:
	cmpl	$1999, -44(%rbp)
	jle	.L8
	movl	$0, -44(%rbp)
	jmp	.L9
.L10:
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-24(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movapd	%xmm0, %xmm1
	addsd	%xmm0, %xmm1
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-16(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm2
	movsd	.LC7(%rip), %xmm0
	mulsd	%xmm2, %xmm0
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	addsd	%xmm1, %xmm0
	movsd	%xmm0, (%rax)
	addl	$1, -44(%rbp)
.L9:
	cmpl	$1999, -44(%rbp)
	jle	.L10
	pxor	%xmm0, %xmm0
	movsd	%xmm0, -32(%rbp)
	movl	$0, -44(%rbp)
	jmp	.L11
.L12:
	movl	-44(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movsd	-32(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, -32(%rbp)
	addl	$1, -44(%rbp)
.L11:
	cmpl	$1999, -44(%rbp)
	jle	.L12
	movq	-32(%rbp), %rax
	movq	%rax, %xmm0
	leaq	.LC8(%rip), %rax
	movq	%rax, %rdi
	movl	$1, %eax
	call	printf@PLT
	movl	$0, -40(%rbp)
	jmp	.L13
.L14:
	addl	$1, -40(%rbp)
.L13:
	cmpl	$9999, -40(%rbp)
	jle	.L14
	leaq	.LC9(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	main, .-main
	.section	.rodata
	.align 8
.LC7:
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
