#include "QuadCopterConfig.h"

#define PRINT_DEBUG(var1) serial.printf("DEBUG PRINT"#var1"\r\n")

xTaskHandle FlightControl_Handle = NULL;
xTaskHandle correction_task_handle = NULL;
xTaskHandle watch_task_handle = NULL;

Sensor_Mode SensorMode = Mode_GyrCorrect;

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{	// to prevent overflow from happening

	while (1) {
		LED_Toggle(LED_R);
		vTaskDelay(200);
	}
}
void vApplicationMallocFailedHook(void)
{	// 當 malloc failed 發生, 卡死在迴圈內並閃紅燈 (LED_Toggle)

	while (1) {
		LED_Toggle(LED_R);
		vTaskDelay(200);
	}
}

void vApplicationIdleHook(void)
{
}

void system_init(void)
{
	LED_Config();
	Serial_Config(Serial_Baudrate);

	system.status = SYSTEM_INITIALIZED;
		//至此初始化完成

}


void lcd_task()  		//LCD ili9341 用來顯示飛行器的資料
{
	while (system.status != SYSTEM_INITIALIZED);

	TM_ILI9341_Init();
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Landscape_2);
	TM_ILI9341_Fill(ILI9341_COLOR_MAGENTA);
	TM_ILI9341_Puts(50, 15, "ICLab - Quadrotor", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
	TM_ILI9341_Puts(20, 50, "1. Roll:", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	TM_ILI9341_Puts(20, 80, "2. Pitch:", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	TM_ILI9341_Puts(20, 110, "3. Yaw:", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	TM_ILI9341_Puts(20, 140, "4. Height:", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	vTaskDelay(50);
//while(1);
	while(1){

		int LCD_Roll = AngE.Roll;
		int LCD_Pitch = AngE.Pitch;
		int LCD_Yaw = AngE.Yaw;
		int LCD_Roll_Tens, LCD_Pitch_Tens, LCD_Yaw_Tens, LCD_Roll_Digits, LCD_Pitch_Digits, LCD_Yaw_Digits;

		if(LCD_Roll<0){
			LCD_Roll = LCD_Roll *(-1);
			TM_ILI9341_Putc(140, 50, '-', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
		}
		else
			TM_ILI9341_Putc(140, 50, ' ', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);

		if(LCD_Pitch<0){
			LCD_Pitch = LCD_Pitch *(-1);
			TM_ILI9341_Putc(140, 80, '-', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
		}
		else
			TM_ILI9341_Putc(140, 80, ' ', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);

		if(LCD_Yaw<0){
			LCD_Yaw = LCD_Yaw *(-1);
			TM_ILI9341_Putc(140, 110, '-', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
		}
		else
			TM_ILI9341_Putc(140, 110, ' ', &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);

		/* 從int轉換為轉char */
	    LCD_Roll_Tens = LCD_Roll * 0.1;		//取roll角度的十位數
	    LCD_Roll_Digits = LCD_Roll % 10;	//取roll角度的個位數
	    LCD_Pitch_Tens = LCD_Pitch * 0.1; 
	    LCD_Pitch_Digits = LCD_Pitch % 10;
	    LCD_Yaw_Tens = LCD_Yaw * 0.1; 
	    LCD_Yaw_Digits = LCD_Yaw % 10;

	    /* 從char轉換為Ascii */
	    LCD_Roll_Tens = LCD_Roll_Tens + 48; // 0~9 加上48 為 Ascii
	    LCD_Pitch_Tens = LCD_Pitch_Tens + 48;
	    LCD_Yaw_Tens = LCD_Yaw_Tens + 48;
	    LCD_Roll_Digits = LCD_Roll_Digits + 48; 
	    LCD_Pitch_Digits = LCD_Pitch_Digits + 48;
	    LCD_Yaw_Digits = LCD_Yaw_Digits + 48;

	    /* 印出字串在螢幕  X,   Y,    string,         字體大小,           字體顏色,             背景顏色        */
		TM_ILI9341_Putc(150, 50, LCD_Roll_Tens, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	    TM_ILI9341_Putc(150, 80, LCD_Pitch_Tens, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	    TM_ILI9341_Putc(150, 110, LCD_Yaw_Tens, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	    TM_ILI9341_Putc(160, 50, LCD_Roll_Digits, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	    TM_ILI9341_Putc(160, 80, LCD_Pitch_Digits, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	    TM_ILI9341_Putc(160, 110, LCD_Yaw_Digits, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_MAGENTA);
	}
}

int main(void) 		//主程式
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	//設定FreeRTOS系統之中斷優先順序為Group 4
	system_init();
	//系統初始化

	vSemaphoreCreateBinary(serial_tx_wait_sem);
	//以Semaphore設定USART Tx, Rx初始設定
	serial_rx_queue = xQueueCreate(5, sizeof(serial_msg));
	Ultrasonic_serial_rx_queue = xQueueCreate(5, sizeof(serial_msg));

	/* IMU Initialization, Attitude Correction Flight Control */

	/* QuadCopter Developing Shell, Ground Station Software */
	xTaskCreate(shell_task,
		    (signed portCHAR *) "Shell",
		    2048, NULL,
		    tskIDLE_PRIORITY + 7, NULL);

	/* Support LCD ili9341 */
	xTaskCreate(lcd_task,
		    (signed portCHAR *) "stm32f429's LCD handler",
		    1024, NULL,
		    tskIDLE_PRIORITY + 5, NULL);

	vTaskStartScheduler();

	return 0;
}

