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

