WALK_ACCEL       equ 0057h
GROUND_FRICTION  equ 002Bh
WALK_MAX_SPEED   equ 0279h
RUN_MAX_SPEED    equ 041Fh
GRAVITY_AMOUNT   equ 0080h
JUMP_START_SPEED equ 0F7CDh
JUMP_CUTOFF      equ 0FD00h
MAX_FALL_SPEED   equ 00500h
LEVEL_WIDTH_PX   equ 960
SCREEN_WIDTH_PX  equ 320
CAMERA_MAX_X     equ 640
CAMERA_FOLLOW_X  equ 160
MOVEMENT_X_MAX   equ 944

UpdateMovement:
    push ax
    push bx
    push cx
    push dx

    mov  al, [key_states+4Bh]
    mov  ah, [key_states+4Dh]
    cmp  al, ah
    je   .apply_friction
    test al, al
    jnz  .move_left

.move_right:
    mov  byte [movement_facing_left], 0
    mov  bx, WALK_MAX_SPEED
    cmp  byte [key_states+2Ah], 0
    jne  .right_run_limit
    cmp  byte [key_states+36h], 0
    je   .right_limit_ready
.right_run_limit:
    mov  bx, RUN_MAX_SPEED
.right_limit_ready:
    add  word [movement_vx], WALK_ACCEL
    mov  ax, [movement_vx]
    test ax, 8000h
    jnz  .integrate_horizontal
    cmp  ax, bx
    jle  .integrate_horizontal
    mov  [movement_vx], bx
    jmp  .integrate_horizontal

.move_left:
    mov  byte [movement_facing_left], 1
    mov  bx, WALK_MAX_SPEED
    cmp  byte [key_states+2Ah], 0
    jne  .left_run_limit
    cmp  byte [key_states+36h], 0
    je   .left_limit_ready
.left_run_limit:
    mov  bx, RUN_MAX_SPEED
.left_limit_ready:
    sub  word [movement_vx], WALK_ACCEL
    mov  ax, [movement_vx]
    test ax, 8000h
    jz   .integrate_horizontal
    neg  bx
    cmp  ax, bx
    jge  .integrate_horizontal
    mov  [movement_vx], bx
    jmp  .integrate_horizontal

.apply_friction:
    mov  ax, [movement_vx]
    test ax, ax
    jz   .integrate_horizontal
    test ax, 8000h
    jnz  .friction_negative
    cmp  ax, GROUND_FRICTION
    jbe  .stop_horizontal
    sub  ax, GROUND_FRICTION
    jmp  .store_horizontal
.friction_negative:
    cmp  ax, -GROUND_FRICTION
    jge  .stop_horizontal
    add  ax, GROUND_FRICTION
    jmp  .store_horizontal
.stop_horizontal:
    xor  ax, ax
.store_horizontal:
    mov  [movement_vx], ax

.integrate_horizontal:
    mov  ax, [movement_vx]
    add  byte [movement_x_frac], al
    mov  al, ah
    cbw
    adc  word [movement_x_int], ax

    cmp  word [movement_x_int], MOVEMENT_X_MAX
    jbe  .horizontal_bounds_ok
    test word [movement_vx], 8000h
    jnz  .horizontal_clamp_left
    mov  word [movement_x_int], MOVEMENT_X_MAX
    mov  byte [movement_x_frac], 0
    mov  word [movement_vx], 0
    jmp  .horizontal_bounds_ok
.horizontal_clamp_left:
    mov  word [movement_x_int], 0
    mov  byte [movement_x_frac], 0
    mov  word [movement_vx], 0
.horizontal_bounds_ok:
    call ResolveHorizontalCollision

    cmp  byte [movement_grounded], 0
    je   .process_jump
    call CheckGroundSupport

