STATE_TITLE          equ 0
STATE_PLAYING        equ 1
STATE_PAUSED         equ 2
STATE_DYING           equ 3
STATE_LIFE_LOST       equ 4
STATE_WIN_SEQUENCE    equ 5
STATE_LEVEL_COMPLETE  equ 6
STATE_GAME_OVER       equ 7
STATE_EXIT            equ 8

ENEMY_COUNT equ 6
ENEMY_SIZE  equ 10
E_TYPE      equ 0
E_STATE     equ 1
E_X         equ 2
E_Y         equ 4
E_DIR       equ 6
E_TIMER     equ 7
E_ACTIVE    equ 8
E_PHASE     equ 9
E_GOOMBA    equ 1
E_KOOPA     equ 2
E_WALK      equ 1
E_FLAT      equ 2
E_SHELL     equ 3
E_SLIDE     equ 4

GameStep:
    xor  ax, ax
    mov  al, [game_state]
    cmp  al, STATE_EXIT
    ja   .invalid
    shl  ax, 1
    mov  bx, ax
    jmp  word [GameStateTable+bx]
.invalid:
    ret

TitleStep:
    cmp  byte [key_states+48h], 0
    je   .check_down
    cmp  byte [menu_latch], 0
    jne  .check_enter
    xor  byte [menu_choice], 1
    mov  byte [menu_latch], 1
    jmp  .check_enter
.check_down:
    cmp  byte [key_states+50h], 0
    je   .clear_menu_latch
    cmp  byte [menu_latch], 0
    jne  .check_enter
    xor  byte [menu_choice], 1
    mov  byte [menu_latch], 1
    jmp  .check_enter
.clear_menu_latch:
    mov  byte [menu_latch], 0
.check_enter:
    cmp  byte [key_states+1Ch], 0
    je   .check_escape
    cmp  byte [enter_latch], 0
    jne  .draw
    mov  byte [enter_latch], 1
    cmp  byte [menu_choice], 0
    jne  .exit
    call NewGame
    mov  byte [game_state], STATE_PLAYING
    jmp  .draw
.check_escape:
    mov  byte [enter_latch], 0
    cmp  byte [key_states+01h], 0
    je   .clear_escape
    cmp  byte [escape_latch], 0
    jne  .draw
    mov  byte [game_state], STATE_EXIT
    jmp  .draw
.clear_escape:
    mov  byte [escape_latch], 0
    jmp  .draw
.exit:
    mov  byte [game_state], STATE_EXIT
.draw:
    mov  ah, STATE_TITLE         ; redraw only when the picture would change
    mov  al, [menu_choice]
    cmp  ax, [screen_key]
    je   .done
    mov  [screen_key], ax
    call DrawTitleScreen
.done:
    ret

PlayingStep:
    cmp  byte [key_states+19h], 0
    je   .pause_released
    cmp  byte [pause_latch], 0
    jne  .check_escape
    mov  byte [pause_latch], 1
    mov  byte [game_state], STATE_PAUSED
    call RenderFrame
    mov  byte [screen_row], 11
    mov  byte [screen_col], 7
    mov  byte [screen_color], 14
    mov  si, paused_text
    call ScreenPrint
    ret
.pause_released:
    mov  byte [pause_latch], 0
.check_escape:
    cmp  byte [key_states+01h], 0
    je   .update
    mov  byte [game_state], STATE_TITLE
    mov  byte [menu_choice], 0
    mov  byte [escape_latch], 1
    ret
.update:
    xor  ax, ax
    mov  al, [movement_y_fp+1]   ; (a WORD read here would also pick up vx!)
    add  ax, [movement_height]
    mov  [previous_feet_y], ax
    call UpdateMovement
    cmp  byte [game_state], STATE_DYING
    je   .draw_dying
    call UpdateEnemies
    call UpdateMushroom
    call ResolveEntityContacts
    call CollectMapCoins
    cmp  byte [game_state], STATE_DYING
    je   .draw_dying
    cmp  word [remaining_seconds], 0
    jne  .check_flag
    call BeginDeath
    jmp  .draw_dying
.check_flag:
    call CheckFlagContact
    cmp  byte [invuln_ticks], 0
    je   .draw
    dec  byte [invuln_ticks]
.draw:
    call RenderFrame
    ret
.draw_dying:
    call DrawDeathScene
    ret

