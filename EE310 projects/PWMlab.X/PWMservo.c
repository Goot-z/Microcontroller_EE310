/*
* The purpose of this program is to generate a PWM with different pulse width 
* The output of the PWM will be on RB3
* myLED is connected to RB0 and it toggles very slowly
* In order to change the PULSE period and width you need to do the following: 

 * PWM Period=
 * [T2PR+1]*4*Tosc*PreScale

 * Pulse Width=
 * Tosc*Prescale*CCPR2

 * Duty Cycle Ratio %=
 * CCPR2 / [4*(T2PR+1)]

 * The presale value for the timer is defined in T2CON register. 
 * The ACTUAL value of CCP2 MUST be varied by changing
 * PWM2_INITIALIZE_DUTY_VALUE set to the equivalent decimal value for CCPR2

 * Author: Farid Farahmand
 */




#include <xc.h> // must have this
#include "PWM.h" // must have this
#include "XC8_ConfigFile.h" // must have this -  XC8_ConfigFile.h
//#include "../../../../../Program Files/Microchip/xc8/v2.40/pic/include/proc/pic18f46k42.h"
//#include "C:\Program Files\Microchip\xc8\v2.40\pic\include\proc\pic18f46k42"


#define _XTAL_FREQ 4000000UL

//#define SERVO_MIN_DUTY     31u   // about 1.0 ms
//#define SERVO_CENTER_DUTY  47u   // about 1.5 ms
//#define SERVO_MAX_DUTY     63u   // about 2.0 ms

// FOR SIMULATION ONLY
#define SERVO_MIN_DUTY     4u    // 1.0 us pulse, equivalent to 1.0 us
#define SERVO_CENTER_DUTY  6u    // 1.5 us pulse, equivalent to 1.5 us
#define SERVO_MAX_DUTY     8u    // 2.0 us pulse, equivalent to 2.0 us

#define SERVO_STEP         1u
#define MOVE_EVERY_N_FRAMES 2u   // update every 40 ms

#define LEFT_PRESSED()     (!PORTBbits.RB4)
#define RIGHT_PRESSED()    (!PORTBbits.RB5)

uint16_t servoDuty = SERVO_CENTER_DUTY;
uint8_t moveCounter = 0;

void main(void)
{
    // Make sure your oscillator is actually 4 MHz.
    // Do not rely on OSCSTATbits.HFOR = 1; that is a status flag.
    OSCFRQ = 0x02;   // 4 MHz HFINTOSC setting in your code/comment

    ANSELB = 0x00;

    // RB0 LED output, RB3 PWM output, RB4/RB5 buttons input
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB3 = 0;
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB5 = 1;

    // Enable weak pullups if your buttons connect pin-to-ground when pressed
    WPUBbits.WPUB4 = 1;
    WPUBbits.WPUB5 = 1;

    LATB = 0x00;

    TMR2_StartTimer();

    PMD1bits.TMR2MD = 0;
    PMD3bits.CCP2MD = 0;

    PPS_Initialize();
    TMR2_Initialize();
    PWM2_Initialize();

T2CONbits.ON = 1;
    
    while (1)
    {
        // One flag per 20 ms PWM frame
        if (PIR4bits.TMR2IF)
        {
            PIR4bits.TMR2IF = 0;

            moveCounter++;

            if (moveCounter >= MOVE_EVERY_N_FRAMES)
            {
                moveCounter = 0;

                if (LEFT_PRESSED() && !RIGHT_PRESSED())
                {
                    if (servoDuty > SERVO_MIN_DUTY)
                    {
                        servoDuty -= SERVO_STEP;
                    }
                    else
                    {
                        servoDuty = SERVO_MIN_DUTY;
                    }

                    PWM2_LoadDutyValue(servoDuty);
                }
                else if (RIGHT_PRESSED() && !LEFT_PRESSED())
                {
                    if (servoDuty < SERVO_MAX_DUTY)
                    {
                        servoDuty += SERVO_STEP;
                    }
                    else
                    {
                        servoDuty = SERVO_MAX_DUTY;
                    }

                    PWM2_LoadDutyValue(servoDuty);
                }

                // If neither button is pressed, duty does not change.
                // The servo holds its current position.
                // If both are pressed, also hold position.
            }
        }
    }
}