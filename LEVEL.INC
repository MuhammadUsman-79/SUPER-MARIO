%define TILE_SKY 0
%define TILE_GROUND 1
%define TILE_BRICK 2
%define TILE_QUESTION_COIN 3
%define TILE_QUESTION_MUSHROOM 4
%define TILE_EMPTY_BLOCK 5
%define TILE_PIPE_TOP_LEFT 6
%define TILE_PIPE_TOP_RIGHT 7
%define TILE_PIPE_BODY_LEFT 8
%define TILE_PIPE_BODY_RIGHT 9
%define TILE_STAIR 10
%define TILE_COIN 11
%define TILE_FLAG 12

DrawTile:
    push bp
    mov  bp, sp
    push ax
    push bx
    push si

    mov  ax, [bp+8]
    cmp  ax, TILE_FLAG
    ja   .done
    shl  ax, 1
    mov  bx, ax
    mov  si, [TileSpriteTable+bx]
    cmp  word [bp+8], TILE_FLAG
    jne  .draw
    cmp  word [bp+6], 32
    jne  .draw
    mov  si, FlagTopTile

.draw:
    test si, si
    jz   .done
    xor  ax, ax
    push ax
    push si
    push word [bp+6]
    push word [bp+4]
    call DrawSprite
    add  sp, 8

.done:
    pop  si
    pop  bx
    pop  ax
    pop  bp
    ret

DrawMap:
    push bp
    mov  bp, sp
    sub  sp, 6
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    mov  ax, [bp+4]
    cmp  ax, 640
    jbe  .camera_ok
    mov  ax, 640
.camera_ok:
    mov  bx, ax
    and  bx, 15
    neg  bx
    mov  [bp-4], bx
    mov  cl, 4
    shr  ax, cl
    mov  [bp-6], ax
    mov  si, LevelMapWorking
    add  si, ax
    mov  word [bp-2], 12
    xor  dx, dx
    mov  di, [bp-4]

.row:
    mov  cx, 21
.col:
    cmp  si, LevelMapWorking+720
    jae  .empty_tile
    xor  ax, ax
    mov  al, [si]
    jmp  .draw_tile
.empty_tile:
    xor  ax, ax
.draw_tile:
    push ax
    push dx
    push di
    call DrawTile
    add  sp, 6
    inc  si
    add  di, 16
    loop .col

    add  si, 39
    mov  di, [bp-4]
    add  dx, 16
    dec  word [bp-2]
    jnz  .row

.done:
    pop  di
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    mov  sp, bp
    pop  bp
    ret

GetTile:
    push bp
    mov  bp, sp
    push bx
    push cx
    push si

    cmp  word [bp+4], 960
    jae  .outside
    cmp  word [bp+6], 192
    jae  .outside

    mov  ax, [bp+6]
    shr  ax, 1
    shr  ax, 1
    shr  ax, 1
    shr  ax, 1
    mov  bx, ax
    shl  ax, 1
    shl  ax, 1
    shl  ax, 1
    shl  ax, 1
    shl  ax, 1
    shl  ax, 1
    mov  cx, bx
    shl  cx, 1
    shl  cx, 1
    sub  ax, cx

    mov  bx, [bp+4]
    shr  bx, 1
    shr  bx, 1
    shr  bx, 1
    shr  bx, 1
    add  ax, bx
    mov  si, LevelMapWorking
    add  si, ax
    xor  ax, ax
    mov  al, [si]
    jmp  .done

.outside:
    xor  ax, ax

.done:
    pop  si
    pop  cx
    pop  bx
    pop  bp
    ret

SetTileAt:
    push bp
    mov  bp, sp
    push ax
    push bx
    push cx
    push si

    cmp  word [bp+4], 960
    jae  .done
    cmp  word [bp+6], 192
    jae  .done

    mov  ax, [bp+6]
    mov  cl, 4
    shr  ax, cl
    mov  bx, ax
    mov  cl, 6
    shl  ax, cl
    mov  cl, 2
    shl  bx, cl
    sub  ax, bx
    mov  bx, [bp+4]
    mov  cl, 4
    shr  bx, cl
    add  ax, bx
    mov  si, LevelMapWorking
    add  si, ax
    mov  ax, [bp+8]
    mov  [si], al

.done:
    pop  si
    pop  cx
    pop  bx
    pop  ax
    pop  bp
    ret

ResetMap:
    push ax
    push cx
    push si
    push di
    push es

    mov  ax, ds
    mov  es, ax
    mov  si, LevelMapOriginal
    mov  di, LevelMapWorking
    mov  cx, 720
    cld
    rep  movsb

    pop  es
    pop  di
    pop  si
    pop  cx
    pop  ax
    ret

TileSpriteTable:
    dw 0
    dw GroundTile
    dw BrickTile
    dw QuestionCoinTile
    dw QuestionMushroomTile
    dw EmptyBlockTile
    dw PipeTopLeftTile
    dw PipeTopRightTile
    dw PipeBodyLeftTile
    dw PipeBodyRightTile
    dw StairTile
    dw CoinTileShape
    dw FlagPoleTile

