;---------------------
; Title: sevenSegment
;---------------------
; Program Details:
; The purpose of this program is to increase or decrease a number on a seven segment
; display from 0-F depending on which of 2 buttons are pressed. If counter goes 
; above/below max then it should wrap back to the bottom/top respectfully.
; pressing both buttons at the same time sets seven segment to 0, pressing
; no buttons displays the previously displayed number
    
; Inputs: RB0,1
; Outputs: RD0-7 
; Setup: PIC18F47K42 Curiosity Nano

; Date: April 1st, 2026
; File Dependencies / Libraries: It is required to include the
; AssemblyConfig.inc in the Header Folder
; Compiler: xc8, v6.30
; Author: Steve Gutierrez
; Versions:
;       V1.0: program created
; Useful links:

;---------------------
; Initialization
;---------------------
#include "C:\Users\gutie\Documents\MPLAB resources\AssemblyConfig.inc"
#include <xc.inc>

;---------------------
; Program Inputs
;---------------------


;---------------------
; Definitions
;---------------------

;---------------------
; Program Constants
;---------------------
counter	equ	0x20	    ;Counter going from 0-F
del1	equ	0x21	    ;delay 1
del2	equ	0x22	    ;delay 2
del3	equ	0x23	    ;delay 3
;---------------------
; Program Organization
;---------------------
    PSECT absdata,abs,ovrld        ; Do not change

    ORG          0                ;Reset vector
    GOTO        _setup

    ORG          0020H           ; Begin assembly at 0020H
;---------------------
; Macros
;---------------------
 

 ;---------------------
; Setup & Main Program
;---------------------   
_setup:
    clrf    counter
    RCALL   _setupPortD
    RCALL   _setupPortB
    movlw   0x20	;start table pointer at program memory address 
    movwf   TBLPTRH	;for seven segment numbers to be used with counter
    clrf    TBLPTRU	;clear upper byte of table pointer
    clrf    LATB
    goto    _display	;start display at 0

_main:
    RCALL _delay
    btfsc   PORTB,0	;check if button A pressed
    goto    _countup	;yes? count up
    btfsc   PORTB,1	;check if button B pressed
    goto    _countdown	;yes? count down
    goto    _main	;if no button pressed, check again

	PSECT statement 
	ORG  0x100

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
    MOVLW	0x03 ; set [0:1] as inputs, [7:2] outputs
    MOVWF	TRISB ;
    RETURN

_delay:
    movlw   0x03        
    movwf   del1		
loop1:
    movlw   0xFF
    movwf   del2
loop2:
    movlw   0xFF
    movwf   del3
loop3:
    decfsz  del3, f
    goto    loop3
    decfsz  del2, f
    goto    loop2
    decfsz  del1, f
    goto    loop1			
    return
    
_countup:
    btfsc   PORTB,1 ;check if button B was ALSO pressed
    goto    _both    ;yes? set to 0
    incf    counter, f
    movlw   0x10
    cpfseq  counter
    goto    _display
    clrf    counter
    goto    _display
    
_countdown:
    btfsc   PORTB,0 ;check if button A was ALSO pressed
    goto    _both    ;yes? set to 0
    movf    counter, f	;check if counter will decrement from 0
    bz	    _wrap	;if yes, wrap instead
    decf    counter,f
    goto    _display
_wrap:
    movlw   0x0F
    movwf   counter
    goto    _display
    
_both:
    clrf    counter   ;if both buttons pressed, set to 0
    goto    _display

_display:
    movff   counter, TBLPTRL	;move counter value into table pointer
    tblrd*  ;read table pointer value into table latch
    movff   TABLAT, LATD    ;move seven segment code stored in table to portD
    goto    _main    ;restart
;-------------------------------------
; Table pointer
;-------------------------------------   
 
    ORG 0x2000
_sevenseg:
;DB values are the binary codes to display
;each number on a seven segment display respectfully
;lowest bit should connect to A pin on seven segment 
;and highest bit should connect to DP pin
    DB  0b00111111  ; 0
    DB  0b00000110  ; 1
    DB  0b01011011  ; 2
    DB  0b01001111  ; 3
    DB  0b01100110  ; 4
    DB  0b01101101  ; 5
    DB  0b01111101  ; 6
    DB  0b00000111  ; 7
    DB  0b01111111  ; 8
    DB  0b01100111  ; 9
    DB  0b01110111  ; A
    DB  0b01111100  ; b
    DB  0b00111001  ; C
    DB  0b01011110  ; d
    DB  0b01111001  ; E
    DB  0b01110001  ; F
    
    
    END


