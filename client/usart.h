#ifndef _USART_H
#define _USART_H
#define CALC_UBRR(x) (F_CPU/16/x-1)

void usart_init(uint16_t ubrr);
void usart_putchar( char data ); 
#endif
