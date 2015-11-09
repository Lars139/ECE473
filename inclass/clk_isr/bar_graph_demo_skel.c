// bar_graph_demo_skel.c 
// R. Traylor
// 10.22.14
// demos interrups, counter timer and SPI

// !!! YOU MUST RENAME THIS FILE TO MATCH YOUR MAKEFILE   !!!
// !!! YOU MUST FILL IN THE AREAS MARKED WITH "@" SIGNS   !!!
// !!! DISCONNECT ALL OTHER CONNECTIONS TO YOUR AVR BOARD !!!

// This code implements a timer interrupt to update the bar graph display
// at 0.5 second intervals. Every half second, a new data value is sent to 
// the bargraph via SPI. The value is displayed as single led in a climbing
// pattern.

// Expected Connections:
// Bargraph board           Mega128 board 
// --------------      ----------------------    
//     reglck            PORTB bit 0 (ss_n)                      
//     srclk             PORTB bit 1 (sclk)
//     sdin              PORTB bit 2 (mosi)
//     oe_n                   ground
//     gnd2                   ground
//     vdd2                     vcc
//     sd_out               no connect

#include <avr/io.h>
#include <avr/interrupt.h>

/***********************************************************************/
//                            spi_init                               
//Initalizes the SPI port on the mega128. Does not do any further   
//external device specific initalizations.  Sets up SPI to be:                        
//master mode, clock=clk/2, cycle half phase, low polarity, MSB first
//interrupts disabled, poll SPIF bit in SPSR to check xmit completion
/***********************************************************************/
void spi_init(void){
  DDRB  |=  (1<<PB2) | (1<<PB1) | (1<<PB0);           //Turn on SS, MOSI, SCLK
  SPCR  |=  ( 1<<SPE | 0<<DORD | 1<<MSTR |
              0<<CPOL | 0<<CPHA | (0<<SPR1 | 0<<SPR0) );  //set up SPI mode
  SPSR  |=  (1<<SPI2X);           // double speed operation
 }//spi_init

/***********************************************************************/
//                              tcnt0_init                             
//Initalizes timer/counter0 (TCNT0). TCNT0 is running in async mode
//with external 32khz crystal.  Runs in normal mode with no prescaling.
//Interrupt occurs at overflow 0xFF.
//
void tcnt0_init(void){
  ASSR   |=  (1<<AS0);  //ext osc TOSC
  TIMSK  |=  (0<<OCIE0);  //enable timer/counter0 overflow interrupt
  TCCR0  |=  (0<<WGM01 | 0<<WGM00) | (1<<CS00);  //normal mode, no prescale
}

/*************************************************************************/
//                           timer/counter0 ISR                          
//When the TCNT0 overflow interrupt occurs, the count_7ms variable is    
//incremented.  Every 7680 interrupts the minutes counter is incremented.
//tcnt0 interrupts come at 7.8125ms internals.
// 1/32768         = 30.517578uS
//(1/32768)*256    = 7.8125ms
//(1/32768)*256*64 = 500mS
/*************************************************************************/
ISR(TIMER0_OVF_vect){
  static uint8_t count_7ms = 0;        //holds 7ms tick count in binary
  static uint8_t display_count = 0x01; //holds count for display 

  count_7ms++;                //increment count every 7.8125 ms 
  if ((count_7ms % 64)==0){ //?? interrupts equals one half second 
    SPDR = display_count;     //send to display 
    while(!(SPSR & (1<<SPIF)));
    //wait till data is sent out (while spin loop)
    PORTB |=  (1<<PB0);       //strobe output data reg in HC595 - rising edge
    PORTB &=  (0<<PB0);       //falling edge
    display_count = display_count<<1; //shift display bit for next time 
  }
  if (display_count == 1<<7){display_count=0;} //display back to 1st positon
}

/***********************************************************************/
//                                main                                 
/***********************************************************************/
int main(){     
tcnt0_init();  //initalize counter timer zero
spi_init();    //initalize SPI port
sei();         //enable interrupts before entering loop
while(1){}     //empty main while loop

} //main
