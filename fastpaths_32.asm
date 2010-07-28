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

%ifidn __OUTPUT_FORMAT__,elf
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
section .text

global fastYUV420ChannelsToYUV0Buffer_SSE2Aligned
global fastYUV420ChannelsToYUV0Buffer_SSE2Unaligned

fastYUV420ChannelsToYUV0Buffer_SSE2Aligned:
; register mapping:
;	EDI -> Y buffer
;	ESI -> U buffer
;	EDX -> V buffer
;	ECX -> out buffer
;	EAX -> used pixels of the line
;	EBX -> used lines

;	XMM0 -> Y1,Y2
;	XMM1 -> U
;	XMM2 -> V
;	XMM3 -> U1,U2
;	XMM4 -> V1,V2

; Save registers
push ebp
mov ebp,esp

push edi
push esi
push ebx

xor eax,eax
xor ebx,ebx
mov edi,[ebp+8]
mov esi,[ebp+12]
mov edx,[ebp+16]
mov ecx,[ebp+20]

outer_loop:
; Load 16 bytes/32 pixels from U/V buffers
	movapd xmm1,[esi]
	movapd xmm2,[edx]
	add esi,16
	add edx,16

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
	movapd xmm0,[edi]
	add edi,16

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
	movntdq [ecx],xmm7
	movntdq [ecx+16],xmm5

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
	movntdq [ecx+32],xmm7
	movntdq [ecx+48],xmm0
	add ecx,64

; Check for end (always 16 aligned)
	cmp eax,[ebp+24]
	je line_end

; Check if we have to reload only Y (16 aligned) or all the buffers (32 aligned)
	test eax,0x10
	jnz inner_loop
	jmp outer_loop

line_end:
; Account a line, reset eax to zero and rewind U and V buffer pointer on odd lines
	inc ebx
	cmp ebx,[ebp+28]
; Exit if this is the last line
	je loop_end
	xor eax,eax
	test ebx,0x1
	jz outer_loop
	; eax = frameWidth/2
	mov eax, [ebp+24]
	shr eax, 0x1
	sub esi, eax
	sub edx, eax
	xor eax,eax
	jmp outer_loop
loop_end:
; Commit stores
	sfence
	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

fastYUV420ChannelsToYUV0Buffer_SSE2Unaligned:
; register mapping:
;	EDI -> Y buffer
;	ESI -> U buffer
;	EDX -> V buffer
;	ECX -> out buffer
;	EAX -> used pixels of the line
;	EBX -> used lines

;	XMM0 -> Y1,Y2
;	XMM1 -> U
;	XMM2 -> V
;	XMM3 -> U1,U2
;	XMM4 -> V1,V2

; Save registers
push ebp
mov ebp,esp

push edi
push esi
push ebx

xor eax,eax
xor ebx,ebx
mov edi,[ebp+8]
mov esi,[ebp+12]
mov edx,[ebp+16]
mov ecx,[ebp+20]

outer_loop2:
; Load 16 bytes/32 pixels from U/V buffers
	movupd xmm1,[esi]
	movupd xmm2,[edx]
	add esi,16
	add edx,16

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
	movupd xmm0,[edi]
	add edi,16

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
	movntdq [ecx],xmm7
	movntdq [ecx+16],xmm5

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
	movntdq [ecx+32],xmm7
	movntdq [ecx+48],xmm0
	add ecx,64

; Be careful, this may be unaligned to 16
	cmp eax,[ebp+24]
	jge line_end2

; Check if we have to reload only Y (16 aligned) or all the buffers (32 aligned)
	test eax,0x10
	jnz inner_loop2
	jmp outer_loop2

line_end2:
; Account a line, reset eax to zero and rewind U and V buffer pointer on odd lines
	inc ebx
	cmp ebx,[ebp+28]
; Exit if this is the last line
	je loop_end2
; Reset the Y pointer to the right place
	mov eax, [ebp+24]
	neg eax
	and eax, 0xf
	sub edi, eax
; Reset the U and V pointers to the right place
	mov eax, [ebp+24]
	shr eax, 0x1
	add eax, 0xf
	and eax, 0x7ffffff0
	sub esi, eax
	sub edx, eax
	xor eax,eax
	test ebx,0x1
	jnz outer_loop2
	mov eax, [ebp+24]
	shr eax, 0x1
	add esi, eax
	add edx, eax
	xor eax,eax
	jmp outer_loop2

loop_end2:
; Commit stores
	sfence
	pop ebx
	pop esi
	pop edi
	pop ebp
	ret
