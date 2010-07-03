;**************************************************************************
;    Lightspark, a free flash player implementation
;
;    Copyright (C) 2009,2010 Alessandro Pignotti (a.pignotti@sssup.it)
;
;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU Lesser General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU Lesser General Public License for more details.
;
;    You should have received a copy of the GNU Lesser General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.
;**************************************************************************

%ifidn __OUTPUT_FORMAT__,elf64
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
section .text

global fastYUV420ChannelsToYUV0Buffer_SSE2Aligned
global fastYUV420ChannelsToYUV0Buffer_SSE2Unaligned

fastYUV420ChannelsToYUV0Buffer_SSE2Aligned:
; register mapping:
;	RDI -> Y buffer
;	RSI -> U buffer
;	RDX -> V buffer
;	RCX -> out buffer
;	R8  -> width (in pixels)
;	R9  -> height (in pixels)
;	EAX -> used pixels of the line
;	R10 -> used lines
;	R11 -> temp

;	XMM0 -> Y1,Y2
;	XMM1 -> U
;	XMM2 -> V
;	XMM3 -> U1,U2
;	XMM4 -> V1,V2

xor eax,eax
xor r10,r10

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

; Account used pixels
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

; Check for end (always 16 aligned)
	cmp eax,r8d
	je line_end

; Check if we have to reload only Y (16 aligned) or all the buffers (32 aligned)
	test eax,0x10
	jnz inner_loop
	jmp outer_loop
line_end:
; Account a line, reset eax to zero and rewind U and V buffer pointer on odd lines
	inc r10
	cmp r10,r9
; Exit if this is the last line
	je loop_end
	xor eax,eax
	test r10d,0x1
	jz outer_loop
	; r11 = frameWidth/2
	mov r11, r8
	shr r11, 0x1
	sub rsi, r11
	sub rdx, r11
	jmp outer_loop

loop_end:
; Commit stores
	sfence
	ret

fastYUV420ChannelsToYUV0Buffer_SSE2Unaligned:
; register mapping:
;	RDI -> Y buffer
;	RSI -> U buffer
;	RDX -> V buffer
;	RCX -> out buffer
;	R8  -> width (in pixels)
;	R9  -> height (in pixels)
;	EAX -> used pixels of the line
;	R10 -> used lines
;	R11 -> temp

;	XMM0 -> Y1,Y2
;	XMM1 -> U
;	XMM2 -> V
;	XMM3 -> U1,U2
;	XMM4 -> V1,V2

xor eax,eax
xor r10,r10
outer_loop2:
; Load 16 bytes/32 pixels from U/V buffers
	movupd xmm1,[rsi]
	movupd xmm2,[rdx]
	add rsi,16
	add rdx,16

; Duplicate U and V samples to match with Y samples
	movapd xmm3,xmm1
	movapd xmm4,xmm2
	punpcklbw xmm3,xmm3
	punpcklbw xmm4,xmm4
	jmp inner_loop_after_dup2

inner_loop2:
	movapd xmm3,xmm1
	movapd xmm4,xmm2
	punpckhbw xmm3,xmm3
	punpckhbw xmm4,xmm4

inner_loop_after_dup2:
; Load 16 bytes/pixels from Y-buffer
	movupd xmm0,[rdi]
	add rdi,16

; Account used pixels
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
; Be careful, this may be unaligned to 16
	cmp eax,r8d
	jge line_end2

; Check if we have to reload only Y (16 aligned) or all the buffers (32 aligned)
	test eax,0x10
	jnz inner_loop2
	jmp outer_loop2
line_end2:
; Account a line, reset eax to zero and rewind U and V buffer pointer on odd lines
	inc r10
	cmp r10,r9
; Exit if this is the last line
	je loop_end2
	xor eax,eax
; Reset the Y pointer to the right place
	mov r11, r8
	neg r11
	and r11, 0xf
	sub rdi, r11
; Reset the U and V pointers to the right place
	mov r11, r8
	shr r11, 0x1
	add r11, 0xf
	and r11, 0x7ffffff0
	sub rsi, r11
	sub rdx, r11
	test r10d,0x1
	jnz outer_loop2
	mov r11, r8
	shr r11, 0x1
	add rsi, r11
	add rdx, r11
	jmp outer_loop2

loop_end2:
; Commit stores
	sfence
	ret
