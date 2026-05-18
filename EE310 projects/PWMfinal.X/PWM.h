#include <xc.h>
#include <stdint.h>

/**************** CONFIG ****************/
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_1MHZ

#pragma config CLKOUTEN = OFF
#pragma config PR1WAY = ON
#pragma config CSWEN = ON
#pragma config FCMEN = ON

#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_OFF
#pragma config MVECEN = OFF
#pragma config IVT1WAY = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS

#pragma config BORV = VBOR_2P45
#pragma config ZCD = OFF
#pragma config PPS1WAY = OFF
#pragma config STVREN = ON
#pragma config DEBUG = OFF
#pragma config XINST = OFF

#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = OFF

#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC

#pragma config BBSIZE = BBSIZE_512
#pragma config BBEN = OFF
#pragma config SAFEN = OFF
#pragma config WRTAPP = OFF

#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config WRTSAF = OFF
#pragma config LVP = ON

#pragma config CP = OFF

#define _XTAL_FREQ 4000000

void Clock_Init(void)
{
    OSCCON1 = 0x60;
    OSCFRQ = 0x02;
}

/**************** PPS: CCP2 -> RC5 ****************/
void PWM_Output_RC5_Enable(void)
{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;

    RC5PPS = 0x0A;      // CCP2 output on RC5

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;
}

void TMR2_Initialize(void)
{
    T2CLKCON = 0x01;
    T2HLT = 0x00;
    T2RST = 0x00;

    T2PR = 155;
    T2TMR = 0x00;

    PIR4bits.TMR2IF = 0;

    T2CON = 0xF0;
}

void PWM2_Initialize(void)
{
    CCP2CON = 0x8C;
    CCPR2H = 0x00;
    CCPR2L = 0x00;

    CCPTMRS0bits.C2TSEL = 0x1;
}

void PWM2_LoadDutyValue(uint16_t dutyValue)
{
    dutyValue &= 0x03FF;

    if(CCP2CONbits.FMT)
    {
        dutyValue <<= 6;
        CCPR2H = dutyValue >> 8;
        CCPR2L = dutyValue;
    }
    else
    {
        CCPR2H = dutyValue >> 8;
        CCPR2L = dutyValue;
    }
}

void main(void)
{
    Clock_Init();

    ANSELCbits.ANSELC5 = 0;
    TRISCbits.TRISC5 = 0;
    LATCbits.LATC5 = 0;

    TMR2_Initialize();
    PWM_Output_RC5_Enable();
    PWM2_Initialize();

    while(1)
    {
        PWM2_LoadDutyValue(47);   // center
        __delay_ms(1500);

        PWM2_LoadDutyValue(31);   // one side
        __delay_ms(1500);

        PWM2_LoadDutyValue(63);   // other side
        __delay_ms(1500);
    }
}