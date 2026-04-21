//---------------------
// Title: Open lockbox with photoresistor code
//---------------------
// Program Details:
// Purpose of program is to open a lockbox (activate dc motor) when the secret code is inputted.
// Secret code is inputted by incrementing 2 counters by pressing on respective photoresistors.
// Correct code will active motor, incorrect code will turn on buzzer
//    
//   
// Inputs: VPR1 (RB1), VPR2 (RB2), Eswitch (RB0)
// Outputs: RD0-RD6 (seven segment), RB5 (5V relay), RB3 (power LED)
// Setup: The Curiosity Board
//    
// Date: 4/13/2025
// File Dependencies / Libraries:
//   C_PIC18F47K42.h
//   C_functions.h
//
// Compiler: xc8, 6.30
// Author: Steve Gutierrez
// Versions:
//       V1.0: Original
//       V1.1: Updates to IO port selection, some code functions
// Useful links: 
//       Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/PIC18(L)F26-27-45-46-47-55-56-57K42-Data-Sheet-40001919G.pdf 
//       PIC18F Instruction Sets: https://onlinelibrary.wiley.com/doi/pdf/10.1002/9781119448457.app4 
//       List of Instructions: http://143.110.227.210/faridfarahmand/sonoma/courses/es310/resources/20140217124422790.pdf 


#include "C:\Users\gutie\Documents\MPLAB resources\C_PIC18F47K42.h"
#include "Initialize.h"
#include "C_functions.h"

/* =========================
   GLOBAL VARIABLES
   ========================= */

volatile uint8_t Check_Emergency_SW = 0; // this flag is raised by the ISR so the main loop knows an emergency happened

// count # times PR activated
uint8_t PR1_Count = 0;                   
uint8_t PR2_Count = 0;                  

// count PR idle time after press
uint16_t PR1_DONE = 0;                   
uint16_t PR2_DONE = 0;                  

// PR debouncer
uint8_t PR1_Debounce = 0;                
uint8_t PR2_Debounce = 0;                

// store previous PR state to check for new press
bool PR1Prev = false;                  
bool PR2Prev = false;                    

// when the system starts, it always expects the first input from PR1
system_state_t SystemState = waitFor_PR1; 


/*
   7-SEGMENT LOOKUP TABLE
   bit0=A, bit1=B, ... bit6=G
*/

// set binary codes for 0-9 on seven segment as matrix
static const uint8_t Seg7_Digits[10] = 
{
    0b00111111, /* 0 */ 
    0b00000110, /* 1 */   
    0b01011011, /* 2 */   
    0b01001111, /* 3 */   
    0b01100110, /* 4 */  
    0b01101101, /* 5 */  
    0b01111101, /* 6 */   
    0b00000111, /* 7 */  
    0b01111111, /* 8 */  
    0b01100111  /* 9 */  
};

/*
   INTERRUPT SERVICE ROUTINE
   Emergency switch on RB0
*/

// ISR runs immediately when RB0 changes and triggers the emergency input
void __interrupt(irq(IRQ_IOC), base(8)) ISR_IOC(void) 
{  
    // did interrupt really come from the RB0 interrupt-on-change source
    if (PIR0bits.IOCIF && IOCBFbits.IOCBF0) 
    {
        IOCBFbits.IOCBF0 = 0;               // clears the RB0-specific interrupt flag so the same interrupt can be detected again later
        PIR0bits.IOCIF = 0;                 // clears the global IOC interrupt flag to complete interrupt servicing

        Check_Emergency_SW = 1;             // tells the main loop that an emergency button press happened
        SystemState = Emergency_Pressed;    // forces the program state into the emergency condition

        
    }
}

/* 
   MAIN
*/

// program starts here
void main(void)                             
{
    SYSTEM_Initialize();         
    Reset_To_Start();                       
    SYS_LED_On();                          

    while (1)                               // the main loop keeps the system running forever
    {
        if (Check_Emergency_SW)             // if the interrupt raised the emergency flag, handle that first before doing anything else
        {
            EmergencyOn();                  // clears the emergency condition and resets the program to a safe starting point
            continue;                       // skips the normal code-entry process for this loop cycle
        }

        Update_SensorsAndCounts();          // reads the photoresistors and updates any valid user input counts
        Process_System();                   // checks the current state and decides what should happen next

        __delay_ms(LOOP_DELAY_MS);          // slows the loop slightly so the timing logic works in controlled steps
    }
}

/*
   INITIALIZATION
*/

