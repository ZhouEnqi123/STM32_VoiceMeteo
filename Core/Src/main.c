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
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "aht20.h"
#include <stdio.h>
#include <string.h>
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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  /* 初始化 AHT20，若不存在设备则在运行时软重试 */
  AHT20_Init();

  /* 温湿度变量：初始化为示例值，若传感器可用会被覆盖 */
  float temperature = 26.2f; /* 初始演示温度 */
  float humidity = 55.2f;    /* 初始演示湿度 */
  char message[64];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 心跳灯 */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    /* 新布局：顶栏(0..15)、中层(16..39)、底层(40..63) */
    OLED_NewFrame();

    /* 顶栏：左 WiFi 图标、居中城市名、右侧闹钟状态（暂空） */
    OLED_DrawImage(2, 0, &WIFIImg, OLED_COLOR_NORMAL); /* 左侧WiFi图标，16x16 */
      const char *city = "Jinhua"; /* 城市名（居中显示） */
      /* 预先计算并写入的常量居中位置（字符数 6 × 8 = 48，(128-48)/2 = 40） */
      OLED_PrintASCIIString(40, 0, (char *)city, &afont16x8, OLED_COLOR_NORMAL);

    /* 中层：大字号时间（目前固定显示 00:00，稍后接入 RTC） */
    const char *time_str = "00:00";
      /* 使用驱动中已有的大号 ASCII 字库 afont24x12（每字宽12，高24）
         预计算居中位置：5×12=60，(128-60)/2=34 */
      OLED_PrintASCIIString(34, 20, (char *)time_str, &afont24x12, OLED_COLOR_NORMAL);

    /* 在绘制底层之前检查 AHT20 是否可用；不可用时软重试并显示 AHT20 OFF */
    int aht_ok = 0;
    if (AHT20_IsReady() == 0) {
      /* 设备就绪，测量并读取温湿度 */
      AHT20_Measure();
      temperature = AHT20_Temperature();
      humidity = AHT20_Humidity();
      aht_ok = 1;
    } else {
      /* 设备不可用：不进行软重启，仅标记为不可用，显示 AHT20 OFF */
      aht_ok = 0;
    }

    if (aht_ok) {
        /* 左侧温度图标从 (0,48) 开始，文本紧随其后 */
        OLED_DrawImage(0, 48, &temperatureImg, OLED_COLOR_NORMAL);
        sprintf(message, "%.1f℃", temperature);
        OLED_PrintString(16, 48, message, &font16x16, OLED_COLOR_NORMAL);

        /* 右侧湿度保持原位（靠右显示） */
        OLED_DrawImage(70, 48, &humidityImg, OLED_COLOR_NORMAL);
        sprintf(message, "%.1f%%", humidity);
        OLED_PrintString(86, 48, message, &font16x16, OLED_COLOR_NORMAL);
    } else {
        /* 居中显示 AHT20 OFF（预计算居中位置：9×8=72，(128-72)/2=28） */
        OLED_PrintASCIIString(28, 48, (char *)"AHT20 OFF", &afont16x8, OLED_COLOR_NORMAL);
    }

    OLED_ShowFrame();

    HAL_Delay(1000);

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
