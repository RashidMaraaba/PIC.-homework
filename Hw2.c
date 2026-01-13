#include <xc.h>
#include <stdio.h>
#include "my_adc.h"
#include "my_pwm.h"
#include "lcd_x8.h"

#define _XTAL_FREQ 4000000UL


/* ============================================================
   ENUMERATIONS
   ============================================================ */
typedef enum {
    MODE_OFF = 0,
    MODE_HEAT = 1,
    MODE_COOL = 2,
    MODE_AUTO_COOL = 3
} Mode;

typedef enum {
    HS_0 = 0,
    HS_1 = 1,
    HS_2 = 2,
    HS_3 = 3
} Hysteresis;


/* ============================================================
   GLOBAL VARIABLES
   ============================================================ */
volatile Mode currentMode = MODE_OFF;
volatile Hysteresis Hs = HS_0;

// Interrupt flags (debounced in main loop)
volatile unsigned char mode_change_flag = 0;
volatile unsigned char hs_change_flag   = 0;
volatile unsigned char off_flag         = 0;

// Analog raw readings
unsigned int raw_AI0, raw_AI1, raw_AI2;

// Derived values
float SP, T, HC;
float AI0_voltage, AI1_voltage, AI2_voltage;

// Device state indicators
unsigned char heaterOn = 0;
unsigned char coolerOn = 0;

// LCD buffer
char Buffer[32];


/* ============================================================
   FUNCTION PROTOTYPES
   ============================================================ */
void initPorts(void);
void setupInterrupts(void);
void setupCompare(void);

void OffMode(void);
void HeatMode(void);
void CoolMode(void);
void AutoCoolMode(void);

void printingOnLCD(void);


/* ============================================================
   MAIN APPLICATION LOOP
   ============================================================ */
void main(void)
{
    initPorts();
    setupInterrupts();
    lcd_init();
    init_adc_no_lib();
    setupCompare();
    init_pwm1();

    while (1)
    {
        CLRWDT();

        /* ------------------------------
           Read ADC and compute values
           ------------------------------ */
        raw_AI0 = read_adc_raw_no_lib(0);
        raw_AI1 = read_adc_raw_no_lib(1);
        raw_AI2 = read_adc_raw_no_lib(2);

        AI0_voltage = (raw_AI0 * 5.0f) / 1023.0f;
        AI1_voltage = (raw_AI1 * 5.0f) / 1023.0f;
        AI2_voltage = (raw_AI2 * 5.0f) / 1023.0f;

        SP = (AI0_voltage / 5.0f) * 100.0f;   // 0?100°C
        T  = AI2_voltage * 100.0f;           // Sensor scaling
        HC = (AI1_voltage / 5.0f) * 100.0f;  // Percentage


        /* ------------------------------
           Handle button events (debounced)
           ------------------------------ */
        if (mode_change_flag)
        {
            mode_change_flag = 0;
            delay_ms(30);

            if (!PORTBbits.RB0)
            {
                currentMode = (currentMode + 1) & 0x03;   // cycle through 0?3
                while (!PORTBbits.RB0) CLRWDT();
                delay_ms(50);
            }
        }

        if (hs_change_flag)
        {
            hs_change_flag = 0;
            delay_ms(30);

            if (!PORTBbits.RB1)
            {
                Hs = (Hs + 1) & 0x03;   // cycle 0?3
                while (!PORTBbits.RB1) CLRWDT();
                delay_ms(50);
            }
        }

        if (off_flag)
        {
            off_flag = 0;
            delay_ms(30);

            if (!PORTBbits.RB2)
            {
                currentMode = MODE_OFF;
                while (!PORTBbits.RB2) CLRWDT();
                delay_ms(50);
            }
        }


        /* ------------------------------
           Refresh LCD
           ------------------------------ */
        printingOnLCD();


        /* ------------------------------
           Execute current mode
           ------------------------------ */
        switch (currentMode)
        {
            case MODE_OFF:        OffMode();       break;
            case MODE_HEAT:       HeatMode();      break;
            case MODE_COOL:       CoolMode();      break;
            case MODE_AUTO_COOL:  AutoCoolMode();  break;
            default:              OffMode();       break;
        }

        delay_ms(50);
    }
}


/* ============================================================
   PORT INITIALIZATION
   ============================================================ */
void initPorts(void)
{
    ADCON1 = 0b00001100;

    LATA = LATB = LATC = LATD = LATE = 0;

    TRISA = 0xFF;   // ADC inputs
    TRISB = 0xFF;   // Buttons
    TRISC = 0x00;   // Outputs (heater/cooler)
    TRISD = 0x00;   // LCD
    TRISE = 0x00;

    PORTCbits.RC5 = 0;  // heater
    PORTCbits.RC2 = 0;  // cooler
}


