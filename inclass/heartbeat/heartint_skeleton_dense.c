//heartint.c
//setup TCNT1 in pwm mode, TCNT3 in normal mode 
//set OC1A (PB5) as pwm output 
//pwm frequency:  (16,000,000)/(1 * (61440 + 1)) = 260hz
//
//Timer TCNT3 is set to interrupt the processor at a rate of 30 times a second.
//When the interrupt occurs, the ISR for TCNTR3 changes the duty cycle of timer 
//TCNT1 to affect the brightness of the LED connected to pin PORTB bit 5.
//
//to download: 
//wget http://www.ece.orst.edu/~traylor/ece473/inclass_exercises/timers_and_counters/heartint_skeleton.c
//

#include <avr/io.h>
#include <avr/interrupt.h>

#define TRUE  1
#define FALSE 0

//Traverse the array up then down with control statements or double the size of
//the array and make the control easier.  Values from 0x0100 thru 0xEF00 work 
//well for setting the brightness level.

uint16_t brightness[20] = {0xEF00, 50000, 41992, 31827, 24202, 18324, 13489, 9426, 5900, 2780, 255, 2780, 5900, 9426, 13489, 18324, 24202, 31827, 41992, 0xEF00} ;

ISR(TIMER3_OVF_vect) {

	//keeping track of the brightness level
	static uint8_t idx = 0;

	//Outputting the brightness level
	OCR1A = brightness[ (idx == 20) ? (idx=0) : (idx++) ];
}

int main() {

  DDRB    = (1<<PB5);                   //set port B bit five to output

//setup timer counter 1 as the pwm source
  TCCR1A |= ((1<<COM1A1 | 1<<COM1A0) | (1<<WGM11 | 0<<WGM10));  
  //fast pwm, set on match, clear@bottom, 
  //(inverting mode) ICR1 holds TOP

  TCCR1B |= (1<<CS10) | (1<<WGM13 | 1<<WGM12);
  //use ICR1 as source for TOP, use clk/1

  ICR1    = 0xF000;
  //clear at 0xF000                               

//setup timer counter 3 as the interrupt source, 30 interrupts/sec
// (16,000,000)/(8 * 2^16) = 30 cycles/sec

  TCCR3B = (1<<CS31); 
  //use clk/8  (15hz)  

  ETIMSK = (1<<TOIE3);
  //enable timer 3 interrupt on TOV

  sei();                                //set GIE to enable interrupts
  while(1) { } //do forever
 
}  // main
