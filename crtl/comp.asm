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
;;;;;;;;;;;;;;;;;;;;;;;;
;;  Compiler Support  ;;
;;;;;;;;;;;;;;;;;;;;;;;;

; R0 = dest address
; R1 = source address
; R2 = byte count
; R3 = return address
	.global	__memcpy
__memcpy:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	neg	%r2,%r2
	beq	memcpy_done
memcpy_loop:
	ldbu	%r3,0(%r1)
	stb	%r3,0(%r0)
	lda	%r0,1(%r0)
	lda	%r1,1(%r1)
	inc	%r2,%r2
	bne	memcpy_loop
memcpy_done:
	ldw	%r3,-64(%r6)
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

; R2 = word count
	.global	__memcpy_w
__memcpy_w:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	neg	%r2,%r2
	beq	memcpy_w_done
memcpy_w_loop:
	ldw	%r3,0(%r1)
	stw	%r3,0(%r0)
	lda	%r0,2(%r0)
	lda	%r1,2(%r1)
	inc	%r2,%r2
	bne	memcpy_w_loop
memcpy_w_done:
	ldw	%r3,-64(%r6)
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

; R2 = double-word count
	.global	__memcpy_ww
__memcpy_ww:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	neg	%r2,%r2
	beq	memcpy_ww_done
memcpy_ww_loop:
	ldw	%r3,0(%r1)
	stw	%r3,0(%r0)
	ldw	%r3,2(%r1)
	stw	%r3,2(%r0)
	lda	%r0,4(%r0)
	lda	%r1,4(%r1)
	inc	%r2,%r2
	bne	memcpy_ww_loop
memcpy_ww_done:
	ldw	%r3,-64(%r6)
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

; R0 = points to 4-word word-aligned output
; R1 = points to 4-word word-aligned input
; R3 = return address
	.global	__memcpy_4w
__memcpy_4w:
	ldw	%r2,0(%r1)
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	stw	%r2,6(%r0)
	lda	%r7,0(%r3)

; R0 = points to 4-word word-aligned input
; R1 = points to 4-word word-aligned output
; R3 = return address
	.global	__memcpy_4w_rev
__memcpy_4w_rev:
	ldw	%r2,0(%r0)
	stw	%r2,0(%r1)
	ldw	%r2,2(%r0)
	stw	%r2,2(%r1)
	ldw	%r2,4(%r0)
	stw	%r2,4(%r1)
	ldw	%r2,6(%r0)
	stw	%r2,6(%r1)
	lda	%r7,0(%r3)

