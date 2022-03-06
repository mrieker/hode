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
; do all arithmetics on pseudo-random numbers
; let raspictl shadow.cc check the results
; uses/tests R0..R7

start:
	br	loop

r4rand:	.word	0x1234
r5rand:	.word	0x5678

loop:
	ldw	%r6,r4rand
	lda	%r5,.+4
	br	dolfsr
	lda	%r5,.+4
	br	dolfsr
	stw	%r6,r4rand

	ldw	%r6,r5rand
	lda	%r5,.+4
	br	dolfsr
	lda	%r5,.+4
	br	dolfsr
	lda	%r5,.+4
	br	dolfsr
	stw	%r6,r5rand

	ldw	%r4,r4rand
	ldw	%r5,r5rand
	lsr	%r3,%r4
	rdps	%r2
	asr	%r2,%r5
	rdps	%r3
	ror	%r3,%r4
	rdps	%r2
	mov	%r2,%r5
	rdps	%r3
	neg	%r3,%r4
	rdps	%r2
	inc	%r2,%r5
	rdps	%r3
	com	%r3,%r4
	rdps	%r2
	or	%r2,%r4,%r5
	rdps	%r3
	and	%r3,%r4,%r5
	rdps	%r2
	xor	%r2,%r4,%r5
	rdps	%r3
	add	%r3,%r4,%r5
	rdps	%r2
	sub	%r2,%r4,%r5
	rdps	%r3
	adc	%r3,%r4,%r5
	rdps	%r2
	sbb	%r2,%r4,%r5
	rdps	%r3

	mov	%r3,%r4
	mov	%r2,%r5
	lsr	%r4,%r3
	rdps	%r5
	asr	%r5,%r2
	rdps	%r4
	ror	%r4,%r3
	rdps	%r5
	mov	%r5,%r2
	rdps	%r4
	neg	%r4,%r3
	rdps	%r5
	inc	%r5,%r2
	rdps	%r4
	com	%r4,%r3
	rdps	%r5
	or	%r5,%r3,%r2
	rdps	%r4
	and	%r4,%r3,%r2
	rdps	%r5
	xor	%r5,%r3,%r2
	rdps	%r4
	add	%r4,%r3,%r2
	rdps	%r5
	sub	%r5,%r3,%r2
	rdps	%r4
	adc	%r4,%r3,%r2
	rdps	%r5
	sbb	%r5,%r3,%r2
	rdps	%r4

	mov	%r0,%r3
	mov	%r1,%r2
	lsr	%r4,%r0
	rdps	%r5
	asr	%r5,%r1
	rdps	%r4
	ror	%r4,%r0
	rdps	%r5
	mov	%r5,%r1
	rdps	%r4
	neg	%r4,%r0
	rdps	%r5
	inc	%r5,%r1
	rdps	%r4
	com	%r4,%r0
	rdps	%r5
	or	%r5,%r0,%r1
	rdps	%r4
	and	%r4,%r0,%r1
	rdps	%r5
	xor	%r5,%r0,%r1
	rdps	%r4
	add	%r4,%r0,%r1
	rdps	%r5
	sub	%r5,%r0,%r1
	rdps	%r4
	adc	%r4,%r0,%r1
	rdps	%r5
	sbb	%r5,%r0,%r1
	rdps	%r4
	br	loop

dolfsr_ret:
	.word	0

dolfsr:
	stw	%r5,dolfsr_ret

	lsr	%r5,%r6
	lsr	%r5,%r5
	xor	%r4,%r6,%r5
	lsr	%r5,%r5
	xor	%r4,%r4,%r5
	lsr	%r5,%r5
	lsr	%r5,%r5
	xor	%r4,%r4,%r5

	lsr	%r4,%r4
	ror	%r6,%r6

	ldw	%pc,dolfsr_ret

