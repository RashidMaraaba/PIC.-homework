
#include "my_pwm.h"


void init_pwm1(void)
{
    PR2 = 255; //10 bit accurcy
    T2CON = 0;
    CCP1CON = 0x0C; // Select PWM mode 0b00001100
   
    T2CONbits.TMR2ON = 1;  // turn the timer on 
    TRISCbits.RC2 =0;  //CCP1 is output
}
void set_pwm1_raw(unsigned int raw_value)
{
    CCPR1L = (raw_value >> 2) & 0x00FF; 
    CCP1CONbits.DC1B = raw_value & 0x0003; 
    
   
}
void set_pwm1_percent(float value)
{
    float tmp = value*1023.0/100.0;//scale 0--100 
    int raw_val = (int)(tmp +0.5); 
    if ( raw_val> 1023) raw_val = 1023;
    set_pwm1_raw(raw_val);
}
void set_pwm1_voltage(float value)
{
    float tmp = value*1023.0/5.0; 
    int raw_val = (int)(tmp +0.5); 
    if ( raw_val> 1023) raw_val = 1023; 
    set_pwm1_raw(raw_val);
    
}

void set_pwm1_general(float value, float min, float max)
{
    float tmp = (value - min)*1023.0/(max - min); 
    int raw_val = (int)(tmp +0.5);
    if ( raw_val> 1023) raw_val = 1023;
    set_pwm1_raw(raw_val);
    
}
