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

; test that a worst-case lsof address calculation can reach raspi
; by end of cycle when raspi takes gpio sample before raising clock

; time ./raspictl.armv7l -cpuhz 77000 -mintimes -oddok wc2loop.hex

start:
	lda	%r0,start-1	; r0 = FFFF
	lda	%r1,start+1	; r1 = 0001

	ldw	%r6,loop+0	; copy loop to FFFF
	stw	%r6,start-1
	ldw	%r6,loop+2
	stw	%r6,start+1
	ldw	%r6,loop+4
	stw	%r6,start+3
	ldw	%r6,loop+6
	stw	%r6,start+5

	lda	%pc,start-1	; jump to FFFF

loop:
	ldw	%r6,loop+1	; FFFF: MA = pc=0001 + lsof=FFFF
	ldw	%r6,1(%r0)	; 0001: MA = r0=FFFF + lsof=0001
	ldw	%r6,0-1(%r1)	; 0003: MA = r1=0001 + lsof=FFFF
	br	loop		; 0005

