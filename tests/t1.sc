

@task _foo:
    MOVL R1 #1000000000
_bar:
    MOVL R0 #20
    MOVL R2 #3.455
    FTOI R4 R2
    ADD  R3 R1 R0
    JMP _exit
    JMP _bar
_exit:
    NOP

@entry
    SPAWN _foo
    START       ; transfer control to the scheduler