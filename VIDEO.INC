VideoMode13:
    push ax
    mov  ax, 0013h
    int  10h
    pop  ax
    ret

VideoTextMode:
    push ax
    mov  ax, 0003h
    int  10h
    pop  ax
    ret
