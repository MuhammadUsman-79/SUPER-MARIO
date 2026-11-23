SCREEN_WIDTH  equ 320
SCREEN_HEIGHT equ 200
VIDEO_SEGMENT equ 0A000h
FRAME_WORDS   equ 32000

PutPixel:
    push bp
    mov  bp, sp
    push ax
    push bx
    push cx
    push di
    push es

    cmp  word [bp+4], SCREEN_WIDTH
    jae  .done
    cmp  word [bp+6], SCREEN_HEIGHT
    jae  .done

    mov  ax, [bp+6]
    mov  bx, ax
    mov  cl, 8
    shl  ax, cl
    mov  cl, 6
    shl  bx, cl
    add  ax, bx
    add  ax, [bp+4]
    mov  di, ax

    mov  ax, [draw_segment]
    mov  es, ax
    mov  ax, [bp+8]
    mov  [es:di], al

.done:
    pop  es
    pop  di
    pop  cx
    pop  bx
    pop  ax
    pop  bp
    ret

FillRect:
    push bp
    mov  bp, sp
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    cmp  word [bp+8], 0
    je   .done
    cmp  word [bp+10], 0
    je   .done
    cmp  word [bp+4], SCREEN_WIDTH
    jae  .done
    cmp  word [bp+6], SCREEN_HEIGHT
    jae  .done

    mov  ax, [bp+4]
    add  ax, [bp+8]
    jc   .done
    cmp  ax, SCREEN_WIDTH
    ja   .done

    mov  ax, [bp+6]
    add  ax, [bp+10]
    jc   .done
    cmp  ax, SCREEN_HEIGHT
    ja   .done

    mov  ax, [bp+6]
    mov  bx, ax
    mov  cl, 8
    shl  ax, cl
    mov  cl, 6
    shl  bx, cl
    add  ax, bx
    add  ax, [bp+4]
    mov  si, ax

    mov  ax, [draw_segment]
    mov  es, ax
    mov  dx, [bp+10]
    mov  al, [bp+12]
    cld

.row:
    mov  di, si
    mov  cx, [bp+8]
    rep  stosb
    add  si, SCREEN_WIDTH
    dec  dx
    jnz  .row

.done:
    pop  es
    pop  di
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    pop  bp
    ret

DrawSprite:
    push bp
    mov  bp, sp
    sub  sp, 10
    push ax
    push bx
    push cx
    push dx
    push si

    mov  si, [bp+8]

.next_rect:
    cmp  byte [si], 0FFh
    jne  .has_rect
    jmp  .done
.has_rect:
    cmp  byte [si+4], 0
    jne  .draw_rect_record
    jmp  .next_record
.draw_rect_record:

    test word [bp+10], 1
    jz   .normal_x
    mov  dx, 16
    xor  ax, ax
    mov  al, [si]
    sub  dx, ax
    xor  cx, cx
    mov  cl, [si+2]
    sub  dx, cx
    add  dx, [bp+4]
    mov  [bp-2], dx
    jmp  .set_y

.normal_x:
    xor  ax, ax
    mov  al, [si]
    add  ax, [bp+4]
    mov  [bp-2], ax

.set_y:
    xor  ax, ax
    mov  al, [si+1]
    add  ax, [bp+6]
    mov  [bp-4], ax

    xor  ax, ax
    mov  al, [si+2]
    mov  [bp-6], ax
    xor  ax, ax
    mov  al, [si+3]
    mov  [bp-8], ax
    xor  ax, ax
    mov  al, [si+4]
    mov  [bp-10], ax

    cmp  word [bp-2], 0
    jge  .x_nonnegative
    mov  ax, [bp-2]
    neg  ax
    mov  cx, [bp-6]
    sub  cx, ax
    jle  .next_record
    mov  [bp-6], cx
    mov  word [bp-2], 0

.x_nonnegative:
    cmp  word [bp-2], SCREEN_WIDTH
    jae  .next_record
    mov  ax, [bp-2]
    add  ax, [bp-6]
    cmp  ax, SCREEN_WIDTH
    jbe  .clip_top
    mov  ax, SCREEN_WIDTH
    sub  ax, [bp-2]
    mov  [bp-6], ax

.clip_top:
    cmp  word [bp-4], 0
    jge  .y_nonnegative
    mov  ax, [bp-4]
    neg  ax
    mov  cx, [bp-8]
    sub  cx, ax
    jle  .next_record
    mov  [bp-8], cx
    mov  word [bp-4], 0

