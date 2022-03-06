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

; char *memchr (char *, char, int)
	.align	2
	.global	memchr
memchr:
	ldw	%r0,-64(%r6)
	ldbs	%r1,-62(%r6)
	ldw	%r2,-60(%r6)
	stw	%r3,-64(%r6)
	neg	%r2,%r2
	beq	memchr_null
memchr_loop:
	ldbs	%r3,0(%r0)
	cmp	%r3,%r1
	beq	memchr_done
	lda	%r0,1(%r0)
	inc	%r2,%r2
	bne	memchr_loop
memchr_null:
	clr	%r0
memchr_done:
	ldw	%r3,-64(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

; int memcmp (char const *, char const *, size_t)
	.align	2
	.global	memcmp
memcmp:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	ldw	%r2,-60(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r0,-56(%r6)
	neg	%r4,%r0
	beq	memcmp_done
memcmp_loop:
	ldbs	%r0,0(%r2)
	ldbs	%r1,0(%r3)
	sub	%r0,%r0,%r1
	bne	memcmp_done
	lda	%r2,1(%r2)
	lda	%r3,1(%r3)
	inc	%r4,%r4
	bne	memcmp_loop
memcmp_done:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)

; void memcpy (char *, char const *, int)
	.align	2
	.global	memcpy
memcpy:
	ldw	%r0,-64(%r6)
	ldw	%r1,-62(%r6)
	ldw	%r2,-60(%r6)
	lda	%r6,6(%r6)
	ldw	%pc,#__memcpy

; void memmove (char *, char const *, int)
	.align	2
	.global	memmove
memmove:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	ldw	%r0,-62(%r6)
	ldw	%r1,-60(%r6)
	ldw	%r2,-58(%r6)
	cmp	%r0,%r1
	beq	memmove_done
	blo	memmove_fwd
	add	%r0,%r0,%r2
	add	%r1,%r1,%r2
	or	%r3,%r0,%r1
	or	%r3,%r3,%r2
	lsr	%r3,%r3
	bcc	memmove_w
	neg	%r2,%r2
	beq	memmove_done
memmove_loop:
	lda	%r1,-1(%r1)
	lda	%r0,-1(%r0)
	ldbu	%r3,0(%r1)
	stb	%r3,0(%r0)
	inc	%r2,%r2
	bne	memmove_loop
memmove_done:
	ldw	%r3,-64(%r6)
	lda	%r6,8(%r6)
	lda	%r7,0(%r3)
memmove_w:
	lsr	%r2,%r2
	neg	%r2,%r2
	beq	memmove_w_done
memmove_w_loop:
	lda	%r1,-2(%r1)
	lda	%r0,-2(%r0)
	ldw	%r3,0(%r1)
	stw	%r3,0(%r0)
	inc	%r2,%r2
	bne	memmove_w_loop
memmove_w_done:
	ldw	%r3,-64(%r6)
	lda	%r6,8(%r6)
	lda	%r7,0(%r3)
memmove_fwd:
	lda	%r6,8(%r6)
	ldw	%pc,#__memcpy

; void memset (char *, int, int)
	.align	2
	.global	memset
memset:
	ldw	%r0,-64(%r6)	; R0 = dest addr
	ldw	%r2,-60(%r6)	; R2 = byte count
	or	%r1,%r0,%r2	; R1 = check for alignment
	lsr	%r1,%r1
	ldbu	%r1,-62(%r6)	; R1 = data byte
	bcc	memset_w
	neg	%r2,%r2		; fill bytes
	beq	memset_done
memset_loop:
	stb	%r1,0(%r0)	; write byte
	lda	%r0,1(%r0)	; increment pointer
	inc	%r2,%r2
	bne	memset_loop
memset_done:
	lda	%r6,6(%r6)
	lda	%r7,0(%r3)
memset_w:
	lsr	%r2,%r2		; R2 = word count
	neg	%r2,%r2
	beq	memset_w_done
	stb	%r1,-61(%r6)	; R1 = double byte
	ldw	%r1,-62(%r6)
memset_w_loop:
	stw	%r1,0(%r0)	; write word
	lda	%r0,2(%r0)	; increment pointer
	inc	%r2,%r2
	bne	memset_w_loop
memset_w_done:
	lda	%r6,6(%r6)
	lda	%r7,0(%r3)

; char *strchr (char *, char)
	.align	2
	.global	strchr
strchr:
	ldw	%r0,-64(%r6)
	ldbu	%r1,-62(%r6)
strchr_loop:
	ldbu	%r2,0(%r0)
	cmp	%r2,%r1
	beq	strchr_done
	lda	%r0,1(%r0)
	tst	%r2
	bne	strchr_loop
	clr	%r0
strchr_done:
	lda	%r6,4(%r6)
	lda	%r7,0(%r3)

; int strcmp (char const *, char const *)
	.align	2
	.global	strcmp
strcmp:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	ldw	%r2,-62(%r6)
	ldw	%r3,-60(%r6)
strcmp_loop:
	ldbs	%r0,0(%r2)
	ldbs	%r1,0(%r3)
	sub	%r0,%r0,%r1
	bne	strcmp_done
	lda	%r2,1(%r2)
	lda	%r3,1(%r3)
	tst	%r1
	bne	strcmp_loop
strcmp_done:
	ldw	%r3,-64(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

; int strcasecmp (char const *, char const *)
	.align	2
	.global	strcasecmp
strcasecmp:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	ldw	%r2,-58(%r6)		; point to A string
	ldw	%r3,-56(%r6)		; point to B string
	ldw	%r4,strcasecmp_A	; lowest upper case letter
	lda	%r5,26-'A'(%r4)		; number of letters
strcasecmp_loop:
	ldbs	%r0,0(%r2)		; get A character
	sub	%r0,%r0,%r4		; make it relative to 'A'
	cmp	%r0,%r5
	bhis	strcasecmp_aok
	lda	%r0,'a'-'A'(%r0)	; update to lower case
strcasecmp_aok:
	ldbs	%r1,0(%r3)		; get B character
	sub	%r1,%r1,%r4		; make it relative to 'A'
	cmp	%r1,%r5
	bhis	strcasecmp_bok
	lda	%r1,'a'-'A'(%r1)	; update to lower case
strcasecmp_bok:
	sub	%r0,%r0,%r1		; compare characters
	bne	strcasecmp_done		; stop if different
	lda	%r2,1(%r2)
	lda	%r3,1(%r3)
	add	%r0,%r1,%r4		; check for null terminator
	bne	strcasecmp_loop
strcasecmp_done:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)
strcasecmp_A: .word 'A'

; void strcpy (char *, char const *)
	.align	2
	.global	strcpy
strcpy:
	ldw	%r0,-64(%r6)
	ldw	%r1,-62(%r6)
strcpy_loop:
	ldbs	%r2,0(%r1)
	stb	%r2,0(%r0)
	lda	%r1,1(%r1)
	lda	%r0,1(%r0)
	tst	%r2
	bne	strcpy_loop
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)

; int strlen (char const *)
	.align	2
	.global	strlen
strlen:
	ldw	%r0,-64(%r6)
	lda	%r1,1(%r0)
strlen_loop:
	ldbu	%r2,0(%r0)
	lda	%r0,1(%r0)
	tst	%r2
	bne	strlen_loop
	sub	%r0,%r0,%r1
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

