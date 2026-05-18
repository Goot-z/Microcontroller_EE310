#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "XC8_ConfigFile.h"

#define _XTAL_FREQ 4000000UL

/*
 * Important config-bit reminder:
 *
 * #pragma config PPS1WAY = OFF
 *
 * Keep PPS1WAY off 
 */

// Set to 1 only for logic analyzer simulation.
// Set to 0 for a real SG90 servo.
#define SIM_FAST_SERVO 0

// ---------------- Servo thresholds ----------------
#if SIM_FAST_SERVO
    // 20 us simulation frame
    #define SERVO_HOME_DUTY    4u    // scaled 0 degrees
    #define SERVO_DEPLOY_DUTY  6u    // scaled 90 degrees
#else
    // Real 20 ms SG90-style frame
    #define SERVO_HOME_DUTY    50u   // about 1.0 ms, approx 0 degrees
    #define SERVO_DEPLOY_DUTY  22u   // about 1.5 ms, approx 90 degrees
    #define SERVO_MAX_DUTY     63u   // about 2.0 ms pulse
#endif

#define DEPLOY_DISTANCE_CM     10u
#define RELEASE_DISTANCE_CM    13u   // hysteresis so it does not chatter

// ---------------- LCD pins ----------------
#define LCD_DATA_LAT   LATB
#define LCD_DATA_TRIS  TRISB

#define LCD_RS         LATDbits.LATD0
#define LCD_E          LATDbits.LATD1

// ---------------- HC-SR04 pins ----------------
#define TRIG_LAT       LATDbits.LATD3
#define ECHO_PORT      PORTDbits.RD4
 
// ---------------- Buzzer pin ----------------
// Assumes active buzzer module:
// RD5 = 1 -> buzzer on
// RD5 = 0 -> buzzer off
#define BUZZER_LAT       LATDbits.LATD5
#define BUZZER_TRIS      TRISDbits.TRISD5

#define BUZZER_OFF_DISTANCE_CM      50u   // farther than this = silent
#define BUZZER_FAST_DISTANCE_CM     10u   // this close = very fast / continuous
#define BUZZER_ON_TIME_MS           30u
#define BUZZER_MIN_INTERVAL_MS      80u
#define BUZZER_MAX_INTERVAL_MS      700u

/*
 * FUNCTION PROTOTYPES
 */

// ---------------- Clock ----------------
void CLK_Initialize(void);

// ---------------- Ports ----------------
void PORT_Initialize(void);

// ---------------- PPS ----------------
void PPS_Initialize(void);

// ---------------- LCD 1602A, 8-bit mode ----------------
void LCD_PulseEnable(void);
void LCD_Command(uint8_t command);
void LCD_Char(char c);
void LCD_Print(const char *text);
void LCD_Print16(const char *text);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Initialize(void);

// ---------------- Timer1 for HC-SR04 ----------------
void TMR1_Initialize(void);
void TMR1_Write(uint16_t value);
uint16_t TMR1_Read(void);
uint16_t HCSR04_ReadDistanceCM(void);

// ---------------- Timer2 + CCP2 PWM for Servo ----------------
void TMR2_PWM_Initialize(void);
void PWM2_LoadDutyValue(uint16_t dutyValue);
void PWM2_Initialize(void);
void PWM2_Start(void);

// ---------------- Buzzer ----------------
void Buzzer_Off(void);
uint16_t Buzzer_GetIntervalMs(uint16_t distance);
void Buzzer_Update(uint16_t distance, uint16_t tick_ms);
void DelayMs_WithBuzzer(uint16_t distance, uint16_t total_ms);

