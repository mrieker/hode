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
;;;;;;;;;;;;;;;;;;
;;  Arithmetic  ;;
;;;;;;;;;;;;;;;;;;

	.include "../asm/magicdefs.asm"

;
; unsigned quad AND
;  input:
;   R0 = output address
;   R1 = input address
;   R2 = input address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2
	.global	__and_QQQ
__and_QQQ:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)

	ldw	%r3,0(%r1)
	ldw	%r4,0(%r2)
	and	%r3,%r3,%r4
	stw	%r3,0(%r0)

	ldw	%r3,2(%r1)
	ldw	%r4,2(%r2)
	and	%r3,%r3,%r4
	stw	%r3,2(%r0)

	ldw	%r3,4(%r1)
	ldw	%r4,4(%r2)
	and	%r3,%r3,%r4
	stw	%r3,4(%r0)

	ldw	%r3,6(%r1)
	ldw	%r4,6(%r2)
	and	%r3,%r3,%r4
	stw	%r3,6(%r0)

	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)

;
; unsigned quad subtract
;  input:
;   R0 = output address
;   R1 = input address
;   R2 = input address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2
	.global	__sub_QQQ
__sub_QQQ:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)

	ldw	%r3,0(%r1)
	ldw	%r4,0(%r2)
	sub	%r3,%r3,%r4
	stw	%r3,0(%r0)

	ldw	%r3,2(%r1)
	ldw	%r4,2(%r2)
	sbb	%r3,%r3,%r4
	stw	%r3,2(%r0)

	ldw	%r3,4(%r1)
	ldw	%r4,4(%r2)
	sbb	%r3,%r3,%r4
	stw	%r3,4(%r0)

	ldw	%r3,6(%r1)
	ldw	%r4,6(%r2)
	sbb	%r3,%r3,%r4
	stw	%r3,6(%r0)

	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)

;
; multiply
;  input:
;   R0 = unsigned long address
;   R1 = unsigned long address
;   R2 = unsigned long address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64 : input R0
;   -62 : input R1
;   -60 : top word partial product
;   -58 : input R3
;   -56 : input R4
;   -54 : input R5
;
	.align	2
	.global	__mul_LLL
__mul_LLL:
	lda	%r6,-12(%r6)
	stw	%r0,-64(%r6)
	stw	%r1,-62(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)
	stw	%r5,-54(%r6)

	ldw	%r3,2(%r2)	; get multiplier in R3R2
	ldw	%r2,0(%r2)

	ldw	%r1,2(%r1)	; R1R0 = multiplier * high multiplicand
	lda	%r5,mul_LLL_hi
	br	__umul
mul_LLL_hi:
	stw	%r1,-60(%r6)	; save high result

	ldw	%r1,-62(%r6)	; R1R0 = multiplier * low multiplicand
	ldw	%r1,0(%r1)
	lda	%r5,mul_LLL_lo
	br	__umul
mul_LLL_lo:
	mov	%r2,%r0		; save low result
	ldw	%r0,-60(%r6)	; add two high results
	add	%r1,%r1,%r0

	ldw	%r0,-64(%r6)	; store result
	stw	%r2,0(%r0)
	stw	%r1,2(%r0)

	ldw	%r3,-58(%r6)	; restore and return
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,12(%r6)
	lda	%pc,0(%r3)

;
; multiply
;  input:
;   R0 = unsigned long address
;   R1 = unsigned long address
;   R2 = unsigned word value
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64 : input R0
;   -62 : input R1
;   -60 : top word partial product
;   -58 : input R3
;   -56 : input R4
;   -54 : input R5
;
	.align	2
	.global	__mul_LLW
__mul_LLW:
	lda	%r6,-8(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)

	mov	%r0,%r2

	ldw	%r2,0(%r1)
	ldw	%r3,2(%r1)

	mov	%r1,%r0

	lda	%r5,mul_LLW_ret
	br	__umul		; R1R0 = R3R2 * R1
mul_LLW_ret:

	mov	%r2,%r0

	ldw	%r0,-64(%r6)
	stw	%r2,0(%r0)
	stw	%r1,2(%r0)

	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)

;
; multiply
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__mul_WWW
__mul_WWW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	clr	%r3
	lda	%r5,mul_WWW_done
	br	__umul
