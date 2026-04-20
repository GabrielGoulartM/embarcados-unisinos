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
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hcsr04.h"
#include "adxl345.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    ESTADO_IDLE,
    ESTADO_DISPARA_SONAR,
    ESTADO_CALCULA_SONAR,
    ESTADO_LEITURA_ADC,
    ESTADO_LEITURA_ACELEROMETRO, // <-- NOVO ESTADO
    ESTADO_ATUALIZA_ALARME
} FSM_Estado_t;

FSM_Estado_t estadoAtual = ESTADO_IDLE;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
HCSR04_Config_t meuSonar;
ADXL345_Config_t meuAcelerometro;
uint32_t tempoUltimaLeitura = 0;
uint32_t tempoUltimoBeep = 0;
uint32_t intervaloBeep = 0;

uint16_t valorADC = 0;
uint16_t volumeBuzzer = 0;
uint8_t estadoBuzzer = 0;
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
  MX_ADC2_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
    HCSR04_Init(&meuSonar, &htim2, TIM_CHANNEL_1);

    // Inicia o ADXL345. Se retornar 0, ocorreu um erro no I2C ou fios invertidos.
    ADXL345_Init(&meuAcelerometro, &hi2c1);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
      while (1)
      {
    	  // A MÁQUINA DE ESTADOS PRINCIPAL
    	        switch (estadoAtual) {

    	            case ESTADO_IDLE:
    	                if (HAL_GetTick() - tempoUltimaLeitura >= 100) {
    	                    tempoUltimaLeitura = HAL_GetTick();
    	                    estadoAtual = ESTADO_DISPARA_SONAR;
    	                }
    	                break;

    	            case ESTADO_DISPARA_SONAR:
    	                HCSR04_Trigger();
    	                estadoAtual = ESTADO_CALCULA_SONAR;
    	                break;

    	            case ESTADO_CALCULA_SONAR:
    	                if (!HCSR04_Read(&meuSonar)) {
    	                    meuSonar.distancia_cm = -1;
    	                }
    	                estadoAtual = ESTADO_LEITURA_ADC;
    	                break;

    	            case ESTADO_LEITURA_ADC:
    	                HAL_ADC_Start(&hadc2);
    	                HAL_ADC_PollForConversion(&hadc2, HAL_MAX_DELAY);
    	                valorADC = HAL_ADC_GetValue(&hadc2);
    	                volumeBuzzer = (valorADC * 999) / 4095;

    	                estadoAtual = ESTADO_LEITURA_ACELEROMETRO; // Aponta para o novo estado
    	                break;

    	            case ESTADO_LEITURA_ACELEROMETRO:
    	                // Lê os eixos X, Y e Z e calcula os ângulos automaticamente
    	                ADXL345_Read(&meuAcelerometro);

    	                estadoAtual = ESTADO_ATUALIZA_ALARME;
    	                break;

    	            case ESTADO_ATUALIZA_ALARME:
    	                if (meuSonar.distancia_cm > 60.0 || meuSonar.distancia_cm < 0) {
    	                    intervaloBeep = 0;
    	                } else if (meuSonar.distancia_cm <= 10.0) {
    	                    intervaloBeep = 1;
    	                } else {
    	                    intervaloBeep = (uint32_t)(meuSonar.distancia_cm * 10);
    	                }
    	                estadoAtual = ESTADO_IDLE;
    	                break;
    	        }

          // ==========================================
          // LÓGICA DE EXECUÇÃO DO BIPE COM CONTROLE DE VOLUME (PWM)
          // ==========================================
          if (intervaloBeep > 1) {
              // Zona de bipe intermitente
              if ((HAL_GetTick() - tempoUltimoBeep) >= intervaloBeep) {
                  tempoUltimoBeep = HAL_GetTick();
                  estadoBuzzer = !estadoBuzzer; // Inverte o estado atual do bipe

                  if (estadoBuzzer) {
                      // LIGA o buzzer com a potência definida pelo potenciômetro
                      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, volumeBuzzer);
                  } else {
                      // DESLIGA o buzzer (potência zero)
                      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                  }
              }
          } else if (intervaloBeep == 1) {
              // Zona de perigo: Bipe contínuo ligado no volume do potenciômetro
              __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, volumeBuzzer);
          } else {
              // Longe: Desligado
              __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
          }

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_TIM1
                              |RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
