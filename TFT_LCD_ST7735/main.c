int main(void)
{

 
  ST7735_Init();

  while (1)
  {
  
	ST7735_FillScreen(0x0000);  
	ST7735_DrawString(10, 20, "Hello, World!", Font_7x10, 0xFFFF, 0x0000);
    HAL_Delay(500);
  }
  
  /* USER CODE END 3 */
}
