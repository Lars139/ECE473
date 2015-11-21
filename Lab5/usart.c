/* UART Example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release
// Version 1.1: Add support for Teensy 2.0, minor optimizations


#include <avr/io.h>

#include "usart.h"

void USART_init(unsigned int baud)
{
	// Set baud rate
	UBRR1 = baud;
	
	// Enable transmit and receive
	UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1<<RXCIE1);
	
	// Set frame format: 8 data bits, 1 stop bit
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

void USART_transmit(unsigned char data)
{
	while (!(UCSR1A & (1 << UDRE1)));
	
	UDR1 = data;
}

unsigned char USART_available()
{
	return (UCSR1A & (1 << RXC1));
}

unsigned char USART_receive()
{
	while (!USART_available());
	
	return UDR1;
}

void USART_send_string(const char* str)
{
	while (*str){
		USART_transmit(*str);
		str++;
	}
}
