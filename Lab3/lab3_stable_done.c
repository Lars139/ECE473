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
#include <avr/interrupt.h>
#include <util/delay.h>

//-------------------Encoder control
#define NUM_OF_ENCO 2
#define ENCO_CW 1
#define ENCO_CCW 2

//////////////////////////////////// GLOBAL VAR /////////////////////////////////////


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

uint16_t sum=0;
uint8_t  digit=0;
uint8_t  inc_step = 1;
//Contributors of 7seg decoder: Keenan Bishop, Matt ????, Bear

//Stages for the buttons
enum stages {NONE=0xFF, INC_1=0x01, INC_2=0x02, INC_4=0x04} mode;

uint8_t first_run_zero = 1;

//Keep tracked of the pressed button
uint8_t pressed_button = 0x00;

   uint8_t ret_enc = 0;
   uint8_t spdr_val = 0;
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
   static uint16_t state = 0; //holds present state
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
      //if I do that my max_i is 2....
      for(uint8_t j=i+1; j<5; ++j){
	 //	segment_data[j] = 0xC0;
	 segment_data[j] = 0xA;
      }
   }else{//In case val == 0
      if(first_run_zero){
	 segment_data[3] = 0x0 ;
	 segment_data[2] = 0x0;
	 segment_data[1] = 0x0;
	 segment_data[0] = 0x0;

      }else{
	 segment_data[3] = 0x1 ;
	 segment_data[2] = 0x0;
	 segment_data[1] = 0x2;
	 segment_data[0] = 0x3;

      }
   }
   //Translate to 7seg
   segment_data[4] = segment_data[3];
   segment_data[3] = segment_data[2];
   //segment_data[2] = (dec_to_7seg[segment_data[2]]);
   segment_data[2] = 0xFF; 
   segment_data[1] = segment_data[1];
   segment_data[0] = segment_data[0];
//Prevent ghosting
   //FIXME: This line prevent ghosting but the button works mcuh slower
   PORTB = ((0x8<<4) & PORTB)|(5<<4); 

   //now move data to right place for misplaced colon position
}//segment_sum

/***********************************************************************/
//                            spi_init                               
//Initalizes the SPI port on the mega128. Does not do any further   
//external device specific initalizations.  Sets up SPI to be:                        
//master mode, clock=clk/2, cycle half phase, low polarity, MSB first
//interrupts disabled, poll SPIF bit in SPSR to check xmit completion
/***********************************************************************/
void spi_init(void){
   DDRB  |=  (1<<PB2) | (1<<PB1) | (1<<PB0);           //Turn on SS, MOSI, SCLK
   SPCR  |=  ( 1<<SPE | 1<<MSTR );  //set up SPI mode
   SPSR  |=  (1<<SPI2X);           // double speed operation
}//spi_init

/***********************************************************************/
//                              tcnt0_init                             
//Initalizes timer/counter0 (TCNT0). TCNT0 is running in async mode
//with external 32khz crystal.  Runs in normal mode with no prescaling.
//Interrupt occurs at overflow 0xFF.
//
void tcnt0_init(void){
   TIMSK |= (1<<TOIE0);
   TCCR0 |= (1<<CS02 | 1<<CS00);
}

//////////////////////////////////////////////////////////// MY CUSTOM FUNCTIONS

/* Func: read_encoder
 * Desc: return value if the encoder is rotating CW or CCW
 *
 * Input : enco_num  - Number of Encoder {0,1,...}
 *         SPDR      - value of current SPDR
 *         
 * Output: 0 - Idle
 *         1 - Encoder1 has rotated CW  1 unit
 *         2 - Encoder1 has rotated CCW 1 unit
 */
uint8_t read_encoder(uint8_t enco_num, uint8_t spdr_val){
   static uint8_t enco_lookup_table[] = {0,1,2,0,2,0,0,1,1,0,0,2,0,2,1,0};
   static uint8_t enco_trend[NUM_OF_ENCO] = {0};
   static uint8_t enco_history[NUM_OF_ENCO] = {0};

   uint8_t cur_enco = 0;
   uint8_t idx = 0;
   uint8_t dir = 0;
   uint8_t out = 0;

   idx = (enco_history[enco_num]<<2) | ( cur_enco=(((0x3<<(enco_num*2)) & spdr_val)>>(enco_num*2)) );
   dir = enco_lookup_table[idx];
   (dir == ENCO_CW) ? (++(enco_trend[enco_num])): 0; 
   (dir == ENCO_CCW) ? (--(enco_trend[enco_num])): 0; 

   if( cur_enco == 0b11 ){
      if( (enco_trend[enco_num]>1) && (enco_trend[enco_num]<100) ){
	 out = ENCO_CW;
	 first_run_zero = 0;
      }
      if( (enco_trend[enco_num]<=0xFF) && (enco_trend[enco_num]>0x90) ){
	 out = ENCO_CCW;
	 first_run_zero = 0;
      }
      enco_trend[enco_num] = 0;
   }
   enco_history[enco_num] = cur_enco;

   return out;
}


