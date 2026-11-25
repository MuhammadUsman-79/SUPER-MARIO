; ----------------------------------------------------------------------------
; CollectMapCoins
; Purpose : collects every coin tile that Mario's body overlaps. Mario's box is
;           shrunk by 2 pixels on each side so a coin must really be touched.
;           For each coin: tile becomes sky, +200 points, coins +1.
; Inputs  : movement_x_int, movement_y_fp (byte reads!), movement_height
; Outputs : working map, score, coin counter
; Changes : no registers
; ----------------------------------------------------------------------------
CollectMapCoins:
    push ax
    push bx
    push cx
    push dx

    xor  ax, ax
    mov  al, [movement_y_fp+1]   ; whole-pixel Mario top (BYTE read)
    mov  bx, ax
    add  ax, 2
    mov  cl, 4
    shr  ax, cl
    mov  [coin_row], ax          ; first tile row touched
    mov  ax, bx
    add  ax, [movement_height]
    sub  ax, 2
    mov  cl, 4
    shr  ax, cl
    mov  [coin_end_row], ax      ; last tile row touched
    mov  ax, [movement_x_int]
    add  ax, 2
    mov  cl, 4
    shr  ax, cl
    mov  [coin_start_col], ax    ; first tile column touched
    mov  [coin_col], ax
    mov  ax, [movement_x_int]
    add  ax, 13
    mov  cl, 4
    shr  ax, cl
    mov  [coin_end_col], ax      ; last tile column touched

.row_loop:
    mov  ax, [coin_row]
    cmp  ax, [coin_end_row]
    ja   .done
.col_loop:
    mov  ax, [coin_col]
    cmp  ax, [coin_end_col]
    ja   .next_row
    mov  cl, 4
    mov  ax, [coin_row]
    shl  ax, cl
    add  ax, 8
    push ax                      ; y (tile centre)
    mov  ax, [coin_col]
    shl  ax, cl
    add  ax, 8
    push ax                      ; x (tile centre)
    call GetTile
    add  sp, 4
    cmp  ax, TILE_COIN
    jne  .next_col
    mov  cl, 4
    mov  ax, TILE_SKY
    push ax
    mov  ax, [coin_row]
    shl  ax, cl
    add  ax, 8
    push ax
    mov  ax, [coin_col]
    shl  ax, cl
    add  ax, 8
    push ax
    call SetTileAt
    add  sp, 6
    mov  ax, 200
    push ax
    call AddScore
    add  sp, 2
    call AddCoin
.next_col:
    inc  word [coin_col]
    jmp  .col_loop
.next_row:
    mov  ax, [coin_start_col]    ; back to the FIRST column for the next row
    mov  [coin_col], ax
    inc  word [coin_row]
    jmp  .row_loop
.done:
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

HandleBlockHit:
    push ax
    push bx
    push dx
    push si

    xor  ax, ax
    mov  al, [movement_y_fp+1]
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 8
    push dx
    push ax
    call GetTile
    add  sp, 4
    mov  [hit_tile], al
    cmp  al, TILE_QUESTION_COIN
    je   .coin_block
    cmp  al, TILE_QUESTION_MUSHROOM
    je   .mushroom_block
    cmp  al, TILE_BRICK
    je   .brick_block
    jmp  near .done
.brick_block:
    cmp  byte [hero_form], 0
    jne  .break_brick
    jmp  near .done
.break_brick:
    mov  ax, TILE_SKY
    jmp  .change_tile
.coin_block:
    mov  ax, TILE_EMPTY_BLOCK
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    push ax
    mov  ax, [movement_x_int]
    add  ax, 8
    push ax
    call SetTileAt
    add  sp, 6
    mov  ax, 200
    push ax
    call AddScore
    add  sp, 2
    call AddCoin
    jmp  .done
.mushroom_block:
    mov  ax, TILE_EMPTY_BLOCK
.change_tile:
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    push ax
    mov  ax, [movement_x_int]
    add  ax, 8
    push ax
    call SetTileAt
    add  sp, 6
    cmp  byte [hit_tile], TILE_BRICK
    je   .brick_points
    mov  ax, [movement_x_int]
    add  ax, 8
    and  ax, 0FFF0h
    mov  [mushroom_x], ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    and  ax, 0FFF0h
    sub  ax, 16
    mov  [mushroom_y], ax
    mov  byte [mushroom_active], 1
    mov  byte [mushroom_dir], 1
    jmp  .done
