#pragma config DEBUG = 1
#pragma config LVP = 0
#pragma config FCMEN = 0
#pragma config IESO = 0
#pragma config BOREN = 00
#pragma config CPD = 0
#pragma config CP = 0
#pragma config MCLRE = 0
#pragma config PWRTE = 1
#pragma config WDTE = 0
#pragma config FOSC = 101

#define _XTAL_FREQ 8000000

#include <xc.h>
#include <stdint.h>
#include <stdio.h>

char buffer[8];
const float resolution = 48.87585533;
uint16_t adc_value = 0;
uint32_t temperature = 0;

void read_adc() 
{
    adc_value = 0;
    ADCON0bits.GO_nDONE = 1;
    while (ADCON0bits.GO_nDONE == 1) {}
    NOP();
    adc_value |= ADRESH;
    adc_value = adc_value << 2;
    adc_value |= (ADRESL >> 6);
    
    temperature = (uint32_t) (adc_value * resolution);
}

void uart_write(char *string) {
    while (*string != '\0') {
        while (PIR1bits.TXIF == 0);
        TXREG = *string;
        string++;
    }
}

void main(void) {
    OSCCON = 0x79;
    
    ANSEL = 0x80;
    ANSELH = 0x00;
    ADCON0 = 0x9D;
    ADCON1 = 0x00;
    ADCON0 |= 0x01;
    
    TXSTAbits.TXEN = 1;
    TXSTAbits.SYNC = 0;
    TXSTAbits.BRGH = 1;
    
    RCSTAbits.SPEN = 1;
    SPBRG = 51;
    
    PORTC = 0x00;
    TRISC = 0x80;
    PORTE = 0x00;
    TRISE = 0x0C;
    
    while (1) {
        read_adc();
        sprintf(buffer, "%d%d.%d%d\r\n", 
                ((uint8_t)(temperature / 1000)),
                ((uint8_t)((temperature / 100) % 10)),
                ((uint8_t)((temperature / 10) % 10)),
                ((uint8_t)(temperature % 10))
                );
        uart_write(buffer);
        __delay_ms(1000);
    }
}
