#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// CONFIG bits same as before
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

// reset pins 
#define RESET_BUTTON PORTDbits.RD3
#define LED LATDbits.LATD2

// LCD pins
#define RS LATDbits.LATD0
#define EN LATDbits.LATD1
#define LCD_DATA LATB

// I2C pins
#define SCL_LAT LATCbits.LATC3
#define SDA_LAT LATCbits.LATC4
#define SCL_TRIS TRISCbits.TRISC3
#define SDA_TRIS TRISCbits.TRISC4
#define SDA_PORT PORTCbits.RC4

#define MPU_ADDR 0x68

/* FUNCTION PROTOTYPES*/

void Button_Interrupt_Init(void);

void LCD_Init(void);
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_String(const char *msg);
void LCD_String_xy(unsigned char row, unsigned char pos, const char *msg);
void LCD_Clear(void);

void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
unsigned char I2C_Write(unsigned char data);
unsigned char I2C_Read(unsigned char ack);

void MPU6050_Init(void);
void MPU6050_Write(unsigned char reg, unsigned char data);
int16_t MPU6050_Read16(unsigned char reg);

void format_accel(char *buffer, int32_t value);

/*Program starts here*/

void main(void)
{
    char line1[21];
    char line2[21];

    int16_t rawX;
    int32_t accelX;
    int32_t lastAccelX = 0;
    int32_t delta;

    
    
    LCD_Init();
    Button_Interrupt_Init();
    I2C_Init();
    MPU6050_Init();

    while(1)
    {
        rawX = MPU6050_Read16(0x3B);

        // Convert raw accelerometer data to m/s^2
        // MPU6050 default range is +/-2g
        // sensitivity = 16384 LSB/g
        accelX = ((int32_t)rawX * 98067L) / 16384L;

        delta = accelX - lastAccelX;
        if(delta < 0)
            delta = -delta;

        if(delta > 35000)
        {
            sprintf(line1, "angle: Shake      ");
        }
        else if(accelX > 20000)
        {
            sprintf(line1, "angle: Tilt_Right ");
        }
        else if(accelX < -20000)
        {
            sprintf(line1, "angle: Tilt_Left  ");
        }
        else
        {
            sprintf(line1, "angle: Flat       ");
        }

        format_accel(line2, accelX);

        LCD_Clear();
        LCD_String_xy(1, 0, line1);
        LCD_String_xy(2, 0, line2);

        lastAccelX = accelX;

        __delay_ms(500);
    }
}

/* Interrupt w reset button*/

void __interrupt(irq(INT0), base(8)) INT0_ISR(void)
{
    PIR1bits.INT0IF = 0;

    __delay_ms(20);   // debounce
    
    
    if(PORTDbits.RD3 == 0)
    {
        LCD_Clear(); // clear 
        
        // Blink LED for ~10 seconds
        for(int i = 0; i < 10; i++)
        {
            LED = 1;
            __delay_ms(500);

            LED = 0;
            __delay_ms(500);
        }

        RESET();   // reset after blinking
    }
}

void Button_Interrupt_Init(void)
{
    ANSELDbits.ANSELD3 = 0;   // RD3 digital input
    TRISDbits.TRISD3 = 1;
    
    ANSELDbits.ANSELD2 = 0;   // digital
    TRISDbits.TRISD2 = 0;     // output
    LED = 0;                  // start OFF
    INT0PPS = 0x1B;           // Map RD3 to INT0

    INTCON0bits.INT0EDG = 0;  // Falling edge: button pulls RD3 LOW

    PIR1bits.INT0IF = 0;      // Clear INT0 flag
    PIE1bits.INT0IE = 1;      // Enable INT0 interrupt

    INTCON0bits.GIE = 1;      // Enable global interrupts
}

/*LCD and accelerometer functions*/

void format_accel(char *buffer, int32_t value)
{
    int32_t whole;
    int32_t decimal;

    // value is scaled by 10000
    // example: 2434 means 0.2434 m/s^2

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

/**************** LCD FUNCTIONS ****************/

void LCD_Init(void)
{
    ANSELB = 0x00;
    ANSELD = 0x00;

    TRISB = 0x00;
    TRISD &= 0xFC;

    LATB = 0x00;
    LATD &= 0xFC;

    __delay_ms(20);

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

/**************** SOFTWARE I2C FUNCTIONS ****************/

void I2C_Init(void)
{
    ANSELCbits.ANSELC3 = 0;
    ANSELCbits.ANSELC4 = 0;

    SCL_LAT = 0;
    SDA_LAT = 0;

    // Release both lines high through pull-up resistors
    SCL_TRIS = 1;
    SDA_TRIS = 1;
}

void I2C_Delay(void)
{
    __delay_us(5);
}

void I2C_Start(void)
{
    SDA_TRIS = 1;
    SCL_TRIS = 1;
    I2C_Delay();

    SDA_TRIS = 0;
    I2C_Delay();

    SCL_TRIS = 0;
    I2C_Delay();
}

void I2C_Stop(void)
{
    SDA_TRIS = 0;
    SCL_TRIS = 1;
    I2C_Delay();

    SDA_TRIS = 1;
    I2C_Delay();
}

unsigned char I2C_Write(unsigned char data)
{
    unsigned char i;
    unsigned char ack;

    for(i = 0; i < 8; i++)
    {
        if(data & 0x80)
            SDA_TRIS = 1;
        else
            SDA_TRIS = 0;

        SCL_TRIS = 1;
        I2C_Delay();

        SCL_TRIS = 0;
        I2C_Delay();

        data <<= 1;
    }

    SDA_TRIS = 1;
    SCL_TRIS = 1;
    I2C_Delay();

    ack = SDA_PORT;

    SCL_TRIS = 0;
    I2C_Delay();

    return ack;
}

unsigned char I2C_Read(unsigned char ack)
{
    unsigned char i;
    unsigned char data = 0;

    SDA_TRIS = 1;

    for(i = 0; i < 8; i++)
    {
        data <<= 1;

        SCL_TRIS = 1;
        I2C_Delay();

        if(SDA_PORT)
            data |= 1;

        SCL_TRIS = 0;
        I2C_Delay();
    }

    if(ack)
        SDA_TRIS = 0;
    else
        SDA_TRIS = 1;

    SCL_TRIS = 1;
    I2C_Delay();

    SCL_TRIS = 0;
    SDA_TRIS = 1;
    I2C_Delay();

    return data;
}

/**************** MPU6050 FUNCTIONS ****************/

void MPU6050_Init(void)
{
    __delay_ms(100);

    MPU6050_Write(0x6B, 0x00); // Wake up MPU6050
    MPU6050_Write(0x1C, 0x00); // Accelerometer +/-2g
    MPU6050_Write(0x1B, 0x00); // Gyroscope +/-250 deg/s
}

void MPU6050_Write(unsigned char reg, unsigned char data)
{
    I2C_Start();
    I2C_Write((MPU_ADDR << 1) | 0);
    I2C_Write(reg);
    I2C_Write(data);
    I2C_Stop();
}

int16_t MPU6050_Read16(unsigned char reg)
{
    unsigned char high;
    unsigned char low;

    I2C_Start();
    I2C_Write((MPU_ADDR << 1) | 0);
    I2C_Write(reg);

    I2C_Start();
    I2C_Write((MPU_ADDR << 1) | 1);

    high = I2C_Read(1);
    low = I2C_Read(0);

    I2C_Stop();

    return ((int16_t)high << 8) | low;
}