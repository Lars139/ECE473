// lab6.c
// Rattanai Sawaspanich
// Dec 8, 2015 
//#define F_CPU 16000000 // cpu speed in hertz 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "music.c"
#include "LCDDriver.h"
#include "lm73_functions_skel.h"
#include "twi_master.h"
#include "usart.h"
#include "si4734.h"

//-------------------Encoder control
#define NUM_OF_ENCO 2
#define ENCO_CW 1
#define ENCO_CCW 2

//------------------Snooze period
// *_S for second, *_M for minute
#define SNOOZE_GAP_S 10
#define SNOOZE_GAP_M 0

//------------------Bare_Status 
#define LCD_N_DONE 8
#define COLON_DISP 7
#define ARM_ALARM 6
#define SOUND_ALARM 5
#define MIL_TIME 4
#define CHANGING_VOL 3
#define CLR_LCD_DISP 2
#define FAHRENHEIT 1
#define CHK_TEMP 0

//------------------LCD String display control
#define LCD_STR_LENGTH 14
#define LCD_STR "Bear was here!"


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
static uint16_t cur_vol=50;
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

//For LED_indication of alarm set
uint8_t run_led;

//Keep tracked of the pressed button
uint8_t pressed_button = 0x00;

//For the SPI communication 
uint8_t ret_enc = 0;    //return val from Encoder
uint8_t spdr_val = 0;   //value in SPDR

//For TWI communication with local (mega128) temperature sensor
uint8_t lm73_rd_buf[2]; 
uint8_t lm73_wr_buf[2]; 

enum radio_band{FM, AM, SW};
volatile enum radio_band current_radio_band;

volatile uint8_t STC_interrupt;  //flag bit to indicate tune or seek is done

uint16_t eeprom_fm_freq;
uint16_t eeprom_am_freq;
uint16_t eeprom_sw_freq;
uint8_t  eeprom_volume;


//Temperature value
char mega128_temp_str[9];
char mega48_temp_str[9];

//For Timekeeping
// now -- current displaying time
// alarm_time -- alarm time
// snooze_time -- time of snooze
static struct time_info{
   uint8_t  hr;
   uint8_t  min;
   uint8_t  sec;
   uint16_t tick;
} now, alarm_time, snooze_time, vol_disp_time;


//A status byte containing all the toggling bits
//status = [ lcd_done |7seg_mode_disp | arm_alarm | sound_alarm | mil_time | changing_vol | LCD_Clr | faren_temp ...??? ]
//         8                                                   0
//
//     8 lcd_n_done - the flag status if the LCD put every character up on the LCD display
//     	   0 - done putting all the character on the LCD
//     	   1 - still putting more character
//
//     7 colon_disp - set mode of display for 7seg display
//         0 - int string (w/o colon)
//         1 - time format (w/ colon)
//
//     6 arm_alarm - whether the alarm has been armed or not
//         0 - disarm
//         1 - arm
//
//     5 sound_alarm - set if the alarm should be going off
//         0 - mute
//         1 - make noise
// 
//     4 mil_time - Set if 12 or 24 hour mode will be displayed
//         0 - 24 hr mode
//         1 - 12 am/pm mode
//
//     3 changing_vol - Set if the volume is changing
//         0 - volumn is not changing
//         1 - volume is changing
//
//     2 LCD_Clr - clear the LCD display
//         0 - not clear yet
//         1 - LCD is all clear
//
//     1 FAHRENHEIT - The unit of temperature sensor
//         0 - display in Celcius
//         1 - display in Fahrenheit
//
//     0 CHK_TEMP - update LCD display for temperature
//         0 - don't update
//         1 - time to update the temperature on the LCD
//
uint16_t bare_status;

///////////////////////////////////////////////////////////////////////    ADC 
/* Func: ADC_init()
 * Desc: Initilize an ADC
 */
void ADC_init(void){

   //Set all PORTF to be inputs
   DDRF &= ~(1<<0);
   //Active high 
   PORTF = 0x00;

   //Using external common GND, Left-Aligned. 
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
   bare_status &= ~(1<<COLON_DISP);
   //determine how many digits there are 
   //Filling in backward
   if( !(bare_status & (1<<COLON_DISP)) ){
      segment_data[0] = val%10;
      segment_data[1] = val/10%10;
      segment_data[2] = 10; 
      segment_data[3] = val/100%10;
      segment_data[4] = val/1000%10;

      //Turning leading 0 off
      if(segment_data[4]==0){
	 segment_data[4] = 10;

	 if(segment_data[3]==0){
	    segment_data[3] = 10;

	    if(segment_data[1]==0){
	       segment_data[1] = 10;
	    }
	 }
      }


   }
   //Prevent ghosting
   PORTB = ((0x8<<4) & PORTB)|(5<<4); 
}//segment_sum

