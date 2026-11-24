InstallTimer:
    push ax
    push dx
    push ds
    push es

    push cs
    pop  ds
    mov  ax, 351Ch
    int  21h
    mov  [old_timer_offset], bx
    mov  [old_timer_segment], es

    mov  dx, TimerISR
    mov  ax, 251Ch
    int  21h

    pop  es
    pop  ds
    pop  dx
    pop  ax
    ret

RestoreTimer:
    push ax
    push dx
    push ds

    push cs
    pop  ds
    mov  dx, [old_timer_offset]
    mov  ax, [old_timer_segment]
    mov  ds, ax
    mov  ax, 251Ch
    int  21h

    pop  ds
    pop  dx
    pop  ax
    ret

TimerISR:
    push ax
    push ds
    push cs
    pop  ds

    inc  word [timer_ticks]
    jnz  .chain_original
    inc  word [timer_ticks+2]

.chain_original:
    cmp  byte [game_state], STATE_PLAYING
    jne  .chain
    add  word [clock_subticks], 549
    cmp  word [clock_subticks], 10000
    jb   .chain
    sub  word [clock_subticks], 10000
    cmp  word [remaining_seconds], 0
    je   .chain
    dec  word [remaining_seconds]

.chain:
    pushf
    call far [cs:old_timer_offset]

    pop  ds
    pop  ax
    iret

old_timer_offset dw 0
old_timer_segment dw 0
timer_ticks      dd 0
clock_subticks   dw 0
remaining_seconds dw 300