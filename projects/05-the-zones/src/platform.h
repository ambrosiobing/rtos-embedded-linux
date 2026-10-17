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

/* HOW MANY TARGETS EACH ZONE REPORTS, and this is a decision rather than a default that was
 * copied. The vendor's own porting header sets it to one and says what it costs: the value tunes
 * the size of a bus transaction and the memory the result occupies, and a lower number means
 * less of both.
 *
 * One is right for this project and not merely cheapest. A zone that reports several targets is
 * reporting a surface and whatever is behind it, which matters to a reader measuring a shelf
 * through a window. What P01 asks of this part is how much of a room is occupied and roughly
 * where, so the nearest surface in each of sixty-four directions is the whole of the question,
 * and the further returns would be sixty-four more numbers nothing consumes.
 *
 * **The number is in the capture when this project takes one**, because a grid read with one
 * target per zone and a grid read with four are not the same measurement, and a figure that
 * does not say which it is cannot be compared with anybody else's. */
#ifndef VL53L8CX_NB_TARGET_PER_ZONE
#define VL53L8CX_NB_TARGET_PER_ZONE (1U)
#endif

/* The vendor's header also offers a raw data format and nine switches that each drop one field
 * from the result to shorten the transaction. **None is used here and that is deliberate for
 * now**: dropping a field is an optimisation, this project has measured nothing yet, and an
 * optimisation chosen before a measurement is a guess that is hard to take back. The fields are
 * listed in the vendor's porting header for whoever reaches that point with a figure in hand. */

typedef struct {
	/* Controller and address in one, from the devicetree, so the porting layer cannot
	 * disagree with the description about where the part is. */
	struct i2c_dt_spec i2c;

	/* THE LIBRARY WRITES THIS AND THIS PORTING LAYER DOES NOT READ IT, which is said here
	 * because that is a limitation rather than an oversight. `vl53l8cx_set_i2c_address`
	 * assigns to it, so the member has to exist or the library does not compile; the six
	 * functions below use the devicetree specification instead, so **changing a part's
	 * address at run time is not supported by this port.** The shield on this bench carries
	 * one part and has no reason to, and a shield with three would: that is the case the
	 * tree's overlay for the single-point generation shows, where three identical parts are
	 * moved to three addresses while each is held in reset in turn. When this project meets
	 * that case the six functions take their address from here instead, and the compiler
	 * will not be the thing that notices. */
	uint16_t address;
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