// ---------------- Main ----------------
void main(void)
{
    uint16_t distance;
    bool deployed = false;
    char line[17];

    CLK_Initialize();
    PORT_Initialize();
    PPS_Initialize();

    LCD_Initialize();
    TMR1_Initialize();

    TMR2_PWM_Initialize();
    PWM2_Initialize();
    PWM2_Start();

    LCD_SetCursor(0, 0);
    LCD_Print16("Airbag Demo");
    LCD_SetCursor(1, 0);
    LCD_Print16("Starting...");
    __delay_ms(1000);
    
    
 /*
 CODE BELOW FOR DEBUGGING SERVO, COMMENT WHEN FIXED
 */
//    while (1)
//    {
//        PWM2_LoadDutyValue(31);   // about 0 degrees
//        __delay_ms(1500);
//
//        PWM2_LoadDutyValue(47);   // about 90 degrees
//        __delay_ms(1500);
//
//        PWM2_LoadDutyValue(63);   // farther rotation
//        __delay_ms(1500);
    
//    }
    
    
    while (1)
    {
        distance = HCSR04_ReadDistanceCM();

        // Deployment logic with hysteresis
        if (!deployed && distance <= DEPLOY_DISTANCE_CM)
        {
            deployed = true;
        }
        else if (deployed && distance >= RELEASE_DISTANCE_CM)
        {
            deployed = false;
        }

        if (deployed)
        {
            PWM2_LoadDutyValue(SERVO_DEPLOY_DUTY);
        }
        else
        {
            PWM2_LoadDutyValue(SERVO_HOME_DUTY);
        }

        // LCD line 1
        LCD_SetCursor(0, 0);

        if (distance == 999)
        {
            LCD_Print16("Dist: no echo");
        }
        else
        {
            sprintf(line, "Dist:%3u cm", distance);
            LCD_Print16(line);
        }

        // LCD line 2
        LCD_SetCursor(1, 0);

        if (deployed)
        {
            LCD_Print16("Servo:90 DEPLOY");
        }
        else
        {
            LCD_Print16("Servo: 0 SAFE");
        }

        // HC-SR04 should not be triggered too rapidly.
        // This delay also updates the buzzer every 10 ms.
        DelayMs_WithBuzzer(distance, 100);
    }
}

/*
 * FUNCTIONS
 */

// ---------------- Clock ----------------
void CLK_Initialize(void)
{
    // HFINTOSC, no divider
    OSCCON1 = 0x60;

    // 4 MHz HFINTOSC
    OSCFRQ = 0x02;
}

// ---------------- Ports ----------------
void PORT_Initialize(void)
{
    // Make PORTB, PORTC, and PORTD digital.
    ANSELB = 0x00;
    ANSELD = 0x00;
    ANSELCbits.ANSELC5 = 0;   // RC5 digital for servo PWM

    // LCD data bus, RB0-RB7
    TRISB = 0x00;
    LATB = 0x00;

    // RD0 = LCD RS, RD1 = LCD E
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;

    // RC5 = servo PWM output, keep input until PWM starts
    TRISCbits.TRISC5 = 1;

    // RD3 = HC-SR04 trigger output
    TRISDbits.TRISD3 = 0;
    TRIG_LAT = 0;

    // RD4 = HC-SR04 echo input
    TRISDbits.TRISD4 = 1;

    // RD5 = buzzer output
    BUZZER_TRIS = 0;
    BUZZER_LAT = 0;

    LATD = 0x00;
}

// ---------------- PPS ----------------
void PPS_Initialize(void)
{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;

    // Route CCP2 PWM output to RD2
    RC5PPS = 0x0A;

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;
}

// ---------------- LCD 1602A, 8-bit mode ----------------
void LCD_PulseEnable(void)
{
    LCD_E = 1;
    __delay_us(2);
    LCD_E = 0;
    __delay_us(50);
}

void LCD_Command(uint8_t command)
{
    LCD_RS = 0;
    LCD_DATA_LAT = command;
    LCD_PulseEnable();

    if (command == 0x01 || command == 0x02)
    {
        __delay_ms(2);
    }
}

void LCD_Char(char c)
{
    LCD_RS = 1;
    LCD_DATA_LAT = (uint8_t)c;
    LCD_PulseEnable();
}

void LCD_Print(const char *text)
{
    while (*text)
    {
        LCD_Char(*text++);
    }
}