.y_nonnegative:
    cmp  word [bp-4], SCREEN_HEIGHT
    jae  .next_record
    mov  ax, [bp-4]
    add  ax, [bp-8]
    cmp  ax, SCREEN_HEIGHT
    jbe  .draw_rect
    mov  ax, SCREEN_HEIGHT
    sub  ax, [bp-4]
    mov  [bp-8], ax

.draw_rect:
    push word [bp-10]
    push word [bp-8]
    push word [bp-6]
    push word [bp-4]
    push word [bp-2]
    call FillRect
    add  sp, 10

.next_record:
    add  si, 5
    jmp  .next_rect

.done:
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    mov  sp, bp
    pop  bp
    ret

ClearFrame:
    push bp
    mov  bp, sp
    push ax
    push cx
    push di
    push es

    mov  ax, [frame_segment]
    mov  es, ax
    xor  di, di
    mov  al, [bp+4]
    mov  ah, al
    mov  cx, FRAME_WORDS
    cld
    rep  stosw

    pop  es
    pop  di
    pop  cx
    pop  ax
    pop  bp
    ret

CopyFrame:
    push ax
    push cx
    push si
    push di
    push ds
    push es

    mov  ax, [frame_segment]
    mov  ds, ax
    mov  ax, VIDEO_SEGMENT
    mov  es, ax
    xor  si, si
    xor  di, di
    mov  cx, FRAME_WORDS
    cld
    rep  movsw

    pop  es
    pop  ds
    pop  di
    pop  si
    pop  cx
    pop  ax
    ret

; ============================================================================
; TEXT, SCALED SPRITES AND SCREEN HELPERS (used by the title / end screens)
; ============================================================================

NAVY_COLOR    equ 32          ; custom DAC colour: dark navy  (set in SetPalette)
REDORANGE     equ 33          ; custom DAC colour: red-orange (set in SetPalette)

; ---- small macros that push stack parameters and call the procedure --------
%macro DRAW_TEXT 5            ; x, y, scale, colour, string address
    mov  ax, %5
    push ax
    mov  ax, %4
    push ax
    mov  ax, %3
    push ax
    mov  ax, %2
    push ax
    mov  ax, %1
    push ax
    call DrawText
    add  sp, 10
%endmacro

%macro OUTLINE_TEXT 5         ; x, y, scale, colour, string address
    mov  ax, %5
    push ax
    mov  ax, %4
    push ax
    mov  ax, %3
    push ax
    mov  ax, %2
    push ax
    mov  ax, %1
    push ax
    call DrawTextOutlined
    add  sp, 10
%endmacro

%macro CENTER_TEXT 4          ; y, scale, colour, string address
    mov  ax, %4
    push ax
    mov  ax, %3
    push ax
    mov  ax, %2
    push ax
    mov  ax, %1
    push ax
    call DrawTextCentered
    add  sp, 8
%endmacro

%macro FILL_RECT 5            ; x, y, width, height, colour
    mov  ax, %5
    push ax
    mov  ax, %4
    push ax
    mov  ax, %3
    push ax
    mov  ax, %2
    push ax
    mov  ax, %1
    push ax
    call FillRect
    add  sp, 10
%endmacro

%macro CLEAR_FRAME 1          ; colour
    mov  ax, %1
    push ax
    call ClearFrame
    add  sp, 2
%endmacro

%macro APPEND_TEXT 1          ; AX = destination, %1 = source text -> AX = new end
    mov  bx, %1
    push bx
    push ax
    call AppendText
    add  sp, 4
%endmacro

%macro APPEND_NUM 0           ; AX = destination, BX = value      -> AX = new end
    push bx
    push ax
    call AppendNumber
    add  sp, 4
%endmacro

