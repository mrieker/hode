;;    Copyright (C) Mike Rieker, Beverly, MA USA
;;    www.outerworldapps.com
;;
;;    This program is free software; you can redistribute it and/or modify
;;    it under the terms of the GNU General Public License as published by
;;    the Free Software Foundation; version 2 of the License.
;;
;;    This program is distributed in the hope that it will be useful,
;;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;    GNU General Public License for more details.
;;
;;    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
;;
;;    You should have received a copy of the GNU General Public License
;;    along with this program; if not, write to the Free Software
;;    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;;
;;    http:;;www.gnu.org/licenses/gpl-2.0.html
# unsigned multiply and unsigned divide routines
# ...and a little test program

# prints product 093F9AE5
#   in decimal 39653

# ./master.sh -sim master.sim
# takes about 13m to run
# (3021 cycles)

	.set	magic 0xFFF0

test:
	br	skipih			# 0000: reset entrypoint
	br	inthand			# 0002: interrupt entrypoint
skipih:
	lda	r5 magic-.-2 pc		# tell RasPiModule to turn on IRQ line
	lda	r1 intson-.-2 pc
	stw	r1 0 r5
	ldw	r2 0 pc			# enable interrupts, set N,V bits
		.word 0x800A
	wrps	r2
	lda	r0 1 r2			# this should execute
ireqhere:
	halt	r0			# this should not execute
intson:	 .word	2 1
intsoff: .word	2 0
inthand:
	xor	r1 pc pc		# change cond codes to Z,C set
	lda	r1 5 r1
	wrps	r1
	lda	r1 intsoff-.-2 pc	# tell RasPiModule to turn off IRQ line
	stw	r1 0 r5
	ldw	r1 0xFFFC-magic r5	# modify return address
	lda	r1 irethere-ireqhere r1
	stw	r1 0xFFFC-magic r5
	iret
	halt	r0			# should not get here
irethere:

# do a bunch of regular arithmetic
# check result value and condition codes
# - Shadow.java will check it as we run
	xor	r2 r7 r7
	xor	r3 r7 r7
	ldw	r4 0 pc
		.word	5432
	ldw	r5 0 pc
		.word	4321
test_loop1:
	add	r1 r2 r3
	rdps	r0

	sub	r1 r2 r3
	rdps	r1

	sub	r1 r3 r2
	rdps	r0

	or	r1 r2 r3
	rdps	r1

	and	r1 r2 r3
	rdps	r0

	xor	r1 r2 r3
	rdps	r1

	sub	pc r3 r2
	rdps	r1

	xor	r1 pc pc
	wrps	r1
	adc	r1 r2 r3
	rdps	r0

	ldw	r1 0 pc
		.word	1
	wrps	r1
	adc	r1 r2 r3
	rdps	r1

	xor	r1 pc pc
	wrps	r1
	sbb	r1 r2 r3
	rdps	r0

	ldw	r1 0 pc
		.word	1
	wrps	r1
	sbb	r1 r2 r3
	rdps	r1

	xor	r1 pc pc
	wrps	r1
	sbb	r1 r3 r2
	rdps	r0

	ldw	r1 0 pc
		.word	1
	wrps	r1
	sbb	r1 r3 r2
	rdps	r1

	add	r2 r2 r4
	add	r3 r3 r5
	bcc	test_loop1

test_loop2:
	lsr	r1 r2
	rdps	r0

	asr	r1 r2
	rdps	r1

	xor	r1 pc pc
	wrps	r1
	csr	r1 r2
	rdps	r0

	mov	r1 r2
	rdps	r1

	neg	r1 r2
	rdps	r0

	inc	r1 r2
	rdps	r1

	com	r1 r2
	rdps	r0

	add	r2 r2 r5
	bcc	test_loop2

# test that the branches do the condition codes
# - Shadow.java monitors the states and PC
	xor	r2 pc pc
	ldw	r3 0 pc
		.word	16
test_loop3:
	wrps	r2
	rdps	r0
	br	.+4
	lda	r0 0 r0
	ble	.+4
	lda	r0 0 r0
	bgt	.+4
	lda	r0 0 r0
	blt	.+4
	lda	r0 0 r0
	bge	.+4
	lda	r0 0 r0
	blos	.+4
	lda	r0 0 r0
	bhi	.+4
	lda	r0 0 r0
	bmi	.+4
	lda	r0 0 r0
	bpl	.+4
	lda	r0 0 r0
	beq	.+4
	lda	r0 0 r0
	bne	.+4
	lda	r0 0 r0
	bvs	.+4
	lda	r0 0 r0
	bvc	.+4
	lda	r0 0 r0
	bcs	.+4
	lda	r0 0 r0
	bcc	.+4
	lda	r0 0 r0

	inc	r2 r2
	sub	r0 r2 r3
	blt	test_loop3

