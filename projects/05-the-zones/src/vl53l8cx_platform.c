/* projects/05-the-zones/src/vl53l8cx_platform.c: the six functions the vendor's interface calls.
 *
 * NOTHING HERE KNOWS WHAT A ZONE IS. These are bytes to and from a register address on a
 * two-wire bus, and that is the whole contract: the library decides what to say and this decides
 * how to say it. Keeping the two apart is why the library can be used unmodified.
 *
 * THE TWO LARGE TRANSFERS ARE WHY THIS IS NOT THREE LINES. Initialisation sends the part a
 * firmware image of tens of kilobytes and a ranging result comes back in one piece, so the
 * multiple-byte calls are chunked, with the register address advanced by the amount already
 * carried. The operating system's driver for the previous generation does the same and makes its
 * chunk a configuration option; this file holds it at a constant, because this project has no
 * figure saying a different one is better and a setting nobody has a reason to change is a
 * setting that invites a wrong reason.
 *
 * A WRITE DOES NOT COPY. The address and the payload go as two messages of one transaction
 * rather than being joined into a buffer, because joining them would mean a copy of the firmware
 * image, and there is no reason to own two of it.
 */
#include "platform.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

/* The vendor's interface reports success as zero and a fault as anything else, which is the
 * opposite way round from this operating system's convention of zero and a negative number, so
 * the two meet here and nowhere else. */
#define VENDOR_OK   0u
#define VENDOR_FAIL 255u

/* Held at a constant rather than offered as a setting, for the reason in the file header. */
#define CHUNK 512u

uint8_t VL53L8CX_RdByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value)
{
	return VL53L8CX_RdMulti(p_platform, RegisterAdress, p_value, 1u);
}

uint8_t VL53L8CX_WrByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value)
{
	return VL53L8CX_WrMulti(p_platform, RegisterAdress, &value, 1u);
}

uint8_t VL53L8CX_RdMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
			 uint8_t *p_values, uint32_t size)
{
	uint32_t done = 0u;

	while (done < size) {
		uint16_t addr = sys_cpu_to_be16((uint16_t)(RegisterAdress + done));
		uint32_t part = MIN(size - done, (uint32_t)CHUNK);
		int rc = i2c_write_read_dt(&p_platform->i2c, &addr, sizeof(addr),
					   &p_values[done], part);

		if (rc != 0) {
			return VENDOR_FAIL;
		}
		done += part;
	}
	return VENDOR_OK;
}

uint8_t VL53L8CX_WrMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
			 uint8_t *p_values, uint32_t size)
{
	uint32_t done = 0u;

	while (done < size) {
		uint16_t addr = sys_cpu_to_be16((uint16_t)(RegisterAdress + done));
		uint32_t part = MIN(size - done, (uint32_t)CHUNK);
		struct i2c_msg msg[2] = {
			{
				.buf = (uint8_t *)&addr,
				.len = sizeof(addr),
				.flags = I2C_MSG_WRITE,
			},
			{
				.buf = &p_values[done],
				.len = part,
				.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
			},
		};
		int rc = i2c_transfer_dt(&p_platform->i2c, msg, 2);

		if (rc != 0) {
			return VENDOR_FAIL;
		}
		done += part;
	}
	return VENDOR_OK;
}

void VL53L8CX_SwapBuffer(uint8_t *buffer, uint16_t size)
{
	uint16_t i;

	/* THE SIZE IS IN BYTES AND THE SWAP IS IN WORDS OF FOUR, which is the vendor's
	 * convention and is easy to read as the other thing. A size that is not a multiple of
	 * four would walk past the end, so the loop is written to stop before it can rather than
	 * to trust the caller. */
	for (i = 0u; (uint32_t)i + 4u <= (uint32_t)size; i = (uint16_t)(i + 4u)) {
		uint32_t word = sys_get_be32(&buffer[i]);

		sys_put_le32(word, &buffer[i]);
	}
}

uint8_t VL53L8CX_WaitMs(VL53L8CX_Platform *p_platform, uint32_t TimeMs)
{
	ARG_UNUSED(p_platform);

	k_msleep((int32_t)TimeMs);
	return VENDOR_OK;
}
