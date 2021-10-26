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

#define HEATER_PR2 255
#define HEATER_CONTROL PORTDbits.RD7 = PORTDbits.RD6

#define FAN_PR2 100
#define FAN_CONTROL PORTDbits.RD5 = PORTDbits.RD4

#define PLAY_SOUND PIE1bits.TMR2IE = 1; T2CONbits.TMR2ON = 1;
#define STOP_SOUND PIE1bits.TMR2IE = 0; T2CONbits.TMR2ON = 0; PORTDbits.RD7 = 0;

char buzzer = 0;
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

// Poor mans PWM
void __interrupt() play_sound() {
    if (PIR1bits.TMR2IF == 1) {
        buzzer = !buzzer;
        PORTCbits.RC3 = buzzer;
        PIR1bits.TMR2IF = 0;
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
    PORTD = 0x00;
    TRISD = 0x00;
    PORTE = 0x00;
    TRISE = 0x0C;
    
    /**
     * ------------------------------
     *  Setup to play sound using PWM
     * ------------------------------
     */
    // Prescaler 16 - TMR2 off
    T2CON = 0x03;
    
    // Enable interrupts
    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    
    
    read_adc();
    temperature = 0;
    while (1) {
        read_adc();
        sprintf(buffer, "%d%d.%d%d\r\n", 
                ((uint8_t)(temperature / 1000)),
                ((uint8_t)((temperature / 100) % 10)),
                ((uint8_t)((temperature / 10) % 10)),
                ((uint8_t)(temperature % 10))
                );
        uart_write(buffer);
        
        if (temperature >= 6000) {
            FAN_CONTROL = 1;
            PR2 = FAN_PR2;
            PLAY_SOUND;
        }
        else {
            FAN_CONTROL = 0;
        }
        if (temperature  <= 1500) {
            HEATER_CONTROL = 1;
            PR2 = HEATER_PR2;
            PLAY_SOUND;
        }
        else {
            HEATER_CONTROL = 0;
        }
        if ((temperature < 6000) && (temperature > 1500)) {
            STOP_SOUND;
        }
    }
}
