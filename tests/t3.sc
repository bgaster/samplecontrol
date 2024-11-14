;@include "lib.sc"

; we really need a calling convention :-) :-)

@segment .data
_x: 
  WORD #1 #50
_y:
  WORD #1 #100 

; support for syscall style files
; R1 is pointer to start of path/filename
; MOVL R1 #"path/filename.slides"
; MOVL R2 #0            ; O_RDONLY
;.File/open R0 R1 R2    ; R0 = fd
;.File/read R1 R0
;.File/close R0

@segment .code

; routine to print string to console
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

@task _display:
_start:
    ; initialize x,y position
    MOVL R10 #50          ; x
    LDR R10 R10
    MOVL R10 _x
    LDR R10 R10

    MOVL R11 #50          ; y 
    MOVL R11 _y
    ;LDR R11 R11
    LDR R11 R11
_start_after:

    ;@await SREAD R0 S0  ; wait until their is a mouse movement (x, y) 
    .Screen/begin       ; start painting

    MOVL R2 #4          ; colour index 
    LDR R2 R2
    .Screen/colour R2
    .Screen/fill

    ; read mouse movement, if there is any
    SREAD R9 S0          
    JMPZ _nomouse       ; SREAD sets cmp flag to 1 if value read, otherwise 0
    ; handle mouse message
    ; check button or motion message
    MOVL R20 #2147483648
    LDR R20 R20
    AND R20 R9 R20 ; 0x80000000
    MOVL R21 #0
    LDR R21 R21
    CMP R20 R21
    JMPZ _mousemotion
    MOV R10 R21
    MOV R11 R20
    JMP _nomouse
_mousemotion:
    ; sc_ushort x = (value >> 16) & 0xFFFF;
    ; sc_ushort y = value & 0xFFFF;
    MOVL R12 #16
    LDR R12 R12
    SHIFTR R12 R9 R12 ; R12 = value >> 16 
    MOVL R13 #65535   ; 0xFFFF (need hex support :-))
    LDR R13 R13
    AND R10 R12 R13   ; R10 = (value >> 16) & 0xFFFF
    AND R11 R9  R13   ; R11  = value & 0xFFFF 

_nomouse:
    ; draw a rectangle
    .Screen/move R10 R11
    MOVL R2 #1          ; colour index 
    LDR R2 R2
    .Screen/colour R2
    MOVL R0 #10         ; (10,10)
    LDR R0 R0
    .Screen/rect R0 R0

    .Screen/end         ; end painting
    YIELD
    JMP _start_after          ; begin again

@func _init:
    ; create a stream for mouse move messages to flow and attach generator
    ; message format (x : ushort, y: ushort)
    ; button status is read via API 
    ; for now we are just assuming G1 is connected to the mouse!!
    MOVL R0 #30 ; sample mouse events at 30hz
    @stream S0 #32 R0 #0   ; mouse messages are 32-bit lower 16-bits are x and top y position
    MOVL R0 #1
    @attach G1 S0 R0
    
    ; create window (VGA resoloution)
    MOVL R0 #640
    LDR R0 R0
    MOVL R1 #480
    LDR R1 R1
    .Screen/resize R0 R1
    MOVL R0 "initialization complete\n"
    CALL _print_str
    RET

@entry
    CALL _init
    MOVL R0 #30
    LDR R0 R0
    SPAWN R0 _display           ; spwan display task at 30 fps
    START                       ; transfer control to the scheduler
    HALT