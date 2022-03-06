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

	.include "../asm/magicdefs.asm"

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
	lsr	%r6,%r4
	rdps	%r6
	asr	%r6,%r5
	rdps	%r6
	ror	%r6,%r4
	rdps	%r6
	mov	%r6,%r5
	rdps	%r6
	neg	%r6,%r4
	rdps	%r6
	inc	%r6,%r5
	rdps	%r6
	com	%r6,%r4
	rdps	%r6
	or	%r6,%r4,%r5
	rdps	%r6
	and	%r6,%r4,%r5
	rdps	%r6
	xor	%r6,%r4,%r5
	rdps	%r6
	add	%r6,%r4,%r5
	rdps	%r6
	sub	%r6,%r4,%r5
	rdps	%r6
	adc	%r6,%r4,%r5
	rdps	%r6
	sbb	%r6,%r4,%r5
	rdps	%r6
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