//Displaying time
void disp_time(void){
   //Colon enabled
   bare_status |= (1<<COLON_DISP);
   if(bare_status & (1<<COLON_DISP)){
      if((mode != EDIT_ATIME)){
	 if(bare_status & (1<<MIL_TIME)){ //AM-PM mode
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
      }else{
	 if(bare_status & (1<<MIL_TIME)){ //AM-PM mode
	    if(alarm_time.hr > 12){
	       segment_data[4] = ((alarm_time.hr-12)/10) + 12; //0. or 1.
	       segment_data[3] = ((alarm_time.hr-12)%10);
	    }else if(alarm_time.hr == 0){
	       segment_data[4] = 1;
	       segment_data[3] = 2;
	    }else if(alarm_time.hr == 12){
	       segment_data[4] = 13; //1.
	       segment_data[3] = 2; 
	    }else{
	       segment_data[4] = alarm_time.hr/10;
	       segment_data[3] = alarm_time.hr%10;
	    }
	    segment_data[1] = alarm_time.min/10;
	    segment_data[0] = alarm_time.min%10;
	 }else{
	    segment_data[4] = alarm_time.hr/10;
	    segment_data[3] = alarm_time.hr%10 ;
	    segment_data[1] = alarm_time.min/10;
	    segment_data[0] = alarm_time.min%10;
	 }

      }
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


/***********************************************************************/
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


/***********************************************************************/
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


/***********************************************************************/
/* Func: tcnt3_init()
 * Desc: Initilize timer/counter3 (TCNT3) as a audio volume controller. 
 * 	 PWM Mode, No pre-scaling
 * Input : None
 * Output: None
 */
void tcnt3_init(void){
   //PORTE Pin3 as an output for PWM waveform
   DDRE  |= (1<<PE3);

   //Fast-PWM mode, no prescaling, ICR3 top, OCR3A comp (clear on match) 
   TCCR3A = (1<<COM3A1 | 1<<COM3A0 | 1<<WGM31);
   TCCR3B = (1<<WGM33 | 1<<WGM32 | 1<<CS30);

   ICR3   = 0x64;
   OCR3A  = 90; 
}



/////////////////////////////////////////////////////////// MY CUSTOM FUNCTIONS

/***********************************************************************/
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
      }
      if( (enco_trend[enco_num]<=0xFF) && (enco_trend[enco_num]>0x90) ){
	 out = ENCO_CCW;
      }
      enco_trend[enco_num] = 0;
   }
   enco_history[enco_num] = cur_enco;

   return out;
}


/***********************************************************************/
/* Func: mega128_temperature(void)
 * Desc: Do TWI communication with temperature sensor
 * Output temperature string is stored in mega128_temp_str w/ size of 9 
 */
void mega128_temperature(){
   uint16_t mega128_temp;
   uint16_t prev_temp= 0;
   char temp_unit = 'C';
   //read temperature data from LM73 (2 bytes) (twi_mas ter.h)
   twi_start_rd(LM73_ADDRESS,lm73_rd_buf,2); 
   //wait for it to finish
   _delay_ms(2); 
   //save high temperature byte into mega128_temp
   mega128_temp = lm73_rd_buf[0]; 
   //shift it into upper byte
   mega128_temp = mega128_temp << 8; 
   //"OR" in the low temp byte to mega128_temp
   mega128_temp |= lm73_rd_buf[1];
   //Convert the unit if needed
   //FIXME
   if(bare_status & (1<<FAHRENHEIT)){
      mega128_temp = ((mega128_temp*9)+160)/5;
      temp_unit = 'F';
   }

   //Formatting the output
   if(mega128_temp & (1<<0xF))//Check signed bit
      snprintf(mega128_temp_str,9, "-%d.%-2d %c",mega128_temp>>7, ((mega128_temp & (0x3<<5))*25)>>5, temp_unit);
   else
      snprintf(mega128_temp_str,9, " %d.%-2d %c",mega128_temp>>7, ((mega128_temp & (0x3<<5))*25)>>5, temp_unit);

   if(!(bare_status & (1<<LCD_N_DONE))){  //check if LCD is busy
      if(prev_temp != mega128_temp){      //check if temp needs to be update
	 prev_temp = mega128_temp;	  //update the old temp
	 bare_status |= (1<<LCD_N_DONE ); //LCD needs to be update
      }
   }






}


