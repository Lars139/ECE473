#include <avr/io.h>

#include "usart.h"

void USART0_init(unsigned int baud)
{
	// Set baud rate
	UBRR0 = baud;
	
	// Enable transmit and receive
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1<<RXCIE0);
	
	// Set frame format: 8 data bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_transmit(unsigned char data)
{
	while (!(UCSR0A & (1 << UDRE0)));
	
	UDR0 = data;
}

unsigned char USART_available()
{
	return (UCSR0A & (1 << RXC0));
}

unsigned char USART_receive()
{
	while (!USART_available());
	
	return UDR0;
}

void USART_send_string(const char* str)
{
	while (*str){
		USART_transmit(*str);
		str++;
	}
}