void SYSTEM_Initialize(void)                // prepares the whole system before the main loop begins
{
    GPIO_Initialize();                      // sets up pin directions and ensures the used pins behave digitally
    Emergency_Initialize();                 // enables the emergency switch interrupt system

    RELAY_Off();        // starts with relay OFF so the motor does not activate immediately at power-up
    BUZZER_Off();
    SEG_Clear();                            // clears the 7-segment so no random digit appears at startup
}

void GPIO_Initialize(void)                  // configures the hardware pins based on their role in the circuit
{
    ANSELB = 0x00;                          // disables analog mode on PORTB so the switch and relay pins work digitally
    ANSELD = 0x00;                          // disables analog mode on PORTD so the display outputs behave correctly

    SYS_LED_TRIS = 0;                       // makes the LED pin an output because the PIC must drive it
    PR1_TRIS = 1;                           // makes PR1 an input because the PIC only reads its voltage level
    PR2_TRIS = 1;                           // makes PR2 an input because the PIC only reads its voltage level
    E_SW_TRIS = 1;                        // makes the emergency switch pin an input because it is an external event source
    RELAY_TRIS = 0;                         // makes the relay control pin an output because the PIC drives the relay module
    BUZZER_TRIS = 0;            
    SEG_PORT_TRIS = 0x00;                   // makes all display lines outputs so the PIC can send digit patterns
    
    LATB = 0x00;                            // clears output latches on PORTB for the same reason
    LATD = 0x00;                            // clears output latches on PORTD so the display starts blank

    WPUBbits.WPUB0 = 1;                     // enables the weak pull-up on RB0 so the switch stays at a stable HIGH level when not pressed
}

void Emergency_Initialize(void)             // configures the interrupt behavior for the emergency switch
{
    IOCBNbits.IOCBN0 = 1;                   // tells the PIC to trigger an interrupt when RB0 goes from HIGH to LOW, which happens when the switch is pressed
    IOCBPbits.IOCBP0 = 0;                   // disables rising-edge interrupt because only the press event matters here

    IOCBFbits.IOCBF0 = 0;                   // clears any old interrupt flag on RB0 before the system begins
    PIR0bits.IOCIF = 0;                     // clears the global interrupt-on-change flag for a clean start

    PIE0bits.IOCIE = 1;                     // enables the interrupt-on-change source so RB0 events can actually trigger the ISR

    INTCON0bits.GIEH = 1;                   // enables high-priority interrupts so the emergency can interrupt normal execution immediately
    INTCON0bits.GIEL = 1;                   // enables low-priority interrupts too, completing the interrupt system setup
}

/* =========================
   BASIC OUTPUT FUNCTIONS
   ========================= */

void SYS_LED_On(void)                       // turns on the system indicator LED
{
    SYS_LED_LAT = 1;                        // drives the LED pin high so current can flow through the LED circuit
}

void SYS_LED_Off(void)                      // turns off the system indicator LED
{
    SYS_LED_LAT = 0;                        // removes the active output from the LED pin so the LED stops glowing
}

void RELAY_On(void)                         // activates the relay module
{
    RELAY_LAT = 0;                          // because the module is active-low, writing 0 energizes the relay input
}

void RELAY_Off(void)                        // deactivates the relay module
{
    RELAY_LAT = 1;                          // for active-low hardware, writing 1 returns the relay to its inactive state
}

void BUZZER_On(void)                         // activates the relay module
{
    BUZZER_LAT = 1;                          // active-high buzzer on
}

void BUZZER_Off(void)                        // deactivates the relay module
{
    BUZZER_LAT = 0;                          // active-high buzzer off
}

void Seg7_Display(uint8_t digit)            // sends one decimal digit to the 7-segment display
{
    uint8_t pattern = 0x00;                 // starts with all segments off until a valid digit pattern is selected

    if (digit <= 9U)                        // makes sure only valid decimal digits use the lookup table
    {
        pattern = Seg7_Digits[digit];       // selects the correct bit pattern for the chosen digit
    }

    SEG_PORT_LAT = pattern;                 // sends the normal pattern directly because the display is treated as common cathode
}

void SEG_Clear(void)                        // blanks the 7-segment display
{
    SEG_PORT_LAT = 0x00;                    // with common cathode, writing zeros turns all segments off
}

/* =========================
   INPUT FUNCTIONS
   ========================= */

// Check if PR is active (active-low)
bool PR1_IsActive(void)                  
{
    return (PR1_PORT == 0);                 
}