.brick_points:
    mov  ax, 50
    push ax
    call AddScore
    add  sp, 2
.done:
    pop  si
    pop  dx
    pop  bx
    pop  ax
    ret

AddCoin:
    cmp  byte [coin_count], 99
    jae  .extra
    inc  byte [coin_count]
    cmp  byte [coin_count], 100
    jb   .done
.extra:
    mov  byte [coin_count], 0
    cmp  byte [lives], 99
    jae  .done
    inc  byte [lives]
.done:
    ret

AddScore:
    push bp
    mov  bp, sp
    push ax
    push dx
    mov  ax, [bp+4]
    add  [score_low], ax
    adc  word [score_high], 0
    cmp  word [score_high], 15
    ja   .clamp
    jb   .done
    cmp  word [score_low], 423Fh
    jbe  .done
.clamp:
    mov  word [score_high], 15
    mov  word [score_low], 423Fh
.done:
    pop  dx
    pop  ax
    pop  bp
    ret

CheckFlagContact:
    push ax
    push bx
    push dx
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    mov  bx, [movement_height]
    shr  bx, 1
    add  ax, bx
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 8
    push dx
    push ax
    call GetTile
    add  sp, 4
    cmp  ax, TILE_FLAG
    jne  .done
    mov  byte [game_state], STATE_WIN_SEQUENCE
    mov  byte [win_frames], 0
    mov  ax, [movement_x_int]
    add  ax, 8
    and  ax, 0FFF0h
    sub  ax, 8
    mov  [win_target_x], ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    shr  ax, 1
    shr  ax, 1
    shr  ax, 1
    shr  ax, 1
    cmp  ax, 3
    jbe  .top_score
    cmp  ax, 4
    jbe  .score_2000
    cmp  ax, 5
    jbe  .score_800
    cmp  ax, 7
    jbe  .score_400
    mov  ax, 100
    jmp  .award
.top_score:
    mov  ax, 5000
    jmp  .award
.score_2000:
    mov  ax, 2000
    jmp  .award
.score_800:
    mov  ax, 800
    jmp  .award
.score_400:
    mov  ax, 400
.award:
    push ax
    call AddScore
    add  sp, 2
.done:
    pop  dx
    pop  bx
    pop  ax
    ret

AddLife:
    cmp  byte [lives], 99
    jae  .done
    inc  byte [lives]
.done:
    ret

RenderGameActors:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov  si, enemy_table
    mov  cx, ENEMY_COUNT
.loop:
    cmp  byte [si+E_ACTIVE], 0
    je   .next
    mov  ax, [si+E_X]
    sub  ax, [camera_x]
    cmp  ax, 320
    jae  .next
    cmp  ax, 0FFF0h
    ja   .next
    cmp  byte [si+E_STATE], E_FLAT
    je   .flat
    cmp  byte [si+E_STATE], E_SHELL
    je   .shell
    cmp  byte [si+E_STATE], E_SLIDE
    je   .shell
    cmp  byte [si+E_TYPE], E_KOOPA
    je   .koopa
    mov  ax, GoombaShape
    jmp  .draw_enemy
.koopa:
    mov  ax, KoopaShape
    jmp  .draw_enemy
.flat:
    mov  ax, GoombaFlatShape
    jmp  .draw_enemy
.shell:
    mov  ax, ShellShape
.draw_enemy:
    mov  bx, ax
    xor  ax, ax
    mov  al, [si+E_DIR]
    push ax
    mov  ax, bx
    push ax
    xor  ax, ax
    mov  al, [si+E_Y]
    push ax
    mov  ax, [si+E_X]
    sub  ax, [camera_x]
    push ax
    call DrawSprite
    add  sp, 8
.next:
    add  si, ENEMY_SIZE
    loop .loop
    cmp  byte [mushroom_active], 0
    je   .done
    xor  ax, ax
    push ax
    mov  ax, MushroomShape
    push ax
    mov  ax, [mushroom_y]
    push ax
    mov  ax, [mushroom_x]
    sub  ax, [camera_x]
    push ax
    call DrawSprite
    add  sp, 8
.done:
    pop  di
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

DrawMario:
    cmp  byte [invuln_ticks], 0
    je   .visible
    test byte [movement_frame], 1
    jnz  .done
