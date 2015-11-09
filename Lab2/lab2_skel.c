// lab2_skel.c 
// R. Traylor
// 9.12.08

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#include <avr/io.h>
#include <util/delay.h>

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];

//decimal to 7-segment LED display encodings, logic "0" turns on segment
//This is based on ACTIVE HIGH. 
//STILL NEED ~ and >>1 or <<1;
uint8_t dec_to_7seg[19]={
	0xC0,//0
	0xF9,//1
	0xA4,//2
	0xB0,//3
	0x99,//4
	0x92,//5
	0x82,//6 
	0xF8,//7
	0x80,//8
	0x98,//9
	0xFF,//OFF (10)
	0x7F};//Full Colon (11)
//Contributors of 7seg decoder: Keenan Bishop, Matt ????, Bear

//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 
//
uint8_t chk_buttons(uint8_t button) {
	//******************************************************************************
//	static uint16_t state[8] = {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000}; //holds present state
//	TODO: I swear the array was the problem. Not sure why...
//	TODO: and the ! in front of the bit_is_clear
	static uint16_t state = 0; //holds present state
//	state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
	state = (state << 1) | ( bit_is_clear(PINA, button)) | 0xE000;
	if (state == 0xF000) return 1;
	return 0;

}
//***********************************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|
void segsum(uint16_t val) {
	//if(sum > 9999)
	//determine how many digits there are 
	//Filling in backward
	uint8_t i;
	for( i=0; (val/10) > 0 ; ++i){
		segment_data[i] = val%10;
		val /= 10;
	}
	if(val != 0){
		segment_data[i] = val;
		val = 0;
		//TODO: Why if I move this line off of the if(val...) it wouldn't work?
		//if I do that my max_i is 2....
		for(uint8_t j=i+1; j<5; ++j){
			//	segment_data[j] = 0xC0;
			segment_data[j] = 0xA;
		}
	}
	//Translate to 7seg
	segment_data[4] = segment_data[3];
	segment_data[3] = segment_data[2];
	//segment_data[2] = (dec_to_7seg[segment_data[2]]);
	//TODO: Why doesn't the line above work even if in main I have no dec_to_7seg?
	segment_data[2] = 0xFF; 
	segment_data[1] = segment_data[1];
	segment_data[0] = segment_data[0];

	//now move data to right place for misplaced colon position
}//segment_sum
//***********************************************************************************


//***********************************************************************************
uint8_t main()
{
	uint16_t sum=0;
	uint8_t  digit=0;
	//set port bits 4-7 B as outputs
	DDRB = 0xF0;
	PORTB = 0x00;
	while(1){
		//insert loop delay for debounce
		_delay_ms(3);
		//make PORTA an input port with pullups 
		DDRA = 0x00;
		PORTA = 0xFF;
		//enable tristate buffer for pushbutton switches
		PORTB |= 0x70;
		//now check each button and increment the count as needed
		for(uint8_t i=0; i<8; ++i){
			sum += chk_buttons(i) << i;
		}
		//disable tristate buffer for pushbutton switches
		PORTB &= 0x5F;
		//bound the count to 0 - 1023
		if(sum>1023){
			//sum = (sum+1)%1024;
			sum = 0;
			segment_data[0] = 0x01;
			segment_data[1] = 0x00;
			segment_data[2] = 0x00;
			segment_data[3] = 0x00;
			segment_data[4] = 0x00;
		}
//		sum %= 1024;
		//break up the disp_value to 4, BCD digits in the array: call (segsum)
		segsum(sum);
		//make PORTA an output
		DDRA = 0xFF;
		//prevent ghosting
		PORTA = 0x00;
		//send 7 segment code to LED segments
		//TODO: why did I put dec_to_7seg here and it works?
		PORTA = dec_to_7seg[segment_data[digit]];
		//send PORTB the digit to display
		PORTB = ((0x8<<4) & PORTB)|(digit<<4); 
		//update digit to display
		digit = (++digit)%5;

	}//while
	return 0;
}//main
