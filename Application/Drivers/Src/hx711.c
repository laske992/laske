/*
 * hx711.c
 *
 *  Created on: 31. sij 2022.
 *      Author: User
 */

#include "gpio.h"

/* gain to pulse and scale conversion */
#define HX711_GAIN_MAX      3
#define HX711_RESET_GAIN    128

struct hx711_gain_to_scale {
    int gain;
    int gain_pulse;
    int scale;
    int channel;
};

/*
 * .scale depends on AVDD which in turn is known as soon as the regulator
 * is available
 * therefore we set .scale in hx711_probe()
 *
 * channel A in documentation is channel 0 in source code
 * channel B in documentation is channel 1 in source code
 */
static struct hx711_gain_to_scale hx711_gain_to_scale[HX711_GAIN_MAX] = {
        { 128, 1, 0, 0 },
        {  32, 2, 0, 1 },
        {  64, 3, 0, 0 }
};

struct hx711_data {
    struct gpio_t   gpio_pd_sck;
    struct gpio_t   gpio_dout;
    uint8_t         gain_set;
    uint8_t         channel;
};

/* Global variables */
struct hx711_data hx711_data = {
        .gpio_pd_sck = {
                .gpio_d = HX711_GPIO_Port,
                .pin = HX711_PD_SCK_Pin
        },
        .gpio_dout = {
                .gpio_d = HX711_GPIO_Port,
                .pin = HX711_DOUT_Pin
        },
        .gain_set = HX711_RESET_GAIN,
        .channel = 0    // channel A
};
/******************************************************/

/* Static Functions */
static int hx711_read();
static int hx711_cycle();
static int hx711_read_average(uint8_t);
static void hx711_power_on();
static void hx711_power_off();
static bool hx711_is_powered_on();
static bool hx711_data_is_ready();
static bool hx711_wait_for_ready();
static int hx711_get_gain_to_pulse(int);
static int hx711_get_gain_to_scale(int);
static int hx711_get_scale_to_gain(int);

/******************************************************/

void
hx711_init()
{

}

/**
 * @brief: A cycle for reading one bit from HX711.
 * @retval: Bit value (0 or 1).
 */
static int
hx711_cycle()
{
    gpio_set_value(&hx711_data.gpio_pd_sck, 1);

    /*
     * wait until DOUT is ready
     * it turned out that parasitic capacities are extending the time
     * until DOUT has reached it's value
     */
    delay_us(1);

    gpio_set_value(&hx711_data.gpio_pd_sck, 0);

    /*
     * make it a square wave for addressing cases with capacitance on
     * PC_SCK
     */
    delay_us(1);

    /* sample as late as possible */
    return gpio_get_value(&hx711_data.gpio_dout);
}

/**
 * @brief: Read measurement from HX711. Implemented
 * according to datasheet.
 * @retval: The 24bit value.
 */
static int
hx711_read()
{
    int i, ret;
    int value = 0;

    /* we double check if it's really down */
    if (hx711_data_is_ready() == false)
    {
        return -1;
    }

    /*
     * if preempted for more then 60us while PD_SCK is high:
     * hx711 is going in reset
     * ==> measuring is false
     */
    taskENTER_CRITICAL();

    for (i = 0; i < 24; i++)
    {
        value <<= 1;
        ret = hx711_cycle();
        if (ret)
        {
            value++;
        }
    }

    value ^= 0x800000;

    // Additional pulses to avoid communication errors. The number
    // of pulses depends on the gain.
    for (i = 0; i < hx711_get_gain_to_pulse(hx711_data.gain_set); i++)
    {
        hx711_cycle();
    }

    taskEXIT_CRITICAL();

    return value;
}

/**
 * @brief: Read as many as n times the value.
 * @retval: Average value off all readings.
 */
static int
hx711_read_average(uint8_t n)
{
    int i;
    int sum = 0;

    if (n == 0)
    {
        return -1;
    }

    for (i = 0; i < n; i++)
    {
        if (hx711_wait_for_ready() == false)
        {
            return -1;
        }
        sum += hx711_read();
    }
    return (sum / n);
}

static int
hx711_reset()
{
    bool val = hx711_wait_for_ready();

    if (val == false) {
        /*
         * an examination with the oscilloscope indicated
         * that the first value read after the reset is not stable
         * if we reset too short;
         * the shorter the reset cycle
         * the less reliable the first value after reset is;
         * there were no problems encountered with a value
         * of 10 ms or higher
         */
        gpio_set_value(&hx711_data.gpio_pd_sck, 1);
        delay_ms(10);
        gpio_set_value(&hx711_data.gpio_pd_sck, 0);

        val = hx711_wait_for_ready();

        /* after a reset the gain is 128 */
        hx711_data.gain_set = HX711_RESET_GAIN;
    }

    return val;
}

/**
 * @brief: Turn HX711 on. Pin PD_SCK is low in normal
 * operating mode.
 */
static void
hx711_power_on()
{
    gpio_set_value(&hx711_data.gpio_pd_sck, 0);
}

/**
 * @brief: Turn HX711 off. Chip will be in power down mode
 * as long as PD_SCK is high.
 *
 */
static void
hx711_power_off()
{
    gpio_set_value(&hx711_data.gpio_pd_sck, 0);
    gpio_set_value(&hx711_data.gpio_pd_sck, 1);
}

/**
 * @brief: Check if HX711 is powered on.
 * @retval: True if powered on, false otherwise.
 */
static bool
hx711_is_powered_on()
{
    return !(gpio_get_value(&hx711_data.gpio_pd_sck));
}

/**
 * @brief: Check if data is ready for retrieval. Data is
 * ready when DOUT is low.
 * @retval: true if ready, false otherwise
 */
static bool
hx711_data_is_ready()
{
    return !(gpio_get_data(&hx711_data.gpio_dout));
}

/**
 * @brief: Wait 1 second for data to be ready for retrieval.
 * @retval: true if data become ready, false otherwise
 */
static bool
hx711_wait_for_ready()
{
    int i;
    bool ret = false;

    /*
     * in some rare cases the reset takes quite a long time
     * especially when the channel is changed.
     * Allow up to one second for it
     */
    for (i = 0; i < 100; i++)
    {
        ret = hx711_data_is_ready();
        if (ret)
        {
            break;
        }
        /* sleep at least 10 ms */
        delay_ms(10);;
    }
    return ret;
}

static int
hx711_get_gain_to_pulse(int gain)
{
    int i;

    for (i = 0; i < HX711_GAIN_MAX; i++)
    {
        if (hx711_gain_to_scale[i].gain == gain)
        {
            return hx711_gain_to_scale[i].gain_pulse;
        }
    }
    return 1;
}

static int
hx711_get_gain_to_scale(int gain)
{
    int i;

    for (i = 0; i < HX711_GAIN_MAX; i++)
        if (hx711_gain_to_scale[i].gain == gain)
            return hx711_gain_to_scale[i].scale;
    return 0;
}

static int
hx711_get_scale_to_gain(int scale)
{
    int i;

    for (i = 0; i < HX711_GAIN_MAX; i++)
        if (hx711_gain_to_scale[i].scale == scale)
            return hx711_gain_to_scale[i].gain;
    return -1;
}
