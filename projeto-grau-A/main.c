/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */
// Variáveis globais para os cálculos do Sonar
uint32_t p1 = 0;       // Tempo na borda de subida (Echo sobe)
uint32_t p2 = 0;       // Tempo na borda de descida (Echo desce)
uint32_t delta = 0;    // Diferença de tempo
float distancia_cm = 0;

// Variáveis de controle do Buzzer (Sensor de Ré)
uint32_t tempoUltimoBeep = 0;
uint32_t intervaloBeep = 0;
/* USER CODE END PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
          // 1. DISPARO: Envia pulso de 10us para o TRIG (PA1)
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
          for(volatile int i=0; i<150; i++) {}; // Atraso simples para os 10us
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

          // 2. CAPTURA: Espera o sinal ECHO (PA0) subir

          // Dica de segurança: Parar o timer antes de mudar a polaridade evita travamentos
          HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
          __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
          HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1);

          uint32_t timeout = 0;
          // Espera o som sair (com limite de tempo para não travar)
          while (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC1) == RESET) {
              if(++timeout > 50000) break;
          }

          if (timeout <= 50000) { // Se o som saiu com sucesso
              p1 = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
              __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);

              // 3. CAPTURA: Espera o sinal ECHO descer
              HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1); // Dica de segurança
              __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
              HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1);

              timeout = 0;
              // Espera o som voltar (com limite de tempo)
              while (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC1) == RESET) {
                  if(++timeout > 50000) break;
              }

              if (timeout <= 50000) { // Se o som voltou com sucesso
                  p2 = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);

                  // 4. CÁLCULO
                  if (p2 >= p1) {
                      delta = p2 - p1;
                  } else {
                      delta = (0xFFFFFFFF - p1) + p2;
                  }
                  // Calcula a distância
                  distancia_cm = (delta * 0.0343) / 2.0;
              } else {
                  distancia_cm = -1; // Sinaliza erro (ex: mão longe demais)
              }
          } else {
              distancia_cm = -1; // Sinaliza erro (ex: sensor desconectado)
          }

          // Limpa e para o timer para a próxima leitura
          HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
          __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);


          // ==========================================
          // LÓGICA DO SENSOR DE RÉ (BUZZER)
          // ==========================================

          if (distancia_cm > 60.0 || distancia_cm < 0) {
              // Longe (>60cm) ou erro de leitura: Buzzer Desligado
              intervaloBeep = 0;
              HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

          } else if (distancia_cm <= 10.0) {
              // Muito perto (<= 10cm): Apito contínuo de perigo
              intervaloBeep = 1;
              HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);

          } else {
              // Zona intermediária (10 a 60cm): Bipe fica mais rápido quanto mais perto.
              // Exemplo: 50cm * 10 = 500ms entre os bipes. 20cm * 10 = 200ms.
              intervaloBeep = (uint32_t)(distancia_cm * 10);
          }

          // Executa o bipe dinâmico baseando-se no tempo (sem travar o código com HAL_Delay)
          if (intervaloBeep > 1) {
              if ((HAL_GetTick() - tempoUltimoBeep) >= intervaloBeep) {
                  tempoUltimoBeep = HAL_GetTick(); // Atualiza o cronômetro
                  HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin); // Inverte o buzzer
              }
          }

          // Um pequeno atraso no final para o HC-SR04 respirar antes do próximo tiro
          HAL_Delay(50);

      /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
