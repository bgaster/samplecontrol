
; simple func and print string to console example

@func _print_str:
    MOVL R2 #0
    LDR R2 R2
_loop:
    LDRSB R1 R0         ; load byte from memory
    CMP R1 R2           ; is byte == '\0'
    JMPZ _exit_loop     ; if so exit loop
    .Console/write R1   ; otherwise write byte to console
    MOVL R1 #1
    LDR R1 R1
    ADD R0 R0 R1
    JMP _loop           ; jump to start of loop
_exit_loop:
    RET


@func _f1:
_start:
    MOVL R0 "Hello, World\n"
    CALL _print_str
    RET

@entry
    CALL _f1
    HALT