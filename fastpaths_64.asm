;**************************************************************************
;    Lightspark, a free flash player implementation
;
;    Copyright (C) 2009,2010 Alessandro Pignotti (a.pignotti@sssup.it)
;
;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.
;**************************************************************************

%ifidn __OUTPUT_FORMAT__,elf64
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
section .text

global fastYUV420ChannelsToBuffer

fastYUV420ChannelsToBuffer:
; register mapping:
;	RDI -> Y buffer
;	RSI -> U buffer
;	RDX -> V buffer
;	RCX -> out buffer
;	R8  -> size (in pixels)
;	EAX -> count of pixels

;	XMM0 -> Y1,Y2
;	XMM1 -> U
;	XMM2 -> V
;	XMM3 -> U1,U2
;	XMM4 -> V1,V2

xor rax,rax

outer_loop:
; Load 16 bytes/32 pixels from U/V buffers
	movapd xmm1,[rsi]
	movapd xmm2,[rdx]
	add rsi,16
	add rdx,16

; Duplicate U and V samples to match with Y samples
	movapd xmm3,xmm1
	movapd xmm4,xmm2
	punpcklbw xmm3,xmm3
	punpcklbw xmm4,xmm4
	jmp inner_loop_after_dup

inner_loop:
	movapd xmm3,xmm1
	movapd xmm4,xmm2
	punpckhbw xmm3,xmm3
	punpckhbw xmm4,xmm4

inner_loop_after_dup:
; Load 16 bytes/pixels from Y-buffer
	movapd xmm0,[rdi]
	add rdi,16

; Sub used pixels from size and move pointers
	add eax,16

; Unpack the low part of Y and U1 [YUYU]
	movapd xmm5,xmm0
	punpcklbw xmm5,xmm3

; Unpack the low part of V1 with zeroes [V1V1]
	movapd xmm6,xmm4
	pcmpeqb xmm7,xmm7
	punpcklbw xmm6,xmm7

; Unpack the low and high part of the those temporaries [YUV0]
	movapd xmm7,xmm5
	punpcklwd xmm7,xmm6 ;low part
	punpckhwd xmm5,xmm6 ;high part

; Store the results using non-temporal hint
	movntdq [rcx],xmm7
	movntdq [rcx+16],xmm5

; Use the temporaries for the high part of Y,U1
	punpckhbw xmm0,xmm3

; Use the temporaries for the high part of V1,1
	movapd xmm6,xmm4
	pcmpeqb xmm7,xmm7
	punpckhbw xmm6,xmm7

; Unpack the low and high part of the those temporaries [YUV0]
	movapd xmm7,xmm0
	punpcklwd xmm7,xmm6 ;low part
	punpckhwd xmm0,xmm6 ;high part

; Store the results using non-temporal hint
	movntdq [rcx+32],xmm7
	movntdq [rcx+48],xmm0
	add rcx,64

; Check for end
	cmp eax,r8d
	je loop_end

; Check if we have to reload only Y (16 aligned) or all the buffers (32 aligned)
	test eax,0x10
	jnz inner_loop
	jmp outer_loop

loop_end:
; Commit stores
	sfence
	ret
