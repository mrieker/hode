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

; rolls the D-bus (alu output) lights using timer interrupts

	.include "../crtl/library/magicdefs.asm"

	.global	__boot
__boot:				; 0000: cpu resets here
	br	start
intreq:				; 0002: interrupts come here
	lda	%r4,.+4		; call armint to re-arm timer interrupt
	br	armint
	iret			; return back where we left off

start:
	ldw	%r5,#MAGIC_SCN	; get current time
	lda	%r2,getnow
	stw	%r2,0(%r5)
	lda	%r4,.+4		; call armint to re-arm timer interrupt
	br	armint
	ldw	%r2,#0x8000	; enable interrupt delivery
	wrps	%r2
	clr	%r1		; reset light counter
incloop:
	inc	%r1,%r1		; increment binary counter
	ldw	%r6,_0xFF
	and	%r1,%r1,%r6
	lsr	%r0,%r1		; get corresponding gray count
	xor	%r0,%r0,%r1
	stb	%r0,temp+1	; save byte-sized gray count
shiftloop:
	ror	%r0,%r0		; flip bits for top byte
	rol	%r6,%r6
	bpl	shiftloop
	stb	%r6,temp+0
	ldw	%r0,temp	; combine two bytes
	halt	%r0		; display gray count in lights
	br	incloop

getnow:	.word	SCN_GETNOWNS
	.word	nextirq

nextirq: .word	0,0,0,0		; 64-bit absolute time in ns
stepns:	.long	100000000	; time step (0.1 sec)

_0xFF:	.word	0xFF
temp:	.word	0

; r2,r3 = scratch
; r4 = return address
; r5 = points to MAGIC_SCN
armint:
	ldw	%r2,nextirq+0	; do 64-bit add to compute time for next interrupt
	ldw	%r3,stepns+0
	add	%r2,%r2,%r3
	stw	%r2,nextirq+0
	ldw	%r2,nextirq+2
	ldw	%r3,stepns+2
	adc	%r2,%r2,%r3
	stw	%r2,nextirq+2
	bcc	noupper
	ldw	%r2,nextirq+4
	inc	%r2,%r2
	stw	%r2,nextirq+4
	bne	noupper
	ldw	%r2,nextirq+6
	inc	%r2,%r2
	stw	%r2,nextirq+6
noupper:
	lda	%r2,lcenab	; tell raspi to request interrupt at that time
	stw	%r2,0(%r5)
	lda	%pc,0(%r4)

lcenab:	.word	SCN_IRQATNS
	.word	nextirq

