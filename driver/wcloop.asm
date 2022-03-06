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

start:
	ldw	%r6,loop+0
	stw	%r6,start-1
	ldw	%r6,loop+2
	stw	%r6,start+1
	ldw	%r6,loop+4
	stw	%r6,start+3

	lda	%r6,start+1	; R6 = 0001
	lda	%pc,start-1	; PC = FFFF

loop:
	sub	%r6,%r6,%pc	; FFFF FETCH1: MD = PC; MREAD
				;      FETCH2: IR = MQ; PC = PC + 2 (maximum ripple carry for FFFF + 0002 => 0001)
				;      ARITH1: R6 = R6 - PC
				;         PC reg update from FETCH2 happens somewhere at beginning of ARITH1
				;         updated PC must filter through Bnot XOR gates cuz of subtract to become FFFE
				;         then R6 - PC takes maximal ripple carry to do 0001 + FFFE + 1 => 0000
	inc	%r6,%r6		; 0001
	br	loop		; 0003

