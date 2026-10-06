/* projects/03-one-sensor/src/main.c: the whole application, and it knows no bus.
 *
 * THIS FILE IS THE CLAIM. It is compiled unchanged for the two-wire build and the four-wire
 * build, and docs/DESIGN.md's first acceptance criterion is that the resulting object is
 * byte for byte identical between them. If it is not, something here depends on the bus and
 * the difference names what.
 *
 * So read what is absent. There is no i2c.h and no spi.h. There is no address, no pin, no
 * bitrate and no chip select. There is no #ifdef and no build flag naming a transport. The
 * entire knowledge this file has of the hardware is one devicetree alias, `motion`, and the
 * promise that whatever is behind it reports acceleration.
 *
 * What makes that work is not discipline, it is the binding: the overlay hangs the part off
 * a controller, the driver implements both transports, and the sensor API is the same shape
 * either way. The application asks for a reading and is told one.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <stdio.h>
#include <stdlib.h>   /* abs, for printing the fractional part unsigned */

/* The one piece of hardware knowledge in the file, and it names no bus. Both overlays end
 * with the same alias, which is what lets this line be the only thing they have in common. */
static const struct device *const motion = DEVICE_DT_GET(DT_ALIAS(motion));

/* How many readings before the application stops. A fixed count rather than a loop forever,
 * so the run ends and a log can be compared against another run. */
#define READINGS 10

int main(void)
{
    struct sensor_value accel[3];
    int i;

    printf("one sensor, on whichever bus the overlay described\n\n");

    /* CRITERION 3 LIVES HERE. A device that is not ready is reported and the application
     * returns. It does not carry on and print zeroes, which is the failure that criterion
     * exists to exclude: zeroes from a sensor that was never reached are indistinguishable
     * from zeroes from a sensor lying flat, and only one of those is a measurement. */
    if (!device_is_ready(motion)) {
        printf("  FAIL %s is not ready, so nothing below would be a reading\n",
               motion->name);
        return 1;
    }

    printf("device %s is ready\n", motion->name);

    for (i = 0; i < READINGS; i++) {
        int rc = sensor_sample_fetch(motion);

        if (rc != 0) {
            printf("  FAIL sample_fetch returned %d\n", rc);
            return 1;
        }

        rc = sensor_channel_get(motion, SENSOR_CHAN_ACCEL_XYZ, accel);
        if (rc != 0) {
            printf("  FAIL channel_get returned %d\n", rc);
            return 1;
        }

        /* Printed in the sensor API's own units, which are metres per second squared, so a
         * board lying flat reads about 9.8 on one axis and about 0 on the other two. That
         * is criterion 2, and it is the reason the axes are printed separately rather than
         * as a magnitude: a magnitude would hide an axis swap between the two builds. */
        printf("x %6d.%06d  y %6d.%06d  z %6d.%06d\n",
               accel[0].val1, abs(accel[0].val2),
               accel[1].val1, abs(accel[1].val2),
               accel[2].val1, abs(accel[2].val2));

        k_msleep(200);
    }

    printf("\n%d readings taken\n", READINGS);
    return 0;
}
