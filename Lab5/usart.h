#ifndef USART_H
#define USART_H

#include <stdint.h>

void USART_init(unsigned int baud);
void USART_transmit(unsigned char data);
unsigned char USART_available(void);
unsigned char USART_receive(void);
void USART_send_string(const char* str);

#endif // USART_H
