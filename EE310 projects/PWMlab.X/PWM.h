/* 
 * File:   PWM.h
 * Author: student
 *
 * Created on May 1, 2023, 9:02 AM
 */

#ifndef PWM_H
#define	PWM_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* PWM_H */



//void PWM_Output_D8_Enable (void);
//void PWM_Output_D8_Disable (void);

///////////////  TIMER 2
//void TMR2_Initialize(void)
//{
//    // Timer2 clock source = FOSC/4
//    T2CLKCON = 0x01;
//
//    T2HLT = 0x00;
//    T2RST = 0x00;
//
//    // 20 ms servo period at FOSC = 4 MHz, prescaler = 1:128
//    T2PR = 155;
//    T2TMR = 0x00;
//
//    PIR4bits.TMR2IF = 0;
//
//    // Use bit names instead of a magic hex value.
//    // CKPS = 7 means 1:128 on this timer family.
//    T2CONbits.OUTPS = 0;      // postscaler 1:1
//    T2CONbits.CKPS = 7;       // prescaler 1:128
//    T2CONbits.TMR2ON = 0;     // start after PWM is initialized
//}

// TMR2 VALUES FOR SIMULATION ONLY, as logic analyzer has limited period
void TMR2_Initialize(void)
{
    T2CLKCON = 0x01;     // Timer2 clock = FOSC/4
    T2HLT = 0x00;
    T2RST = 0x00;

    T2PR = 19;           // 20 us PWM period at Fosc = 4 MHz, prescale 1:1
    T2TMR = 0x00;

    PIR4bits.TMR2IF = 0;

    T2CONbits.OUTPS = 0; // postscaler 1:1
    T2CONbits.CKPS = 0;  // prescaler 1:1
    T2CONbits.TMR2ON = 0;
}

//void TMR2_ModeSet(TMR2_HLT_MODE mode)
//{
//   // Configure different types HLT mode
//    T2HLTbits.MODE = mode;
//
//
//void TMR2_ExtResetSourceSet(TMR2_HLT_EXT_RESET_SOURCE reset)
//{
//    //Configure different types of HLT external reset source
//    T2RSTbits.RSEL = reset;
//}

void TMR2_Start(void)
{
    // Start the Timer by writing to TMRxON bit
    T2CONbits.TMR2ON = 1;
}

void TMR2_StartTimer(void)
{
    TMR2_Start();
}

void TMR2_Stop(void)
{
    // Stop the Timer by writing to TMRxON bit
    T2CONbits.TMR2ON = 0;
}

void TMR2_StopTimer(void)
{
    TMR2_Stop();
}

uint8_t TMR2_Counter8BitGet(void)
{
    uint8_t readVal;

    readVal = TMR2;

    return readVal;
}

uint8_t TMR2_ReadTimer(void)
{
    return TMR2_Counter8BitGet();
}

void TMR2_Counter8BitSet(uint8_t timerVal)
{
    // Write to the Timer2 register
    TMR2 = timerVal;
}

void TMR2_WriteTimer(uint8_t timerVal)
{
    TMR2_Counter8BitSet(timerVal);
}

void TMR2_Period8BitSet(uint8_t periodVal)
{
   PR2 = periodVal;
}

void TMR2_LoadPeriodRegister(uint8_t periodVal)
{
   TMR2_Period8BitSet(periodVal);
}

//bool TMR2_HasOverflowOccured(void)
//{
//    // check if  overflow has occurred by checking the TMRIF bit
//    bool status = PIR4bits.TMR2IF;
//    if(status)
//    {
//        // Clearing IF flag.
//        PIR4bits.TMR2IF = 0;
//    }
//    return status;
//}



///////////// END OF TIMER 

void PPS_Initialize(void)
{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;    // unlock PPS

    RB3PPS = 0x0A;                // RB3 = CCP2 output

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;    // lock PPS
}

void PWM_Output_D8_Enable (void){
    PPSLOCK = 0x55; 
    PPSLOCK = 0xAA; 
    PPSLOCKbits.PPSLOCKED = 0x00; // unlock PPS

    // Set D8 as the output of CCP2
    RB3PPS = 0x0A;

    PPSLOCK = 0x55; 
    PPSLOCK = 0xAA; 
    PPSLOCKbits.PPSLOCKED = 0x01; // lock PPS
}

void PWM_Output_D8_Disable (void){
    PPSLOCK = 0x55; 
    PPSLOCK = 0xAA; 
    PPSLOCKbits.PPSLOCKED = 0x00; // unlock PPS

    // Set D8 as GPIO pin
    RB3PPS = 0x00;

    PPSLOCK = 0x55; 
    PPSLOCK = 0xAA; 
    PPSLOCKbits.PPSLOCKED = 0x01; // lock PPS
    
    TRISBbits.TRISB3 = 0;
}

void PWM2_Initialize(void)
{
    CCP2CONbits.EN = 0;

    // PWM mode, right-aligned
    CCP2CONbits.MODE = 0x0C;
    CCP2CONbits.FMT = 0;

    // Select Timer2 for CCP2
    CCPTMRS0bits.C2TSEL = 0x1;

    CCPR2H = 0;
    CCPR2L = 47;     // center position, about 1.5 ms

    CCP2CONbits.EN = 1;
}

void PWM2_LoadDutyValue(uint16_t dutyValue)
{
    dutyValue &= 0x03FF;

    CCPR2H = dutyValue >> 8;
    CCPR2L = dutyValue;
}

 _Bool PWM2_OutputStatusGet(void)
{
    // Returns the output status
    return(CCP2CONbits.OUT);
}