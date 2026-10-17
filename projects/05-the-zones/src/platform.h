/* projects/05-the-zones/src/platform.h: what the vendor's interface asks a system to provide.
 *
 * THE VENDOR'S LIBRARY IS WRITTEN AGAINST NO OPERATING SYSTEM. It calls six functions and passes
 * a pointer it never looks inside, so a system supplies this header, puts whatever it needs in
 * the structure, and implements the six. That is the whole porting layer and it is the reason
 * the same library serves a bare-metal project and this one.
 *
 * THE STRUCTURE IS OURS AND NOT THE VENDOR'S REFERENCE. ST's own porting header carries three
 * function pointers, Write, Read and GetTick, because their board support reaches hardware
 * through a layer of its own. Nothing here needs that indirection: a devicetree address
 * specification already names the controller and the address together, and the six functions
 * below can use it directly. The operating system's driver for the previous generation makes the
 * same choice, which is a point in its favour rather than a coincidence.
 *
 * WHAT IS NOT KNOWN YET AND WILL BE SAID BY A COMPILER. The vendor's interface may require more
 * from this header than the six functions: a count of targets per zone, a buffer size, a
 * compile-time option. **Those are not guessed here.** The first build will name each one it
 * cannot find, which is a better source than a reading of somebody else's header, and each will
 * be added with the line of theirs that asked for it.
 */
#ifndef P05_VL53L8CX_PLATFORM_H
#define P05_VL53L8CX_PLATFORM_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/types.h>

typedef struct {
	/* Controller and address in one, from the devicetree, so the porting layer cannot
	 * disagree with the description about where the part is. */
	struct i2c_dt_spec i2c;
} VL53L8CX_Platform;

uint8_t VL53L8CX_RdByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value);
uint8_t VL53L8CX_WrByte(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value);
uint8_t VL53L8CX_RdMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
			 uint8_t *p_values, uint32_t size);
uint8_t VL53L8CX_WrMulti(VL53L8CX_Platform *p_platform, uint16_t RegisterAdress,
			 uint8_t *p_values, uint32_t size);
void VL53L8CX_SwapBuffer(uint8_t *buffer, uint16_t size);
uint8_t VL53L8CX_WaitMs(VL53L8CX_Platform *p_platform, uint32_t TimeMs);

#endif /* P05_VL53L8CX_PLATFORM_H */
