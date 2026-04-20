#include "adxl345.h"
#include <math.h>

uint8_t ADXL345_Init(ADXL345_Config_t *dev, I2C_HandleTypeDef *hi2c) {
    dev->hi2c = hi2c;
    uint8_t chip_id = 0;

    // 1. Verifica se o sensor existe (Lê o ID dele, que deve ser sempre 0xE5)
    HAL_I2C_Mem_Read(dev->hi2c, ADXL345_ADDR, REG_DEVID, 1, &chip_id, 1, 100);
    if (chip_id != 0xE5) {
        return 0; // Erro: Sensor não encontrado ou fio invertido
    }

    // 2. Configura Resolução Completa (+-16g)
    uint8_t format = 0x0B;
    HAL_I2C_Mem_Write(dev->hi2c, ADXL345_ADDR, REG_DATA_FORMAT, 1, &format, 1, 100);

    // 3. Tira do modo Standby e inicia as medições
    uint8_t power = 0x08;
    HAL_I2C_Mem_Write(dev->hi2c, ADXL345_ADDR, REG_POWER_CTL, 1, &power, 1, 100);

    return 1; // Sucesso
}

void ADXL345_Read(ADXL345_Config_t *dev) {
    uint8_t data[6];

    // Lê 6 bytes consecutivos (X0, X1, Y0, Y1, Z0, Z1) a partir do registrador 0x32
    HAL_I2C_Mem_Read(dev->hi2c, ADXL345_ADDR, REG_DATAX0, 1, data, 6, 100);

    // Junta os bytes (LSB com MSB) para formar inteiros de 16 bits
    dev->eixo_x = (int16_t)((data[1] << 8) | data[0]);
    dev->eixo_y = (int16_t)((data[3] << 8) | data[2]);
    dev->eixo_z = (int16_t)((data[5] << 8) | data[4]);

    // Calcula a inclinação (angulação) usando trigonometria
    dev->inclinacao_x = atan2(dev->eixo_y, dev->eixo_z) * 180.0 / 3.14159;
    dev->inclinacao_y = atan2(-dev->eixo_x, sqrt(dev->eixo_y * dev->eixo_y + dev->eixo_z * dev->eixo_z)) * 180.0 / 3.14159;
}