.visible:
    cmp  byte [hero_form], 0
    je   .small
    xor  ax, ax
    mov  al, [movement_facing_left]
    push ax
    mov  ax, SuperMarioBottomShape
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, 16
    push ax
    mov  ax, [movement_x_int]
    sub  ax, [camera_x]
    push ax
    call DrawSprite
    add  sp, 8
    xor  ax, ax
    mov  al, [movement_facing_left]
    push ax
    mov  ax, SuperMarioTopShape
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    push ax
    mov  ax, [movement_x_int]
    sub  ax, [camera_x]
    push ax
    call DrawSprite
    add  sp, 8
    ret
.small:
    call RenderMovement
.done:
    ret

DrawHUD:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    FILL_RECT 0, 0, 320, 16, 0
    mov  byte [screen_row], 0
    mov  byte [screen_col], 0
    mov  byte [screen_color], 15
    mov  si, hud_score_text
    call HUDPrint
    call BuildScoreString
    mov  byte [screen_row], 0
    mov  byte [screen_col], 6
    mov  byte [screen_color], 14
    mov  si, number_buffer
    call HUDPrint
    mov  byte [screen_row], 0
    mov  byte [screen_col], 13
    mov  byte [screen_color], 15
    mov  si, hud_coin_text
    call HUDPrint
    call BuildCoinString
    mov  byte [screen_row], 0
    mov  byte [screen_col], 19
    mov  byte [screen_color], 14
    mov  si, small_number_buffer
    call HUDPrint
    mov  byte [screen_row], 0
    mov  byte [screen_col], 22
    mov  byte [screen_color], 15
    mov  si, hud_world_text
    call HUDPrint
    mov  byte [screen_row], 0
    mov  byte [screen_col], 32
    mov  byte [screen_color], 15
    mov  si, hud_time_text
    call HUDPrint
    mov  ax, [remaining_seconds]
    mov  di, small_number_buffer
    mov  bx, 3
    call NumberToFixedString
    mov  byte [screen_row], 0
    mov  byte [screen_col], 37
    mov  byte [screen_color], 14
    mov  si, small_number_buffer
    call HUDPrint
    mov  byte [screen_row], 1
    mov  byte [screen_col], 0
    mov  byte [screen_color], 15
    mov  si, hud_lives_text
    call HUDPrint
    xor  ax, ax
    mov  al, [lives]
    mov  di, small_number_buffer
    mov  bx, 2
    call NumberToFixedString
    mov  byte [screen_row], 1
    mov  byte [screen_col], 7
    mov  byte [screen_color], 14
    mov  si, small_number_buffer
    call HUDPrint
    mov  byte [screen_row], 1
    mov  byte [screen_col], 12
    mov  byte [screen_color], 15
    mov  si, hud_health_text
    call HUDPrint
    xor  ax, ax
    mov  al, [hero_form]
    inc  ax
    mov  di, small_number_buffer
    mov  bx, 1
    call NumberToFixedString
    mov  byte [screen_row], 1
    mov  byte [screen_col], 17
    mov  byte [screen_color], 12
    mov  si, small_number_buffer
    call HUDPrint
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

DrawScoreLine:
    mov  byte [screen_row], 11
    mov  byte [screen_col], 9
    mov  byte [screen_color], 15
    mov  si, hud_score_text
    call ScreenPrint
    call BuildScoreString
    mov  byte [screen_row], 11
    mov  byte [screen_col], 15
    mov  byte [screen_color], 14
    mov  si, number_buffer
    call ScreenPrint
    ret

BuildScoreString:
    push word [score_high]
    push word [score_low]
    call BuildScoreFrom
    add  sp, 4
    ret

; BuildScoreFrom(low, high): 32-bit value -> 6 ASCII digits in number_buffer
;   Inputs [bp+4]=low word, [bp+6]=high word.   Changes: no registers
BuildScoreFrom:
    push bp
    mov  bp, sp
    push ax
    push bx
    push cx
    push dx
    push di
    mov  ax, [bp+4]
    mov  [score_work_low], ax
    mov  ax, [bp+6]
    mov  [score_work_hi], ax
    mov  di, number_buffer+5
    mov  byte [number_buffer+6], 0
    mov  cx, 6
.loop:
    mov  ax, [score_work_hi]
    xor  dx, dx
    mov  bx, 10
    div  bx
    mov  [score_work_hi], ax
    mov  ax, [score_work_low]
    div  bx
    mov  [score_work_low], ax
    add  dl, '0'
    mov  [di], dl
    dec  di
    loop .loop
    pop  di
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    pop  bp
    ret

BuildCoinString:
    xor  ax, ax
    mov  al, [coin_count]
    mov  di, small_number_buffer
    mov  bx, 2
    call NumberToFixedString
    ret