mul_WWW_done:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

	.align	2
	.global	__mul_wwW
__mul_wwW:
	lda	%r6,-8(%r6)
	stw	%r1,-64(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)
	tst	%r1
	bpl	mul_wwW_posi
	neg	%r1,%r1
mul_wwW_posi:
	clr	%r3
	lda	%r5,mul_wwW_done
	br	__umul
mul_wwW_done:
	ldw	%r1,-64(%r6)
	tst	%r1
	bpl	mul_wwW_poso
	neg	%r0,%r0
mul_wwW_poso:
	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
;
; internal multiply
;
;	%r3%r2
;     x	   %r1
;	%r1%r0
;
;  %r3,%r2 = unchanged
;  %r4 = zeroed
;  %r5 = return address
;
	.global	__umul
__umul:
.if RASPIMULDIV
	lda	%r6,-8(%r6)
	stw	%r2,-62(%r6)
	stw	%r3,-60(%r6)
	stw	%r1,-58(%r6)
	ldw	%r0,ss_umul
	stw	%r0,-64(%r6)
	lda	%r1,-64(%r6)
	stw	%r1,MAGIC_SCN-SCN_UMUL(%r0)
	ldw	%r0,-62(%r6)
	ldw	%r1,-60(%r6)
	clr	%r4
	lda	%r6,8(%r6)
	lda	%pc,0(%r5)
ss_umul: .word	SCN_UMUL
.else
	clr	%r0
	lda	%r4,-16(%r0)
umul_loop:
	shl	%r0,%r0
	rol	%r1,%r1
	bcc	umul_skip
	add	%r0,%r0,%r2
	adc	%r1,%r1,%r3
umul_skip:
	inc	%r4,%r4
	bne	umul_loop
	lda	%pc,0(%r5)
.endif

;
; divide
;  input:
;   R0 = address of output quotient long
;   R1 = address of input dividend long
;   R2 = address of input divisor long
;   R3 = return address
;  output:
;   *R0 = filled in
;   R1,R2,R3 = trashed
;   R0,R4,R5 = as on input
;
;  stack:
;   DI.VI.DE.ND : -52
;         DV.SR : -56
;            R5 : -58
;            R4 : -60
;            R3 : -62
;            R0 : -64
;
	dvden = -52
	dvsor = -56
	.align	2
	.global	__mod_LLL
	.global	__div_LLL
__mod_LLL:
	lda	%r0,1(%r0)
__div_LLL:
	lda	%r6,-20(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)

	; copy divisor where it's easy to access
	ldw	%r0,0(%r2)
	stw	%r0,dvsor+0(%r6)
	ldw	%r0,2(%r2)
	stw	%r0,dvsor+2(%r6)

	; copy dividend where it's easy to access
	ldw	%r3,0(%r1)
	ldw	%r0,2(%r1)
	stw	%r0,dvden+2(%r6)
	clr	%r2
	clr	%r4

	; divide out:
	;  R4.R2.DE.R3
	;    /   DV.SR
	;  -- -- -- --
	;  rr rr qq qq

	lda	%r5,-32(%r4)
div_LLL_loop:

	; shift dividend left one bit
	shl	%r3,%r3
	ldw	%r0,dvden+2(%r6)
	rol	%r0,%r0
	stw	%r0,dvden+2(%r6)
	rol	%r2,%r2
	rol	%r4,%r4

	; subtract R1R0 = top dividend - divisor
	ldw	%r1,dvsor+0(%r6)
	sub	%r0,%r2,%r1
	ldw	%r1,dvsor+2(%r6)
	sbb	%r1,%r4,%r1

	; if R1R0 >= 0 {
	;   top dividend = R1R0
	;   set low quotient bit
	; }
	bcs	div_LLL_next
	mov	%r2,%r0
	mov	%r4,%r1
	inc	%r3,%r3
div_LLL_next:

	; repeat for 32 bits
	inc	%r5,%r5
	bne	div_LLL_loop

	; store quotient as result
	ldw	%r0,-64(%r6)
	lsr	%pc,%r0
	bcs	div_LLL_mod
	stw	%r3,0(%r0)
	ldw	%r1,dvden+2(%r6)
	stw	%r1,2(%r0)
	br	div_LLL_ret

	; store remainder as result
