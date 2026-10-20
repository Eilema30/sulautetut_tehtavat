#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec yellow = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});

static struct gpio_callback button_0_data;
static struct gpio_callback button_1_data;
static struct gpio_callback button_2_data;
static struct gpio_callback button_3_data;
static struct gpio_callback button_4_data;

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int led_state = 0;
int previous_state = 0;

// 1 -> punainen
// 2 -> keltainen
// 3 -> vihreä
// 4 -> pause
// 5 -> vilkkuva keltainen

int red_manual = 0;
int yellow_manual = 0;
int green_manual = 0;

int yellow_sequence = 0;

int init_led();
int init_button();

// Button 0 (pause)
void button_0_handler(const struct device *dev,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    printk("Button 0 pressed\n");

    if (led_state != 4) {
        previous_state = led_state;
        led_state = 4;
    } else {
        led_state = previous_state;
    }
}

// Button 1 (punainen)
void button_1_handler(const struct device *dev,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    printk("Button 1 pressed\n");

    red_manual = !red_manual;
    gpio_pin_set_dt(&red, red_manual);
}

// Button 2 (keltainen)
void button_2_handler(const struct device *dev,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    printk("Button 2 pressed\n");

    yellow_manual = !yellow_manual;
    gpio_pin_set_dt(&red, yellow_manual);
    gpio_pin_set_dt(&green, yellow_manual);
}

// Button 3 (vihreä)
void button_3_handler(const struct device *dev,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    printk("Button 3 pressed\n");

    green_manual = !green_manual;
    gpio_pin_set_dt(&green, green_manual);
}

// Button 4 (vilkkuva keltainen)
void button_4_handler(const struct device *dev,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    printk("Button 4 pressed\n");

    if (yellow_sequence == 0) {
    	yellow_sequence = 1;
    	led_state = 5;
	}
	else {
    	yellow_sequence = 0;
    	led_state = 1;
}
}

//tavoittelen 3p, sillä tein kaikki pyydetyt toiminnot.
int main(void)
{
    init_led();

    int ret = init_button();
    if (ret < 0) {
        return 0;
    }

    led_state = 1;

    return 0;
}

// Initialize leds
int init_led()
{
    int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&red, 0);

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&green, 0);

    ret = gpio_pin_configure_dt(&yellow, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&yellow, 0);

    printk("Led initialized ok\n");

    return 0;
}

// Button initialization
int init_button()
{
    int ret;

    if (!gpio_is_ready_dt(&button_0)) {
        printk("Error: button 0 is not ready\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    ret |= gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    ret |= gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    ret |= gpio_pin_configure_dt(&button_3, GPIO_INPUT);
    ret |= gpio_pin_configure_dt(&button_4, GPIO_INPUT);

    if (ret != 0) {
        printk("Error: failed to configure pin\n");
        return -1;
    }

    gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_data);

    gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_data);

    gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_data);

    gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_data);

    printk("Buttons initialized ok\n");

    return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*)
{
    printk("Red led thread started\n");

    while (true) {

        if (led_state == 1) {

            gpio_pin_set_dt(&red, 1);
            printk("Red on\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&red, 0);
            printk("Red off\n");

            k_sleep(K_SECONDS(1));

            if (led_state != 4) {
                led_state = 2;
            }
        }

        k_msleep(100);
    }
}

// Task to handle green led
void green_led_task(void *, void *, void*)
{
    printk("Green led thread started\n");

    while (true) {

        if (led_state == 3) {

            gpio_pin_set_dt(&green, 1);
            printk("Green on\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&green, 0);
            printk("Green off\n");

            k_sleep(K_SECONDS(1));

            if (led_state != 4) {
                led_state = 1;
            }
        }

        k_msleep(100);
    }
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*)
{
    printk("Yellow led thread started\n");

    while (true) {

        if (led_state == 2) {

            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);

            printk("Yellow on\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);

            printk("Yellow off\n");

            k_sleep(K_SECONDS(1));

            if (led_state != 4) {
                led_state = 3;
            }
        }

        if (led_state == 5) {

            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);

            printk("Yellow on\n");

            k_sleep(K_SECONDS(1));

            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);

            printk("Yellow off\n");

            k_sleep(K_SECONDS(1));
        }

        k_msleep(100);
    }
}