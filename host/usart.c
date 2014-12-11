#include "usart.h"

void usart_init( uint16_t ubrr) {
    // Set baud rate
    UBRR1 = ubrr;
    // Enable receiver and transmitter
    UCSR1B = (1<<TXEN1);
    // Set frame format: 8data, 1stop bit
    UCSR1C = (3<<UCSZ10);
}
void usart_putchar(char data) {
    // Wait for empty transmit buffer
    while ( !(UCSR1A & (_BV(UDRE1))) );
    // Start transmission
    UDR1 = data; 
}