PausedStep:
    cmp  byte [key_states+19h], 0
    je   .pause_release
    cmp  byte [pause_latch], 0
    jne  .show
    mov  byte [pause_latch], 1
    mov  byte [game_state], STATE_PLAYING
    ret
.pause_release:
    mov  byte [pause_latch], 0
.show:
    ret

DyingStep:
    inc  byte [death_frames]
    cmp  byte [death_frames], 8
    ja   .fall
    sub  word [movement_y_fp], 512
    jmp  .draw
.fall:
    cmp  byte [movement_y_fp+1], 208 ; already below the screen: stop (avoids 8-bit wrap)
    jae  .draw
    add  word [movement_y_fp], 1024
.draw:
    call DrawDeathScene
    cmp  byte [death_frames], 27
    jb   .done
    cmp  byte [lives], 0
    je   .game_over
    dec  byte [lives]
    cmp  byte [lives], 0
    je   .game_over
    mov  byte [life_lost_frames], 0
    mov  byte [game_state], STATE_LIFE_LOST
.done:
    ret
.game_over:
    mov  byte [game_state], STATE_GAME_OVER
    ret

LifeLostStep:
    mov  ah, STATE_LIFE_LOST     ; draw the picture once, then just count frames
    xor  al, al
    cmp  ax, [screen_key]
    je   .count
    mov  [screen_key], ax
    call DrawLifeLostScreen
.count:
    inc  byte [life_lost_frames]
    cmp  byte [life_lost_frames], 36
    jb   .done
    call ResetLevel
    mov  byte [game_state], STATE_PLAYING
.done:
    ret

WinSequenceStep:
    inc  byte [win_frames]
    mov  ax, [movement_x_int]
    cmp  ax, [win_target_x]
    je   .at_flag
    ja   .slide_left
    inc  word [movement_x_int]
    mov  byte [movement_facing_left], 0
    mov  word [movement_vx], 0100h
    jmp  .render
.slide_left:
    dec  word [movement_x_int]
    mov  byte [movement_facing_left], 1
    mov  word [movement_vx], 0FF00h
    jmp  .render
.at_flag:
    mov  word [movement_vx], 0
.render:
    call UpdateCamera
    call RenderFrame
    cmp  byte [win_frames], 36
    jb   .done
    mov  ax, [remaining_seconds]
    mov  bx, 50
    mul  bx
    mov  [win_bonus], ax         ; remember bonus and the score BEFORE it,
    mov  bx, [score_low]         ; so the Level Complete screen can show both
    mov  [win_score_low], bx
    mov  bx, [score_high]
    mov  [win_score_high], bx
    push ax
    call AddScore
    add  sp, 2
    mov  byte [game_state], STATE_LEVEL_COMPLETE
.done:
    ret

LevelCompleteStep:
    mov  ah, STATE_LEVEL_COMPLETE
    xor  al, al
    cmp  ax, [screen_key]
    je   .input
    mov  [screen_key], ax
    call DrawLevelCompleteScreen
.input:
    cmp  byte [key_states+1Ch], 0
    jne  .press
    mov  byte [enter_latch], 0
    ret
.press:
    cmp  byte [enter_latch], 0
    jne  .done
    mov  byte [enter_latch], 1
    call NewGame
.done:
    ret

GameOverStep:
    mov  ah, STATE_GAME_OVER
    xor  al, al
    cmp  ax, [screen_key]
    je   .input
    mov  [screen_key], ax
    call DrawGameOverScreen
.input:
    cmp  byte [key_states+1Ch], 0
    jne  .press
    mov  byte [enter_latch], 0
    ret
.press:
    cmp  byte [enter_latch], 0
    jne  .done
    mov  byte [enter_latch], 1
    call NewGame
.done:
    ret

ResetLevel:
    call ResetMap
    call ResetMovement
    call ResetEnemies
    mov  byte [mushroom_active], 0
    mov  byte [hero_form], 0
    mov  word [movement_height], 16
    mov  byte [invuln_ticks], 0
    mov  byte [stomp_chain], 0
    call ResetGameTimer
    ret

