#include "ST7735.h"
#include "stm32f1xx_hal.h"

// SPI Handle
extern SPI_HandleTypeDef hspi1;

// Định nghĩa các hàm gửi lệnh và dữ liệu tới màn hình
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
                delay((uint32_t)(num*10));
        }
        else {
            ST7735_WriteCommand(cmd);

            for (dat = 0; dat < num; ++dat)
                ST7735_WriteData(*cmdList++);
        }
    }
}


// Khởi tạo màn hình ST7735
void ST7735_Init(SPI_HandleTypeDef *hspi)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // CS = 0
  
    ST7735_WriteCommand(0x01); // Software Reset
    HAL_Delay(150);
    ST7735_WriteCommand(0x11); // Sleep Out
    HAL_Delay(255);

    ST7735_WriteCommand(ST7735_MADCTL); // Memory Access Control
    ST7735_WriteData(0x08);  // Row/Column exchange, RGB color filter


    ST7735_WriteCommand(0x3A); //Interface Pixel Format
    ST7735_WriteData(0x05);


    ST7735_SetWindow(0, 0, 160, 128);

    ST7735_SendCommandList(u8InitCmdList);

    ST7735_WriteCommand(LCD_CMD_SET_DISPLAY_ON)
    HAL_Delay(100);
}

// Fill Screen
void ST7735_FillScreen(uint16_t color)
{
    ST7735_WriteCommand(ST7735_CASET);
    ST7735_WriteData(0x00);
    ST7735_WriteData(0x00);
    ST7735_WriteCommand(ST7735_RASET);
    ST7735_WriteData(0x00);
    ST7735_WriteData(0x00);

    ST7735_WriteCommand(ST7735_RAMWR);
    for (int i = 0; i < 128 * 160; i++) {
        ST7735_WriteData(color >> 8);
        ST7735_WriteData(color & 0xFF);
    }
}

// Vẽ điểm
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    ST7735_WriteCommand(ST7735_CASET);
    ST7735_WriteData(x >> 8);
    ST7735_WriteData(x & 0xFF);

    ST7735_WriteCommand(ST7735_RASET);
    ST7735_WriteData(y >> 8);
    ST7735_WriteData(y & 0xFF);

    ST7735_WriteCommand(ST7735_RAMWR);
    ST7735_WriteData(color >> 8);
    ST7735_WriteData(color & 0xFF);
}

// Hàm hiển thị chuỗi
void ST7735_WriteString(char* str, uint16_t color, uint16_t bgColor, uint8_t size)
{
    while (*str) {
        // Gọi hàm vẽ ký tự tại đây
        str++;
    }
}

// Set text color
void ST7735_SetTextColor(uint16_t color)
{
    // Set color cho chữ
}

// Set text size
void ST7735_SetTextSize(uint8_t size)
{
    // Set kích thước chữ
}
