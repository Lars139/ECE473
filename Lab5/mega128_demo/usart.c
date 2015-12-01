#include <avr/io.h>

void USART0_init(uint16_t baud)
{
	// Set baud rate
	UBRR0H = baud>>8;
	UBRR0L = baud;
	
	// Enable transmit and receive
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	
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
    uint16_t timer = 0;

    while (!(UCSR0A & (1<<RXC0))) {
	timer++;
	if(timer >= 16000){ return(0);}
	//what should we return if nothing comes in?
	//return the data into a global variable
	//give uart_getc the address of the variable
	//return a -1 if no data comes back.
    } // Wait for byte to arrive
    return(UDR0); //return the received data
}

void USART_send_string(const char* str)
{
   while (*str){
      USART_transmit(*str);
      str++;
   }
}