NewGame:
    mov  word [score_low], 0
    mov  word [score_high], 0
    mov  byte [coin_count], 0
    mov  byte [lives], 3
    mov  byte [hero_form], 0
    mov  word [movement_height], 16
    mov  byte [menu_choice], 0
    mov  byte [menu_latch], 0
    mov  byte [pause_latch], 0
    mov  byte [escape_latch], 0
    mov  byte [game_state], STATE_TITLE
    call ResetLevel
    ret

ResetEnemies:
    mov  byte [enemy_table+0*ENEMY_SIZE+E_TYPE], E_GOOMBA
    mov  byte [enemy_table+0*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+0*ENEMY_SIZE+E_X], 160
    mov  word [enemy_table+0*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+0*ENEMY_SIZE+E_DIR], 1
    mov  byte [enemy_table+0*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+0*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+0*ENEMY_SIZE+E_PHASE], 0

    mov  byte [enemy_table+1*ENEMY_SIZE+E_TYPE], E_GOOMBA
    mov  byte [enemy_table+1*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+1*ENEMY_SIZE+E_X], 280
    mov  word [enemy_table+1*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+1*ENEMY_SIZE+E_DIR], 0
    mov  byte [enemy_table+1*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+1*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+1*ENEMY_SIZE+E_PHASE], 0

    mov  byte [enemy_table+2*ENEMY_SIZE+E_TYPE], E_KOOPA
    mov  byte [enemy_table+2*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+2*ENEMY_SIZE+E_X], 360
    mov  word [enemy_table+2*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+2*ENEMY_SIZE+E_DIR], 0
    mov  byte [enemy_table+2*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+2*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+2*ENEMY_SIZE+E_PHASE], 0

    mov  byte [enemy_table+3*ENEMY_SIZE+E_TYPE], E_GOOMBA
    mov  byte [enemy_table+3*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+3*ENEMY_SIZE+E_X], 520
    mov  word [enemy_table+3*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+3*ENEMY_SIZE+E_DIR], 1
    mov  byte [enemy_table+3*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+3*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+3*ENEMY_SIZE+E_PHASE], 0

    mov  byte [enemy_table+4*ENEMY_SIZE+E_TYPE], E_KOOPA
    mov  byte [enemy_table+4*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+4*ENEMY_SIZE+E_X], 680
    mov  word [enemy_table+4*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+4*ENEMY_SIZE+E_DIR], 0
    mov  byte [enemy_table+4*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+4*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+4*ENEMY_SIZE+E_PHASE], 0

    mov  byte [enemy_table+5*ENEMY_SIZE+E_TYPE], E_GOOMBA
    mov  byte [enemy_table+5*ENEMY_SIZE+E_STATE], E_WALK
    mov  word [enemy_table+5*ENEMY_SIZE+E_X], 840
    mov  word [enemy_table+5*ENEMY_SIZE+E_Y], 144
    mov  byte [enemy_table+5*ENEMY_SIZE+E_DIR], 0
    mov  byte [enemy_table+5*ENEMY_SIZE+E_TIMER], 0
    mov  byte [enemy_table+5*ENEMY_SIZE+E_ACTIVE], 1
    mov  byte [enemy_table+5*ENEMY_SIZE+E_PHASE], 0
    ret

BeginDeath:
    cmp  byte [game_state], STATE_DYING
    je   .done
    mov  byte [game_state], STATE_DYING
    mov  byte [death_frames], 0
    mov  word [movement_vx], 0
    mov  word [movement_vy], 0
    mov  byte [movement_grounded], 0
.done:
    ret

UpdateEnemies:
    push ax
    push bx
    push cx
    push dx
    push si

    mov  si, enemy_table
    mov  cx, ENEMY_COUNT
.loop:
    cmp  byte [si+E_ACTIVE], 0
    jne  .active_entity
    jmp  near .next
.active_entity:
    cmp  byte [si+E_STATE], E_FLAT
    jne  .not_flat
    dec  byte [si+E_TIMER]
    jz   .flat_expired
    jmp  near .next
.flat_expired:
    mov  byte [si+E_ACTIVE], 0
    jmp  .next
.not_flat:
    cmp  byte [si+E_STATE], E_SHELL
    jne  .not_stationary_shell
    jmp  near .next
.not_stationary_shell:
    mov  ax, [si+E_X]
    cmp  ax, [camera_x]
    jae  .not_behind
    mov  bx, [camera_x]
    sub  bx, ax
    cmp  bx, 32
    jbe  .not_behind
    jmp  near .remove
