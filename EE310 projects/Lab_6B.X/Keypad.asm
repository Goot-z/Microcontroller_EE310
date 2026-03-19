;---------------------
; Title: Keypad
;---------------------
; Program Details:
; The purpose of this program is to check which number has been pressed 
; on the keypad and then turn on PORD accordingly, showing the binary
; representation. 
    
; Inputs: RB3. 4. 6. 7
; Outputs: RD0-7 & RB0,1,2
; Setup: The Curiosity Board

; Date: Feb 24, 2024
; File Dependencies / Libraries: It is required to include the
;   AssemblyConfig.inc in the Header Folder
; Compiler: xc8, 2.4
; Author: Farid Farahmand
; Versions:
;       V1.0: Original
; Useful links:
; Keypad: https://www.circuitbasics.com/how-to-set-up-a-keypad-on-an-arduino/ 

;---------------------
; Initialization
;---------------------
#include "C:\Users\gutie\Documents\MPLAB resources\AssemblyConfig.inc"
#include <xc.inc>
#include "C:\Users\gutie\MPLABXProjects\EE310 labs\Lab_6B.X\headerfile.inc"

;---------------------
; Program Inputs
;---------------------


;---------------------
; Definitions
;---------------------

;---------------------
; Program Constants
;---------------------
BYTE01	EQU		0xF2		;Data bytes
BYTE02	EQU		0x32
REG00	EQU		0x00		;Data Register addresses
REG01	EQU		0x01
REG02	EQU		0x02
REG10	EQU		0x10

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
 
 BYTE	MACRO	REGXX
    LFSR	1,REGXX	    ; 1 represents FSR1
    MOVFF	POSTDEC1,REG00
    MOVFF	INDF1,REG00
ENDM

 ;---------------------
; Setup & Main Program
;---------------------   
_setup:
    clrf what_button
    clrf WREG
    RCALL _setupPortD
    RCALL _setupPortB
    clrf PORTB


_main:
    RCALL _check_keypad
    MOVFF   what_button,PORTD
    
;    MOVLW	0xAA
;    MOVWF	REG10
;    BYTE	0x10
    
    GOTO    _main


				

    END