.process_jump:
    cmp  byte [key_states+39h], 0
    je   .space_released
    cmp  byte [movement_space_previous], 0
    jne  .space_held
    cmp  byte [movement_grounded], 0
    je   .space_held
    mov  word [movement_vy], JUMP_START_SPEED
    mov  byte [movement_grounded], 0
.space_held:
    mov  byte [movement_space_previous], 1
    jmp  .check_air
.space_released:
    mov  byte [movement_space_previous], 0

.check_air:
    cmp  byte [movement_grounded], 0
    jne  .update_camera
    call UpdateVertical

.update_camera:
    call UpdateCamera

.done:
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

ResolveHorizontalCollision:
    push ax
    push bx
    push cx
    push dx

    mov  ax, [movement_vx]
    test ax, ax
    jnz  .has_velocity
    jmp  .done
.has_velocity:
    test ax, 8000h
    jz   .moving_right
    jmp  .moving_left

.moving_right:
    mov  ax, [movement_x_int]
    add  ax, 15
    mov  dx, ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, 2
    push ax
    push dx
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_right

    mov  ax, [movement_x_int]
    add  ax, 15
    mov  dx, ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    sub  ax, 2
    push ax
    push dx
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .right_lower_hit
    jmp  .done

.right_lower_hit:
.hit_right:
    mov  ax, [movement_x_int]
    add  ax, 15                  
    and  ax, 0FFF0h             
    sub  ax, 16                 
    mov  [movement_x_int], ax
    mov  byte [movement_x_frac], 0
    mov  word [movement_vx], 0
    jmp  .done

.moving_left:
    mov  ax, [movement_x_int]
    mov  dx, ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, 2
    push ax
    push dx
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_left

    mov  ax, [movement_x_int]
    mov  dx, ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    sub  ax, 2
    push ax
    push dx
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .left_lower_hit
    jmp  .done

.left_lower_hit:
.hit_left:
    mov  ax, [movement_x_int]
    and  ax, 0FFF0h
    add  ax, 16
    mov  [movement_x_int], ax
    mov  byte [movement_x_frac], 0
    mov  word [movement_vx], 0

.done:
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

CheckGroundSupport:
    push ax
    push bx
    push cx

    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    mov  bx, ax
    mov  ax, [movement_x_int]
    add  ax, 2
    push bx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .supported

    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    mov  bx, ax
    mov  ax, [movement_x_int]
    add  ax, 13
    push bx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .supported

    mov  byte [movement_grounded], 0
    jmp  .done

.supported:
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    and  ax, 0FFF0h
    sub  ax, [movement_height]
    mov  cl, 8
    shl  ax, cl
    mov  [movement_y_fp], ax
    mov  word [movement_vy], 0
    mov  byte [movement_grounded], 1
    mov  byte [stomp_chain], 0

.done:
    pop  cx
    pop  bx
    pop  ax
    ret

UpdateVertical:
    push ax
    push bx
    push cx
    push dx

    cmp  byte [key_states+39h], 0
    jne  .add_gravity
    mov  ax, [movement_vy]
    test ax, 8000h
    jz   .add_gravity
    cmp  ax, JUMP_CUTOFF
    jge  .add_gravity
    mov  word [movement_vy], JUMP_CUTOFF

.add_gravity:
    add  word [movement_vy], GRAVITY_AMOUNT
    cmp  word [movement_vy], MAX_FALL_SPEED
    jle  .integrate_y
    mov  word [movement_vy], MAX_FALL_SPEED

.integrate_y:
    mov  ax, [movement_y_fp]
    add  ax, [movement_vy]
    mov  [movement_y_fp], ax
    test word [movement_vy], 8000h
    jnz  .resolve_ceiling
    jmp  .resolve_floor

.resolve_ceiling:
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 2
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_ceiling

    xor  ax, ax
    mov  al, [movement_y_fp+1]
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 13
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_ceiling
    jmp  .done

