/*
 * LCDDriver.c
 *
 *  R. Traylor 10.7.09
 *  Justin Goins 10.14.14
 *	For protocol information see: http://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller 
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "LCDDriver.h"

char lcd_str[16];  //holds string to send to lcd

// Macros to communicate with the LCD
// RS bit = 1
#define LCD_DATA(d) {			\
	SPDR = 0x01;				\
	while (!(SPSR & 0x80)) {}	\
	SPDR = (d);					\
	while (!(SPSR & 0x80)) {}	\
	PORTF |= 0x08;				\
	PORTF &= ~0x08;				\
	_delay_us(100);				\
}

// RS bit = 0
#define LCD_CMD(c) {			\
	SPDR = 0x00;				\
	while (!(SPSR & 0x80)) {}	\
	SPDR = (c);					\
	while (!(SPSR & 0x80)) {}	\
	PORTF |= 0x08;				\
	PORTF &= ~0x08;				\
	_delay_us(100);				\
}


void strobe_lcd(void) {
	//twiddles bit 3, PORTF creating the enable signal for the LCD
	PORTF |= 0x08;
	PORTF &= ~0x08;
}

void LCD_Clr(void) {
	LCD_CMD(0x01);
	_delay_ms(2);   //obligatory waiting for slow LCD
}

void LCD_MovCursorLn1(void) {
	LCD_CMD(0x80);
}

void LCD_MovCursorLn2(void) {
	LCD_CMD(0xC0);
}

void LCD_CursorBlinkOn(void) {
	LCD_CMD(0x0F);
}

void LCD_CursorBlinkOff(void) {
	LCD_CMD(0x0C);
}

/*
	line = {1,2}
	position = {0-15}
		
	The cursor will not be moved if an invalid location is provided
*/
void LCD_MovCursor(uint8_t line, uint8_t position) {
	uint8_t addr;
	if (position > 15)
		return;
	if (line == 1) {
		addr = 0x80 + position;
	} else if (line == 2) {
		addr = 0xC0 + position;
	} else {
		// invalid line number
		return;
	}
	LCD_CMD(addr);
}

void LCD_FillSpaces(void) {
	int count;
	for (count=0; count<=15; count++){
		LCD_DATA(' ');
	}
}

/*
	Convert an 8 bit unsigned number to ASCII
	and display the result.
*/
void LCD_PutDec8(uint8_t num) {
	uint8_t digit = 0;
	uint8_t flag = 0;
	while (num >= 100) {
		digit++;
		num -= 100;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 10) {
		digit++;
		num -= 10;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 1) {
		digit++;
		num -= 1;
	}
	LCD_DATA(digit + 48);
}

/*
	Convert a 16 bit unsigned number to ASCII
	and display the result.
*/
void LCD_PutDec16(uint16_t num) {
	uint8_t digit = 0;
	uint8_t flag = 0;
	while (num >= 10000) {
		digit++;
		num -= 10000;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 1000) {
		digit++;
		num -= 1000;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 100) {
		digit++;
		num -= 100;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 10) {
		digit++;
		num -= 10;
	}
	if (digit > 0 || flag == 1) {
		LCD_DATA(digit + 48);
		flag = 1;
	}
	digit = 0;
	while (num >= 1) {
		digit++;
		num -= 1;
	}
	LCD_DATA(digit + 48);
}

/*
	Send an ASCII character to the LCD
	usage: LCD_PutChar('H');
*/
void LCD_PutChar(char a_char) {
	LCD_DATA(a_char);
}

/*
	Send an ASCII string to the LCD
*/
void LCD_PutStr(char *lcd_str) {
	uint8_t count;
	for (count=0; count<=(strlen(lcd_str)-1); count++){
		LCD_DATA(lcd_str[count]);
	}
}

void LCD_SPIInit(void) {
	DDRF |= 0x08;  //port F bit 3 is enable for LCD
	PORTB |= 0x00; //port B initialization for SPI
	DDRB |= 0x07;  //Turn on SS, MOSI, SCLK
	//Master mode, Clock=clk/2, Cycle half phase, Low polarity, MSB first
	SPCR = 0x50;
	SPSR = 0x01;
}

void LCD_Init(void) {
	/*
		Setup External SRAM configuration
		$0460 - $7FFF / $8000 - $FFFF
		Lower page wait state(s): None
		Upper page wait state(s): 2r/w
	*/
   /*
	MCUCR=0x80;
	XMCRA=0x42;
	XMCRB=0x80;
	*/
	
	/*
		Initialize the LCD screen
	*/
	
	// Set the SPI settings
//	LCD_SPIInit();

	DDRF |= 0x08;  // port F bit 3 is the enable strobe for the LCD
	_delay_ms(15);

	// request 8 bit interface mode
	LCD_CMD(0x38);
	_delay_ms(5);

	// display off
	LCD_CMD(0x08);
	_delay_ms(2);

	// choose entry mode so that the cursor is incremented
	LCD_CMD(0x06);
	
	/*
		We can add up to 8 custom characters to the LCD
		They can be accessed via ASCII characters 0..7
		Each digit on the display uses a 5x8 matrix
		In order to customize the icon, you specify 5 bits
		for each of the 8 rows.
	*/
	// Create a custom battery logo in slot 0
	LCD_CMD(0x40); // <-- address of custom slot 0
	LCD_DATA(0x0E); // <-- top 5 pixels of icon
	LCD_DATA(0x1B); // <-- next 5 pixels of icon
	LCD_DATA(0x11);
	LCD_DATA(0x11);
	LCD_DATA(0x11);
	LCD_DATA(0x11);
	LCD_DATA(0x11);
	LCD_DATA(0x1F); // <-- bottom 5 pixels of icon
	
	/*
		Clear the screen and enable the LCD
	*/	
	// clear display
	LCD_CMD(0x01);
	_delay_ms(5);
	
	// display on
	LCD_CMD(0x0C);
}
