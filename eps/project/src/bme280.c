#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <uart.h>

// BME280 Registers
#define BME280_CS_LOW()  (PORTB &= ~(1 << PB0)) // CS low
#define BME280_CS_HIGH() (PORTB |= (1 << PB0))  // CS high
#define BME280_TEMP_MSB_REG   0xFA
#define BME280_TEMP_LSB_REG   0xFB
#define BME280_TEMP_XLSB_REG  0xFC
#define BME280_CTRL_MEAS_REG  0xF4
#define BME280_CONFIG_REG     0xF5
#define BME280_CALIB_START    0x88

// Global Variables for Calibration Coefficients
uint16_t dig_T1;
int16_t dig_T2, dig_T3;

// SPI Transfer Function
uint8_t SPI_Transfer(uint8_t data) {
	SPDR = data; // Load data into SPI data register
	while (!(SPSR & (1 << SPIF))); // Wait until transmission is complete
	return SPDR; // Return received data
}

// SPI Initialization
void SPI_Init(void) {
	// Set MOSI, SCK, and CS as output, MISO as input
	DDRB = (1 << PB2) | (1 << PB1) | (1 << PB0);
	PORTB |= (1 << PB0); // CS high (inactive)
	// Enable SPI, Set as Master, Clock rate fosc/16
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

// Read Register from BME280
uint8_t BME280_ReadReg(uint8_t reg) {
	uint8_t value;
	BME280_CS_LOW();
	SPI_Transfer(reg | 0x80); // Set MSB to 1 for read
	value = SPI_Transfer(0x00); // Send dummy byte to receive data
	BME280_CS_HIGH();
	return value;
}

// Read Raw Temperature
int32_t BME280_ReadRawTemperature(void) {
	int32_t raw_temp;
	uint8_t msb, lsb, xlsb;
	
	BME280_CS_LOW();
	SPI_Transfer(BME280_TEMP_MSB_REG | 0x80); // Set the register to read the MSB
	msb = SPI_Transfer(0x00);
	lsb = SPI_Transfer(0x00);
	xlsb = SPI_Transfer(0x00);
	BME280_CS_HIGH();
	
	// Combine the 3 bytes into a single 24-bit value
	raw_temp = ((int32_t)msb << 12) | ((int32_t)lsb << 4) | (xlsb >> 4);
	return raw_temp;
}

// Read Calibration Coefficients
void BME280_ReadCalibration(void) {
	dig_T1 = (BME280_ReadReg(BME280_CALIB_START + 1) << 8) | BME280_ReadReg(BME280_CALIB_START);
	dig_T2 = (BME280_ReadReg(BME280_CALIB_START + 3) << 8) | BME280_ReadReg(BME280_CALIB_START + 2);
	dig_T3 = (BME280_ReadReg(BME280_CALIB_START + 5) << 8) | BME280_ReadReg(BME280_CALIB_START + 4);
}

// BME280 Initialization
void BME280_Init(void) {
	BME280_WriteReg(BME280_CTRL_MEAS_REG, 0x27); // Normal mode, Temp/Press oversampling x1
	BME280_WriteReg(BME280_CONFIG_REG, 0xA0);   // Standby time 1000 ms, filter off
	BME280_ReadCalibration();                   // Read calibration coefficients
}

// Calculate Actual Temperature
float BME280_CalculateTemperature(int32_t raw_temp) {
	int32_t var1, var2, t_fine;
	
	// Calibration formula
	var1 = ((((raw_temp >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	var2 = (((((raw_temp >> 4) - ((int32_t)dig_T1)) * ((raw_temp >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;

	// Return calibrated temperature in Celsius
	return (float)((t_fine * 5 + 128)>> 8) / 100.0;
}

// Write to Register
void BME280_WriteReg(uint8_t reg, uint8_t value) {
	BME280_CS_LOW();
	SPI_Transfer(reg & 0x7F); // Set MSB to 0 for write
	SPI_Transfer(value);
	BME280_CS_HIGH();
}

// Main Function
int test(void) {
	uart_init(); // Initialize UART

	// Initialize SPI and BME280 sensor
	SPI_Init();
	BME280_Init();

	// Read chip ID
	uint8_t chip_id = BME280_ReadReg(0xD0);
	printf("Chip ID: %d\r\n", chip_id);

	// Main loop
	while (1) {
		int32_t raw_temp = BME280_ReadRawTemperature();  // Read raw temperature
		printf("Raw Temperature: %ld\r\n", raw_temp);    // Print raw temperature

		// Calculate the calibrated temperature
		float temperature = BME280_CalculateTemperature(raw_temp);
		//float temperature = 0.20;
		printf("Temperature: %d C\r\n", (int)temperature);  // Print calibrated temperature

		_delay_ms(1000);  // Delay for 1 second
	}
}
