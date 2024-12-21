/*
 * tasks.c
 *
 * Created: 18/8/2024 5:04:08 pm
 *  Author: huimin
 */ 

#include <tasks.h>
#include <protocol.h>
#include <avr/io.h>
#include <uart.h>

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/*csp lib*/
#include <csp/csp.h>
#include <csp/interfaces/csp_if_i2c.h>
#include <csp/drivers/i2c.h>
#include <csp/csp_endian.h>

#include <bme280.h>

TaskHandle_t I2C_task;

typedef struct {
	uint8_t current;
	uint8_t temperature;
} TM_data;

TM_data get_telemetry(unsigned char *args) {
	int32_t raw_temp = BME280_ReadRawTemperature();
	printf("raw temp: %ld\r\n", raw_temp);
	float calibrated_temp = BME280_CalculateTemperature(raw_temp);
	printf("cal temp: %d\r\n", (int) calibrated_temp);
	TM_data tm_data;
	tm_data.current = 1;
	tm_data.temperature = 27;
	return tm_data;
}

void port_map(void *pvParameters) {
	
	csp_socket_t *socket;
	csp_conn_t *conn;
	csp_packet_t *packet;
	
	socket = csp_socket(0);
	
	int ret = csp_listen(socket, 10);
	
	if (ret != CSP_ERR_NONE) {
		printf("csp listen error\r\n");
	}
		
	csp_bind(socket, CSP_ANY);
	
	int port;
	
	while (1) {
		conn = csp_accept(socket, 10);
		if (conn == NULL) {
			continue;
		}
		printf("conn received\r\n");
		packet = csp_read(conn, PROTOCOL_TIMEOUT);
		
		TM_data ret;
		
		switch (csp_conn_dport(conn))
		{
			case 1:
				ret = get_telemetry(&(packet->data));
				printf("ret: %d\r\n", ret);
				break;
			default:
				ret.current = 0;
				ret.temperature = 0;
				break;
		}
		
		memcpy(packet->data, &ret, sizeof(ret));
		packet->length = sizeof(ret);
		
		if (!csp_send(conn, packet, PROTOCOL_TIMEOUT)) {
			csp_buffer_free(packet);
		}
		
		csp_close(conn);
	}
}