void LCD_Print16(const char *text)
{
    uint8_t i = 0;

    while (*text && i < 16)
    {
        LCD_Char(*text++);
        i++;
    }

    while (i < 16)
    {
        LCD_Char(' ');
        i++;
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    if (row == 0)
    {
        LCD_Command(0x80 + col);
    }
    else
    {
        LCD_Command(0xC0 + col);
    }
}

void LCD_Initialize(void)
{
    __delay_ms(20);

    LCD_RS = 0;
    LCD_E = 0;

    // Robust 8-bit startup sequence
    LCD_Command(0x30);
    __delay_ms(5);
    LCD_Command(0x30);
    __delay_us(150);
    LCD_Command(0x30);

    LCD_Command(0x38);   // 8-bit, 2-line, 5x8 font
    LCD_Command(0x0C);   // display on, cursor off
    LCD_Command(0x06);   // increment cursor
    LCD_Command(0x01);   // clear display
}

// ---------------- Timer1 for HC-SR04 echo measurement ----------------
// Timer1/3/5 are 16-bit timers. With T1CLK = FOSC/4 and FOSC = 4 MHz,
// Timer1 increments every 1 us.

void TMR1_Initialize(void)
{
    T1CLK = 0x01;     // FOSC/4
    T1GCON = 0x00;   // gate disabled

    // bit 1 RD16 = 1, bit 0 ON = 0, prescaler 1:1
    T1CON = 0x02;

    TMR1H = 0x00;
    TMR1L = 0x00;
}

void TMR1_Write(uint16_t value)
{
    TMR1H = (uint8_t)(value >> 8);
    TMR1L = (uint8_t)(value & 0xFF);
}

uint16_t TMR1_Read(void)
{
    uint8_t low;
    uint8_t high;

    low = TMR1L;
    high = TMR1H;

    return ((uint16_t)high << 8) | low;
}

uint16_t HCSR04_ReadDistanceCM(void)
{
    uint16_t echo_us;
    uint16_t timeout;

    // Trigger pulse: low, then high for 10 us, then low
    TRIG_LAT = 0;
    __delay_us(2);

    TRIG_LAT = 1;
    __delay_us(10);
    TRIG_LAT = 0;

    // Wait for echo to go high
    timeout = 30000;
    while (!ECHO_PORT)
    {
        if (timeout == 0)
        {
            return 999;     // no echo
        }

        timeout--;
        __delay_us(1);
    }

    // Measure echo high time
    TMR1_Write(0);
    T1CON |= 0x01;          // Timer1 ON

    while (ECHO_PORT)
    {
        if (TMR1_Read() > 30000)
        {
            T1CON &= 0xFE;  // Timer1 OFF
            return 999;     // out of range / timeout
        }
    }

    T1CON &= 0xFE;          // Timer1 OFF

    echo_us = TMR1_Read();

    // HC-SR04 approximate conversion
    return echo_us / 58u;
}

// ---------------- Timer2 + CCP2 for servo PWM ----------------
void TMR2_PWM_Initialize(void)
{
    T2CLKCON = 0x01;    // FOSC/4
    T2HLT = 0x00;
    T2RST = 0x00;
    T2TMR = 0x00;

    PIR4bits.TMR2IF = 0;

#if SIM_FAST_SERVO
    // Simulation only:
    // FOSC = 4 MHz, prescaler 1:1, T2PR = 19
    // Period = 20 us
    T2PR = 19;
    T2CON = 0x00;       // ON=0, CKPS=1:1, OUTPS=1:1
#else
    // Real servo:
    // FOSC = 4 MHz, prescaler 1:128, T2PR = 155
    // Period = 19.968 ms
    T2PR = 155;
    T2CON = 0x70;       // ON=0, CKPS=1:128, OUTPS=1:1
#endif
}

void PWM2_LoadDutyValue(uint16_t dutyValue)
{
    dutyValue &= 0x03FF;

    // Right-aligned duty value
    CCPR2H = (uint8_t)(dutyValue >> 8);
    CCPR2L = (uint8_t)(dutyValue & 0xFF);
}

void PWM2_Initialize(void)
{
    CCP2CON = 0x00;

    // C2TSEL = 01:
    // CCP2 uses Timer1 in Capture/Compare mode,
    // Timer2 in PWM mode.
    CCPTMRS0 = (CCPTMRS0 & 0xF3) | 0x04;

    PWM2_LoadDutyValue(SERVO_HOME_DUTY);

    // EN=1, FMT=0 right-aligned, MODE=1100 PWM
    CCP2CON = 0x8C;
}

void PWM2_Start(void)
{
    PIR4bits.TMR2IF = 0;

    // Start Timer2
    T2CON |= 0x80;

    // Wait for first Timer2 overflow before enabling output driver
    while (!PIR4bits.TMR2IF);

    PIR4bits.TMR2IF = 0;

    // Enable RD2 output driver
    TRISCbits.TRISC5 = 0;
}

// ---------------- Buzzer ----------------

void Buzzer_Off(void)
{
    BUZZER_LAT = 0;
}

uint16_t Buzzer_GetIntervalMs(uint16_t distance)
{
    uint16_t interval;

    // No echo or far away = silent
    if (distance == 999 || distance >= BUZZER_OFF_DISTANCE_CM)
    {
        return 0;
    }

    // Very close = fastest beep interval
    if (distance <= BUZZER_FAST_DISTANCE_CM)
    {
        return BUZZER_MIN_INTERVAL_MS;
    }

    /*
     * Map distance to beep interval:
     *
     * distance = 10 cm -> 80 ms interval
     * distance = 50 cm -> 700 ms interval
     *
     * As distance gets smaller, interval gets smaller,
     * so the buzzer beeps faster.
     */
    interval = BUZZER_MIN_INTERVAL_MS +
        (uint16_t)(
            ((uint32_t)(distance - BUZZER_FAST_DISTANCE_CM) *
            (BUZZER_MAX_INTERVAL_MS - BUZZER_MIN_INTERVAL_MS)) /
            (BUZZER_OFF_DISTANCE_CM - BUZZER_FAST_DISTANCE_CM)
        );

    return interval;
}

void Buzzer_Update(uint16_t distance, uint16_t tick_ms)
{
    static uint16_t elapsed_ms = 0;
    static bool buzzerOn = false;

    uint16_t interval;
    uint16_t offTime;

    interval = Buzzer_GetIntervalMs(distance);

    // Silent condition
    if (interval == 0)
    {
        BUZZER_LAT = 0;
        buzzerOn = false;
        elapsed_ms = 0;
        return;
    }

    // If object is in deploy range, make buzzer continuous
    if (distance <= DEPLOY_DISTANCE_CM)
    {
        BUZZER_LAT = 1;
        buzzerOn = true;
        elapsed_ms = 0;
        return;
    }

    elapsed_ms += tick_ms;

    if (buzzerOn)
    {
        // Turn off after short ON pulse
        if (elapsed_ms >= BUZZER_ON_TIME_MS)
        {
            BUZZER_LAT = 0;
            buzzerOn = false;
            elapsed_ms = 0;
        }
    }
    else
    {
        // Wait for OFF time, then start next beep
        if (interval > BUZZER_ON_TIME_MS)
        {
            offTime = interval - BUZZER_ON_TIME_MS;
        }
        else
        {
            offTime = 10;
        }

        if (elapsed_ms >= offTime)
        {
            BUZZER_LAT = 1;
            buzzerOn = true;
            elapsed_ms = 0;
        }
    }
}

void DelayMs_WithBuzzer(uint16_t distance, uint16_t total_ms)
{
    while (total_ms >= 10)
    {
        Buzzer_Update(distance, 10);
        __delay_ms(10);
        total_ms -= 10;
    }

    while (total_ms > 0)
    {
        __delay_ms(1);
        total_ms--;
    }
}
