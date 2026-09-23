#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>
#include <string.h>
#include <stdlib.h>

/****************************
 * Remember to add line:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 * to prj.conf
 *
 * CONFIG_TIMING_FUNCTIONS=y
 * to prj.conf
 ****************************/

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec yellow = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5


// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);


// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);


// FIFO dispatcher data type
struct data_t {
	void *fifo_reserved;
	char msg[20];
};


K_MUTEX_DEFINE(light_mutex);

K_CONDVAR_DEFINE(red_condvar);
K_CONDVAR_DEFINE(yellow_condvar);
K_CONDVAR_DEFINE(green_condvar);

K_SEM_DEFINE(release_sem, 0, 1);


// Task timing
uint64_t red_time = 0;
uint64_t yellow_time = 0;
uint64_t green_time = 0;


int init_led(void);
int init_uart(void);

void uart_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);


//tavoittelen toistaiseksi 1 pistettä, sillä tein 1p arvoisen ohjelman.
int main(void)
{
	int ret = init_led();

	if (ret != 0) {
		return ret;
	}

	ret = init_uart();

	if (ret != 0) {
		return ret;
	}

	//printk("Program started\n");

	return 0;
}


// Initialize leds
int init_led() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");
		return ret;
	}

	// set led off
	gpio_pin_set_dt(&red,0);


	// Led pin initialization
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");
		return ret;
	}

	// set led off
	gpio_pin_set_dt(&green,0);


	// Led pin initialization
	ret = gpio_pin_configure_dt(&yellow, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");
		return ret;
	}

	// set led off
	gpio_pin_set_dt(&yellow,0);


	//printk("Led initialized ok\n");

	return 0;
}


// Initialize UART
int init_uart() {

	if (!device_is_ready(uart_dev)) {
		printk("UART initialization failed!\n");
		return 1;
	}

	//printk("UART initialized ok\n");

	return 0;
}


// UART task
void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;

	// Message from UART
	char uart_msg[20];

	memset(uart_msg,0,20);

	int uart_msg_cnt = 0;


	while (true) {

		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {

			// If character is not newline, add to UART message buffer
			if (rc != '\r' && rc != '\n') {

				
				if (rc == 'r' ||
				    rc == 'y' ||
				    rc == 'g') {

					if (uart_msg_cnt < 19) {

						uart_msg[uart_msg_cnt] = rc;
						uart_msg_cnt++;
					}
				}
			}

			// Character is newline, copy dispatcher data and put to FIFO buffer
			else {

				if (uart_msg_cnt > 0) {

					//printk("UART msg: %s\n", uart_msg);


					// Create dispatcher data
					struct data_t *buf =
						k_malloc(sizeof(struct data_t));

					if (buf == NULL) {
						printk("Memory allocation failed\n");
						return;
					}


					// Copy UART message to dispatcher data
					strncpy(buf->msg,
						uart_msg,
						sizeof(buf->msg) - 1);

					buf->msg[sizeof(buf->msg) - 1] = '\0';

					// You need to:
					// Put dispatcher data to FIFO buffer
					k_fifo_put(&dispatcher_fifo, buf);


					// Clear UART message buffer
					uart_msg_cnt = 0;
					memset(uart_msg,0,20);
				}
			}
		}

		k_msleep(10);
	}
}


// Dispatcher task
void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {

		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item =
			k_fifo_get(&dispatcher_fifo, K_FOREVER);


		char sequence[20];

		memcpy(sequence,rec_item->msg,20);

		k_free(rec_item);


		//printk("Dispatcher: %s\n", sequence);

		for (int i = 0; sequence[i] != '\0'; i++) {

			char color = sequence[i];


			// Red
			if (color == 'r') {

				//printk("RED\n");

				k_mutex_lock(&light_mutex, K_FOREVER);

				k_condvar_signal(&red_condvar);

				k_mutex_unlock(&light_mutex);
			}


			// Yellow
			else if (color == 'y') {

				//printk("YELLOW\n");

				k_mutex_lock(&light_mutex, K_FOREVER);

				k_condvar_signal(&yellow_condvar);

				k_mutex_unlock(&light_mutex);
			}


			// Green
			else if (color == 'g') {

				//printk("GREEN\n");

				k_mutex_lock(&light_mutex, K_FOREVER);

				k_condvar_signal(&green_condvar);

				k_mutex_unlock(&light_mutex);
			}

			k_sem_take(&release_sem, K_FOREVER);
		}

		printk("Sequence total time: %llu us\n",
		       red_time + yellow_time + green_time);
	}
}