div_LLL_mod:
	lda	%r0,-1(%r0)
	stw	%r2,0(%r0)
	stw	%r4,2(%r0)

	; restore and return
div_LLL_ret:
	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)

;
; divide
;  input:
;   R1 = signed word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = signed word
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	2
	.global	__div_wwW
__div_wwW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r3,%r1
	mov	%r0,%r1
	bpl	div_wwW_posr1
	neg	%r0,%r1
div_wwW_posr1:
	clr	%r1
	lda	%r5,div_wwW_subret
	br	udiv
div_wwW_subret:
	tst	%r3
	bpl	div_wwW_posr0
	neg	%r0,%r0
div_wwW_posr0:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

;
; divide
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	2
	.global	__div_WWW
__div_WWW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r0,%r1
	clr	%r1
	lda	%r5,div_WWW_done
	br	udiv
div_WWW_done:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

;
; divide
;  input:
;   R0 = unsigned long address
;   R1 = unsigned long address
;   R2 = unsigned word
;  output:
;   R0 = same as input
;
	.align	2
	.global	__div_LLW
__div_LLW:
	lda	%r6,-10(%r6)
	stw	%r0,-64(%r6)
	stw	%r1,-62(%r6)
	stw	%r3,-60(%r6)
	stw	%r4,-58(%r6)
	stw	%r5,-56(%r6)

	ldw	%r0,2(%r1)
	clr	%r1
	lda	%r5,div_LLW_3116
	br	udiv
