/*
 * File:   DIY_ADC.c
 * Author: Steve Gutierrez
 * IDE version: MPLAB X v6.30
 *
 * Version:
 * 1.0 Created program
 * 1.1 Edited for Y Z input
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma config FEXTOSC = XT
#pragma config RSTOSC = EXTOSC
#pragma config CLKOUTEN = OFF
#pragma config PR1WAY = ON
#pragma config CSWEN = ON
#pragma config FCMEN = ON
#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_OFF
#pragma config MVECEN = ON
#pragma config IVT1WAY = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS
#pragma config BORV = VBOR_2P45
#pragma config ZCD = OFF
#pragma config PPS1WAY = ON
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
#pragma config LVP = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000UL

// Reset pins
#define RESET_BUTTON PORTDbits.RD3
#define LED LATDbits.LATD2

// LCD pins
#define RS LATDbits.LATD0
#define EN LATDbits.LATD1
#define LCD_DATA LATB

// ADC channels
#define ADC_X_CHANNEL 0x00   // RA0 / AN0
#define ADC_Y_CHANNEL 0x01   // RA1 / AN1
#define ADC_Z_CHANNEL 0x02   // RA2 / AN2

// ADXL335 constants
#define ADC_MAX       4095L
#define VREF_MV       3300L
#define ADXL_MV_PER_G 300L
#define GRAVITY_10000 98067L

void Button_Interrupt_Init(void);

void LCD_Init(void);
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_String(const char *msg);
void LCD_String_xy(unsigned char row, unsigned char pos, const char *msg);
void LCD_Clear(void);

void ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);

void format_accel(char *buffer, int32_t value);
int32_t Convert_ADC_To_Accel(uint16_t adc_value, uint16_t zero_value);

uint16_t zeroX = 0;
uint16_t zeroY = 0;
uint16_t zeroZ = 0;

void main(void)
{
    char line1[21];
    char line2[21];

    uint16_t rawX;
    int32_t accelX;
    int32_t lastAccelX = 0;
    int32_t delta;

    LCD_Init();
    Button_Interrupt_Init();
    ADC_Init();

    LCD_Clear();
    LCD_String_xy(1, 0, "Keep sensor flat ");
    LCD_String_xy(2, 0, "Calibrating...   ");
    __delay_ms(2000);

    zeroX = ADC_Read(ADC_X_CHANNEL);
    zeroY = ADC_Read(ADC_Y_CHANNEL);
    zeroZ = ADC_Read(ADC_Z_CHANNEL);

    LCD_Clear();
    LCD_String_xy(1, 0, "ADXL335 Ready   ");
    __delay_ms(1000);

    while(1)
    {
        rawX = ADC_Read(ADC_X_CHANNEL);

        accelX = Convert_ADC_To_Accel(rawX, zeroX);

        delta = accelX - lastAccelX;
        if(delta < 0)
            delta = -delta;

        if(delta > 35000)
        {
            sprintf(line1, "angle: Shake     ");
        }
        else if(accelX > 20000)
        {
            sprintf(line1, "angle: Tilt_Right");
        }
        else if(accelX < -20000)
        {
            sprintf(line1, "angle: Tilt_Left ");
        }
        else
        {
            sprintf(line1, "angle: Flat      ");
        }

        format_accel(line2, accelX);

        LCD_Clear();
        LCD_String_xy(1, 0, line1);
        LCD_String_xy(2, 0, line2);

        lastAccelX = accelX;

        __delay_ms(500);
    }
}

/*************** INTERRUPT ***************/

void __interrupt(irq(INT0), base(8)) INT0_ISR(void)
{
    PIR1bits.INT0IF = 0;

    __delay_ms(20);

    if(RESET_BUTTON == 0)
    {
        LCD_Clear();

        for(int i = 0; i < 10; i++)
        {
            LED = 1;
            __delay_ms(500);

            LED = 0;
            __delay_ms(500);
        }

        RESET();
    }
}

void Button_Interrupt_Init(void)
{
    ANSELDbits.ANSELD3 = 0;
    TRISDbits.TRISD3 = 1;

    ANSELDbits.ANSELD2 = 0;
    TRISDbits.TRISD2 = 0;
    LED = 0;

    INT0PPS = 0x1B;

    INTCON0bits.INT0EDG = 0;

    PIR1bits.INT0IF = 0;
    PIE1bits.INT0IE = 1;

    INTCON0bits.GIE = 1;
}

/*************** ADC FUNCTIONS ***************/

void ADC_Init(void)
{
    // RA0, RA1, RA2 as analog inputs
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    TRISAbits.TRISA2 = 1;

    ANSELAbits.ANSELA0 = 1;
    ANSELAbits.ANSELA1 = 1;
    ANSELAbits.ANSELA2 = 1;

    // ADC reference: VDD and VSS
    ADREF = 0x00;

    // ADC clock
    ADCLK = 0x3F;

    // Acquisition time
    ADACQ = 0x20;

    // Right justified result, ADC enabled
    ADCON0bits.FM = 1;
    ADCON0bits.ON = 1;
}

uint16_t ADC_Read(uint8_t channel)
{
    ADPCH = channel;

    __delay_us(20);

    ADCON0bits.GO = 1;

    while(ADCON0bits.GO)
    {
        ;
    }

    return ADRES;
}

int32_t Convert_ADC_To_Accel(uint16_t adc_value, uint16_t zero_value)
{
    int32_t adc_delta;
    int32_t mv_delta;
    int32_t accel;

    adc_delta = (int32_t)adc_value - (int32_t)zero_value;

    mv_delta = (adc_delta * VREF_MV) / ADC_MAX;

    accel = (mv_delta * GRAVITY_10000) / ADXL_MV_PER_G;

    return accel;
}

/*************** DISPLAY FORMAT ***************/

void format_accel(char *buffer, int32_t value)
{
    int32_t whole;
    int32_t decimal;

    if(value < 0)
    {
        value = -value;
        whole = value / 10000;
        decimal = value % 10000;
        sprintf(buffer, "-%ld.%04ld X m/s2", whole, decimal);
    }
    else
    {
        whole = value / 10000;
        decimal = value % 10000;
        sprintf(buffer, "%ld.%04ld X m/s2 ", whole, decimal);
    }
}

/*************** LCD FUNCTIONS ***************/

void LCD_Init(void)
{
    ANSELB = 0x00;
    ANSELD = 0x00;

    TRISB = 0x00;
    TRISD &= 0xFC;

    LATB = 0x00;
    LATD &= 0xFC;

    __delay_ms(100);

    LCD_Command(0x38);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
    __delay_ms(2);
}

void LCD_Command(unsigned char cmd)
{
    LCD_DATA = cmd;

    RS = 0;
    EN = 1;
    __delay_us(1);
    EN = 0;

    if(cmd == 0x01 || cmd == 0x02)
        __delay_ms(2);
    else
        __delay_us(50);
}

void LCD_Char(unsigned char data)
{
    LCD_DATA = data;

    RS = 1;
    EN = 1;
    __delay_us(1);
    EN = 0;

    __delay_us(50);
}

void LCD_String(const char *msg)
{
    while(*msg)
    {
        LCD_Char(*msg++);
    }
}

void LCD_String_xy(unsigned char row, unsigned char pos, const char *msg)
{
    unsigned char location;

    if(row == 1)
        location = 0x80 + pos;
    else
        location = 0xC0 + pos;

    LCD_Command(location);
    LCD_String(msg);
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    __delay_ms(2);
}