// Task to handle red led
void red_led_task(void *, void *, void*) {

	//printk("Red led thread started\n");


	while (true) {

		k_mutex_lock(&light_mutex, K_FOREVER);

		k_condvar_wait(&red_condvar,
			       &light_mutex,
			       K_FOREVER);

		k_mutex_unlock(&light_mutex);


		uint64_t red_start_time = k_uptime_get();


		// led paalle
		gpio_pin_set_dt(&red,1);

		//printk("Red on\n");


		// nuku 1s
		k_sleep(K_SECONDS(1));


		// led pois
		gpio_pin_set_dt(&red,0);

		//printk("Red off\n");


		uint64_t red_end_time = k_uptime_get();

		uint64_t timing_us =
			(red_end_time - red_start_time) * 1000;

		red_time = timing_us;

		printk("Red task time: %llu us\n", timing_us);


		k_sem_give(&release_sem);
	}
}


// Task to handle green led
void green_led_task(void *, void *, void*) {

	//printk("Green led thread started\n");


	while (true) {

		k_mutex_lock(&light_mutex, K_FOREVER);

		k_condvar_wait(&green_condvar,
			       &light_mutex,
			       K_FOREVER);

		k_mutex_unlock(&light_mutex);


		uint64_t green_start_time = k_uptime_get();

		gpio_pin_set_dt(&green,1);

		//printk("Green on\n");

		k_sleep(K_SECONDS(1));

		gpio_pin_set_dt(&green,0);

		//printk("Green off\n");

		uint64_t green_end_time = k_uptime_get();

		uint64_t timing_us =
			(green_end_time - green_start_time) * 1000;

		green_time = timing_us;

		printk("Green task time: %llu us\n", timing_us);


		k_sem_give(&release_sem);
	}
}


// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {

	//printk("Yellow led thread started\n");


	while (true) {

		
		k_mutex_lock(&light_mutex, K_FOREVER);

		k_condvar_wait(&yellow_condvar,
			       &light_mutex,
			       K_FOREVER);

		k_mutex_unlock(&light_mutex);


		uint64_t yellow_start_time = k_uptime_get();


		// Yellow
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);

		//printk("Yellow on\n");


		// nuku 1s
		k_sleep(K_SECONDS(1));


		// led pois
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);

		//printk("Yellow off\n");


		uint64_t yellow_end_time = k_uptime_get();

		uint64_t timing_us =
			(yellow_end_time - yellow_start_time) * 1000;

		yellow_time = timing_us;

		printk("Yellow task time: %llu us\n", timing_us);


		k_sem_give(&release_sem);
	}
}


K_THREAD_DEFINE(
	dis_thread,
	STACKSIZE,
	dispatcher_task,
	NULL,
	NULL,
	NULL,
	PRIORITY,
	0,
	0
);


K_THREAD_DEFINE(
	uart_thread,
	STACKSIZE,
	uart_task,
	NULL,
	NULL,
	NULL,
	PRIORITY,
	0,
	0
);


K_THREAD_DEFINE(
	red_thread,
	STACKSIZE,
	red_led_task,
	NULL,
	NULL,
	NULL,
	PRIORITY,
	0,
	0
);


K_THREAD_DEFINE(
	green_thread,
	STACKSIZE,
	green_led_task,
	NULL,
	NULL,
	NULL,
	PRIORITY,
	0,
	0
);


K_THREAD_DEFINE(
	yellow_thread,
	STACKSIZE,
	yellow_led_task,
	NULL,
	NULL,
	NULL,
	PRIORITY,
	0,
	0
);