/* ============================================================
   INTERRUPT CONFIGURATION
   ============================================================ */
void setupInterrupts(void)
{
    RCONbits.IPEN = 0;

    // INT0 (RB0)
    INTCON2bits.INTEDG0 = 0;
    INTCONbits.INT0IF = 0;
    INTCONbits.INT0IE = 1;

    // INT1 (RB1)
    INTCON2bits.INTEDG1 = 0;
    INTCON3bits.INT1IF = 0;
    INTCON3bits.INT1IE = 1;

    // INT2 (RB2)
    INTCON2bits.INTEDG2 = 0;
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT2IE = 1;

    // Heater compare & timer interrupts
    PIE1bits.TMR1IE = 0;
    PIR1bits.TMR1IF = 0;

    PIE2bits.CCP2IE = 0;
    PIR2bits.CCP2IF = 0;

    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
}


/* ============================================================
   COMPARE MODULE (HEATER)
   ============================================================ */
void setupCompare(void)
{
    CCP2CON = 0x09;         // Compare mode 9
    T1CON = 0x00;
    T1CONbits.TMR1CS = 0;
    T1CONbits.T1CKPS = 0b10;
    T1CONbits.RD16  = 1;

    TMR1 = 0;
    T1CONbits.TMR1ON = 1;
}


/* ============================================================
   INTERRUPT SERVICE ROUTINES
   ============================================================ */
void __interrupt(high_priority) highIsr(void)
{
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        mode_change_flag = 1;
    }

    if (INTCON3bits.INT1IF) {
        INTCON3bits.INT1IF = 0;
        hs_change_flag = 1;
    }

    if (INTCON3bits.INT2IF) {
        INTCON3bits.INT2IF = 0;
        off_flag = 1;
    }

    if (PIR2bits.CCP2IF) {
        PIR2bits.CCP2IF = 0;
        PORTCbits.RC5 = 0;   // heater OFF
    }

    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        PORTCbits.RC5 = 1;   // heater ON
        TMR1 = 0;
    }
}


/* ============================================================
   MODE LOGIC
   ============================================================ */

void OffMode(void)
{
    heaterOn = 0;
    coolerOn = 0;

    PORTCbits.RC5 = 0;
    PORTCbits.RC2 = 0;

    CCP1CON = 0x00;
    PIE1bits.TMR1IE = 0;
    PIE2bits.CCP2IE = 0;
}


void HeatMode(void)
{
    heaterOn = 1;
    coolerOn = 0;

    // Cooler OFF
    CCP1CON = 0x00;
    PORTCbits.RC2 = 0;

    // Enable heater control
    PIE1bits.TMR1IE = 1;
    PIE2bits.CCP2IE = 1;

    unsigned int compare_value = raw_AI1 * 64u;

    CCPR2H = compare_value >> 8;
    CCPR2L = compare_value & 0xFF;
}


void CoolMode(void)
{
    heaterOn = 0;
    coolerOn = 1;

    PORTCbits.RC5 = 0;

    PIE1bits.TMR1IE = 0;
    PIE2bits.CCP2IE = 0;

    T2CONbits.T2CKPS = 0b11;
    T2CONbits.TMR2ON = 1;

    init_pwm1();
    set_pwm1_raw(raw_AI1);
}


void AutoCoolMode(void)
{
    float coolError = T - SP;

    if (coolError > 0.0)
    {
        CoolMode();
    }
    else if (T < (SP - Hs))
    {
        HeatMode();
        }
    else
    {
        // Within hysteresis band
        PORTCbits.RC5 = 0;
        PORTCbits.RC2 = 0;

        CCP1CON = 0x00;
        PIE1bits.TMR1IE = 0;
        PIE2bits.CCP2IE = 0;
    }
}


/* ============================================================
   LCD DISPLAY
   ============================================================ */
void printingOnLCD(void)
{
    char h = heaterOn ? 'Y' : 'N';
    char c = coolerOn ? 'Y' : 'N';

    lcd_gotoxy(1,1);
    sprintf(Buffer, "RT:%4.1fC   H C", T);
    lcd_puts(Buffer);

    lcd_gotoxy(1,2);
    sprintf(Buffer, "SP:%4.1fC   %c %c", SP, h, c);
    lcd_puts(Buffer);

    lcd_gotoxy(1,3);
    sprintf(Buffer, "HS:%d  HC:%4.1f%%", Hs, HC);
    lcd_puts(Buffer);

    lcd_gotoxy(1,4);
    switch (currentMode)
    {
        case MODE_OFF:        lcd_puts("MD: OFF       "); break;
        case MODE_HEAT:       lcd_puts("MD: HEAT      "); break;
        case MODE_COOL:       lcd_puts("MD: COOL      "); break;
        case MODE_AUTO_COOL:  lcd_puts("MD: AUTO COOL "); break;
    }
}