.not_behind:
    mov  ax, [si+E_X]
    sub  ax, [camera_x]
    cmp  ax, 336
    jbe  .within_view
    jmp  near .next
.within_view:
    cmp  byte [si+E_STATE], E_SLIDE
    jne  .walk
    cmp  byte [si+E_DIR], 0
    jne  .slide_right
    sub  word [si+E_X], 5
    jmp  .slide_wall
.slide_right:
    add  word [si+E_X], 5
.slide_wall:
    cmp  byte [si+E_TIMER], 0
    je   .slide_move_check
    dec  byte [si+E_TIMER]
.slide_move_check:
    mov  ax, [si+E_X]
    add  ax, 8
    xor  dx, dx
    mov  dl, [si+E_Y]
    add  dx, 8
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .shell_blocked
    jmp  near .next
.shell_blocked:
    xor  byte [si+E_DIR], 1
    jmp  near .next
.walk:
    inc  byte [si+E_PHASE]
    test byte [si+E_PHASE], 1
    jz   .walk_step
    jmp  near .next
.walk_step:
    cmp  byte [si+E_DIR], 0
    jne  .walk_right
    sub  word [si+E_X], 1
    jmp  near .walk_check
.walk_right:
    add  word [si+E_X], 1
.walk_check:
    mov  ax, [si+E_X]
    cmp  byte [si+E_DIR], 0
    je   .wall_left
    add  ax, 15
    jmp  .check_wall
.wall_left:
    dec  ax
.check_wall:
    xor  dx, dx
    mov  dl, [si+E_Y]
    add  dx, 8
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .turn
    cmp  byte [si+E_TYPE], E_KOOPA
    jne  .goomba_floor
    mov  ax, [si+E_X]
    cmp  byte [si+E_DIR], 0
    je   .ledge_left
    add  ax, 15
    jmp  .ledge_check
.ledge_left:
    dec  ax
.ledge_check:
    xor  dx, dx
    mov  dl, [si+E_Y]
    add  dx, 17
    push dx
    push ax
    call GetTile
    add  sp, 4
    test ax, ax
    jnz  .next
.turn:
    xor  byte [si+E_DIR], 1
    jmp  .next
.goomba_floor:
    mov  ax, [si+E_X]
    add  ax, 8
    xor  dx, dx
    mov  dl, [si+E_Y]
    add  dx, 17
    push dx
    push ax
    call GetTile
    add  sp, 4
    test ax, ax
    jnz  .next
    add  byte [si+E_Y], 2
    cmp  byte [si+E_Y], 192
    jae  .remove
    jmp  .next
.remove:
    mov  byte [si+E_ACTIVE], 0
.next:
    add  si, ENEMY_SIZE
    dec  cx
    jz   .enemy_updates_done
    jmp  .loop

.enemy_updates_done:
    mov  si, enemy_table
    mov  cx, ENEMY_COUNT
.shell_loop:
    cmp  byte [si+E_ACTIVE], 0
    je   .shell_next
    cmp  byte [si+E_STATE], E_SLIDE
    jne  .shell_next
    push si
    mov  di, enemy_table
    mov  bx, ENEMY_COUNT
.hit_loop:
    cmp  byte [di+E_ACTIVE], 0
    je   .hit_next
    cmp  si, di
    je   .hit_next
    cmp  byte [di+E_STATE], E_FLAT
    je   .hit_next
    mov  ax, [si+E_X]
    add  ax, 16
    cmp  ax, [di+E_X]
    jbe  .hit_next
    mov  ax, [di+E_X]
    add  ax, 16
    cmp  ax, [si+E_X]
    jbe  .hit_next
    mov  ax, [si+E_Y]
    add  ax, 16
    cmp  ax, [di+E_Y]
    jbe  .hit_next
    mov  ax, [di+E_Y]
    add  ax, 16
    cmp  ax, [si+E_Y]
    jbe  .hit_next
    mov  byte [di+E_ACTIVE], 0
    mov  ax, 500
    push ax
    call AddScore
    add  sp, 2
.hit_next:
    add  di, ENEMY_SIZE
    dec  bx
    jnz  .hit_loop
    pop  si
.shell_next:
    add  si, ENEMY_SIZE
    loop .shell_loop

    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; UpdateMushroom
