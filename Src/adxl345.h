#ifndef ADXL345_H
#define ADXL345_H

#include "stm32f3xx_hal.h"

// Endereço do sensor (0x53 deslocado 1 bit para a esquerda para o HAL)
#define ADXL345_ADDR (0x53 << 1)

// Registradores principais segundo o Datasheet
#define REG_DEVID       0x00
#define REG_POWER_CTL   0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0      0x32

typedef struct {
    I2C_HandleTypeDef *hi2c;
    int16_t eixo_x;
    int16_t eixo_y;
    int16_t eixo_z;
    float inclinacao_x; // Ângulo em graus
    float inclinacao_y; // Ângulo em graus
} ADXL345_Config_t;

// Funções da biblioteca
uint8_t ADXL345_Init(ADXL345_Config_t *dev, I2C_HandleTypeDef *hi2c);
void ADXL345_Read(ADXL345_Config_t *dev);

#endif