div_LLW_3116:
	ldw	%r3,-64(%r6)
	stw	%r0,2(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,0(%r3)
	lda	%r5,div_LLW_1500
	br	udiv
div_LLW_1500:
	ldw	%r3,-64(%r6)
	stw	%r0,0(%r3)

	ldw	%r0,-64(%r6)
	ldw	%r3,-60(%r6)
	ldw	%r4,-58(%r6)
	ldw	%r5,-56(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)

;
; divide
;  input:
;   R0 = unsigned quad address
;   R1 = unsigned quad address
;   R2 = unsigned word
;  output:
;   R0 = same as input
;
	.align	2
	.global	__div_QQW
__div_QQW:
	lda	%r6,-10(%r6)
	stw	%r0,-64(%r6)
	stw	%r1,-62(%r6)
	stw	%r3,-60(%r6)
	stw	%r4,-58(%r6)
	stw	%r5,-56(%r6)

	ldw	%r0,6(%r1)
	clr	%r1
	lda	%r5,div_QQW_6348
	br	udiv
div_QQW_6348:
	ldw	%r3,-64(%r6)
	stw	%r0,6(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,4(%r3)
	lda	%r5,div_QQW_4732
	br	udiv
div_QQW_4732:
	ldw	%r3,-64(%r6)
	stw	%r0,4(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,2(%r3)
	lda	%r5,div_QQW_3116
	br	udiv
div_QQW_3116:
	ldw	%r3,-64(%r6)
	stw	%r0,2(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,0(%r3)
	lda	%r5,div_QQW_1500
	br	udiv
div_QQW_1500:
	ldw	%r3,-64(%r6)
	stw	%r0,0(%r3)

	ldw	%r0,-64(%r6)
	ldw	%r3,-60(%r6)
	ldw	%r4,-58(%r6)
	ldw	%r5,-56(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)

;
; modulus
;  input:
;   R1 = signed word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = signed word
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	2
	.global	__mod_wwW
__mod_wwW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r3,%r1
	mov	%r0,%r1
	bpl	mod_wwW_posr1
	neg	%r0,%r1
mod_wwW_posr1:
	clr	%r1
	lda	%r5,mod_wwW_subret
	br	udiv
mod_wwW_subret:
	mov	%r0,%r1
	tst	%r3
	bpl	mod_wwW_posr0
	neg	%r0,%r1
mod_wwW_posr0:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

;
; modulus
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__mod_WWW
__mod_WWW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r0,%r1
	clr	%r1
	lda	%r5,mod_WWW_done
	br	udiv
mod_WWW_done:
	mov	%r0,%r1
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)
;
; modulus
;  input:
;   R1 = unsigned quad address
;   R2 = unsigned word
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__mod_WQW
__mod_WQW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r3,%r1
	ldw	%r0,6(%r3)
	clr	%r1
	lda	%r5,mod_WQW_6348
	br	udiv
mod_WQW_6348:

	ldw	%r0,4(%r3)
	lda	%r5,mod_WQW_4732
	br	udiv
mod_WQW_4732:

	ldw	%r0,2(%r3)
	lda	%r5,mod_WQW_3116
	br	udiv
mod_WQW_3116:

	ldw	%r0,0(%r3)
	lda	%r5,mod_WQW_1500
	br	udiv
mod_WQW_1500:
	mov	%r0,%r1

	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

;
; internal divide
;
;	%r1%r0
;     /	   %r2
;	   %r0 r %r1
;
;  %r3,%r2 = unchanged
;  %r4 = zeroed
;  %r5 = return address
;
udiv:
.if RASPIMULDIV
	lda	%r6,-8(%r6)
	stw	%r0,-62(%r6)
	stw	%r1,-60(%r6)
	stw	%r2,-58(%r6)
	ldw	%r0,ss_udiv
	stw	%r0,-64(%r6)
	lda	%r1,-64(%r6)
	stw	%r1,MAGIC_SCN-SCN_UDIV(%r0)
	ldw	%r0,-62(%r6)
	ldw	%r1,-60(%r6)
	clr	%r4
	lda	%r6,8(%r6)
	lda	%pc,0(%r5)
ss_udiv: .word	SCN_UDIV
.else
	ldw	%r4,udiv__16
udiv_loop:
	shl	%r0,%r0
	rol	%r1,%r1
	bcs	udiv_fits
	cmp	%r1,%r2
	blo	udiv_next
udiv_fits:
	inc	%r0,%r0
	sub	%r1,%r1,%r2
udiv_next:
	inc	%r4,%r4
	bne	udiv_loop
	lda	%pc,0(%r5)
udiv__16: .word	-16
.endif

;
; shift right
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__shr_WWW
__shr_WWW:
	ldw	%r0,#15
	and	%r2,%r2,%r0
	shl	%r2,%r2
	sub	%r2,%pc,%r2
shr_WWW_base:
	lda	%pc,shr_WWW_zero-shr_WWW_base(%r2)
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r1,%r1
shr_WWW_zero:
	mov	%r0,%r1
	lda	%pc,0(%r3)

	.align	2
	.global	__shr_wwW
__shr_wwW:
	ldw	%r0,#15
	and	%r2,%r2,%r0
	shl	%r2,%r2
	sub	%r2,%pc,%r2
shr_wwW_base:
	lda	%pc,shr_wwW_zero-shr_wwW_base(%r2)
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
shr_wwW_zero:
	mov	%r0,%r1
	lda	%pc,0(%r3)

;
; shift right
;  input:
;   R0 = long address result
;   R1 = long address operand
;   R2 = shift count
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2
	.global	__shr_llW
__shr_llW:
	lda	%r6,-6(%r6)
	stw	%r5,-60(%r6)
	ldw	%r5,2(%r1)	; sign-extend result
	shl	%r5,%r5
	sbb	%r5,%r5,%r5
	br	shr_LLW_common

	.global	__shr_LLW
__shr_LLW:
	lda	%r6,-6(%r6)
	stw	%r5,-60(%r6)
	clr	%r5		; zero-extend result

shr_LLW_common:
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)

	; build 48-bit value in R5.R4.R3
	ldw	%r3,0(%r1)
	ldw	%r4,2(%r1)

	; R2 = masked number of bits to shift by
	ldw	%r1,#31
	and	%r2,%r2,%r1

	; see if shifting at least one whole word
	lsr	%r1,%r1
	cmp	%r2,%r1
	blos	shr_LLW_small
	and	%r2,%r2,%r1
	mov	%r3,%r4
	mov	%r4,%r5
shr_LLW_small:

	; shift R5.R4.R3 right by R2 bits
	; don't shift into R5 so it keeps feeding bits
	neg	%r2,%r2
	beq	shr_LLW_done
shr_LLW_loop:
	lsr	%pc,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	inc	%r2,%r2
	bne	shr_LLW_loop
shr_LLW_done:

	; store result
	stw	%r3,0(%r0)
	stw	%r4,2(%r0)

	; all done
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

;
; shift right
;  input:
;   R0 = quad address result
;   R1 = quad address operand
;   R2 = shift count
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64(R6) = saved R0
;   -62(R6) = operand pointer
;   -60(R6) = result pointer
;   -58(R6) = saved R3
;   -56(R6) = saved R4
;   -54(R6) = saved R5
;   -52(R6) = copy of operand if overlaps result
;
	.align	2
	.global	__shr_qqW
__shr_qqW:
	lda	%r6,-20(%r6)
	stw	%r5,-54(%r6)
	ldw	%r5,6(%r1)	; sign-extend result
	shl	%r5,%r5
	sbb	%r5,%r5,%r5
	br	shr_QQW_common

	.global	__shr_QQW
__shr_QQW:
	lda	%r6,-20(%r6)
	stw	%r5,-54(%r6)
	clr	%r5		; zero-extend result

shr_QQW_common:
	stw	%r0,-64(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = raw shift count
	; R5 = sign/zero-extend word

	; R2 = masked number of bits to shift by
	ldw	%r3,#63
	and	%r2,%r2,%r3

	; R3 = number of whole words to shift by
	lsr	%r3,%r2
	lsr	%r3,%r3
	lsr	%r3,%r3
	lsr	%r3,%r3

	; if R0==R1, result overlaps operand
	; copy operand to temp buffer so it doesn't get stomped writing result
	; but not needed if shifting less than one word of bits
	beq	shr_QQW_copyskip
	cmp	%r0,%r1
	bne	shr_QQW_copyskip
	ldw	%r4,  2(%r1)
	stw	%r4,-50(%r6)
	ldw	%r4,  4(%r1)
	stw	%r4,-48(%r6)
	ldw	%r4,  6(%r1)
	stw	%r4,-46(%r6)
	lda	%r1,-52(%r6)
shr_QQW_copyskip:

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = 6-bit shift count
	; R3 = whole word shift count

	; save operand pointer
	stw	%r1,-62(%r6)

	; sign/zero-extend that many top result words
	shl	%r3,%r3
	sub	%r4,%pc,%r3
shr_QQW_padbase:
	lda	%pc,shr_QQW_padskip-shr_QQW_padbase(%r4)
	stw	%r5,2(%r0)
	stw	%r5,4(%r0)
	stw	%r5,6(%r0)
shr_QQW_padskip:

	; offset result pointer by that much
	sub	%r0,%r0,%r3
	stw	%r0,-60(%r6)

	; get number of bits in word to shift by
	ldw	%r0,#15
	and	%r2,%r2,%r0

	; get negative word count to shift
	;  skip=0: negcount=-4
	;  skip=1: negcount=-3
	;  skip=2: negcount=-2
	;  skip=3: negcount=-1
	lda	%r3,-4(%r3)

shr_QQW_wordloop:
	; R5 = bits shifted out bottom of next higher word
	; R3 = negative number of words to shift
	; R2 = number of bits to shift word by

	; R1 = bits shifted out bottom of next higher word
	mov	%r1,%r5

	; R0 = word to shift
	ldw	%r5,-62(%r6)
	ldw	%r0,6(%r5)
	lda	%r5,-2(%r5)
	stw	%r5,-62(%r6)

	; save copy in R5
	mov	%r5,%r0

	; shift R1R0 right by R2
	neg	%r4,%r2
	beq	shr_QQW_bitsnull
shr_QQW_bitsloop:
	lsr	%r1,%r1
	ror	%r0,%r0
	inc	%r4,%r4
	bne	shr_QQW_bitsloop
shr_QQW_bitsnull:

	; store shifted value
	ldw	%r4,-60(%r6)
	stw	%r0,6(%r4)
	lda	%r4,-2(%r4)
	stw	%r4,-60(%r6)

	; maybe have another word to shift
	inc	%r3,%r3
	bne	shr_QQW_wordloop

	; all done
	ldw	%r0,-64(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)


;
; shift left
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__shl_WWW
	.global	__shl_WwW
	.global	__shl_wWW
	.global	__shl_wwW
__shl_WWW:
__shl_WwW:
__shl_wWW:
__shl_wwW:
	ldw	%r0,#15
	and	%r2,%r2,%r0
	shl	%r2,%r2
	sub	%r2,%pc,%r2
shl_WWW_base:
	lda	%pc,shl_WWW_zero-shl_WWW_base(%r2)
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
shl_WWW_zero:
	mov	%r0,%r1
	lda	%pc,0(%r3)

;
; shift left
;  input:
;   R0 = long address result
;   R1 = long address operand
;   R2 = shift count
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.global	__shl_llB
	.global	__shl_llW
	.global	__shl_LLB
	.global	__shl_LLW
__shl_llB:
__shl_llW:
__shl_LLB:
__shl_LLW:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)

	; build 32-bit value in R4.R3
	ldw	%r3,0(%r1)
	ldw	%r4,2(%r1)

	; R2 = masked number of bits to shift by
	ldw	%r1,#31
	and	%r2,%r2,%r1

	; see if shifting at least one whole word
	lsr	%r1,%r1
	cmp	%r2,%r1
	blos	shl_LLW_small
	and	%r2,%r2,%r1
	mov	%r4,%r3
	clr	%r3
shl_LLW_small:

	; shift R4.R3 left by R2 bits
	neg	%r2,%r2
	beq	shl_LLW_done
shl_LLW_loop:
	shl	%r3,%r3
	rol	%r4,%r4
	inc	%r2,%r2
	bne	shl_LLW_loop
shl_LLW_done:

	; store result
	stw	%r3,0(%r0)
	stw	%r4,2(%r0)

	; all done
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)

;
; shift left
;  input:
;   R0 = quad address result
;   R1 = quad address operand
;   R2 = shift count
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64(R6) = saved R0
;   -62(R6) = operand pointer
;   -60(R6) = result pointer
;   -58(R6) = saved R3
;   -56(R6) = saved R4
;   -54(R6) = saved R5
;   -52(R6) = copy of operand if overlaps result
;
	.align	2
	.global	__shl_qqB
	.global	__shl_qqW
	.global	__shl_QQB
	.global	__shl_QQW
__shl_qqB:
__shl_qqW:
__shl_QQB:
__shl_QQW:
	lda	%r6,-20(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)
	stw	%r5,-54(%r6)

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = raw shift count

	; R2 = masked number of bits to shift by
	ldw	%r3,#63
	and	%r2,%r2,%r3

	; R3 = number of whole words to shift by
	lsr	%r3,%r2
	lsr	%r3,%r3
	lsr	%r3,%r3
	lsr	%r3,%r3

	; if R0==R1, result overlaps operand
	; copy operand to temp buffer so it doesn't get stomped writing result
	; but not needed if shifting less than one word of bits
	beq	shl_QQW_copyskip
	cmp	%r0,%r1
	bne	shl_QQW_copyskip
	ldw	%r4,  0(%r1)
	stw	%r4,-52(%r6)
	ldw	%r4,  2(%r1)
	stw	%r4,-50(%r6)
	ldw	%r4,  4(%r1)
	stw	%r4,-48(%r6)
	lda	%r1,-52(%r6)
shl_QQW_copyskip:

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = 6-bit shift count
	; R3 = whole word shift count

	; save operand pointer
	stw	%r1,-62(%r6)

	; zero that many bottom result words
	clr	%r1
	shl	%r3,%r3
	sub	%r4,%pc,%r3
shl_QQW_padbase:
	lda	%pc,shl_QQW_padskip-shl_QQW_padbase(%r4)
	stw	%r1,4(%r0)
	stw	%r1,2(%r0)
	stw	%r1,0(%r0)
shl_QQW_padskip:

	; offset result pointer by that much
	add	%r0,%r0,%r3
	stw	%r0,-60(%r6)

	; get number of bits in word to shift by
	ldw	%r0,#15
	and	%r2,%r2,%r0

	; get negative word count to shift
	;  skip=0: negcount=-4
	;  skip=1: negcount=-3
	;  skip=2: negcount=-2
	;  skip=3: negcount=-1
	lda	%r3,-4(%r3)

	; shift zeroes into bottom of low word
	clr	%r5

shl_QQW_wordloop:
	; R5 = bits shifted out top of next lower word
	; R3 = negative number of words to shift
	; R2 = number of bits to shift word by

	; R1 = bits shifted out top of next lower word
	mov	%r1,%r5

	; R0 = word to shift
	ldw	%r5,-62(%r6)
	ldw	%r0,0(%r5)
	lda	%r5,2(%r5)
	stw	%r5,-62(%r6)

	; save copy in R5
	mov	%r5,%r0

	; shift R0R1 left by R2
	neg	%r4,%r2
	beq	shl_QQW_bitsnull
shl_QQW_bitsloop:
	shl	%r1,%r1
	rol	%r0,%r0
	inc	%r4,%r4
	bne	shl_QQW_bitsloop
shl_QQW_bitsnull:

	; store shifted value
	ldw	%r4,-60(%r6)
	stw	%r0,0(%r4)
	lda	%r4,2(%r4)
	stw	%r4,-60(%r6)

	; maybe have another word to shift
	inc	%r3,%r3
	bne	shl_QQW_wordloop

	; all done
	ldw	%r0,-64(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)


;
; complement quadword
;  input:
;   R0 = points to output quadword
;   R1 = points to input quadword
;   R3 = return address
;  output:
;   R0,R4,R5 = as on input
;   R1,R2,R3 = trashed
	.global	__com_QQ
__com_QQ:
	ldw	%r2,0(%r1)
	com	%r2,%r2
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	com	%r2,%r2
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	com	%r2,%r2
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	com	%r2,%r2
	stw	%r2,6(%r0)
	lda	%pc,0(%r3)

;
; negate quadword
;  input:
;   R0 = points to output quadword
;   R1 = points to input quadword
;   R3 = return address
;  output:
;   R0,R4,R5 = as on input
;   R1,R2,R3 = trashed
	.global	__neg_QQ
__neg_QQ:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	clr	%r3

	ldw	%r2,0(%r1)
	sub	%r2,%r3,%r2
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	sbb	%r2,%r3,%r2
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	sbb	%r2,%r3,%r2
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	sbb	%r2,%r3,%r2
	stw	%r2,6(%r0)

	ldw	%r3,-64(%r6)
	lda	%r6,2(%r6)
	lda	%pc,0(%r3)

;
; compare two quadwords
;  input:
;   R0 = branch opcode (0=blt, 4=blo, 8=bge, 12=bhis, 16=ble, 20=blos, 24=bgt, 28=bhi, 32/36=beq, 40/44=bne)
;   R1 = left operand address
;   R2 = right operand address
;  output:
;   R0 = 0 : branch opcode taken
;        1 : branch opcode not taken
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	8
	.word	0		; to align cmp_qq_base on 8-byte boundary
cmp_qq_four:
	.word	4

	.global	__cmp_QQ
	.global	__cmp_qq
__cmp_QQ:
__cmp_qq:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	ldw	%r5,cmp_qq_four
	add	%r0,%pc,%r0	; test table has 4 bytes per entry
cmp_qq_base:
	ldw	%r3,6(%r1)	; compare bits <63:48>
	ldw	%r4,6(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	or	%r0,%r0,%r5	; switch to unsigned compares if not already
	ldw	%r3,4(%r1)	; compare bits <47:32>
	ldw	%r4,4(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	ldw	%r3,2(%r1)	; compare bits <31:16>
	ldw	%r4,2(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	ldw	%r3,0(%r1)	; compare bits <15:00>
	ldw	%r4,0(%r2)
	cmp	%r3,%r4
cmp_qq_test:
	lda	%pc,cmp_qq_here-cmp_qq_base(%r0)
cmp_qq_here:
	blt	cmp_qq_zero	; given 0=BLT, return 0 if branch taken
	br	cmp_qq_one	; given 0=BLT, return 1 if not taken
	blo	cmp_qq_zero
	br	cmp_qq_one
	bge	cmp_qq_zero
	br	cmp_qq_one
	bhis	cmp_qq_zero
	br	cmp_qq_one
	ble	cmp_qq_zero
	br	cmp_qq_one
	blos	cmp_qq_zero
	br	cmp_qq_one
	bge	cmp_qq_zero
	br	cmp_qq_one
	bhi	cmp_qq_zero
	br	cmp_qq_one
	beq	cmp_qq_zero
	br	cmp_qq_one
	beq	cmp_qq_zero
	br	cmp_qq_one
	bne	cmp_qq_zero
	br	cmp_qq_one
	bne	cmp_qq_zero
cmp_qq_one:
	inc	%r0,%r0		; R0 was even, make it odd to return 1
cmp_qq_zero:
	ldw	%r1,#1		; mask off upper bits of R0
	and	%r0,%r0,%r1
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

