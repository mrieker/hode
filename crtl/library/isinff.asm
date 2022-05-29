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

	.align	2
	.global	isinff
isinff:
	clr	%r0
	ldw	%r2,2-64(%r6)
	ldw	%r1,#0x80
	add	%r2,%r2,%r1
	shl	%r2,%r2
	ldw	%r1,0-64(%r6)
	or	%r2,%r2,%r1
	bne	notinff
	inc	%r0,%r0
notinff:
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