; ----------------------------------------------------------------------------
; InitFont
; Purpose : asks the BIOS where its 8x8 ROM font lives (INT 10h, AX=1130h,
;           BH=03h returns ES:BP) and remembers it in font_seg / font_off.
; Inputs  : none        Outputs: font_seg, font_off
; Changes : no registers (everything is saved)
; ----------------------------------------------------------------------------
InitFont:
    push ax
    push bx
    push cx
    push dx
    push bp
    push es
    mov  ax, 1130h
    mov  bh, 03h
    int  10h
    mov  [font_seg], es
    mov  [font_off], bp
    pop  es
    pop  bp
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; SetPalette
; Purpose : programs two custom DAC colours (INT 10h, AX=1010h):
;           NAVY_COLOR (dark blue background) and REDORANGE (title / game over).
;           Must be called after the video mode is set (a mode set resets DAC).
; Inputs  : none        Outputs: none        Changes: no registers
; ----------------------------------------------------------------------------
SetPalette:
    push ax
    push bx
    push cx
    push dx
    mov  ax, 1010h
    mov  bx, NAVY_COLOR
    xor  dh, dh                  ; red   = 0
    xor  ch, ch                  ; green = 0
    mov  cl, 12                  ; blue  = 12 (of 63)
    int  10h
    mov  ax, 1010h
    mov  bx, REDORANGE
    mov  dh, 63                  ; red   = 63
    mov  ch, 14                  ; green = 14
    xor  cl, cl                  ; blue  = 0
    int  10h
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; DrawText(x, y, scale, colour, string)
; Purpose : draws a zero-terminated string into the frame buffer using the
;           BIOS 8x8 ROM font. Every font pixel becomes a scale x scale block.
;           text_spacing (pixels) is added after every character.
; Inputs  : [bp+4]=x  [bp+6]=y  [bp+8]=scale  [bp+10]=colour  [bp+12]=string
; Outputs : none        Changes: no registers
; ----------------------------------------------------------------------------
DrawText:
    push bp
    mov  bp, sp
    sub  sp, 8                   ; [bp-2]=pen x  [bp-4]=cell x  [bp-6]=cell y  [bp-8]=rows left
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    mov  si, [bp+12]
    mov  ax, [bp+4]
    mov  [bp-2], ax
.next_char:
    xor  ax, ax
    mov  al, [si]
    test al, al
    jz   .done
    inc  si
    mov  cl, 3
    shl  ax, cl                  ; glyph offset = char * 8
    add  ax, [font_off]
    mov  di, ax
    mov  ax, [bp+6]
    mov  [bp-6], ax
    mov  word [bp-8], 8
.row:
    mov  ax, [bp-2]
    mov  [bp-4], ax
    mov  es, [font_seg]
    mov  al, [es:di]             ; one row of the glyph (bit 7 = left pixel)
    inc  di
    mov  bl, 80h
    mov  cx, 8
.col:
    test al, bl
    jz   .skip_pixel
    push word [bp+10]
    push word [bp+8]
    push word [bp+8]
    push word [bp-6]
    push word [bp-4]
    call FillRect
    add  sp, 10
.skip_pixel:
    mov  dx, [bp+8]
    add  [bp-4], dx
    shr  bl, 1
    loop .col
    mov  dx, [bp+8]
    add  [bp-6], dx
    dec  word [bp-8]
    jnz  .row
    mov  ax, [bp+8]
    mov  cl, 3
    shl  ax, cl
    add  ax, [text_spacing]
    add  [bp-2], ax
    jmp  .next_char
.done:
    pop  es
    pop  di
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    mov  sp, bp
    pop  bp
    ret

; ----------------------------------------------------------------------------
; DrawTextOutlined(x, y, scale, colour, string)
; Purpose : same as DrawText but first stamps the text in black at the eight
;           neighbouring pixels, giving a 1-pixel dark outline.
; Inputs  : same as DrawText     Outputs: none     Changes: no registers
; ----------------------------------------------------------------------------
DrawTextOutlined:
    push bp
    mov  bp, sp
    push ax
    push cx
    push si

    mov  si, outline_offsets
    mov  cx, 8
.outline:
    push word [bp+12]
    xor  ax, ax
    push ax                      ; black
    push word [bp+8]
    mov  al, [si+1]              ; dy (signed byte)
    cbw
    add  ax, [bp+6]
    push ax
    mov  al, [si]                ; dx (signed byte)
    cbw
    add  ax, [bp+4]
    push ax
    call DrawText
    add  sp, 10
    add  si, 2
    loop .outline

    push word [bp+12]
    push word [bp+10]
    push word [bp+8]
    push word [bp+6]
    push word [bp+4]
    call DrawText
    add  sp, 10

    pop  si
    pop  cx
    pop  ax
    pop  bp
    ret

; ----------------------------------------------------------------------------
; DrawTextCentered(y, scale, colour, string)
; Purpose : draws a string horizontally centred on the 320-pixel screen.
; Inputs  : [bp+4]=y  [bp+6]=scale  [bp+8]=colour  [bp+10]=string
; Outputs : none        Changes: no registers
; ----------------------------------------------------------------------------
DrawTextCentered:
    push bp
    mov  bp, sp
    push ax
    push bx
    push dx
    push si

    mov  si, [bp+10]
    xor  bx, bx                  ; bx = number of characters
