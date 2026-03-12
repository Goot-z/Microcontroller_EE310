//-----------------------------
// Title: Pic Temperature Sensor
//-----------------------------
// Purpose: This program compares the actual temperature to a reference temperature
//	    and turns on a cooling system or heating system depending on if the actual
//	    temperature is higher or lower than the reference. The temperatures are
//	    stored as decimal values in registers
// Dependencies: AssemblyConfig.inc
// Compiler: MPLAB X IDE v6.30
// Author: Steve Gutierrez
// OUTPUTS: Digital output to PORTD.1 and PORTD.2 which are connected to LEDs and
//	    a heating and cooling system respectively
// INPUTS:  Reference temperature into to a keypad, actual temperature into a 
//	    temperature sensor
// Versions:
//  	V1.0: 3/10/2026
//  	V1.1: TBD
//-----------------------------

;---------------------
; Initialization - make sure the path is correct
;---------------------
#include "C:\Users\gutie\Documents\MPLAB resources\AssemblyConfig.inc"
#include <xc.inc>
	
;----------------
; PROGRAM INPUTS
;----------------
;The DEFINE directive is used to create macros or symbolic names for values.
;It is more flexible and can be used to define complex expressions or sequences of instructions.
;It is processed by the preprocessor before the assembly begins.

#define  measuredTempInput 	-5 ; this is the input value
#define  refTempInput		15 ; this is the input value

;---------------------
; Definitions
;---------------------
#define SWITCH    LATD,2  
#define LED0      PORTD,0
#define LED1	  PORTD,1
    
 
;---------------------
; Program Constants
;---------------------
; The EQU (Equals) directive is used to assign a constant value to a symbolic name or label.
; It is simpler and is typically used for straightforward assignments.
;It directly substitutes the defined value into the code during the assembly process.
    
REG20   equ     0x20   // in HEX
REG21   equ     0x21
REG22   equ     0x22

;---------------------
; Main Program
;---------------------
    PSECT absdata,abs,ovrld        ; Do not change
	org 0x00
	
	
	
	org 0x20
	clrf	TRISD		; set portd as output
	movlw	refTempInput	; move measured to W
	movwf	REG20
; test reference against specified min and max. If below/above min/max,
; for to allowed min/max.
	movlw	10
	btfsc   REG20, 7     ; test sign bit (bit 7), set to 10 if signed (neg)
	movwf	REG20
	cpfsgt	REG20	    ; check if f<10, set to 10 if lower than
	movwf	REG20
	movlw	45
	cpfslt	REG20	    ; check if f>5, set to 45 if greater than
	movwf	REG20	    ; move temp to hex and dec registers	    ; ^
	movff	REG20, 0x60
	
; store temp into 3 registers, one as signed 
; bit for negatives, and 2 for the decimal values of said temperature
	movlw	10		; set to 10 for future dec conversion

Br:	incf	0x61, F, B	; inc by 1, this will end up as the remainder
	cpfslt	0x60
	subwf	0x60, F, B	; subtract by 10 (hex)
	cpfslt	0x60
	bc	Br		; branch name "branch reference"

; now test measured temperature
	movlw	measuredTempInput
	movwf	REG21
	movlw	10
	btfsc	REG21, 7		; check if MSB is set
	setf	0x72		; set 0x72 if number is negative
	movff	REG21, 0x70
	btfsc	REG21, 7
	negf	0x70		; Change to positive
	cpfsgt	0x70		; check if temp>10
	goto	sys		; if no, then skip division process
	
Bm:	incf	0x71, F		; inc by 1, remainder
	cpfslt	0x70
	subwf	0x70, F		; subtract by 10 (hex)
	cpfslt	0x70
	bc	Bm		; branch name "branch measured"

;Now that numbers have been tested, compare temps to enable system
sys:	btfsc	REG21, 7		; if actual <0, we know it's below ref
	goto	cold			; go to heating system
	movlw	measuredTempInput	; move actual to W
	cpfseq	REG20			; ref = actual 
	goto	continue		; continue other checks if not equal
	goto	equal		
continue:
	cpfsgt	REG20			; ref > actual 
	goto	hot			; false --> go to heating 
	cpfslt	REG20			; ref < actual (too hot)
	goto	cold			; false --> go to cooling


;; heating system
cold:
	movlw    1
	movwf    REG22		; set contreg to 1 (heating sys)
        goto     finish

hot:
	movlw    2
	movwf    REG22		; set contreg to 2 (cooling sys)
        goto     finish
    
equal:
	movlw    0
	movwf    REG22		; set contreg to 0 (no display)
        goto     finish	
	
finish:	
	movff	 REG22, LATD
	movlw	0
    ; the end for now! update with more processes in future versions

	END