// lm73_functions.c       
// Rattanai Sawaspanich (Nov 28, 2015) 

#include <util/twi.h>
#include "lm73_functions_skel.h"
#include <util/delay.h>

//TODO:WHY? remove volatile type modifier?  I think so.
//TODO: write functions to change resolution, alarm etc.

uint8_t lm73_wr_buf[2];
uint8_t lm73_rd_buf[2];

//******************************************************************************
void lm73_set_max_resolution(void){
   //FIXME
   lm73_wr_buf[0] = LM73_PTR_CTRL_STATUS;
   //lm73_wr_buf[0] = LM73_PTR_TEMP;   
   lm73_wr_buf[1] = (1<<RES1 | 1<<RES0);
   twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2);
   //twi_start_wr(LM73_ADDRESS, lm73_wr_buf[1], 1);

   return ;
}


//******************************************************************************
void lm73_set_ptr_to_read(void){
   //set LM73 mode for reading temperature by loading pointer register
   //this is done outside of the normal interrupt mode of operation 
   //load lm73_wr_buf[0] with temperature pointer address
   lm73_wr_buf[0] = LM73_PTR_TEMP;   
   //start the TWI write process (twi_start_wr())
   twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 1);

   return;
}

//******************************************************************************
uint8_t lm73_temp_convert(char temp_digits[], uint16_t lm73_temp, uint8_t f_not_c){
   //given a temperature reading from an LM73, the address of a buffer
   //array, and a format (deg F or C) it formats the temperature into ascii in 
   //the buffer pointed to by the arguement.
   //TODO:Returns what???(uint8_t)??? Probably a BUG?

   //Yeah, this is for you to do! ;^)

}//lm73_temp_convert
//******************************************************************************
