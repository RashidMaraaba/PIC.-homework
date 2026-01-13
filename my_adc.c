


#include "my_adc.h"

void init_adc_no_lib(void) {

    ADCON0 = 0;
    ADCON0bits.ADON = 1; // turn adc on 

    ADCON2 = 0b10001001; 

   
}



int read_adc_raw_no_lib(unsigned char channel) {
    int raw_value; //0--1023
    ADCON0bits.CHS = channel;

    //start conversion
    ADCON0bits.GO = 1; // start conversion

    while (ADCON0bits.GO) {}; // wait until conversion is done

    raw_value = ADRESH << 8 | ADRESL; // 10 bit, need to shift the bits right

  
    
    return raw_value;
}

float read_adc_voltage(unsigned char channel) {
    int raw_value;
    float voltage;
    raw_value = read_adc_raw_no_lib(channel);
    voltage = (raw_value * 5) / 1023.0;
    return voltage;
   

}