; Purpose : moves the mushroom one frame. On the ground it walks sideways
;           (turning at walls). When it has no floor under it, it drops
;           STRAIGHT DOWN with a little acceleration (no sideways drift), lands
;           exactly on top of the floor tile, then keeps walking.
; Inputs  : mushroom_* variables      Outputs: mushroom_* variables
; Changes : no registers
; ----------------------------------------------------------------------------
UpdateMushroom:
    push ax
    push dx
    cmp  byte [mushroom_active], 0
    jne  .active
    jmp  .done
.active:
    call MushroomOnGround
    test ax, ax
    jnz  .walk

.fall:                           ; ---- falling: vertical movement only ----
    mov  ax, [mushroom_fall]
    cmp  ax, 4
    jae  .fall_speed_ready
    inc  ax
    mov  [mushroom_fall], ax
.fall_speed_ready:
    add  [mushroom_y], ax
    call MushroomOnGround
    test ax, ax
    jz   .check_pit
    mov  ax, [mushroom_y]        ; landed: snap onto the top of the tile
    add  ax, 16
    and  ax, 0FFF0h
    sub  ax, 16
    mov  [mushroom_y], ax
    mov  word [mushroom_fall], 0
    jmp  .done
.check_pit:
    cmp  word [mushroom_y], 192
    jb   .done
    mov  byte [mushroom_active], 0
    jmp  .done

.walk:                           ; ---- on the ground: walk left/right ----
    mov  word [mushroom_fall], 0
    cmp  byte [mushroom_dir], 0
    jne  .right
    mov  ax, [mushroom_x]
    dec  ax
    jmp  .wall_probe
.right:
    mov  ax, [mushroom_x]
    add  ax, 16
.wall_probe:
    mov  dx, [mushroom_y]
    add  dx, 8
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jz   .move
    xor  byte [mushroom_dir], 1  ; wall in front: turn around
    jmp  .left_behind
.move:
    cmp  byte [mushroom_dir], 0
    jne  .move_right
    dec  word [mushroom_x]
    jmp  .left_behind
.move_right:
    inc  word [mushroom_x]
.left_behind:
    mov  ax, [mushroom_x]        ; far to the left of the camera: remove it
    add  ax, 32
    cmp  ax, [camera_x]
    jae  .done
    mov  byte [mushroom_active], 0
.done:
    pop  dx
    pop  ax
    ret

; ----------------------------------------------------------------------------
; MushroomOnGround
; Purpose : tests whether a solid tile lies directly under either foot of the
;           mushroom (left foot x+2, right foot x+13, row y+16).
; Inputs  : mushroom_x, mushroom_y      Outputs: AX = 1 if supported, else 0
; Changes : AX
; ----------------------------------------------------------------------------
MushroomOnGround:
    push dx
    mov  dx, [mushroom_y]
    add  dx, 16
    mov  ax, [mushroom_x]
    add  ax, 2
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .done
    mov  dx, [mushroom_y]
    add  dx, 16
    mov  ax, [mushroom_x]
    add  ax, 13
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
.done:
    pop  dx
    ret

ResolveEntityContacts:
    push ax
    push bx
    push cx
    push dx
    push si

    mov  si, enemy_table
    mov  cx, ENEMY_COUNT
.enemy_loop:
    cmp  byte [si+E_ACTIVE], 0
    jne  .active_contact
    jmp  near .next
.active_contact:
    mov  ax, [movement_x_int]
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    push ax
    mov  ax, 16
    push ax
    mov  ax, [movement_height]
    push ax
    mov  ax, [si+E_X]
    push ax
    xor  ax, ax
    mov  al, [si+E_Y]
    push ax
    mov  ax, 16
    push ax
    push ax
    call RectOverlap
    add  sp, 16
    test ax, ax
    jnz  .overlapping
    jmp  near .next
.overlapping:
    cmp  byte [si+E_STATE], E_FLAT
    jne  .not_flat_contact
    jmp  near .next
.not_flat_contact:
    cmp  byte [si+E_STATE], E_SHELL
    je   .kick_shell
    cmp  byte [si+E_STATE], E_SLIDE
    jne  .check_stomp
    cmp  byte [si+E_TIMER], 0
    je   .check_stomp
    jmp  near .next
