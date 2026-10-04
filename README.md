# EECS373 Lab 6 I2C - Infineon XMC

Two functions for sending or receiving bytes on the XMC4700 using Infineon XMCLib. Abstracts XMC's implementation. 

```c
bool i2c_write_bytes(XMC_USIC_CH_t *hw, uint8_t address, const uint8_t *data, size_t count);
bool i2c_read_bytes(XMC_USIC_CH_t *hw, uint8_t address, uint8_t *data, size_t count);
```

| Arguments | Description |
| --- | --- |
| `hw` | The configured microcontroller channel, such as `XMC_I2C1_CH0` for USIC 1, channel 0. |
| `address` | The external device's 7-bit address shifted left once, such as `0x11 << 1` (`0x22`). Use the same value for both functions. |
| `data` | An array. Write reads from this buffer, read stores received bytes into it. |
| `count` | The number of data bytes, excluding the device address. Must be greater than zero and fit in the buffer. |

Both return `true` on success and `false` on NACK or wait timeout. 

## Add it to your project

1. Copy `i2c.c` and `i2c.h` into the ModusToolbox project directory.
2. Add `#include "i2c.h"` to your application.
3. Configure I2C and its pins in the Device Configurator, and call
   `cybsp_init()` before using these functions.

Pass the XMCLib channel name into each call. For example, this sends one raw
byte(0x20) using USIC 0, channel 0 to an i2c device with address 0x11:

```c
uint8_t data = 0x20;
bool success = i2c_write_bytes(XMC_I2C0_CH0, 0x22, &data, 1);
```
