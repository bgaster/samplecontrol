_foo:
    MOVL R1 #1000000000
    MOVL R0 #20
    MOVL R2 #3.455
    ADD  R3 R1 R0
    JMP _exit
    NOP
_exit:
    NOP


