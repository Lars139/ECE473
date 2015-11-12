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
#include "music.c"

//-------------------Encoder control
#define NUM_OF_ENCO 2
#define ENCO_CW 1
#define ENCO_CCW 2

/////////////////////////////////////////////////////////////////    GLOBAL VAR


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
   0xFC,//Full Colon (11)
   0x40,//0. (12)
   0x79,//1. (13)
};


//For 7seg number string display
uint16_t sum=0;
uint8_t  digit=0;
uint8_t  inc_step = 1;

//Stages for the buttons
enum stages {
   NONE=0x0, 
   EDIT_STIME=0b10000001, EDIT_12_24=0b10000010, EDIT_ATIME=0b10000100,
   EDIT_ALREN=0b10001000,
   SNOOZE=0b01111111
} mode;
uint8_t snooze_button = 5;

//uint8_t first_run_zero = 1;

//Keep tracked of the pressed button
uint8_t pressed_button = 0x00;

//For the SPI communicatino 
uint8_t ret_enc = 0;    //return val from Encoder
uint8_t spdr_val = 0;   //value in SPDR

//For Timekeeping
static struct time_info{
   uint8_t hr;
   uint8_t min;
   uint8_t sec;
   uint16_t tick;
} now;

//A status byte containing all the toggling bits
//status = [ 7seg_mode_disp | arm_alarm | sound_alarm | ...??? ]
//         7                                                   0
//     7seg_mode_disp - set mode of display for 7seg display
//         0 - int string (w/o colon)
//         1 - time format (w/ colon)
//
//     arm_alarm - whether the alarm has been armed or not
//         0 - disarm
//         1 - arm
//
//     sound_alarm - set if the alarm should be going off
//         0 - mute
//         1 - make noise
// 
//     mil_time - Set if 12 or 24 hour mode will be displayed
//         0 - 24 hr mode
//         1 - 12 am/pm mode
//
uint8_t bare_status;

///////////////////////////////////////////////////////////////////////    ADC 
/* Func: ADC_init()
 * Desc: Initilize an ADC
 */
void ADC_init(void){

   //Set all PORTF to be inputs
   DDRF &= ~(1<<0);
   //Active high 
   PORTF = 0x00;

   //Using external commong GND, Left-Aligned. 
   ADMUX = (1<<REFS0 | 1<<ADLAR);

   //Enable ADC, Start Conversion, Free Running Mode, Interrupt, /128 prescale.
   ADCSRA = (1<<ADEN | 1<<ADSC | 1<<ADFR | 1<<ADIE | 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0);

}


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
   //Let's not use colon here
   bare_status &= ~(1<<7);
   //determine how many digits there are 
   //Filling in backward
   uint8_t i;
   for( i=0; (val/10) > 0 ; ++i){
      if(i==2){
	 segment_data[i] = 0xFF;
	 ++i;
      } 
      segment_data[i] = val%10;
      val /= 10;
   }
   if(val != 0){
      segment_data[i] = val;
      val = 0;
      for(uint8_t j=i+1; j<5; ++j){
	 segment_data[j] = 0xA;
      }
   }else{//In case val == 0
      segment_data[4] = 0x0 ;
      segment_data[3] = 0x0 ;
      segment_data[1] = 0x0;
      segment_data[0] = 0x0;
   }

   //Prevent ghosting
   PORTB = ((0x8<<4) & PORTB)|(5<<4); 
}//segment_sum

//Displaying time
void disp_time(void){
   //Colon enabled
   bare_status |= (1<<7);

   if(bare_status & (1<<4)){ //AM-PM mode
      if(now.hr > 12){
	 segment_data[4] = ((now.hr-12)/10) + 12; //0. or 1.
	 segment_data[3] = ((now.hr-12)%10);
      }else if(now.hr == 0){
	 segment_data[4] = 1;
	 segment_data[3] = 2;
      }else if(now.hr == 12){
	 segment_data[4] = 13; //1.
	 segment_data[3] = 2; 
      }else{
	 segment_data[4] = now.hr/10;
	 segment_data[3] = now.hr%10;
      }
      segment_data[1] = now.min/10;
      segment_data[0] = now.min%10;
   }else{
      segment_data[4] = now.hr/10;
      segment_data[3] = now.hr%10 ;
      segment_data[1] = now.min/10;
      segment_data[0] = now.min%10;
   }

   //Prevent ghosting
   PORTB = ((0x8<<4) & PORTB)|(5<<4); 
}


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


////////////////////////////////////////////////////////////////////////  TCNT 
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


