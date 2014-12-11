#ifndef F_CPU
# define F_CPU 8000000
#endif
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/sleep.h>

#ifdef DEBUG
#define DEBUG_BAUD 115200
#include "usart.h"
#define DEBUG_SEND(x) usart_putchar(x)
#define DEBUG_INIT() usart_init(CALC_UBRR(DEBUG_BAUD))
#else
#define DEBUG_SEND(x)
#define DEBUG_INIT()
#endif

#include "i2c.h"

#define MASTER 0x3C

static void adc_setup(void);
static void get_address(void);
int main(void);

uint8_t address = 0;

static void adc_setup(void) {
	// ADC source is PA0, reference is VCC, left-adjust results
	ADMUX = (0<<MUX0) | (1<<REFS0) | (1<<ADLAR);
	// Enable ADC, enable auto-trigger, prescaler 64
	ADCSRA = (1<<ADEN) | (1<<ADFR) | (6<<ADPS0);
	// Start
	ADCSRA |= (1<<ADSC);
}
static void get_address(void) {
	uint8_t ret;
 start:
	PORTC |= (1<<PC3);
	PORTC &= ~(1<<PC3);
	i2c_send_start();
        
	ret = i2c_master_send_byte(I2C_WRITE_ADDR(MASTER));
	if(ret != I2C_OK) {
		goto fail; // *giggles*
	}
	
	// Command to get an address
	ret = i2c_master_send_byte(0x7F);
	if(ret != I2C_OK) {
		goto fail;
	}
	
	// Want one address
	ret = i2c_master_send_byte(1);
	if(ret != I2C_OK) {
		goto fail;
	}
	
	i2c_send_start();
	ret = i2c_master_send_byte(I2C_READ_ADDR(MASTER));
	if(ret != I2C_OK) {
		goto fail;
	}

	// Check for answer to get_address command (high bit set)
	ret = i2c_master_read_byte(1);
	if(ret != 0xFF) {
		goto fail;
	}

	ret = i2c_master_read_byte(0);
	if(ret == 0xFF) {
		goto fail;
	}
	i2c_send_stop();

	address = ret;

	return;

 fail:
	if(ret != I2C_ARB_LOST) {
		i2c_send_stop();
		DEBUG_SEND(TWSR);
	}
	_delay_ms(5);
	goto start;
}

int main() {
	DEBUG_INIT();

	//WDTCR = (1<<WDE) | (7 << WDP0);
	DDRC |= (1<<PC3);
	adc_setup();
	i2c_setup();
	get_address();

	PORTC |= (1<<PC3);
	PORTC &=~(1<<PC3);
	PORTC |= (1<<PC3);
	PORTC &=~(1<<PC3);

	uint8_t last_voltage = 0;
	while(1) {
		uint8_t voltage = ADCH >> 1;
		if(voltage != last_voltage) {
			uint8_t ret;
			goto send;
		fail:
			DEBUG_SEND(TWSR);
			if(ret != I2C_ARB_LOST) {
				i2c_send_stop();
			}
		send:
			i2c_send_start();
			if((ret = i2c_master_send_byte(I2C_WRITE_ADDR(MASTER))) != I2C_OK) goto fail;
			if((ret = i2c_master_send_byte(0xB0)) != I2C_OK) goto fail;
			if((ret = i2c_master_send_byte(address)) != I2C_OK) goto fail;
			if((ret = i2c_master_send_byte(voltage)) != I2C_OK) goto fail;
			i2c_send_stop();
			last_voltage = voltage;
		}

		//wdt_reset();
                _delay_ms(10);
	}

	return 0;
}

