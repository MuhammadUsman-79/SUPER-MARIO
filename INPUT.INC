InstallKeyboard:
    push ax
    push bx
    push dx
    push ds
    push es

    push cs
    pop  ds
    mov  ax, 3509h
    int  21h
    mov  [old_int09_offset], bx
    mov  [old_int09_segment], es

    mov  dx, KeyboardISR
    mov  ax, 2509h
    int  21h

    pop  es
    pop  dsa
    pop  dx
    pop  bx
    pop  ax
    ret

RestoreKeyboard:
    push ax
    push dx
    push ds

    push cs
    pop  ds
    mov  dx, [old_int09_offset]
    mov  ax, [old_int09_segment]
    mov  ds, ax
    mov  ax, 2509h
    int  21h

    pop  ds
    pop  dx
    pop  ax
    ret

KeyboardISR:
    push ax
    push bx
    push ds
    push cs
    pop  ds

    in   al, 60h
    cmp  al, 0E0h
    je   .extended_prefix
    cmp  al, 0E1h
    je   .acknowledge
    mov  byte [extended_pending], 0
    xor  bx, bx
    mov  bl, al
    test bl, 80h
    jnz  .key_release

    mov  byte [key_states+bx], 1
    jmp  .acknowledge

.key_release:
    and  bl, 7Fh
    mov  byte [key_states+bx], 0
    jmp  .acknowledge

.extended_prefix:
    mov  byte [extended_pending], 1

.acknowledge:
    mov  al, 20h
    out  20h, al
    pop  ds
    pop  bx
    pop  ax
    iret

old_int09_offset dw 0
old_int09_segment dw 0
extended_pending db 0
exit_requested db 0
key_states times 128 db 0