#include "i2c.h"
#include "xmc_common.h"

/* Hardware selector syntax: XMC_I2C<USIC number>_CH<channel number>.
 * Example: XMC_I2C1_CH0 means USIC 1, channel 0 (not the device's I2C address). */

#define WAIT_BUDGET_MS 10U
#define ACK_FLAGS (XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED | \
                   XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED)
#define RX_FLAGS (XMC_I2C_CH_STATUS_FLAG_RECEIVE_INDICATION | \
                  XMC_I2C_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)

/* Return true for ACK or received data; false for NACK or wait timeout.
 * delay_ms counts requested delays, will be slightly slower with the processing inside for loop.*/
static bool wait_for_flags(XMC_USIC_CH_t *hw, uint32_t flags)
{
    for (uint32_t delay_us = 0; delay_us < WAIT_BUDGET_MS*1000; delay_us=delay_us+10)
    {
        /* Read the hardware status, keep only the bits we are waiting for. */
        uint32_t received = XMC_I2C_CH_GetStatusFlag(hw) & flags;
        if (received != 0U)
        {
            /* Reading the status does not consume the event. Clear it so an
             * old ACK/receive flag cannot satisfy the next byte's wait. */
            XMC_I2C_CH_ClearStatusFlag(hw, flags);
            return (received & XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED) == 0U;
        }

        /* Busy-wait 10 microseconds after an unsuccessful check. */
        XMC_DelayUs(10);
    }
    return false;
}

/* Reset I2C, triggered when there is a fault*/
static void reset_i2c(XMC_USIC_CH_t *hw)
{
    XMC_USIC_CH_SetMode(hw, XMC_USIC_CH_OPERATING_MODE_IDLE);
    XMC_USIC_CH_TXFIFO_Flush(hw);
    XMC_USIC_CH_SetTransmitBufferStatus(hw, XMC_USIC_CH_TBUF_STATUS_SET_IDLE);
    XMC_USIC_CH_InvalidateReadData(hw);
    XMC_I2C_CH_ClearStatusFlag(hw, 0xFFFFFFFFU);
    XMC_I2C_CH_Start(hw);
}

/* START, address + write, all data bytes, STOP.
 * hw:      Hardware channel, e.g. XMC_I2C1_CH0 = USIC 1, channel 0.
 * address: Already-shifted device address (i.e. 0x11 << 1 = 0x22).
 * data:    Source array.
 * count:   Number of data bytes, excluding the device address. */
bool i2c_write_bytes(XMC_USIC_CH_t *hw, uint8_t address, const uint8_t *data, size_t count)
{
    /* Discard old ACK/NACK flags, including replies left after a timeout. */
    XMC_I2C_CH_ClearStatusFlag(hw, ACK_FLAGS);
    /* Queue START and the device address with the write bit (0). */
    XMC_I2C_CH_MasterStart(hw, address, XMC_I2C_CH_CMD_WRITE);
    /* Only ACK means success; NACK or no response means failure. */
    bool success = wait_for_flags(hw, ACK_FLAGS);

    for (size_t i = 0; i < count && success; i++)
    {
        /* Queue the data byte for transmission, then wait for the device's ACK. */
        XMC_I2C_CH_MasterTransmit(hw, data[i]);
        success = wait_for_flags(hw, ACK_FLAGS);
    }
    if(!success) {
        reset_i2c(hw);
        return success;
    }
    /* Queue STOP to end the transaction, including after a failed ACK. */
    XMC_I2C_CH_MasterStop(hw);
    return success;
}

/* START, address + read, all data bytes, STOP.
 * hw:      Hardware channel, e.g. XMC_I2C1_CH0 = USIC 1, channel 0.
 * address: Same shifted address as a write (i.e. 0x22, not 0x23).
 * data:    Destination array.
 * count:   Number of data bytes to receive, excluding the device address. */
bool i2c_read_bytes(XMC_USIC_CH_t *hw, uint8_t address, uint8_t *data, size_t count)
{
    /* Discard old ACK/NACK and receive flags, including those after a timeout. */
    XMC_I2C_CH_ClearStatusFlag(hw, ACK_FLAGS | RX_FLAGS);
    /* Queue START and the device address; XMCLib sets the read bit (1). */
    XMC_I2C_CH_MasterStart(hw, address, XMC_I2C_CH_CMD_READ);
    bool success = wait_for_flags(hw, ACK_FLAGS);

    for (size_t i = 0; i < count && success; i++)
    {
        if (i == count - 1)
        {
            /* Receive the last byte and send NACK to say we are done.
             * This outgoing NACK is normal; it is not a device-reported error. */
            XMC_I2C_CH_MasterReceiveNack(hw);
        }
        else
        {
            /* Receive a byte and send ACK to say we want another byte. */
            XMC_I2C_CH_MasterReceiveAck(hw);
        }
        success = wait_for_flags(hw, RX_FLAGS);
        if (success)
        {
            /* Copy the completed byte from the hardware receive buffer. */
            data[i] = XMC_I2C_CH_GetReceivedData(hw);
        }
        else {
            reset_i2c(hw);
            return false;
        }
    }
    /* Queue STOP to end the transaction, even if the read failed. */
    XMC_I2C_CH_MasterStop(hw);
    return success;
}
