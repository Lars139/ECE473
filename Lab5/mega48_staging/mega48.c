// thermo3_skel.c
// Rattanai Sawaspanich 
// 28 November 2015

//Demonstrates basic functionality of the LM73 temperature sensor
//Uses the mega128 board and interrupt driven TWI.
//Display is the raw binary output from the LM73.
//PD0 is SCL, PD1 is SDA. 

//#define MEGA48_DEBUG 1
#define MEGA48 1
//#define TESTING 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "lm73_functions_skel.h"
#include "twi_master.h"
#include "usart.h"

char    lcd_string_array[16];  //holds a string to refresh the LCD
uint8_t i;                     //general purpose index

//delclare the 2 byte TWI read and write buffers (lm73_functions_skel.c)
extern uint8_t lm73_rd_buf[2];
extern uint8_t lm73_wr_buf[2];


//********************************************************************
//                            spi_init                               
//Initalizes the SPI port on the mega128. Does not do any further    
// external device specific initalizations.                          
//********************************************************************
void spi_init(void){
   DDRB |=  0x07;  //Turn on SS, MOSI, SCLK
   //mstr mode, sck=clk/2, cycle 1/2 phase, low polarity, MSB 1st, 
   //no interrupts, enable SPI, clk low initially, rising edge sample
   SPCR=(1<<SPE) | (1<<MSTR); 
   SPSR=(1<<SPI2X); //SPI at 2x speed (8 MHz)  
}//spi_init


/***********************************************************************/
/*                                main                                 */
/***********************************************************************/
int main ()
{     
   DDRD |= 0x02;
   //a place to assemble the temperature from the lm73
   uint16_t lm73_temp;  
   //USART read buffer
   unsigned char usart_rd_buf;

   spi_init();   //initalize SPI 
   init_twi();   //initalize TWI (twi_master.h)  
   USART0_init(51);

   //   lm73_set_max_resolution();
   lm73_set_ptr_to_read();
	_delay_ms(100);

   sei();             //enable interrupts to allow start_wr to finish

   while(1){          //main while loop

      //-------------------------------------------------- USART_REC
      usart_rd_buf = USART_receive();
#ifdef TESTING
      switch(usart_rd_buf){
	 case 'A':
	    USART_transmit(0b10101010);
	    USART_transmit(0b10101010);
	    break;
	 case 'B':
	    USART_transmit(0b11100011);
	    USART_transmit(0b11100011);
	    break;
	 case 'C':
	    USART_transmit(0b11001100);
	    USART_transmit(0b11001100);
	    break;
	 case 'D':
	    USART_transmit(0b11110000);
	    USART_transmit(0b11110000);
	    break;
      }
#endif
      //------------------------------------------------------ TWI
	 twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2); //read temperature data from LM73 (2 bytes)  (twi_start_rd())
	 _delay_ms(2);    //wait for it to finish
      //now assemble the two bytes read back into one 16-bit value
      lm73_temp = lm73_rd_buf[0]; //save high temperature byte into lm73_temp
      lm73_temp <<= 8; //shift it into upper byte 
      lm73_temp |= lm73_rd_buf[1]; //"OR" in the low temp byte to lm73_temp 

      //-------------------------------------------------- USART_TRANS
      //Transmit the string byte by byte to MEGA128
      USART_transmit( lm73_temp>>8);
      USART_transmit((lm73_temp<<8)>>8);
   } //while
} //main
