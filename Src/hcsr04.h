#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f3xx_hal.h"

// Estrutura para facilitar a passagem de dados
typedef struct {
    TIM_HandleTypeDef *htim; // Qual timer estamos usando (ex: &htim2)
    uint32_t channel;        // Qual canal (ex: TIM_CHANNEL_1)
    float distancia_cm;      // Última distância calculada
} HCSR04_Config_t;

// Protótipos das funções
void HCSR04_Init(HCSR04_Config_t *config, TIM_HandleTypeDef *htim, uint32_t channel);
void HCSR04_Trigger(void);
uint8_t HCSR04_Read(HCSR04_Config_t *config); // Retorna 1 se sucesso, 0 se erro

#endif
