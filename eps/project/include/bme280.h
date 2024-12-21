/*
 * IncFile1.h
 *
 * Created: 2/12/2024 9:44:56 am
 *  Author: w_sim
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

int32_t BME280_ReadRawTemperature(void);
float BME280_CalculateTemperature(int32_t raw_temp);
void SPI_Init(void);
void BME280_Init(void);

uint8_t BME280_ReadReg(uint8_t reg);


#endif /* INCFILE1_H_ */