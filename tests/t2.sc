
; simple func and print string to console example

@func _print_str:
_loop:
    LDRSB R1 R0         ; load byte from memory
    MOVL R2 #0
    CMP R1 R2           ; is byte == '\0'
    JMPZ _exit_loop     ; if so exit loop
    .Console/write R1   ; otherwise write byte to console
    JMP _loop           ; jump to start of loop
_exit_loop:
    RET


@func _f1:
_start:
    LDRSB R0 "Hello, World\n"
    CALL _print_str
    RET

@entry
    CALL _f1
    HALT