/* Func: tcnt1_init()
 * Desc: Initilize timer/counter1 (TCNT1) as a alarm_tone generator.
 * 	 Compare A, CTC Mode, /1024 pre-scaling
 * Input : None
 * Output: None
 */
void tcnt1_init(void){
   //PORTC Pin2 as an output for TCNT waveform
   DDRC  |= (1<<PC2);
   //Enable timer interrupt mask, comparing w/ OCF1A
   TIMSK |= (1<<OCIE1A);
   //CTC mode, /1024 prescaling 
   TCCR1B = (1<<WGM12 | 1<<CS12 | 1<<CS10);
   OCR1A = 0xFFFF;
}


/* Func: tcnt2_init()
 * Desc: Initilize timer/counter2 (TCNT2) as a light dimmer (PWM) 
 * 	 PWM Mode, No pre-scaling
 *
 * 	 The Overflow of this counter will be used to check the 
 * 	 button and encoder
 *
 * Input : None
 * Output: None
 */
void tcnt2_init(void){
   //PORTB Pin7 as an output for PWM waveform
   DDRB  |= (1<<PB7);

   //Enable interrupt for the TCNT2 for button check and encoder
   TIMSK |= (1<<TOIE2);

   //Fast-PWM mode, no prescaling, Set OCR2 on compare match
   TCCR2 = (1<<WGM21 | 1<<WGM20 | 1<<CS20 | 1<<COM21 | 1<<COM20);

   //Clear at 0x64
   OCR2  = 0xFF; 
}


/* Func: tcnt3_init()
 * Desc: Initilize timer/counter3 (TCNT3) as a audio volume controller. 
 * 	 PWM Mode, No pre-scaling
 * Input : None
 * Output: None
 */
void tcnt3_init(void){
   //PORTE Pin1 as an output for PWM waveform
   DDRE  |= (1<<PE3);

   //Fast-PWM mode, no prescaling, ICR3 top, OCR3A comp (clear on match) 
   TCCR3A = (1<<COM3A1 | 1<<COM3A0 | 1<<WGM31);
   TCCR3B = (1<<WGM33 | 1<<WGM32 | 1<<CS30);

   //FIXME: NOT WORKING!
   ICR3   = 0xFF;
   OCR3A  = 0xFF; 
}



/////////////////////////////////////////////////////////// MY CUSTOM FUNCTIONS

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
	 //first_run_zero = 0;
      }
      if( (enco_trend[enco_num]<=0xFF) && (enco_trend[enco_num]>0x90) ){
	 out = ENCO_CCW;
	 //first_run_zero = 0;
      }
      enco_trend[enco_num] = 0;
   }
   enco_history[enco_num] = cur_enco;

   return out;
}


///////////////////////////////////////////////////////////  ISR_SECTION


//-------------------------------------------------------------- ADC_ISR
ISR(ADC_vect){
   OCR2 = (ADCH < 100) ? (100-ADCH) : 1; 
}


//-------------------------------------------------------------- ISR_TIMER0
ISR(TIMER0_OVF_vect){
   //------------------------------------------------------- Time Keeper (sec)
   ++now.tick;
   //TODO
   if(now.tick == 489){
      ++now.sec;
      now.tick = 0;
      if(bare_status & (1<<7))
	 segment_data[2] = (segment_data[2] == 10) ? 11 : 10;
   }
   /*
      if(now.sec >= 60){
      bare_flags |= (1<<0);
      now.sec = 0;
      }
      */

   //----------------------------------------------------------- Music Timing
   static uint8_t ms = 0;
   ms++;
   if(ms % 8 == 0) {
      //for note duration (64th notes) 
      beat++;
   }        

}

//---------------------------------------------------------------- ISR_TIMER1
/* ISR_TIMER1_COMPA_vect
 *     The interrupt is involked to generate TCNT1_alarm_tone
 *
 * Song: Beep Boop Beep Boop
 * Composer: Bear's AVR Mega128
 * Date 10 Nov 2015
 *
 */
ISR(TIMER1_COMPA_vect){
   //Should it be making sound? 
   if(bare_status & (1<<5)){
      //Using PORTC Pin2 as an output
      PORTC ^= (1<<PC2);
      if(beat >= max_beat){
	 play_note('A'+(rand()%7),rand()%2,6+(rand()%3),rand()%16);
      }
   }
}


//---------------------------------------------------------------- ISR_TIMER2
/* ISR_TIMER2_OVF_vect
 *     The interrupt is a general purpose interrupt
 *     tick 0x0: Check Button
 *     tick 0x1: State Machine 
 *     tick 0x2: Time Keeper (minutes, hours)
 *     tick 0x3: Mode Enforcer (Mealy Output)
 *     tick 0xF: SPI
 *
 */
