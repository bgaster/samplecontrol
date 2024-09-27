; pass audio from channel 1 input into stream
; use task to copy from that stream into another, 
; which is than pass channel 1 output

; create a stream for audio to flow from
@stream S0 #32 SR #1

; attach input (G1) to stream (S0)
@attach G1 S0 #128 

; attach stream (S0) to output (C1)
@attach S0 C1 #128

@task _t1:
_start:             ; local label (_t1start)
    MOVL R0 #128
    ; loop to process 128 samples
	_l1:
	    ; S0 is filled "automatically" by generator
        READ R1 S0 ; load a value from S0 into R1
        WRITE S1 R1 ; store a value from R1 into S1
        MOVL R1 #-1
        ADD R0 R0 R1
        MOVL R1 #0
        CMP R0 R1 ; compare R0 and 0, flag is 0, if true
        JMPZ _l1
    YIELD ; yield back to scheduler 
	JMP _start ; jmp to local label

@entry
    SPAWN SR _t1
    START       ; transfer control to the scheduler