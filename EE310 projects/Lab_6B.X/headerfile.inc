	PSECT statement 
	ORG  0x100
	what_button EQU 0x20

;-------------------------------------
; Call Functions
;-------------------------------------
_setupPortD:
    BANKSEL	PORTD ;
    CLRF	PORTD ;Init PORTA
    BANKSEL	LATD ;Data Latch
    CLRF	LATD ;
    BANKSEL	ANSELD ;
    CLRF	ANSELD ;digital I/O
    BANKSEL	TRISD ;
    MOVLW	0b00000000 ;Set RD[7:1] as outputs
    MOVWF	TRISD ;and set RD0 as ouput
    RETURN

_setupPortB:
    BANKSEL	PORTB ;
    CLRF	PORTB ;Init PORTB
    BANKSEL	LATB ;Data Latch
    CLRF	LATB ;
    BANKSEL	ANSELB ;
    CLRF	ANSELB ;digital I/O
    BANKSEL	TRISB ;
    MOVLW	0xFF ;
    MOVWF	TRISB ;
    RETURN

_check_keypad:
    movf what_button, w			;	we want to copy our last digit that was pressed into w

    btfss PORTB, 4			;	lets scan the first column of keys
    goto    _col2
    btfsc PORTB, 0			;	has the 1 key been pressed? if yes then
    movlw 1				;	copy decimal number 01 into w. but if not then continue on.
    btfsc PORTB, 1			;	has the 4 key been pressed? if yes then
    movlw 4				;	copy decimal number 04 into w. but if not then continue on.
    btfsc PORTB, 2			;	has the 7 key been pressed? if yes then
    movlw 7				;	copy decimal number 07 into w. but if not then continue on.
    btfsc PORTB, 3			;	has the * key been pressed? if yes then
    movlw 15				;	copy the decimal number 10 into w. but if not then continue on.
    bcf PORTB, 4			;	now we have finished scanning the first column of keys
goto finalSelection
_col2:
    btfss PORTB, 5			;	let's scan the middle column of keys
    goto _col3
    btfsc PORTB, 0			;	has the 2 key been pressed? if yes then
    movlw 2				;	copy decimal number 02 into w. but if not then continue on.
    btfsc PORTB, 1			;	has the 5 key been pressed? if yes then
    movlw 5				;	copy decimal number 05 into w. but if not then continue on.
    btfsc PORTB, 2			;	has the 8 key been pressed? if yes then
    movlw 8				;	copy the decimal number 08 into w. but if not then continue on.
    btfsc PORTB, 3			;	has the 0 key been pressed? if yes then
    movlw 0				;	copy decimal number 00 into w. but if not then continue on.
    bcf PORTB, 5			;	now we have finished scanning the middle column of keys
goto finalSelection
_col3:
    btfss PORTB, 6			;	let's scan the last column of keys
    goto _col4
    btfsc PORTB, 0			;	has the 3 key been pressed? if yes then
    movlw 3				;	copy decimal number 03 into w. but if not then continue on.
    btfsc PORTB, 1			;	has the 6 key been pressed? if yes then
    movlw 6				;	copy decimal number 06 into w. but if not then continue on.
    btfsc PORTB, 2			;	has the 9 key been pressed? if yes then
    movlw 9				;	copy decimal number 09 into w. but if not then continue on.
    btfsc PORTB, 3			;	has the # key been pressed? if yes then
    movlw 14				;	copy the decimal number 11 into w. but if not then continue on.
    bcf PORTB, 6			;	now we have finished scanning the last column of keys
goto finalSelection
_col4:
    btfss PORTB, 7			;	let's scan the last column of keys
    goto finalSelection
    btfsc PORTB, 0			;	has the 3 key been pressed? if yes then
    movlw 10				;	copy decimal number 03 into w. but if not then continue on.
    btfsc PORTB, 1			;	has the 6 key been pressed? if yes then
    movlw 11				;	copy decimal number 06 into w. but if not then continue on.
    btfsc PORTB, 2			;	has the 9 key been pressed? if yes then
    movlw 12				;	copy decimal number 09 into w. but if not then continue on.
    btfsc PORTB, 3			;	has the # key been pressed? if yes then
    movlw 13				;	copy the decimal number 11 into w. but if not then continue on.
    bcf PORTB, 7			;	now we have finished scanning the last column of keys
finalSelection:
    movwf what_button	
return		