LevelMapOriginal:
    times 60 db 0
    times 60 db 0
    times 58 db 0
    db 12
    times 1 db 0
    times 58 db 0
    db 12
    times 1 db 0
    times 2 db 0
    db 11
    times 2 db 0
    db 11
    times 4 db 0
    db 11
    times 4 db 0
    db 11
    times 2 db 0
    db 11
    times 4 db 0
    db 11
    times 20 db 0
    db 11
    times 13 db 0
    db 12
    times 1 db 0
    times 32 db 0
    db 11
    times 2 db 0
    db 11
    times 4 db 0
    times 3 db 2
    times 6 db 0
    db 11
    times 8 db 0
    db 12
    times 1 db 0
    times 4 db 0
    db 3
    times 3 db 0
    db 4
    times 3 db 0
    db 3
    times 5 db 0
    times 3 db 2
    times 5 db 0
    db 4
    times 3 db 0
    db 3
    times 25 db 0
    db 10
    times 1 db 0
    db 12
    times 1 db 0
    times 24 db 0
    db 6
    db 7
    times 29 db 0
    times 2 db 10
    times 1 db 0
    db 12
    times 1 db 0
    times 6 db 0
    db 6
    db 7
    times 16 db 0
    db 8
    db 9
    times 28 db 0
    times 3 db 10
    times 1 db 0
    db 12
    times 1 db 0
    times 6 db 0
    db 8
    db 9
    times 16 db 0
    db 8
    db 9
    times 27 db 0
    times 4 db 10
    times 1 db 0
    db 1
    times 1 db 0
    times 14 db 1
    times 3 db 0
    times 17 db 1
    times 3 db 0
    times 23 db 1
    times 14 db 1
    times 3 db 0
    times 17 db 1
    times 3 db 0
    times 23 db 1

LevelMapWorking:
    times 720 db 0

GroundTile:
    db 0, 0, 16, 16, 6
    db 0, 0, 16, 3, 10
    db 2, 5, 3, 2, 4
    db 9, 9, 4, 2, 4
    db 0FFh

BrickTile:
    db 0, 0, 16, 16, 6
    db 1, 1, 14, 2, 4
    db 1, 7, 14, 2, 4
    db 1, 13, 14, 2, 4
    db 7, 3, 2, 4, 4
    db 4, 9, 2, 4, 4
    db 11, 9, 2, 4, 4
    db 0FFh

QuestionCoinTile:
    db 0, 0, 16, 16, 14
    db 1, 1, 14, 2, 6
    db 1, 13, 14, 2, 6
    db 1, 3, 2, 10, 6
    db 13, 3, 2, 10, 6
    db 5, 3, 6, 2, 15
    db 9, 5, 3, 2, 15
    db 7, 7, 3, 2, 15
    db 7, 11, 3, 2, 15
    db 0FFh

QuestionMushroomTile:
    db 0, 0, 16, 16, 14
    db 1, 1, 14, 2, 6
    db 1, 13, 14, 2, 6
    db 1, 3, 2, 10, 6
    db 13, 3, 2, 10, 6
    db 4, 4, 8, 3, 4
    db 5, 5, 2, 2, 15
    db 9, 5, 2, 2, 15
    db 7, 7, 2, 4, 15
    db 6, 11, 4, 2, 15
    db 0FFh

EmptyBlockTile:
    db 0, 0, 16, 16, 8
    db 1, 1, 14, 2, 7
    db 1, 13, 14, 2, 7
    db 1, 3, 2, 10, 7
    db 13, 3, 2, 10, 7
    db 0FFh

PipeTopLeftTile:
    db 0, 2, 16, 14, 2
    db 0, 0, 16, 4, 10
    db 1, 1, 14, 2, 2
    db 2, 5, 4, 11, 10
    db 0FFh

PipeTopRightTile:
    db 0, 2, 16, 14, 2
    db 0, 0, 16, 4, 10
    db 1, 1, 14, 2, 2
    db 10, 5, 4, 11, 10
    db 0FFh

PipeBodyLeftTile:
    db 0, 0, 16, 16, 2
    db 2, 0, 4, 16, 10
    db 0FFh

PipeBodyRightTile:
    db 0, 0, 16, 16, 2
    db 10, 0, 4, 16, 10
    db 0FFh

StairTile:
    db 0, 0, 16, 16, 6
    db 1, 1, 14, 2, 4
    db 1, 7, 14, 2, 4
    db 1, 13, 14, 2, 4
    db 0FFh

CoinTileShape:
    db 5, 1, 6, 14, 14
    db 3, 3, 10, 10, 14
    db 2, 5, 12, 6, 14
    db 3, 11, 10, 3, 14
    db 5, 14, 6, 1, 14
    db 6, 3, 2, 10, 15
    db 0FFh

FlagPoleTile:
    db 7, 0, 2, 16, 10
    db 0FFh

