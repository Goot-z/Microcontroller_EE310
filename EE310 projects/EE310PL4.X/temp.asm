    PROCESSOR 18F47K42
    #include <xc.inc>
    
    ORG    0x20
    MOVLW    0x00
    MOVWF    TRISC
    MOVLW    0x30
    MOVWF     PORTC, 0
    NOP
    MOVLW    0x31
    MOVWF     PORTC, 0
    NOP
    MOVLW    0x32
    MOVWF     PORTC, 0
    SLEEP





