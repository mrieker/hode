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
;;;;;;;;;;;;;;;;;;;;;;;
;;  System Services  ;;
;;;;;;;;;;;;;;;;;;;;;;;

	.include "../asm/magicdefs.asm"

; int16 close (int16)
	.align	2
	.global	close
close:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_close
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_CLOSE(%r0)
	ldw	%r0,MAGIC_SCN-SCN_CLOSE(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_close: .word	SCN_CLOSE

; void exit (int16) noreturn
	.align	2
	.global	exit
exit:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_exit
	stw	%r0,0(%r1)
exit_loop:
	stw	%r1,MAGIC_SCN-SCN_EXIT(%r0)
	br	exit_loop
ss_exit: .word	SCN_EXIT

; int getcmdarg (int idx, int len, char *buf);
	.align	2
	.global	getcmdarg
getcmdarg:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_getcmdarg
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_GETCMDARG(%r0)
	ldw	%r0,MAGIC_SCN-SCN_GETCMDARG(%r0)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
ss_getcmdarg: .word SCN_GETCMDARG

; uint64 getnowns () aka getnowns (uint64 *)
   .align  2
   .global getnowns
getnowns:
   lda %r6,-2(%r6)
   lda %r1,-64(%r6)
   ldw %r0,ss_getnowns
   stw %r0,0(%r1)
   stw %r1,MAGIC_SCN-SCN_GETNOWNS(%r0)
   ldw %r0,MAGIC_SCN-SCN_GETNOWNS(%r0)
   lda %r6,4(%r6)
   lda %pc,0(%r3)
ss_getnowns: .word SCN_GETNOWNS

; int16 open (char const *path, uint16, uint16)
	.align	2
	.global	open
open:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_open
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_OPEN(%r0)
	ldw	%r0,MAGIC_SCN-SCN_OPEN(%r0)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
ss_open: .word	SCN_OPEN

; int16 read (int16, void *, int16)
	.align	2
	.global	read
read:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_read
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_READ(%r0)
	ldw	%r0,MAGIC_SCN-SCN_READ(%r0)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
ss_read: .word	SCN_READ

; int16 unlink (char const *)
	.align	2
	.global	unlink
unlink:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_unlink
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_UNLINK(%r0)
	ldw	%r0,MAGIC_SCN-SCN_UNLINK(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_unlink: .word SCN_UNLINK

; int16 write (int16, void const *, int16)
	.align	2
	.global	write
write:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_write
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_WRITE(%r0)
	ldw	%r0,MAGIC_SCN-SCN_WRITE(%r0)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
ss_write: .word	SCN_WRITE

; int16 setrosize (uint16)
	.align	2
	.global	setrosize
setrosize:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_setrosize
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_SETROSIZE(%r0)
	ldw	%r0,MAGIC_SCN-SCN_SETROSIZE(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_setrosize: .word SCN_SETROSIZE

; int16 setstklim (uint16)
	.align	2
	.global	setstklim
setstklim:
	ldw	%r0,-64(%r6)	; make sure R6 stays 64 above end of malloc area
	lda	%r0,32(%r0)
	lda	%r0,32(%r0)
	stw	%r0,-64(%r6)
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_setstklim
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_SETSTKLIM(%r0)
	ldw	%r0,MAGIC_SCN-SCN_SETSTKLIM(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_setstklim: .word SCN_SETSTKLIM

; int16 printinstr (uint16)
	.align	2
	.global	printinstr
printinstr:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_printinstr
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_PRINTINSTR(%r0)
	ldw	%r0,MAGIC_SCN-SCN_PRINTINSTR(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_printinstr: .word SCN_PRINTINSTR

; uint16 rdps ()
	.align	2
	.global	rdps
rdps:
	rdps	%r0
	lda	%pc,0(%r3)

; uint16 wrps (uint16)
	.align	2
	.global	wrps
wrps:
	ldw	%r1,-64(%r6)
	rdps	%r0
	wrps	%r1
	lda	%r6,2(%r6)
	lda	%pc,0(%r3)

; void watchwrite (uint16)
	.align	2
	.global	watchwrite
watchwrite:
	lda	%r6,-2(%r6)
	lda	%r1,-64(%r6)
	ldw	%r0,ss_watchwrite
	stw	%r0,0(%r1)
	stw	%r1,MAGIC_SCN-SCN_WATCHWRITE(%r0)
	ldw	%r0,MAGIC_SCN-SCN_WATCHWRITE(%r0)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
ss_watchwrite: .word SCN_WATCHWRITE

