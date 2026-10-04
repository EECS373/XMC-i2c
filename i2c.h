#ifndef LAB_I2C_H
#define LAB_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "xmc_i2c.h"

/* Address convention for BOTH functions:
 * Shift a 7-bit device address LEFT by one bit before passing it here.
 * Example: 0x11 << 1 = 0x22. Pass 0x22 for BOTH reading and writing.
 * This library does not shift address again. Keep bit 0 clear, XMCLib sets
 * the read bit when needed. Do not pass the read-form address (i.e. 0x23).
 */

/**
 * Send count data bytes in one START/STOP transaction.
 * @param hw Configured hardware channel, e.g. XMC_I2C1_CH0 (USIC 1, channel 0).
 * @param address External device address, already shifted left once as above.
 * @param data Pointer to the bytes to send.
 * @param count Number of data bytes to send.
 * @return true if the address and every data byte were acknowledged,
 *         false on NACK or wait timeout.
 */
bool i2c_write_bytes(XMC_USIC_CH_t *hw, uint8_t address, const uint8_t *data, size_t count);

/**
 * Receive count data bytes in one START/STOP transaction.
 * @param hw Configured hardware channel, e.g. XMC_I2C1_CH0 (USIC 1, channel 0).
 * @param address External device address, already shifted left once as above.
 * @param data Pointer to writable storage for the received bytes.
 * @param count Number of data bytes to receive.
 * @return true if all bytes were received, false on NACK or wait timeout.
 *         A failed read may fill only part of data, discard it on failure.
 */
bool i2c_read_bytes(XMC_USIC_CH_t *hw, uint8_t address, uint8_t *data, size_t count);

#endif