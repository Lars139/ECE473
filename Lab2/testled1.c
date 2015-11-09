// testled1.c
// R. Traylor
// 10.3.05
// Tests wiring of LED board to mega128.
// Select a digit with PORTB upper nibble then with
// port D push buttons illuminate a single segment.

// Port mapping:
// Port A:  bit0 brown  segment A
//          bit1 red    segment B
//          bit2 orange segment C
//          bit3 yellow segment D
//          bit4 green  segment E
//          bit5 blue   segment F
//          bit6 purple segment G
//          bit7 grey   decimal point
//               black  Vdd
//               white  Vss

// Port B:  bit4 green  seg0
//          bit5 blue   seg1
//          bit6 purple seg2
//          bit7 grey   pwm 
//               black  Vdd
//               white  Vss

#include <avr/io.h>
#include <util/delay.h>
int main()
{
DDRA  = 0xFF;   //set port A to all outputs
DDRB  = 0xF0;   //set port bits 4-7 B as outputs
DDRD  = 0x00;   //set port D all inputs 
PORTD = 0xFF;   //set port D all pullups 
PORTA = 0x01;   //set port A to all ones  (off, active low)

uint32_t temp=0;
while(1){
  PORTB = 0x00; //digit zero  on 
//  PORTB = 0x10; //digit one   on 
//  PORTB = 0x20; //colon, indicator leds  on
//  PORTB = 0x30; //digit two   on 
//  PORTB = 0x40; //digit three on 
  PORTA = 0xFF & 1<<((temp++)%8); //push button determines which segment is on
  _delay_ms(5000);
  } //while
}  //main
