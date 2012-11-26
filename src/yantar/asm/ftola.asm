; Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
; 
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License.

; MASM ftol conversion functions using SSE or FPU
; assume __cdecl calling convention is being used for x86, __fastcall for x64

IFNDEF idx64
.model flat, c
ENDIF

.data

ifndef idx64
  fpucw WORD 0F7Fh
endif

.code

IFDEF idx64
; qftol using SSE

  qftolsse PROC
    cvttss2si eax, xmm0
	ret
  qftolsse ENDP

  qvmftolsse PROC
    movss xmm0, dword ptr [rdi + rbx * 4]
	cvttss2si eax, xmm0
	ret
  qvmftolsse ENDP

ELSE
; qftol using FPU

  qftolx87m macro src
    sub esp, 2
    fnstcw word ptr [esp]
    fldcw fpucw
    fld dword ptr src
	fistp dword ptr src
	fldcw [esp]
	mov eax, src
	add esp, 2
	ret
  endm
  
  qftolx87 PROC
    qftolx87m [esp + 6]
  qftolx87 ENDP

  qvmftolx87 PROC
    qftolx87m [edi + ebx * 4]
  qvmftolx87 ENDP

; qftol using SSE
  qftolsse PROC
    movss xmm0, dword ptr [esp + 4]
    cvttss2si eax, xmm0
	ret
  qftolsse ENDP

  qvmftolsse PROC
    movss xmm0, dword ptr [edi + ebx * 4]
	cvttss2si eax, xmm0
	ret
  qvmftolsse ENDP
ENDIF

end