bool PR2_IsActive(void)                     
{
    return (PR2_PORT == 0);            
}

/* =========================
   RESET
   ========================= */

void Reset_InputData(void)                  // clears all user-entry values and helper timers
{
    PR1_Count = 0;                          // removes any previous first-digit count so the next code entry starts fresh
    PR2_Count = 0;                          // removes any previous second-digit count for the same reason

    PR1_DONE = 0;                           // clears the idle timer used to decide when PR1 entry is finished
    PR2_DONE = 0;                           // clears the idle timer used to decide when PR2 entry is finished

    PR1_Debounce = 0;                       // clears any debounce delay left from earlier PR1 triggers
    PR2_Debounce = 0;                       // clears any debounce delay left from earlier PR2 triggers

    PR1Prev = false;                        // resets the remembered PR1 state so edge detection starts cleanly
    PR2Prev = false;                        // resets the remembered PR2 state so edge detection starts cleanly
}

void Reset_To_Start(void)                   // returns the whole system to its normal starting condition
{
    Reset_InputData();                      // first clears all counters and helper values from the previous cycle
    SystemState = waitFor_PR1;              // then puts the state machine back to waiting for the first sensor input
    SEG_Clear();                            // clears the display so no old digit remains visible to the user
    RELAY_Off();                            // makes sure the motor output is off before a new attempt begins
}

/* =========================
   SENSOR / COUNTING LOGIC
   ========================= */

void Update_SensorsAndCounts(void)          // reads the sensors and updates valid counts only when a new trigger happens
{
    bool pr1_active = PR1_IsActive();       // gets the current active/inactive condition of PR1
    bool pr2_active = PR2_IsActive();       // gets the current active/inactive condition of PR2

    if (PR1_Debounce > 0)                   // if PR1 is still in its debounce period
    {
        PR1_Debounce--;                     // count down until PR1 is allowed to be accepted again
    }

    if (PR2_Debounce > 0)                   // if PR2 is still in its debounce period
    {
        PR2_Debounce--;                     // count down until PR2 is allowed to be accepted again
    }

    switch (SystemState)                    // counting rules depend on which part of the code entry the user is in
    {
        case waitFor_PR1:                   // first stage: only PR1 input is meaningful here
        {
            if (pr1_active && !PR1Prev && (PR1_Debounce == 0)) // counts only a new PR1 edge, not a sensor being held continuously
            {
                if (PR1_Count < 4U)         // stops the value from growing beyond the allowed range
                {
                    PR1_Count++;            // stores one more valid PR1 trigger
                    Seg7_Display(PR1_Count);// shows the current first-digit count so the user gets feedback
                }

                PR1_DONE = 0;               // resets the timeout counter because a fresh PR1 trigger just happened
                PR1_Debounce = DEBOUNCE_TICKS; // starts the debounce delay so noise or holding does not count again immediately
            }

            if (PR1_Count > 0U)             // once at least one PR1 entry exists, begin timing the pause after it
            {
                PR1_DONE++;                 // each loop increases the idle counter until the pause is long enough
            }

            break;                          // leaves this state after finishing the first-input handling
        }

        case STATE_WAIT_PR2:                // second stage: now only PR2 input is meaningful
        {
            if (pr2_active && !PR2Prev && (PR2_Debounce == 0)) // counts only a new PR2 edge, not a held condition
            {
                if (PR2_Count < 4U)         // keeps the second digit within the same allowed maximum
                {
                    PR2_Count++;            // stores one more valid PR2 trigger
                    Seg7_Display(PR2_Count);// shows the current second-digit count on the display
                }

                PR2_DONE = 0;               // resets the second-input timeout because a new PR2 trigger just happened
                PR2_Debounce = DEBOUNCE_TICKS; // starts the debounce timer to prevent accidental repeated counts
            }

            if (PR2_Count > 0U)             // once PR2 starts being entered, begin timing the pause after it
            {
                PR2_DONE++;                 // this allows the code to decide when the second digit is finished
            }

            break;                          // leaves this state after handling the second-input logic
        }

        default:                            // in all other states, sensors are not counted as code-entry inputs
            break;
    }

    PR1Prev = pr1_active;                   // stores current PR1 condition so next loop can detect a change properly
    PR2Prev = pr2_active;                   // stores current PR2 condition so next loop can detect a change properly
}

/* =========================
   MAIN STATE MACHINE
   ========================= */