FlagTopTile:
    db 7, 0, 2, 16, 10
    db 0, 0, 14, 2, 4
    db 0, 2, 14, 2, REDORANGE
    db 0, 4, 14, 2, 14
    db 0, 6, 14, 2, 2
    db 0, 8, 14, 2, 1
    db 0, 10, 14, 2, 5
    db 0FFh

CloudShape:
    db 4, 6, 8, 5, 15
    db 2, 8, 12, 5, 15
    db 0, 11, 16, 3, 15
    db 2, 14, 12, 1, 11
    db 0FFh

MarioStandShape:
    db 5, 0, 6, 3, 2
    db 3, 3, 10, 2, 2
    db 5, 5, 6, 3, 14
    db 10, 6, 3, 2, 14
    db 4, 8, 8, 3, 2
    db 5, 8, 6, 6, 15
    db 3, 13, 4, 2, 15
    db 9, 13, 4, 2, 15
    db 2, 15, 5, 1, 6
    db 9, 15, 5, 1, 6
    db 0FFh

MarioRunFrameA:
    db 5, 0, 6, 3, 2
    db 3, 3, 10, 2, 2
    db 5, 5, 6, 3, 14
    db 10, 6, 3, 2, 14
    db 4, 8, 8, 3, 2
    db 5, 8, 6, 6, 15
    db 2, 13, 5, 2, 15
    db 10, 13, 4, 2, 15
    db 1, 15, 6, 1, 6
    db 10, 15, 5, 1, 6
    db 0FFh

MarioRunFrameB:
    db 5, 0, 6, 3, 2
    db 3, 3, 10, 2, 2
    db 5, 5, 6, 3, 14
    db 10, 6, 3, 2, 14
    db 4, 8, 8, 3, 2
    db 5, 8, 6, 6, 15
    db 4, 13, 3, 2, 15
    db 9, 13, 4, 2, 15
    db 2, 15, 5, 1, 6
    db 9, 15, 6, 1, 6
    db 0FFh

MarioJumpShape:
    db 5, 0, 6, 3, 2
    db 3, 3, 10, 2, 2
    db 5, 5, 6, 3, 14
    db 10, 6, 3, 2, 14
    db 4, 8, 8, 3, 2
    db 5, 8, 6, 6, 15
    db 1, 12, 5, 2, 15
    db 10, 12, 5, 2, 15
    db 0FFh

SuperMarioTopShape:
    db 5, 0, 6, 3, 2
    db 3, 3, 10, 2, 2
    db 5, 5, 6, 3, 14
    db 10, 6, 3, 2, 14
    db 4, 8, 8, 3, 2
    db 5, 8, 6, 6, 15
    db 0FFh

SuperMarioBottomShape:
    db 5, 0, 6, 10, 15
    db 3, 10, 5, 4, 15
    db 10, 10, 4, 4, 15
    db 1, 14, 7, 2, 6
    db 9, 14, 6, 2, 6
    db 0FFh

GoombaShape:
    db 4, 1, 8, 3, 6
    db 2, 4, 12, 6, 6
    db 1, 7, 14, 4, 6
    db 4, 8, 2, 3, 15
    db 10, 8, 2, 3, 15
    db 4, 9, 1, 2, 8
    db 11, 9, 1, 2, 8
    db 2, 12, 5, 3, 6
    db 9, 12, 5, 3, 6
    db 0FFh

GoombaFlatShape:
    db 1, 8, 14, 5, 6
    db 3, 7, 3, 2, 15
    db 10, 7, 3, 2, 15
    db 0FFh

KoopaShape:
    db 6, 0, 6, 3, 10
    db 4, 3, 9, 7, 2
    db 5, 4, 6, 5, 10
    db 2, 2, 3, 4, 14
    db 12, 2, 2, 4, 14
    db 4, 10, 3, 4, 14
    db 10, 10, 3, 4, 14
    db 2, 14, 5, 2, 6
    db 10, 14, 5, 2, 6
    db 0FFh

ShellShape:
    db 3, 5, 10, 8, 10
    db 5, 3, 6, 2, 10
    db 5, 7, 6, 2, 2
    db 0FFh

MushroomShape:
    db 4, 2, 8, 5, 4
    db 2, 5, 12, 4, 4
    db 6, 9, 4, 6, 14
    db 4, 3, 2, 2, 15
    db 10, 3, 2, 2, 15
    db 0FFh

CoinFrameA:
    db 6, 0, 4, 16, 14
    db 7, 2, 2, 12, 15
    db 0FFh

CoinFrameB:
    db 4, 1, 8, 14, 14
    db 6, 3, 4, 10, 15
    db 0FFh

; Decorative elder: no collision, AI or gameplay state.
ElderShape:
    db 3, 0, 10, 3, 15
    db 4, 3, 8, 6, 14
    db 5, 5, 1, 1, 0
    db 10, 5, 1, 1, 0
    db 3, 8, 10, 5, 15
    db 5, 13, 6, 3, 15
    db 2, 13, 12, 15, 2
    db 6, 13, 4, 8, 15
    db 1, 28, 6, 4, 15
    db 9, 28, 6, 4, 15
    db 0FFh