NumberToFixedString:
    push ax
    push bx
    push cx
    push dx
    push di
    mov  cx, bx
    add  di, bx
    mov  byte [di], 0
    dec  di
.loop:
    xor  dx, dx
    mov  bx, 10
    div  bx
    add  dl, '0'
    mov  [di], dl
    dec  di
    loop .loop
    pop  di
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

ScreenPrint:
    push ax
    push bx
    push dx
    push si
    mov  ah, 02h
    xor  bh, bh
    mov  dh, [screen_row]
    mov  dl, [screen_col]
    int  10h
.next:
    lodsb
    test al, al
    jz   .done
    mov  ah, 0Eh
    xor  bh, bh
    mov  bl, [screen_color]
    int  10h
    jmp  .next
.done:
    pop  si
    pop  dx
    pop  bx
    pop  ax
    ret

ClearScreen:
    push ax
    mov  ax, 1
    push ax
    call ClearFrame
    add  sp, 2
    call CopyFrame
    pop  ax
    ret

DrawDeathScene:
    mov  word [screen_key], 0FFFFh
    mov  ax, 1
    push ax
    call ClearFrame
    add  sp, 2
    call DrawMario
    call CopyFrame
    mov  byte [screen_row], 10
    mov  byte [screen_col], 13
    mov  byte [screen_color], 12
    mov  si, dying_text
    call ScreenPrint
    ret

; ============================================================================
; STATIC SCREENS  (each is drawn into the frame buffer, then copied once)
; ============================================================================

; ----------------------------------------------------------------------------
; DrawTitleScreen
; Purpose : brick frame, sky, logo "GREEN MARIO / E N G I N E", START/EXIT
;           menu box (yellow = selected, with a ">" marker), grass + dirt
;           strip with Mario, a Goomba and the team line.
; Inputs  : menu_choice (0 = START, 1 = EXIT)    Outputs: video memory
; Changes : no registers
; ----------------------------------------------------------------------------
DrawTitleScreen:
    push ax
    push bx
    push cx

    CLEAR_FRAME 2

    xor  bx, bx                  ; top and bottom brick rows (20 tiles each)
.brick_rows:
    mov  cl, 4
    mov  ax, TILE_BRICK
    push ax
    xor  ax, ax
    push ax
    mov  ax, bx
    shl  ax, cl
    push ax
    call DrawTile
    add  sp, 6
    mov  ax, TILE_BRICK
    push ax
    mov  ax, 176
    push ax
    mov  ax, bx
    shl  ax, cl
    push ax
    call DrawTile
    add  sp, 6
    inc  bx
    cmp  bx, 20
    jb   .brick_rows

    mov  bx, 1                   ; left and right brick columns (rows 1..10)
.brick_columns:
    mov  cl, 4
    mov  ax, TILE_BRICK
    push ax
    mov  ax, bx
    shl  ax, cl
    push ax
    xor  ax, ax
    push ax
    call DrawTile
    add  sp, 6
    mov  ax, TILE_BRICK
    push ax
    mov  ax, bx
    shl  ax, cl
    push ax
    mov  ax, 304
    push ax
    call DrawTile
    add  sp, 6
    inc  bx
    cmp  bx, 11
    jb   .brick_columns

    FILL_RECT 16, 160, 288, 4, 10        ; grass
    FILL_RECT 16, 164, 288, 12, 6        ; dirt

    xor  ax, ax                          ; Mario on the left, Goomba on the right
    push ax
    mov  ax, MarioStandShape
    push ax
    mov  ax, 144
    push ax
    mov  ax, 40
    push ax
    call DrawSprite
    add  sp, 8
    xor  ax, ax
    push ax
    mov  ax, GoombaShape
    push ax
    mov  ax, 144
    push ax
    mov  ax, 250
    push ax
    call DrawSprite
    add  sp, 8

    OUTLINE_TEXT 72, 42, 2, 15, title_logo_text
    mov  word [text_spacing], 8          ; wide letter spacing for "E N G I N E"
    CENTER_TEXT 72, 1, 15, title_engine_text
    mov  word [text_spacing], 0

    FILL_RECT 96, 100, 128, 52, 10       ; menu box: white frame, black inside
    FILL_RECT 98, 102, 124, 48, 0
    cmp  byte [menu_choice], 0
    jne  .exit_selected
    DRAW_TEXT 108, 112, 1, 14, arrow_text
    DRAW_TEXT 124, 112, 1, 14, start_game_text
    DRAW_TEXT 124, 132, 1, 15, exit_game_text
    jmp  .menu_done