.count:
    cmp  byte [si], 0
    je   .counted
    inc  si
    inc  bx
    jmp  .count
.counted:
    mov  ax, [bp+6]
    shl  ax, 1
    shl  ax, 1
    shl  ax, 1                   ; 8 * scale
    add  ax, [text_spacing]
    mul  bx                      ; dx:ax = chars * advance
    sub  ax, [text_spacing]      ; no spacing after the last character
    mov  bx, SCREEN_WIDTH
    sub  bx, ax
    shr  bx, 1                   ; bx = left x

    push word [bp+10]
    push word [bp+8]
    push word [bp+6]
    push word [bp+4]
    push bx
    call DrawText
    add  sp, 10

    pop  si
    pop  dx
    pop  bx
    pop  ax
    pop  bp
    ret

; ----------------------------------------------------------------------------
; DrawSpriteScaled(x, y, shape, scale)
; Purpose : draws a rectangle-list sprite (same format as DrawSprite) enlarged
;           by an integer factor. No mirroring, no clipping (keep it on screen).
; Inputs  : [bp+4]=x  [bp+6]=y  [bp+8]=shape address  [bp+10]=scale
; Outputs : none        Changes: no registers
; ----------------------------------------------------------------------------
DrawSpriteScaled:
    push bp
    mov  bp, sp
    push ax
    push bx
    push si

    mov  si, [bp+8]
    mov  bl, [bp+10]
.next:
    cmp  byte [si], 0FFh
    je   .done
    cmp  byte [si+4], 0
    je   .skip
    xor  ax, ax
    mov  al, [si+4]
    push ax                      ; colour
    mov  al, [si+3]
    mul  bl
    push ax                      ; height * scale
    mov  al, [si+2]
    mul  bl
    push ax                      ; width * scale
    mov  al, [si+1]
    mul  bl
    add  ax, [bp+6]
    push ax                      ; y
    mov  al, [si]
    mul  bl
    add  ax, [bp+4]
    push ax                      ; x
    call FillRect
    add  sp, 10
.skip:
    add  si, 5
    jmp  .next
.done:
    pop  si
    pop  bx
    pop  ax
    pop  bp
    ret

; ----------------------------------------------------------------------------
; AppendText(dest, source)
; Purpose : copies the zero-terminated source string to dest (including the 0).
; Inputs  : [bp+4]=dest  [bp+6]=source
; Outputs : AX = address of the terminating 0 in dest (use it as the next dest)
; Changes : AX only
; ----------------------------------------------------------------------------
AppendText:
    push bp
    mov  bp, sp
    push si
    push di
    push es
    push ds
    pop  es
    cld
    mov  si, [bp+6]
    mov  di, [bp+4]
.copy:
    lodsb
    stosb
    test al, al
    jnz  .copy
    dec  di
    mov  ax, di
    pop  es
    pop  di
    pop  si
    pop  bp
    ret

; ----------------------------------------------------------------------------
; AppendNumber(dest, value)
; Purpose : writes value as decimal digits (no leading zeros) followed by 0.
;           Digits are produced by repeated division by 10 and popped in
;           reverse order from the stack.
; Inputs  : [bp+4]=dest  [bp+6]=value (0..65535)
; Outputs : AX = address of the terminating 0 in dest
; Changes : AX only
; ----------------------------------------------------------------------------
AppendNumber:
    push bp
    mov  bp, sp
    push bx
    push cx
    push dx
    push di
    mov  di, [bp+4]
    mov  ax, [bp+6]
    mov  bx, 10
    xor  cx, cx
.divide:
    xor  dx, dx
    div  bx
    push dx
    inc  cx
    test ax, ax
    jnz  .divide
.emit:
    pop  dx
    add  dl, '0'
    mov  [di], dl
    inc  di
    loop .emit
    mov  byte [di], 0
    mov  ax, di
    pop  di
    pop  dx
    pop  cx
    pop  bx
    pop  bp
    ret

font_seg        dw 0F000h        ; BIOS 8x8 font (filled in by InitFont)
font_off        dw 0FA6Eh
text_spacing    dw 0             ; extra pixels after every character
screen_key      dw 0FFFFh        ; which static screen is currently drawn (FFFFh = none)
line_buffer     times 40 db 0
outline_offsets db -1,-1, 0,-1, 1,-1, -1,0, 1,0, -1,1, 0,1, 1,1