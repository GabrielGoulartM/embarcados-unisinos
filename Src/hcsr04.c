#include "hcsr04.h"

void HCSR04_Init(HCSR04_Config_t *config, TIM_HandleTypeDef *htim, uint32_t channel) {
    config->htim = htim;
    config->channel = channel;
    config->distancia_cm = -1; // Inicia com valor inválido
}

void HCSR04_Trigger(void) {
    // 1. DISPARO: Envia pulso de 10us para o TRIG (PA1)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    for(volatile int i=0; i<150; i++) {};
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
}

uint8_t HCSR04_Read(HCSR04_Config_t *config) {
    uint32_t p1 = 0, p2 = 0, delta = 0;
    uint32_t timeout = 0;

    // Garante que o timer está limpo
    HAL_TIM_IC_Stop(config->htim, config->channel);
    __HAL_TIM_SET_CAPTUREPOLARITY(config->htim, config->channel, TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_CLEAR_FLAG(config->htim, TIM_FLAG_CC1);
    HAL_TIM_IC_Start(config->htim, config->channel);

    // Espera o sinal ECHO subir
    while (__HAL_TIM_GET_FLAG(config->htim, TIM_FLAG_CC1) == RESET) {
        if(++timeout > 50000) return 0; // Erro de timeout
    }
    p1 = HAL_TIM_ReadCapturedValue(config->htim, config->channel);
    __HAL_TIM_CLEAR_FLAG(config->htim, TIM_FLAG_CC1);

    // Espera o sinal ECHO descer
    HAL_TIM_IC_Stop(config->htim, config->channel);
    __HAL_TIM_SET_CAPTUREPOLARITY(config->htim, config->channel, TIM_INPUTCHANNELPOLARITY_FALLING);
    HAL_TIM_IC_Start(config->htim, config->channel);

    timeout = 0;
    while (__HAL_TIM_GET_FLAG(config->htim, TIM_FLAG_CC1) == RESET) {
        if(++timeout > 50000) return 0; // Erro de timeout
    }
    p2 = HAL_TIM_ReadCapturedValue(config->htim, config->channel);

    // CÁLCULO
    if (p2 >= p1) {
        delta = p2 - p1;
    } else {
        delta = (0xFFFFFFFF - p1) + p2;
    }
    config->distancia_cm = (delta * 0.0343) / 2.0;

    HAL_TIM_IC_Stop(config->htim, config->channel);
    return 1; // Sucesso
}