/***********************************************************************/
/* Func: mega48_temperature(void)
 * Desc: Do USART communication with mega48 to get  remote temperature
 * Output temperature string is stored in mega48_temp_str w/ size of 9 
 */
void mega48_temperature(){

   //For USART communication with remote (meag48) temperature sensor
   uint16_t mega48_temp = 0; 
   char temp_unit = 'C';
   if(bare_status & (1<<FAHRENHEIT))
      USART_transmit('F');
   else
      USART_transmit('C');

   //save high temperature byte into mega128_temp
   mega48_temp = (USART_receive()<<7); 
   mega48_temp <<= 1;
   //"OR" in the low temp byte to mega48_temp
   mega48_temp |= USART_receive();

   //FIXME
   //Convert the unit if needed
   if(bare_status & (1<<FAHRENHEIT)){
      mega48_temp = ((mega48_temp*9)+160)/5;
      temp_unit = 'F';
   }

   //Formatting the output
   if(mega48_temp & (1<<0xF))//Check signed bit
      snprintf(mega48_temp_str,9, "-%2d.%-2d %c",(mega48_temp>>7)-2, ((mega48_temp & (0x3<<5))*25)>>5, temp_unit);
   else
      snprintf(mega48_temp_str,9, " %2d.%-2d %c",(mega48_temp>>7)-2, ((mega48_temp & (0x3<<5))*25)>>5, temp_unit);
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
   if(now.tick == 245)
      ++run_led;
   if(now.tick == 489){
      ++run_led;
      ++now.sec;
      now.tick = 0;
      if(bare_status & (1<<COLON_DISP))
	 segment_data[2] = (segment_data[2] == 10) ? 11 : 10;
      //Update the LCD
      bare_status |= (1<<CHK_TEMP);
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
//TODO: time this ISR!
ISR(TIMER1_COMPA_vect){
   //Should it be making sound? 
   if(bare_status & (1<<SOUND_ALARM)){
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
 *     tick 0x4: Check Alarm! 
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
   //toggler = [ EDIT_12_24 | EDIT_ALREN | SNOOZE | sound_alarm | change_vol ...?]
   static uint8_t toggler;

   if(!(pressed_button & (1<<7)))
      toggler = 0x00;

   switch (tcnt2_cntr){
      case 0x0:
	 //----------------------------------------------------- BUTTON_&_7-SEG
	 //make PORTA an input port with pullups 
	 PORTA = 0xFF;
	 DDRA = 0x00;
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
	 if( !(bare_status & (1<<COLON_DISP)) )
	    segment_data[2] = 0x10;
	 switch(mode){
	    case EDIT_STIME:
	       //Send the value to read_encoder(num_enco, spdr_val);
	       //Store the returning value to decide if inc or dec
	       ret_enc = read_encoder(1, spdr_val);
	       //Inc/Dec the hour accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the hour
		  now.hr += 1;
	       }else if(ret_enc == ENCO_CCW){
		  now.hr -= 1;
	       }

	       ret_enc = read_encoder(0, spdr_val);
	       //Inc/Dec the minute accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the minute
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
		  bare_status ^= (1<<MIL_TIME);
		  toggler |= (1<<7);
	       }
	       break;

	    case EDIT_ATIME:
	       //Send the value to read_encoder(num_enco, spdr_val);
	       //Store the returning value to decide if inc or dec
	       ret_enc = read_encoder(1, spdr_val);
	       //Inc/Dec the sum accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the sum
		  alarm_time.hr += 1;
	       }else if(ret_enc == ENCO_CCW){
		  alarm_time.hr -= 1;
	       }

	       ret_enc = read_encoder(0, spdr_val);
	       //Inc/Dec the sum accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the sum
		  alarm_time.min += 1;
	       }else if(ret_enc == ENCO_CCW){
		  if(alarm_time.min != 0)
		     alarm_time.min -= 1;
	       }

	       if(alarm_time.hr >= 24){
		  alarm_time.hr = 0;
	       }
	       if(alarm_time.min >= 60){
		  alarm_time.min = 0;
		  ++alarm_time.hr;
	       }
	       if(!(toggler & (1<<6))){//Has the bare_status been toggled?

		  //Enable arm the alarm
		  bare_status |= (1<<ARM_ALARM); 
		  toggler |= (1<<6);
	       }
	       break;

	    case EDIT_ALREN:
	       if(!(toggler & (1<<6))){//Has the bare_status been toggled?

		  //Enable arm the alarm
		  bare_status ^= (1<<ARM_ALARM); 
		  toggler |= (1<<6);

		  if(!(bare_status & (1<<ARM_ALARM))){
		     //Turn off the alarm
		     bare_status &= ~(1<<SOUND_ALARM);
		  }
	       }
	       break;

	    case SNOOZE:
	       if(!(toggler & (1<<5))){//Has the bare_status been toggled?
		  //Turn off the noise
		  bare_status ^= (1<<SOUND_ALARM); 
		  toggler |= (1<<5);
		  //Clear alarm bit so it can go off again
		  toggler &= ~(1<<4);
	       }

	       //Set snooze time
	       snooze_time.hr = now.hr;
	       snooze_time.min = now.min + SNOOZE_GAP_M;
	       snooze_time.sec = now.sec + SNOOZE_GAP_S;

	       if(snooze_time.sec >= 60){
		  snooze_time.sec %= 60;
		  ++snooze_time.min;
	       }

	       if(snooze_time.min >= 60){
		  snooze_time.min %= 60;
		  ++snooze_time.hr;
	       }
	       break;

	    case NONE:
	       // make the sound adjustment AND segsum-disp_time select
	       ret_enc = read_encoder(0, spdr_val);
	       //Inc/Dec the volume accordingly
	       if(ret_enc == ENCO_CW){ //CW adding the minute
		  if(cur_vol < 100)
		     cur_vol += 10;
		  bare_status |= (1<<CHANGING_VOL);
		  vol_disp_time.hr = now.hr;
		  vol_disp_time.min= now.min;
		  vol_disp_time.sec = now.sec + 3;
	       }else if(ret_enc == ENCO_CCW){
		  bare_status |= (1<<CHANGING_VOL);
		  if(cur_vol != 0)
		     cur_vol -= 10;
		  vol_disp_time.hr = now.hr;
		  vol_disp_time.min= now.min;
		  vol_disp_time.sec = now.sec + 3;
	       }

	       //Wrap around
	       if(vol_disp_time.sec >=60 ){
		  ++vol_disp_time.min;
		  vol_disp_time.sec %= 60;
	       }
	       if(vol_disp_time.min>=60 ){
		  ++vol_disp_time.hr;
		  vol_disp_time.min %= 60;
	       }
	       if(vol_disp_time.hr>=24 ){
		  vol_disp_time.hr %= 24;
	       }


	       if( (now.hr == vol_disp_time.hr)&&(now.min == vol_disp_time.min)
		     &&(now.sec == vol_disp_time.sec) ){
		  if(!(toggler & (1<<3))){

		     //Back to time display mode 
		     bare_status &= ~(1<<CHANGING_VOL); 
		     segment_data[2]=11;
		     toggler |= (1<<3);
		  }
	       }//End Here -- check now == vol_disp_time

	       OCR3A = 100 - cur_vol;
	       break;
	 }
	 break;

      case 0x4:
	 //------------------------------------------------- Setting Alarm Off 
	 if(bare_status & (1<<ARM_ALARM)){
	    if(((now.hr == alarm_time.hr)&&(now.min == alarm_time.min)
		     &&(now.sec == alarm_time.sec)) || ((now.hr == snooze_time.hr)&&
		     (now.min == snooze_time.min)&&(now.sec == snooze_time.sec)))
	       if(!(toggler & (1<<4))){//Has the bare_status been toggled?

		  //Set the alarm off
		  bare_status |= (1<<SOUND_ALARM); 
		  toggler |= (1<<4);
	       }


	 }else{
	    if(!(toggler & (1<<4))){//Has the bare_status been toggled?
	       //Turn off alarm if alarm is not set
	       bare_status &= ~(1<<SOUND_ALARM); 
	       toggler |= (1<<4);
	    }
	 }
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
   init_twi();
   LCD_Init();
   //lcd_init();
   USART0_init(103);

   //Set TWI pointer to the temperature output
   lm73_set_ptr_to_read();

   //get the LCD going
   bare_status |= (1<<LCD_N_DONE);

   //Set a Colon display
   segment_data[2]=11;
   //Set an initial state machine
   mode = NONE;
   radio_int_init();

   //Enable interrupt
   sei();

   //PortE Pin6 for Encoder strobe
   //PORTE Pin6 as an output for Encoder Strobe
   DDRE  |= (1<<PE6);
   PORTE = 0xFF;


   DDRE  |= 0x04; //Port E bit 2 is active high reset for radio
   PORTE |= 0x04; //radio reset is on at powerup (active high)
   PORTE |= 0x40; //pulse low to load switch values, else its in shift mode

   //PORTD Pin as an output for the BarGraph
   DDRD |= (1<<PD2);	 

   reset_radio();
   while(twi_busy());
   fm_pwr_up();
   while(twi_busy());
   _delay_ms(1000);
   fm_tune_freq();
   fm_tune_freq();

   //PORTB=[ pwm | encd_sel2 | encd_sel1 | encd_sel0 | MISO | MOSI | SCK | SS ]
   //set port bits 4-7 B as outputs

   while(1){

      //------------------------------------------------ Display 7seg
      //break up the disp_value to 4, BCD digits in the array: call (segsum)
      if(bare_status & (1<<CHANGING_VOL) && mode==NONE)
	 segsum(cur_vol);
      else
	 disp_time();
      //make PORTA an output
      DDRA = 0xFF;
      //prevent ghosting
      PORTA = 0xFF;

      if(digit==2 && !(bare_status&(1<<COLON_DISP)))
	 ++digit;
      //send 7 segment code to LED segments
      PORTA = dec_to_7seg[segment_data[digit]];
      //send PORTB the digit to display
      PORTB = ((0x8<<4) & PORTB)|(digit<<4); 
      //update digit to display
      digit = (++digit)%5;

      //----------------------------------------------- SPI
      DDRB |= 0xF1;
      //Load the mode into SPDR
      if(bare_status & (1<<ARM_ALARM)){
	 SPDR = (pressed_button & (1<<7)) | (0xFF & 1<<((run_led)%7)); 
      }else{
	 SPDR = mode | (pressed_button & 0b10000000);
	 //        SPDR = pressed_button;
      }
      //Prevent ghosting for 7seg
      //PORTB = ((0x8<<4) & PORTB)|(5<<4); 
      //Wait until you're done sending
      while(!(SPSR & (1<<SPIF)));

      //Strobe BarGraph
      PORTD |= (1<<PD2);
      PORTD &= ~(1<<PD2);

      //Strobe Encoder 
      PORTE &= ~(1<<PD6);
      PORTE |= (1<<PD6);

      //Wait for the SPDR to be filled
      while(!(SPSR & (1<<SPIF)));
      //Store the SPDR value
      spdr_val = SPDR;

      //Once in so very often (1 sec) 
      if(bare_status & (1<<CHK_TEMP)){ 
	 //------------------------------------------------ TWI Control
	 mega128_temperature();

	 //----------------------------------------------- USART
	 mega48_temperature();

	 bare_status &= ~(1<<CHK_TEMP);
      }

      //------------------------------------------------ LCD Display Control
      //lcd control section for itck 0x5;
      static uint8_t lcd_string[LCD_STR_LENGTH] = LCD_STR;
      static uint8_t lcd_cursor=0;
      static uint8_t lcd_line = 0;
      //Turning the LCD on when the alarm goes off
      //if(bare_status & (1<<SOUND_ALARM)){
      if(bare_status & (1<<ARM_ALARM)){
	 bare_status |= 1<<CLR_LCD_DISP;
	 LCD_PutChar(lcd_string[lcd_cursor]);
	 if(lcd_cursor == (LCD_STR_LENGTH-1)){
	    lcd_cursor=0;
	    LCD_MovCursor(1,0);
	 }else{
	    ++lcd_cursor;
	 }
      }else{
	 //Turing the LCD off when the alarm is not on (only do it once)
	 /*
	    if(bare_status & (1<<CLR_LCD_DISP)){
	    LCD_Clr();
	    bare_status &= ~(1<<CLR_LCD_DISP);
	    }
	    */
	 //FIXME: Have temperature display here w/o interfering w/ the other 
	 if(bare_status & (1<<LCD_N_DONE)){
	    if(lcd_line == 0){
	       LCD_PutChar(mega128_temp_str[lcd_cursor]);
	       if(lcd_cursor == 7){
		  lcd_cursor=0;
		  LCD_MovCursor(2,0);
		  ++lcd_line;
	       }else{
		  ++lcd_cursor;
	       }
	    }else{
	       LCD_PutChar(mega48_temp_str[lcd_cursor]);
	       if(lcd_cursor == 7){
		  lcd_cursor=0;
		  LCD_MovCursor(1,0);
		  --lcd_line;
		  bare_status &= ~(1<<LCD_N_DONE);
	       }else{
		  ++lcd_cursor;
	       }

	    }
	 }//END HERE- Bare_status LCD_N_DONE

      }




   }//while
   return 0;
   }//main
