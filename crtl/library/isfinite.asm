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
;;    http://www.gnu.org/licenses/gpl-2.0.html

	.include "magicdefs.asm"

	.align	2

	.global	isfinite
isfinite:
	clr	%r0
	ldw	%r2,6-64(%r6)
	ldw	%r1,isfin_7FF0
	and	%r2,%r2,%r1
	cmp	%r2,%r1
	beq	notfin
	inc	%r0,%r0
notfin:
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
isfin_7FF0: .word 0x7FF0