.hit_ceiling:
    call TriggerQuestionBlock
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    and  ax, 0FFF0h
    add  ax, 16
    mov  cl, 8
    shl  ax, cl
    mov  [movement_y_fp], ax
    mov  word [movement_vy], 0
    jmp  .done

.resolve_floor:
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 2
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_floor

    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    mov  dx, ax
    mov  ax, [movement_x_int]
    add  ax, 13
    push dx
    push ax
    call IsSolidAt
    add  sp, 4
    test ax, ax
    jnz  .hit_floor

    cmp  byte [movement_y_fp+1], 192
    jae  .pit_reached
    jmp  .done
.pit_reached:
    call BeginDeath
    jmp  .done

.hit_floor:
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    add  ax, [movement_height]
    and  ax, 0FFF0h
    sub  ax, [movement_height]
    mov  cl, 8
    shl  ax, cl
    mov  [movement_y_fp], ax
    mov  word [movement_vy], 0
    mov  byte [movement_grounded], 1
    mov  byte [stomp_chain], 0

.done:
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

TriggerQuestionBlock:
    call HandleBlockHit
    ret

IsSolidAt:
    push bp
    mov  bp, sp
    push bx

    push word [bp+6]
    push word [bp+4]
    call GetTile
    add  sp, 4

    cmp  ax, 1
    jb   .not_solid
    cmp  ax, 10
    ja   .not_solid
    mov  ax, 1
    jmp  .done
.not_solid:
    xor  ax, ax
.done:
    pop  bx
    pop  bp
    ret

UpdateCamera:
    push ax
    push bx

    mov  ax, [movement_x_int]
    cmp  ax, [camera_x]
    jae  .check_follow
    mov  ax, [camera_x]
    mov  [movement_x_int], ax
    mov  byte [movement_x_frac], 0
    mov  word [movement_vx], 0

.check_follow:
    mov  ax, [movement_x_int]
    sub  ax, [camera_x]
    cmp  ax, CAMERA_FOLLOW_X
    jbe  .done
    mov  ax, [movement_x_int]
    sub  ax, CAMERA_FOLLOW_X
    cmp  ax, CAMERA_MAX_X
    jbe  .candidate_ready
    mov  ax, CAMERA_MAX_X
.candidate_ready:
    cmp  ax, [camera_x]
    jbe  .done
    mov  [camera_x], ax

.done:
    pop  bx
    pop  ax
    ret

ResetMovement:
    mov  word [movement_x_int], 32
    mov  byte [movement_x_frac], 0
    mov  word [movement_y_fp], 144*256
    mov  word [movement_vx], 0
    mov  word [movement_vy], 0
    mov  byte [movement_grounded], 1
    mov  byte [movement_space_previous], 0
    mov  byte [movement_facing_left], 0
    mov  word [camera_x], 0
    ret

RenderMovement:
    push ax

    xor  ax, ax
    mov  al, [movement_facing_left]
    push ax
    cmp  byte [movement_grounded], 0
    jne  .on_ground
    mov  ax, MarioJumpShape
    jmp  .push_sprite
.on_ground:
    cmp  word [movement_vx], 0
    je   .standing
    cmp  byte [movement_frame], 0
    jne  .frame_b
    mov  ax, MarioRunFrameA
    jmp  .push_sprite
.frame_b:
    mov  ax, MarioRunFrameB
    jmp  .push_sprite
.standing:
    mov  ax, MarioStandShape
.push_sprite:
    push ax
    xor  ax, ax
    mov  al, [movement_y_fp+1]
    push ax
    mov  ax, [movement_x_int]
    sub  ax, [camera_x]
    push ax
    call DrawSprite
    add  sp, 8

    pop  ax
    ret

movement_x_int          dw 32
movement_x_frac         db 0
movement_y_fp           dw 144*256
movement_vx             dw 0
movement_vy             dw 0
movement_grounded       db 1
movement_space_previous db 0
movement_facing_left    db 0
movement_frame          db 0
camera_x                dw 0