# --------------------------------------------

	ldw	r6 0 r7		# set stack pointer to end of memory
		.word	magic

	ldw	r1 0 r7		# multiply some values
		.word	0x4321
	ldw	r2 0 r7
		.word	0x2345
	xor	r3 r7 r7
	lda	r5 4 pc
	ldw	pc 0 pc
		.word	umul

	lda	r6 0-2 r6	# save low-order on stack
	stw	r0 0 r6

	mov	r0 r1		# convert high-order to string
	lda	r1 hexbuf-.-2 r7
	lda	r5 2 r7
	br	hextostr

	ldw	r0 0 r6		# convert low-order to string
	lda	r5 2 r7
	br	hextostr

	ldw	r0 0 r6		# convert low-order to decimal string
	ldw	r1 4 r7
	lda	r5 4 r7
	br	dectostr
	.word	decbuf

	xor	r0 r7 r7	# null terminate string
	stb	r0 0 r1

	ldw	r1 0 r7		# print message
		.word	magic
	lda	r0 printctl-.-2 r7
	stw	r0 0 r1

	lda	r0 exitctl-.-2 r7
	stw	r0 0 r1		# exit(0)

	halt	r0

printctl:
	.word	1
	.word	message

message: .ascii	multiply result
	.byte	32
hexbuf:	.ascii	xxxxxxxx
	.byte	10 32 32
	.ascii	in decimal
	.byte	32
decbuf:	.ascii	xxxxxx
	.byte	0

	.p2align 1

exitctl: .word	0 0

#	R3R2
#     x	  R1
#	R1R0
umul:
	lda	r6 0-2 r6
	stw	r4 0 r6
	xor	r0 r7 r7
	lda	r4 0-16 r0
umul_loop:
	add	r0 r0 r0
	adc	r1 r1 r1
	bcc	umul_skip
	add	r0 r0 r2
	adc	r1 r1 r3
umul_skip:
	inc	r4 r4
	bne	umul_loop
	ldw	r4 0 r6
	lda	r6 2 r6
	lda	r7 0 r5

#	R1R0
#     /	  R2
#	  R0 r R1
udiv:
	lda	r6 0-2 r6
	stw	r4 0 r6
	ldw	r4 0 r7
		.word	0-16
udiv_loop:
	add	r0 r0 r0
	adc	r1 r1 r1
	bcs	udiv_fits
	sub	pc r1 r2
	blo	udiv_next
udiv_fits:
	lda	r0 1 r0
	sub	r1 r1 r2
udiv_next:
	inc	r4 r4
	bne	udiv_loop
	ldw	r4 0 r6
	lda	r6 2 r6
	lda	r7 0 r5

# input:
#  r0 = number
#  r1 = buffer
# output:
#  r0,r2,r3,r4 = scratch
#  r1 = points just past string
hextostr:
	ldw	r4 0 r7		# number of digits to produce
		.word	0-4
	sub	r1 r1 r4	# point to end of buffer
hextostr_loop:
	ldw	r3 0 r7		# mask low digit
		.word	15
	and	r2 r0 r3	# ...into r2
	ldw	r3 0 r7		# convert to ascii 0..?
		.word	48
	add	r2 r2 r3
	ldw	r3 0 r7		# see if it should be A..F
		.word	48+10
	sub	r3 r2 r3
	blo	hextostr_0_9
	lda	r2 65-48-10 r2	# if so, convert to A..F
hextostr_0_9:
	lda	r1 0-1 r1	# decrement pointer
	stb	r2 0 r1		# store character
	lsr	r0 r0		# shift out bottom digit
	lsr	r0 r0
	lsr	r0 r0
	lsr	r0 r0
	inc	r4 r4		# decrement counter
	bne	hextostr_loop
	lda	r1 4 r1		# point past output string
	lda	r7 0 r5


#  input:
#   r0 = number
#   r1 = buffer
#   r5 = return address
#   r6 = stack pointer
#  output:
#   r0,r2,r3 = scratch
#   r1 = points just past string
dectostr:
	lda	r6 0-6 r6
	stw	r5   4 r6
	stw	r4   2 r6
	stw	r1   0 r6
	lda	r4   0 r6
dectostr_loop:
	xor	r1 pc pc
	lda	r2 10 r1
	lda	r5 2 pc
	br	udiv
	lda	r1 48 r1
	lda	r6 0-2 r6
	stw	r1 0 r6
	mov	r0 r0
	bne	dectostr_loop
	ldw	r1  0 r4
dectostr_fini:
	ldw	r0  0 r6
	lda	r6  2 r6
	stb	r0  0 r1
	lda	r1  1 r1
	sub	r0 r4 r6
	bne	dectostr_fini
	ldw	r4  2 r6
	ldw	r5  4 r6
	lda	r6  6 r6
	lda	r7  0 r5