.check_stomp:
    cmp  word [movement_vy], 0
    jle  .hurt_or_shell
    mov  ax, [previous_feet_y]
    mov  bx, [si+E_Y]
    add  bx, 8
    cmp  ax, bx
    jae  .hurt_or_shell
    cmp  byte [si+E_STATE], E_SLIDE
    je   .stop_shell
    cmp  byte [si+E_TYPE], E_KOOPA
    je   .make_shell
    mov  byte [si+E_STATE], E_FLAT
    mov  byte [si+E_TIMER], 18
    jmp  .stomp_score
.make_shell:
    mov  byte [si+E_STATE], E_SHELL
.stomp_score:
    call AwardStomp
    mov  word [movement_vy], 0FB00h
    mov  byte [movement_grounded], 0
    jmp  .next
.stop_shell:
    mov  byte [si+E_STATE], E_SHELL
    mov  byte [si+E_TIMER], 0
    call AwardStomp
    mov  word [movement_vy], 0FB00h
    mov  byte [movement_grounded], 0
    jmp  .next
.kick_shell:
    mov  byte [si+E_STATE], E_SLIDE
    mov  byte [si+E_TIMER], 8
    mov  ax, 400
    push ax
    call AddScore
    add  sp, 2
    mov  ax, [movement_x_int]
    cmp  ax, [si+E_X]
    jae  .kick_left
    mov  byte [si+E_DIR], 1
    jmp  .next
.kick_left:
    mov  byte [si+E_DIR], 0
    jmp  .next
.hurt_or_shell:
    cmp  byte [si+E_STATE], E_SLIDE
    jne  .damage
    cmp  byte [si+E_TIMER], 0
    jne  .next
.damage:
    call HurtMario
.next:
    add  si, ENEMY_SIZE
    dec  cx
    jz   .enemy_contacts_done
    jmp  .enemy_loop

.enemy_contacts_done:
    cmp  byte [mushroom_active], 0
    je   .done
    mov  ax, [movement_x_int]
    add  ax, 16
    cmp  ax, [mushroom_x]
    jbe  .done
    mov  ax, [mushroom_x]
    add  ax, 16
    cmp  ax, [movement_x_int]
    jbe  .done
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    cmp  ax, [mushroom_y]
    jbe  .done
    mov  ax, [mushroom_y]
    add  ax, 16
    xor  bx, bx
    mov  bl, [movement_y_fp+1]
    cmp  ax, bx
    jbe  .done
    mov  byte [mushroom_active], 0
    mov  ax, 1000
    push ax
    call AddScore
    add  sp, 2
    cmp  byte [hero_form], 0
    jne  .done
    mov  byte [hero_form], 1
    mov  word [movement_height], 32
    sub  word [movement_y_fp], 4096
.done:
    pop  si
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

RectOverlap:
    push bp
    mov  bp, sp
    push bx
    mov  ax, [bp+18]
    mov  bx, [bp+10]
    add  bx, [bp+6]
    cmp  ax, bx
    jae  .no_overlap
    mov  ax, [bp+18]
    add  ax, [bp+14]
    cmp  ax, [bp+10]
    jbe  .no_overlap
    mov  ax, [bp+16]
    mov  bx, [bp+8]
    add  bx, [bp+4]
    cmp  ax, bx
    jae  .no_overlap
    mov  ax, [bp+16]
    add  ax, [bp+12]
    cmp  ax, [bp+8]
    jbe  .no_overlap
    mov  ax, 1
    jmp  .done
.no_overlap:
    xor  ax, ax
.done:
    pop  bx
    pop  bp
    ret

HurtMario:
    cmp  byte [invuln_ticks], 0
    jne  .done
    cmp  byte [hero_form], 0
    je   .death
    mov  byte [hero_form], 0
    mov  word [movement_height], 16
    add  word [movement_y_fp], 4096
    mov  byte [invuln_ticks], 36
    ret
.death:
    call BeginDeath
.done:
    ret

AwardStomp:
    cmp  byte [stomp_chain], 10
    jae  .extra_life
    inc  byte [stomp_chain]
    xor  bx, bx
    mov  bl, [stomp_chain]
    dec  bx
    shl  bx, 1
    mov  ax, [stomp_scores+bx]
    push ax
    call AddScore
    add  sp, 2
    ret
.extra_life:
    cmp  byte [lives], 99
    jae  .done
    inc  byte [lives]
.done:
    ret