ISR(TIMER2_OVF_vect){
   //Scaling down the ISR so it doesn't hord up all the processing time
   static uint8_t tcnt2_cntr = 0;
   //Calibrating for the crystal time off
   static uint8_t sec_calibrate = 0;
   //Use toggler to check if the bit value has been change from the previos 
   //interrupt (you want to change it only once) 
   //toggler = [ EDIT_12_24 | ...?]
   static uint8_t toggler;

   if(!(pressed_button & (1<<7)))
      toggler = 0x00;

   switch (tcnt2_cntr){
      case 0x0:
	 //----------------------------------------------------- BUTTON_&_7-SEG
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
	 break;

      case 0x1:
	 //----------------------------------------------------- STATE_MACHINE 
	 switch(mode){
	    case NONE:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    case EDIT_STIME:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    case EDIT_12_24:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    case EDIT_ATIME:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    case EDIT_ALREN:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    case SNOOZE:
	       if(pressed_button & (1<<7)){
		  if (pressed_button & (1<<0)){
		     mode = EDIT_STIME;
		  }else if(pressed_button & (1<<1)){
		     mode = EDIT_12_24;
		  }else if(pressed_button & (1<<2)){
		     mode = EDIT_ATIME;
		  }else if(pressed_button & (1<<3)){
		     mode = EDIT_ALREN;
		  }
	       }else if( ~(pressed_button & (1<<7)) ){
		  pressed_button &= 0b10100000;
		  if(pressed_button & (1<<snooze_button)){
		     mode = SNOOZE;
		  }else{
		     mode = NONE;
		  }
	       }
	       break;

	    default:
	       mode = NONE;
	       break;

	 }//End Here -- Switch(mode)
	 break;//End Here -- 0xA;

      case 0x2:
	 //------------------------------------------------------ Time Keeper

	 //Check if a delayed second has been full.
	 if((sec_calibrate == 11) && (now.sec == 20)){
	    ++now.sec;
	    sec_calibrate = 0;
	 }

	 //Check if a full minute
	 if(now.sec == 60){
	    ++now.min;
	    ++sec_calibrate;
	    now.sec = 0;
	 }

	 //Check if a full hour
	 if(now.min == 60){
	    now.min = 0;
	    ++now.hr;
	 }

	 //Check if a full day
	 if(now.hr ==24){
	    now.hr = 0;
	 }
	 break;

      case 0x3:
	 //----------------------------------------------------- SM Enforcer
	 //Enforcing no colon policy
	 if( !(bare_status & (1<<7)) )
	    segment_data[2] = 0xFF;
	 switch(mode){
	    case EDIT_STIME:
	       //Send the value to read_encoder(num_enco, spdr_val);
	       //Store the returning value to decide if inc or dec
	       ret_enc = read_encoder(1, spdr_val);
	       //Inc/Dec the sum accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the sum
		  now.hr += 1;
	       }else if(ret_enc == ENCO_CCW){
		  now.hr -= 1;
	       }

	       ret_enc = read_encoder(0, spdr_val);
	       //Inc/Dec the sum accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the sum
		  now.min += 1;
	       }else if(ret_enc == ENCO_CCW){
		  if(now.min != 0)
		     now.min -= 1;
	       }

	       if(now.hr >= 24){
		  now.hr = 0;
	       }
	       if(now.min >= 60){
		  now.min = 0;
		  ++now.hr;
	       }
	       break;

	    case EDIT_12_24:
	       if(!(toggler & (1<<7))){
		  bare_status ^= (1<<4);
		  toggler |= (1<<7);
	       }
	       break;
	    case EDIT_ATIME:
	       break;
	    case EDIT_ALREN:
	       break;
	    case SNOOZE:
	       break;
	    case NONE:
	       break;
	 }



	 break;

      case 0xF:
	 break;

   }
   ++tcnt2_cntr;
}



////////////////////////////////////////////////////////////////////////  MAIN
uint8_t main(){
   ADC_init();
   tcnt0_init();
   tcnt1_init();
   tcnt2_init();
   tcnt3_init();
   spi_init();
   mode = NONE;
   segment_data[2] = 10; 
   sei();
   //PORTB=[ pwm | encd_sel2 | encd_sel1 | encd_sel0 | MISO | MOSI | SCK | SS ]
   //set port bits 4-7 B as outputs

   while(1){

      //------------------------------------------------ Display 7seg
      //break up the disp_value to 4, BCD digits in the array: call (segsum)
      //      segsum(sum);
      disp_time();
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
      SPDR = mode | (pressed_button & 0b10000000);
      //        SPDR = pressed_button;

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