void Process_System(void)                   // controls the overall behavior by moving between the program states
{
    switch (SystemState)                    // decides what should happen based on the current operating state
    {
        case waitFor_PR1:                   // first stage: system is waiting for the user to finish PR1 entry
        {
            /*
             * The first number is considered complete only after
             * PR1 has been triggered at least once and then stays idle
             * long enough to reach the timeout.
             */
            if ((PR1_Count > 0U) && (PR1_DONE >= DIGIT_DONE_TIMEOUT_TICKS)) // checks whether the first digit entry is finished
            {
                SystemState = STATE_WAIT_PR2; // once the first digit is done, move to waiting for the second digit
                PR2_DONE = 0;                 // clears PR2 timeout counter so the second stage starts fresh
            }
            break;                            // end of first-state processing
        }

        case STATE_WAIT_PR2:                  // second stage: system is waiting for the user to finish PR2 entry
        {
            if ((PR2_Count > 0U) && (PR2_DONE >= DIGIT_DONE_TIMEOUT_TICKS)) // checks whether the second digit entry is finished
            {
                SystemState = STATE_CHECK_CODE; // both digits are ready, so move on to compare them with the secret code
            }
            break;                            // end of second-state processing
        }

        case STATE_CHECK_CODE:                // compare what the user entered against the stored secret code
        {
            if ((PR1_Count == SECRET_CODE_PR1) && (PR2_Count == SECRET_CODE_PR2)) // both digits must match exactly for success
            {
                SystemState = Correct_Secret_Code; // marks the code as correct so the success action can run
            }
            else
            {
                SystemState = Wrong_Secret_Code;   // any mismatch sends the program to the wrong-code action
            }
            break;                                 // end of code-check state
        }

        case Correct_Secret_Code:                 // state reached when the entered code is correct
        {
            Handle_CorrectCode();                 // performs the success behavior for the current design
            Reset_To_Start();                     // clears everything so the next attempt starts from the beginning
            break;                                // end of success state
        }

        case Wrong_Secret_Code:                   // state reached when the entered code is wrong
        {
            Handle_WrongCode();                   // performs the failure behavior, which is the buzzer
            Reset_To_Start();                     // resets all values after the warning sound finishes
            break;                                // end of failure state
        }

        case Emergency_Pressed:                   // state entered after the ISR marks an emergency event
        {
            EmergencyOn();                        // handles the software-side emergency cleanup after the melody
            break;                                // end of emergency state
        }

        default:                                  // safety case in case the state ever becomes invalid unexpectedly
        {
            Reset_To_Start();                     // safest response is to reset to the known idle state
            break;                                // end of default state
        }
    }
}

/* =========================
   ACTION HANDLERS
   ========================= */

void Handle_CorrectCode(void)                // runs when the correct code is entered
{
    RELAY_On();                              // turns on the relay so the motor receives power
    DelayMs_Blocking(CORRECT_CODE_ON_MS);      // keeps the motor active long enough for the user to clearly hear the warning
    RELAY_Off();                             // turns the motor back off

    SEG_Clear();                             // clears the display so the next attempt starts without leftover numbers
}

void Handle_WrongCode(void)                  // runs when the entered code is incorrect
{
    BUZZER_On();                              // turns on buzzer
    DelayMs_Blocking(WRONG_CODE_ON_MS);      // keeps the buzzer active long enough for the user to clearly hear the warning
    BUZZER_Off();                             // turns the buzzer back off

    SEG_Clear();                             // clears the display so the next attempt starts without leftover numbers
}

void EmergencyOn(void)                       // runs after the interrupt-driven emergency behavior has finished
{
    // The relay is switched ON and OFF here to produce the emergency buzzer pattern.
        for (uint8_t i = 0; i < 5; i++)     // first group of emergency pulses
        {
            BUZZER_On();                     // energizes the relay so the buzzer sounds
            __delay_ms(200);                // keeps the buzzer active long enough to hear the first tone
            BUZZER_Off();                    // silences the buzzer between tones
            __delay_ms(200);                // creates spacing so the sound pattern is distinct
        }
    Check_Emergency_SW = 0;                  // clears the emergency flag so the main loop stops treating the event as active
    Reset_To_Start();                        // resets the whole program so it returns to normal operation from the beginning
}

/* =========================
   SIMPLE BLOCKING DELAY
   ========================= */

void DelayMs_Blocking(uint16_t ms)           // creates a manual blocking delay one millisecond at a time
{
    while (ms--)                             // repeats until the requested number of milliseconds has fully passed
    {
        __delay_ms(1);                       // each loop adds one millisecond, giving a simple controlled total delay
    }
}
