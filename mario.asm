BITS 16
ORG 100h

jmp start

%include "VIDEO.INC"
%include "DRAW.INC"
%include "LEVEL.INC"
%include "INPUT.INC"
%include "TIMER.INC"
%include "MOVEMENT.INC"
%include "GAME.INC"

COLOR_SKY equ 9

start:
    push cs
    pop  ds
    cli
    mov  ax, cs
    mov  ss, ax
    mov  sp, StackTop
    sti

    mov  ax, cs
    mov  es, ax
    mov  bx, StackTop
    add  bx, 15
    mov  cl, 4
    shr  bx, cl
    mov  ah, 4Ah
    int  21h
    jc   .allocation_failure

    mov  bx, 1000h
    mov  ah, 48h
    int  21h
    jc   .allocation_failure
    mov  [frame_segment], ax
    mov  [draw_segment], ax

    call NewGame
    call InstallKeyboard
    call InstallTimer
    call VideoMode13
    call SetPalette              ; custom colours (after the mode set)
    call InitFont                ; BIOS 8x8 font pointer for DrawText
    call GameStep

.main_loop:
    cmp  byte [game_state], STATE_EXIT
    je   .shutdown
.wait_for_tick:
    mov  ax, [timer_ticks]
    cmp  ax, [last_tick]
    je   .main_loop
    mov  [last_tick], ax
    xor  byte [movement_frame], 1
    call GameStep
    jmp  .main_loop

.shutdown:
    call RestoreTimer
    call RestoreKeyboard
    call VideoTextMode
    mov  ax, [frame_segment]
    mov  es, ax
    mov  ah, 49h
    int  21h
    mov  ax, 4C00h
    int  21h

.allocation_failure:
    mov  ax, 4C01h
    int  21h

RenderFrame:
    push ax

    mov  word [screen_key], 0FFFFh   ; a game frame replaces any static screen

    mov  ax, COLOR_SKY
    push ax
    call ClearFrame
    add  sp, 2

    mov  ax, [camera_x]
    push ax
    call DrawMap
    add  sp, 2

    ; Decorative elder at world x=904, ground y=176.
    push ax
    push bx
    mov ax, 904
    sub ax, [camera_x]
    cmp ax, 320
    jae .no_elder
    mov bx, ax
    push word 0
    push word ElderShape
    push word 144
    push bx
    call DrawSprite
    add sp, 8
.no_elder:
    pop bx
    pop ax
    call RenderGameActors
    call DrawMario

    xor  ax, ax
    push ax
    mov  ax, 8
    push ax
    mov  ax, 320
    push ax
    mov  ax, 192
    push ax
    xor  ax, ax
    push ax
    call FillRect
    add  sp, 10

    call DrawHUD                 ; present HUD and scene together
    call CopyFrame

    pop  ax
    ret

ResetGameTimer:
    mov  word [timer_ticks], 0
    mov  word [timer_ticks+2], 0
    mov  word [clock_subticks], 0
    mov  word [remaining_seconds], 300
    mov  ax, [timer_ticks]
    mov  [last_tick], ax
    ret

frame_segment dw 0
draw_segment  dw 0
last_tick     dw 0
stack_space   times 1024 db 0
StackTop: