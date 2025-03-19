/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>

#include "fonts.h"
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
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define LCD_CMD_DELAY_MS 0xFF
#define LCD_CMD_EOF 0xFF
void ST7735_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // CS LOW
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);  // DC LOW
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);  // CS HIGH
}

void ST7735_WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // CS LOW
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);  // DC HIGH
    HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);  // CS HIGH
}

void ST7735_SetWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    ST7735_WriteCommand(0x2A); //Column address set
    ST7735_WriteData(0x00);
    ST7735_WriteData(x1);
    ST7735_WriteData(0x00);
    ST7735_WriteData(x2);
    
    ST7735_WriteCommand(0x2B);   //Row address set
    ST7735_WriteData(0x00);
    ST7735_WriteData(y1);
    ST7735_WriteData(0x00);
    ST7735_WriteData(y2);
}




static const uint8_t u8InitCmdList[] = {
//  Command     Length      Data
    0xB1,       0x03,       0x01, 0x2C, 0x2D,                       // Frame Rate Control (In normal mode/ Full colors)
    0xB2,       0x03,       0x01, 0x2C, 0x2D,                       // Frame Rate Control (In Idle mode/ 8-colors)
    0xB3,       0x06,       0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,     // Frame Rate Control (In Partial mode/ full colors)
    0xB4,       0x01,       0x07,                                   // Display Inversion Control
    0xC0,       0x03,       0xA2, 0x02, 0x84,                       // Power Control 1
    0xC1,       0x01,       0xC5,                                   // Power Control 2
    0xC2,       0x02,       0x0A, 0x00,                             // Power Control 3 (in Normal mode/ Full colors)
    0xC3,       0x02,       0x8A, 0x2A,                             // Power Control 4 (in Idle mode/ 8-colors)
    0xC4,       0x02,       0x8A, 0xEE,                             // Power Control 5 (in Partial mode/ full-colors)
    0xC5,       0x01,       0x0E,                                   // VCOM Control 1
    0xE0,       0x10,       0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,     //Gamma adjustment(+ polarity)
    0xE1,       0x10,       0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,     //Gamma adjustment(- polarity)
    LCD_CMD_DELAY_MS, LCD_CMD_EOF
};

static void ST7735_SendCommandList(const uint8_t* cmdList)
{
    uint8_t dat = 0;
    uint8_t cmd = 0;
    uint8_t num = 0;

    while (1)
    {
        cmd = *cmdList++;
        num = *cmdList++;

        if (cmd == LCD_CMD_DELAY_MS)  {
            if (num == LCD_CMD_EOF)         /* end of list */
                break;
            else                            /* delay in 10 ms units*/
                HAL_Delay((uint32_t)(num*10));
        }
        else {
            ST7735_WriteCommand(cmd);

            for (dat = 0; dat < num; ++dat)
                ST7735_WriteData(*cmdList++);
        }
    }
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_WriteCommand(0x2C);
    for (int i = 0; i < 128 * 160; i++) {
        ST7735_WriteData(color >> 8);
        ST7735_WriteData(color & 0xFF);
    }
}

void ST7735_DrawPixel(uint8_t x, uint8_t y, uint16_t color) {
    if (x >= 128 || y >= 160) return; // Ki?m tra gi?i h?n màn hình

    ST7735_SetWindow(x, y, x + 1, y + 1); // Ð?t c?a s? 1 pixel
    ST7735_WriteCommand(0x2C); // L?nh vi?t pixel
    ST7735_WriteData(color >> 8);
    ST7735_WriteData(color & 0xFF);
}

void ST7735_DrawChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bg) {
    uint16_t i, j;
    uint16_t pixelData;

    for (i = 0; i < font.height; i++) {
        pixelData = font.data[(ch - 32) * font.height + i]; // L?y d? li?u font

        for (j = 0; j < font.width; j++) {
            if ((pixelData << j) & 0x8000) // Ki?m tra bit cao nh?t
                ST7735_DrawPixel(x + j, y + i, color);
            else
                ST7735_DrawPixel(x + j, y + i, bg);
        }
    }
}

void ST7735_DrawString(uint16_t x, uint16_t y, char* str, FontDef font, uint16_t color, uint16_t bg) {
    while (*str) {
        ST7735_DrawChar(x, y, *str, font, color, bg);
        x += font.width;
        str++;
    }
}


void ST7735_Init()
{
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); // RESET = 0
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);   // RESET = 1
    HAL_Delay(150);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // CS = 0
  
    ST7735_WriteCommand(0x01); // Software Reset
    HAL_Delay(150);
    ST7735_WriteCommand(0x11); // Sleep Out
    HAL_Delay(255);

    ST7735_WriteCommand(0x36); // Memory Access Control
    ST7735_WriteData(0x08);  // Row/Column exchange, RGB color filter


    ST7735_WriteCommand(0x3A); //Interface Pixel Format
    ST7735_WriteData(0x05);


    ST7735_SetWindow(0, 0, 128, 160);

    ST7735_SendCommandList(u8InitCmdList);

    ST7735_WriteCommand(0x29);
    HAL_Delay(100);
}



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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  ST7735_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	
		//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,1);
		//HAL_Delay(1000);
			//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,0);
		//HAL_Delay(1000);
	  ST7735_FillScreen(0x0000);  // Xóa màn hình thành màu den
		ST7735_DrawString(10, 20, "Hello, World!", Font_7x10, 0xFFFF, 0x0000);
    HAL_Delay(500);

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
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB0 PB1 PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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

#ifdef  USE_FULL_ASSERT
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