///////////////////////////////////////////////////////////  ISR_SECTION

ISR(TIMER0_OVF_vect){
   //------------------------------------------------------- BUTTON_&_7-SEG
   //make PORTA an input port with pullups 
   DDRA = 0x00;
   PORTA = 0xFF;
   //enable tristate buffer for pushbutton switches
   PORTB = 0x70;
   //now check each button and increment the count as needed
   for(uint8_t i=0; i<8; ++i){
      if(chk_buttons(i))
	 pressed_button ^= (1<<i);
   }
   //disable tristate buffer for pushbutton switches
   //To do so, we use enable Y5 on the decoder (NC)
   PORTB &= 0x5F;

   //----------------------------------------------------------- SPI_SECTION 

   //Send the value to read_encoder(num_enco, spdr_val);
   //Store the returning value to decide if inc or dec
   ret_enc = read_encoder(0, spdr_val);
   //Inc/Dec the sum accordingly
   if(ret_enc == ENCO_CW){ //CW adding the sum
      sum += inc_step;
   }else if(ret_enc == ENCO_CCW){
      sum -= inc_step;
   }

   ret_enc = read_encoder(1, spdr_val);
   //Inc/Dec the sum accordingly
   if(ret_enc == ENCO_CW){ //CW adding the sum
      sum += inc_step;
   }else if(ret_enc == ENCO_CCW){
      sum -= inc_step;
   }

   //----------------------------------------------------------- STATE_MACHINE 
   switch(mode){

      case INC_1:
	 if( (pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = NONE;
	 }else if(   (pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_4;
	 }else if(  !(pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = INC_2;
	 }else if(  !(pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_1;
	 }
	 inc_step = 1;


	 break;

      case INC_2:
	 if( (pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = NONE;
	 }else if(   (pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_4;
	 }else if(  !(pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = INC_2;
	 }else if(  !(pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_1;
	 }

	 inc_step = 2;

	 break;

      case INC_4:
	 if( (pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = NONE;
	 }else if(   (pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_4;
	 }else if(  !(pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = INC_2;
	 }else if(  !(pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_1;
	 }

	 inc_step = 4;
	 break;
      case NONE:
	 //Only care about S1, S0  button
	 if( (pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = NONE;
	 }else if(  (pressed_button & (1<<1)) &&
	       !(pressed_button & (1<<0))    ){
	    mode = INC_4;
	 }else if(  !(pressed_button & (1<<1)) &&
	       (pressed_button & (1<<0))    ){
	    mode = INC_2;
	 }
	 inc_step = 0;


	 break;

      default:
	 mode = INC_1;
	 break;
   }//End Here -- Switch(mode)

   return;
}



////////////////////////////////////////////////////////////////////////  MAIN
uint8_t main(){
   tcnt0_init();
   spi_init();
   sei();

   //PORTB=[ pwm | encd_sel2 | encd_sel1 | encd_sel0 | MISO | MOSI | SCK | SS ]
   //set port bits 4-7 B as outputs
   mode = INC_1;

   //enum stages {NONE=0xFF, INC_1=0x01, INC_2=0x02, INC_4=0x04} mode;
   while(1){
      //---------------------------------------- Edge cases
   if(sum>0xFFF0){
      sum += 1023;
   }
   //bound the count to 0 - 1023
   if(sum>1023){
      sum -= 1023;
   }

   //------------------------------------------------ Display 7seg
   //break up the disp_value to 4, BCD digits in the array: call (segsum)
   segsum(sum);
   //make PORTA an output
   DDRA = 0xFF;
   //prevent ghosting
   PORTA = 0xFF;
   //send 7 segment code to LED segments
   PORTA = dec_to_7seg[segment_data[digit]];
   //send PORTB the digit to display
   PORTB = ((0x8<<4) & PORTB)|(digit<<4); 
   //update digit to display
   digit = (++digit)%5;

   //----------------------------------------------- SPI
   DDRB |= 0xF1;
   //Load the mode into SPDR
   //SPDR = pressed_button;
   SPDR = mode;

   //Wait until you're done sending
   while(!(SPSR & (1<<SPIF)));

   DDRD = 0x04;
   PORTD = 0x00;

   DDRE = 0x40;
   PORTE = 0x40;
   //Strobe the BarGraph
   PORTD |= (1<<PD2);
   PORTD &= ~(1<<PD2);

   PORTE = 0x00;
   PORTE = 0x40;
   //Wait for the SPDR to be filled
   while(!(SPSR & (1<<SPIF)));
   //Store the SPDR value
   spdr_val = SPDR;


   }//while
   return 0;
}//main
