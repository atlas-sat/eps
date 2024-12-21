#define F_CPU 16000000L // Specify oscillator frequency

#include <protocol.h>
#include <tasks.h>
#include <uart.h>

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

// CSP Libs
#include <csp/csp.h>
#include <csp/interfaces/csp_if_i2c.h>
#include <csp/drivers/i2c.h>

#include <bme280.h>
#include <avr/delay.h>
extern TaskHandle_t I2C_task;

int main(void)
{
	// Initialize UART
	// Baud Rate: 57600
	uart_init();
	
	SPI_Init();
	_delay_ms(1000);
	BME280_Init();
	
	uint8_t id = BME280_ReadReg(0xD0);
	printf("id: %d\r\n", id);
	
	// while(id != 0x58);
	
	printf("node\r\n");
	
	// CSP-related settings
	csp_buffer_init(2, 300); // Creating 2 buffers of size 300 bytes each
	csp_init(PROTOCOL_SUBSYS_ADDR); // Initialize to node's address
	csp_i2c_init(PROTOCOL_SUBSYS_ADDR, 0, 400); // Setting the I2C node to node's address and speeds
	csp_route_set(PROTOCOL_OBC_ADDR, &csp_if_i2c, CSP_NODE_MAC);
	csp_route_start_task(500, 1); // Start the router task in CSP
	csp_rtable_print();
	printf("\r\n");
	
	//// Debugging task to see that the FreeRTOS works
	//extern void task_led_blinky(void *pvParameters);
	//xTaskCreate(task_led_blinky, "Task to blink led pin 7", 100, NULL, 2, NULL);
	
	//// Setup the CSP OBC ping task
	//extern void csp_twoway_server(void *pvParameters);
	//xTaskCreate(csp_twoway_server, "Task for CSP server", 250, NULL, 4, &I2C_task);
	
	extern void port_map(void *pvParameters);
	xTaskCreate(port_map, "ABC", 400, NULL, 2, NULL);
	
	// Start Scheduler
	vTaskStartScheduler();
	
	/*--------------------------------*/

	/* Execution will only reach here if there was insufficient heap to start the scheduler. */
	for ( ;; );
	
	return 0;
}