.exit_selected:
    DRAW_TEXT 124, 112, 1, 15, start_game_text
    DRAW_TEXT 108, 132, 1, 14, arrow_text
    DRAW_TEXT 124, 132, 1, 14, exit_game_text
.menu_done:
    CENTER_TEXT 166, 1, 15, team_text

    call CopyFrame
    pop  cx
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; DrawLifeLostScreen
; Purpose : black screen: "WORLD 1-1", Mario icon (2x) with "x lives",
;           and "LIVES LEFT" in yellow.
; Inputs  : lives        Outputs: video memory        Changes: no registers
; ----------------------------------------------------------------------------
DrawLifeLostScreen:
    push ax
    push bx

    CLEAR_FRAME 0
    CENTER_TEXT 36, 2, 15, world_text

    mov  ax, 2                           ; Mario icon, scaled x2
    push ax
    mov  ax, MarioStandShape
    push ax
    mov  ax, 80
    push ax
    mov  ax, 60
    push ax
    call DrawSpriteScaled
    add  sp, 8

    mov  ax, line_buffer                 ; "x  N"
    APPEND_TEXT times_text
    xor  bx, bx
    mov  bl, [lives]
    APPEND_NUM
    DRAW_TEXT 160, 88, 2, 15, line_buffer

    CENTER_TEXT 148, 2, 14, lives_left_text

    call CopyFrame
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; DrawLevelCompleteScreen
; Purpose : navy screen with a yellow frame listing SCORE (before bonus),
;           COINS, TIME LEFT x 50, TIME BONUS and FINAL SCORE.
; Inputs  : win_score_low/high, win_bonus, remaining_seconds, coin_count,
;           score_low/high                 Outputs: video memory
; Changes : no registers
; ----------------------------------------------------------------------------
DrawLevelCompleteScreen:
    push ax
    push bx

    CLEAR_FRAME 2
    FILL_RECT 16, 16, 288, 168, 14
    FILL_RECT 18, 18, 284, 164, 2
    CENTER_TEXT 36, 2, 15, level_complete_text

    DRAW_TEXT 56, 70, 1, 15, lbl_score
    push word [win_score_high]
    push word [win_score_low]
    call BuildScoreFrom
    add  sp, 4
    DRAW_TEXT 160, 70, 1, 15, number_buffer

    DRAW_TEXT 56, 88, 1, 15, lbl_coins
    mov  ax, line_buffer
    APPEND_TEXT times_one_text
    xor  bx, bx
    mov  bl, [coin_count]
    APPEND_NUM
    DRAW_TEXT 160, 88, 1, 15, line_buffer

    DRAW_TEXT 56, 106, 1, 15, lbl_time_left
    mov  ax, line_buffer                 ; "187  x 50 = 9350"
    mov  bx, [remaining_seconds]
    APPEND_NUM
    APPEND_TEXT times_fifty_text
    mov  bx, [win_bonus]
    APPEND_NUM
    DRAW_TEXT 160, 106, 1, 15, line_buffer

    DRAW_TEXT 56, 124, 1, 15, lbl_time_bonus
    mov  ax, line_buffer                 ; "+ 9350"
    APPEND_TEXT plus_text
    mov  bx, [win_bonus]
    APPEND_NUM
    DRAW_TEXT 160, 124, 1, 15, line_buffer

    DRAW_TEXT 56, 142, 1, 15, lbl_final_score
    call BuildScoreString
    DRAW_TEXT 160, 142, 1, 10, number_buffer

    CENTER_TEXT 168, 1, 10, press_enter_continue

    call CopyFrame
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; DrawGameOverScreen
; Purpose : black screen with a red frame: spaced "G A M E  O V E R",
;           final score, coins and time, and the ENTER hint.
; Inputs  : score_low/high, coin_count, remaining_seconds
; Outputs : video memory        Changes: no registers
; ----------------------------------------------------------------------------
DrawGameOverScreen:
    push ax
    push bx
    push di

    CLEAR_FRAME 0
    FILL_RECT 30, 26, 260, 150, REDORANGE
    FILL_RECT 32, 28, 256, 146, 0

    mov  word [text_spacing], 8
    CENTER_TEXT 62, 2, REDORANGE, game_over_text
    mov  word [text_spacing], 0

    call BuildScoreString
    mov  ax, line_buffer                 ; "Total Points: 001250"
    APPEND_TEXT final_score_text
    APPEND_TEXT number_buffer
    CENTER_TEXT 99, 1, 15, line_buffer

    mov  ax, line_buffer                 ; "Coins: 12     Time: 000"
    APPEND_TEXT coins_label_text
    xor  bx, bx
    mov  bl, [coin_count]
    APPEND_NUM
    APPEND_TEXT time_label_text
    push ax                              ; keep the destination pointer
    mov  ax, [remaining_seconds]
    mov  di, small_number_buffer
    mov  bx, 3                           ; time always shown as 3 digits (000)
    call NumberToFixedString
    pop  ax
    APPEND_TEXT small_number_buffer
    CENTER_TEXT 119, 1, 14, line_buffer

    CENTER_TEXT 149, 1, 10, press_enter_title

    call CopyFrame
    pop  di
    pop  bx
    pop  ax
    ret

