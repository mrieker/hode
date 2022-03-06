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

	.include "../asm/magicdefs.asm"

	br	start
intreq:
	lda	%r4,.+4
	br	armint
	iret

start:
	ldw	%r5,#MAGIC_SCN
	lda	%r0,getnow
	stw	%r0,0(%r5)
	lda	%r4,.+4
	br	armint
	ldw	%r0,#0x8000
	wrps	%r0
	clr	%r3
loop:
	inc	%r3,%r3
	lsr	%r2,%r3
	xor	%r2,%r2,%r3
	halt	%r2
	br	loop

getnow:	.word	SCN_GETNOWNS
	.word	nextirq

nextirq: .word	0,0,0,0		; 64-bit absolute time in ns
stepns:	.long	2000000000/3

; r0,r1 = scratch
; r4 = return address
; r5 = points to MAGIC_SCN
armint:
	ldw	%r0,nextirq+0
	ldw	%r1,stepns+0
	add	%r0,%r0,%r1
	stw	%r0,nextirq+0
	ldw	%r0,nextirq+2
	ldw	%r1,stepns+2
	adc	%r0,%r0,%r1
	stw	%r0,nextirq+2
	bcc	noupper
	ldw	%r0,nextirq+4
	inc	%r0,%r0
	stw	%r0,nextirq+4
	bne	noupper
	ldw	%r0,nextirq+6
	inc	%r0,%r0
	stw	%r0,nextirq+6
noupper:
	lda	%r0,lcenab
	stw	%r0,0(%r5)
	lda	%pc,0(%r4)

lcenab:	.word	SCN_IRQATNS
	.word	nextirq

