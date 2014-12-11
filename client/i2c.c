#include "i2c.h"

/** @brief Waits for the I2C interrupt */
static inline void i2c_wait_int(void);

static inline void i2c_wait_int(void)    { while((TWCR & (1<<TWINT)) == 0); }

void i2c_setup(void) {
        //DDRC |= (1<<PC5) | (1<<PC4);
        TWBR = 5;
	TWCR &= ~((1<<TWSTA) | (1<<TWSTO));
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        TWAR = 0; // no slave address, don't respond to general call
}

void i2c_send_start(void) {
        TWCR |= (1<<TWINT) | (1<<TWSTA);
        i2c_wait_int();
        //TWCR &= ~(1<<TWSTA);
}

void i2c_send_stop(void) {
        TWCR |= (1<<TWINT) | (1<<TWSTO);
}

uint8_t i2c_master_read_byte(uint8_t send_ack) {
        TWCR |= (1<<TWINT) | (send_ack << TWEA);
        i2c_wait_int();
        return TWDR;
}

uint8_t i2c_master_send_byte(uint8_t v) {
        TWDR = v;
        TWCR = TWCR & ~((1<<TWSTA) | (1<<TWSTO)) | (1<<TWINT);
        i2c_wait_int();
        uint8_t status = TWSR & ((1<<TWS7) | (1<<TWS6) | (1<<TWS5) | (1<<TWS4) | (1<<TWS3));
        if(status == 0x18 || status == 0x28 || status == 0x40) {
                return I2C_OK;
        }
        if(status == 0x20 || status == 0x30 || status == 0x48) {
                return I2C_NAK;
        }
        if(status == 0x38) {
                return I2C_ARB_LOST;
        }
        return I2C_UNEXPECTED;
}
