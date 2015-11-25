// mega48_lab5.c 
// Rattanai Sawaspanich 
// 24 Nov. 2015

//Demonstrates basic functionality of the LM73 temperature sensor
//Uses the mega128 board and interrupt driven TWI.
//Display is the raw binary output from the LM73.
//PD0 is SCL, PD1 is SDA. 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "lcd_functions.h"
#include "lm73_functions_skel.h"
#include "twi_master.h"

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
uint16_t lm73_temp;  //a place to assemble the temperature from the lm73

spi_init();   //initalize SPI 
lcd_init();   //initalize LCD (lcd_functions.h)
init_twi();   //initalize TWI (twi_master.h)  

//set LM73 mode for reading temperature by loading pointer register
//this is done outside of the normal interrupt mode of operation 
lm73_wr_buf[0] = LM73_PTR_TEMP;   //load lm73_wr_buf[0] with temperature pointer address
twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 1);   //start the TWI write process (twi_start_wr())
sei();             //enable interrupts to allow start_wr to finish

clear_display();   //clean up the display

while(1){          //main while loop
  _delay_ms(100);  //tenth second wait
  clear_display(); //wipe the display
  twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2); //read temperature data from LM73 (2 bytes)  (twi_start_rd())
  _delay_ms(2);    //wait for it to finish
//now assemble the two bytes read back into one 16-bit value
  lm73_temp = lm73_rd_buf[0]; //save high temperature byte into lm73_temp
  lm73_temp <<= 8; //shift it into upper byte 
  lm73_temp |= lm73_rd_buf[1]; //"OR" in the low temp byte to lm73_temp 
  itoa(lm73_temp, lcd_string_array, 16); //convert to string in array with itoa() from avr-libc                           
  string2lcd(lcd_string_array); //send the string to LCD (lcd_functions)
  } //while
} //main
