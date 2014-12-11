#ifndef _I2C_H
#define _I2C_H
#include <avr/interrupt.h>

#define I2C_READ_ADDR(x) ((x)<<1 | 1)
#define I2C_WRITE_ADDR(x) ((x)<<1)

enum {
	I2C_OK = 0, I2C_NAK = 1, I2C_ARB_LOST = 2, I2C_UNEXPECTED = 3
};

/** @brief Sets up I2C */
void i2c_setup(void);
/** @brief Waits for any communication by other masters to finish, then sends a START condition */
void i2c_send_start(void);
/** @brief Sends a STOP condition */
void i2c_send_stop(void);
/** @brief Reads a single byte, as master
	@param[in] send_ack Set to 1 to send an ACK, 0 for NAK
	@return The byte that was read
 */
uint8_t i2c_master_read_byte(uint8_t send_ack);
/** @brief Sends a single byte, as master
	@param[in] v The byte to be written
	@return I2C_OK, I2C_NAK, or I2C_ARB_LOST
 */
uint8_t i2c_master_send_byte(uint8_t v);

#endif