GameExitStep:
    ret

GameStateTable:
    dw TitleStep
    dw PlayingStep
    dw PausedStep
    dw DyingStep
    dw LifeLostStep
    dw WinSequenceStep
    dw LevelCompleteStep
    dw GameOverStep
    dw GameExitStep

screen_row db 0
screen_col db 0
screen_color db 15
game_state db STATE_TITLE
menu_choice db 0
menu_latch db 0
enter_latch db 0
pause_latch db 0
escape_latch db 0
hero_form db 0
invuln_ticks db 0
stomp_chain db 0
lives db 3
coin_count db 0
score_low dw 0
score_high dw 0
previous_feet_y dw 0
death_frames db 0
life_lost_frames db 0
win_frames db 0
win_target_x dw 0
mushroom_active db 0
mushroom_dir db 1
mushroom_x dw 0
mushroom_y dw 0
movement_height dw 16
coin_row dw 0
coin_end_row dw 0
coin_col dw 0
coin_end_col dw 0
coin_start_col dw 0
mushroom_fall dw 0
win_bonus dw 0
win_score_low dw 0
win_score_high dw 0
hit_tile db 0
enemy_table times ENEMY_COUNT*ENEMY_SIZE db 0
score_work_hi dw 0
number_buffer times 7 db 0
small_number_buffer times 4 db 0
score_work_low dw 0
stomp_scores dw 100,200,400,500,800,1000,2000,4000,5000,8000

paused_text db 'PAUSED - P TO CONTINUE',0
dying_text db 'TRY AGAIN!',0
hud_score_text db 'POINT:',0
hud_coin_text db 'GOLD:',0
hud_world_text db 'STAGE:1-1',0
hud_time_text db 'LEFT:',0
hud_lives_text db 3,' x',0
hud_health_text db 'FORM:',0

; ---- text for the new screens ------------------------------------------------
title_logo_text      db 'GREEN MARIO',0
title_engine_text    db 'ADVENTURE',0
start_game_text      db 'PLAY',0
exit_game_text       db 'QUIT',0
arrow_text           db '>',0
team_text            db 'Usman and Talha',0    ; <-- put your own group / names here
world_text           db 'WORLD 1-1',0
times_text           db 'x  ',0
lives_left_text      db 'LIVES LEFT',0
level_complete_text  db 'STAGE CLEARED!',0
lbl_score            db 'SCORE',0
lbl_coins            db 'COINS',0
lbl_time_left        db 'TIME LEFT',0
lbl_time_bonus       db 'TIME BONUS',0
lbl_final_score      db 'FINAL SCORE',0
times_one_text       db 'x ',0
times_fifty_text     db '  x 50 = ',0
plus_text            db '+ ',0
press_enter_continue db 'Press ENTER to continue',0
game_over_text       db 'RUN ENDED',0
final_score_text     db 'Total Points: ',0
coins_label_text     db 'Coins: ',0
time_label_text      db '     Time: ',0
press_enter_title    db 'Press ENTER for the title',0

; Same 8x8 HUD coordinates; draw into the frame before presenting it.
HUDPrint:
    push ax
    push bx
    push cx
    push si
    xor ax, ax
    mov al, [screen_color]
    push si
    push ax
    mov ax, 1
    push ax
    xor ax, ax
    mov al, [screen_row]
    mov cl, 3
    shl ax, cl
    push ax
    xor ax, ax
    mov al, [screen_col]
    shl ax, cl
    push ax
    call DrawText
    add sp, 10
    pop si
    pop cx
    pop bx
    pop ax
    ret
