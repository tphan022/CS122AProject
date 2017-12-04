/* Partner(s) Name & E-mail: Tony Phan (tphan022@ucr.edu)
* Lab Section: 21
* Assignment: Micro controller 1 : LEDs, keypad, and LCD display
* code, is my own original work.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "headers/keypad.h"
#include "headers/lcd.h"
#include "headers/usart_ATmega1284.h"

volatile unsigned char change;
volatile unsigned char val;
volatile unsigned char val2;
volatile unsigned char LED_run;
volatile unsigned char Run_tick;
volatile unsigned int standby;
volatile unsigned char standby_tick;
volatile unsigned int triggerDelay;
volatile unsigned char LED_trigger;
volatile unsigned char trigger_tick;
volatile unsigned char keypad;

volatile unsigned char enabled; // Bool, if the system is on
volatile unsigned int keynum; //holds position of pass code iterator
volatile unsigned char key[4]; // holds pass code values
volatile unsigned char key2[4]; //holds value to be compared
volatile unsigned int startDelay; // 5 second delay after starting

volatile unsigned char sendData2; // USART sending data to Microcontroller 2
volatile unsigned char receiveData2; // USART receiving data from M 2

volatile unsigned char pop;

//volatile unsigned char pwr2;

void shift_reg_data(unsigned char select, unsigned char data) {
	int i;
	if(select == 0x00) {
		for(i = 7; i >= 0; --i){
			PORTC = 0x08; // clears first reg--Modify bc need to set rclk low for shift reg 2
			PORTC |= ((data >> i) & 0x01); //0x01 is SER
			PORTC |= 0x04; // modify this value for changing shift registers
		} // i can create another for loop to iterate the second shift reg
		PORTC |=0x02; // Rclk
		PORTC = 0x00;
	}
	else if(select == 0x01) {
		for(i = 7; i >= 0; --i){
			PORTC = 0x20; // clears first reg--Modify bc need to set rclk low for shift reg 2
			PORTC |= ((data >> i) & 0x01); //0x01 is SER
			PORTC |= 0x04; // modify this value for changing shift registers
		} // i can create another for loop to iterate the second shift reg
		PORTC |=0x10; // Rclk
		PORTC = 0x00;
	}
}



enum ShiftState {INIT,Swait,Srun, Strigger} shift_state;
enum button {INITB, prompt1, prompt2, waitup, waitdown, iterate, iterate2,} button_state;
enum USRT {INITU, Urun} USART_state;

void shift_Init(){
	shift_state = INIT;
}

void button_INIT(){
	button_state = INITB;
}

void USART_INIT(){
	USART_state = INITU;
}

void shift_Tick(){
	//Actions
	switch(shift_state){
		case INIT:
		val = 0x01;
		val2 = 0x01;
		change = 0x00;
		LED_run = 0x01;
		standby = 0;
		standby_tick = 0x00;
		Run_tick = 0x00;
		LED_trigger = 0x01;
		trigger_tick = 0x00;
		triggerDelay = 0;
		pop = 0x00;
		break;
		case Swait:
		standby++;
		if(standby > 20 && standby_tick == 0x00) {
			shift_reg_data(0x00, 0x00);
			shift_reg_data(0x01, 0x00);
			standby_tick = 0x01;
			standby = 0;
		}
		else if(standby > 20 && standby_tick == 0x01) {
			shift_reg_data(0x00, 0xFF);
			shift_reg_data(0x01, 0xFF);
			standby_tick = 0x00;
			standby = 0;
		}
		break;
		case Srun:
		
		if((Run_tick == 0x00)) {
			
			if(LED_run == 0x80) {
				//LED_run = LED_run << 1;
				Run_tick = 0x01;
				shift_reg_data(0x00, LED_run);
				shift_reg_data(0x01, 0x00);
				LED_run = 0x01;
			}
			else {
				shift_reg_data(0x00, LED_run);
				shift_reg_data(0x01, 0x00);
				LED_run = LED_run << 1;
			}
		}
		else if((Run_tick == 0x01)) {
			
			if(LED_run == 0x80) {
				//LED_run = LED_run << 1;
				Run_tick = 0x00;
				shift_reg_data(0x01, LED_run);
				shift_reg_data(0x00, 0x00);
				LED_run = 0x01;
			}
			else {
				shift_reg_data(0x01, LED_run);
				shift_reg_data(0x00, 0x00);
				LED_run = LED_run << 1;
			}
		}
		
		break;
		
		case Strigger:
		if(triggerDelay > 15 && trigger_tick == 0x00){
			triggerDelay = 0;
			if(LED_trigger == 0xFF) {
				shift_reg_data(0x00, LED_trigger);
				shift_reg_data(0x01, 0x00);
				trigger_tick = 0x01;
				LED_trigger = 0x01;
			}
			else {
				shift_reg_data(0x00, LED_trigger);
				shift_reg_data(0x01, 0x00);
				LED_trigger = (LED_trigger << 1) + 0x01;
			}
		}
		else if(triggerDelay > 15 && trigger_tick == 0x01){
			triggerDelay = 0;
			if(LED_trigger == 0xFF) { // ends
				shift_reg_data(0x01, LED_trigger);
				pop = 0x04;
				//shift_reg_data(0x01, LED_trigger);
				//shift_reg_data(0x01, 0x00);
				//trigger_tick = 0x00;
				//LED_trigger = 0x01;
			}
			else {
				shift_reg_data(0x01, LED_trigger);
				//shift_reg_data(0x01, 0x00);
				LED_trigger = (LED_trigger << 1) + 0x01;
			}
		}
		triggerDelay++;
		break;
		
		default:
		PORTC = 0;
		break;
	}
	//Transitions
	switch(shift_state){
		case INIT:
		shift_state = Swait;
		break;
		case Swait:
		pop = 0x00;
		if(change == 0x01) {
			shift_state = Srun;
		}
		else if(change == 0x02) {
			LED_trigger = 0x01;
			trigger_tick = 0x00;
			triggerDelay = 0;
			shift_state = Strigger;
		}
		else{
			shift_state = Swait;
		}
		break;
		
		case Srun:
		if(change == 0x01) {
			shift_state = Srun;
		}
		else if(change == 0x02) {
			LED_trigger = 0x01;
			trigger_tick = 0x00;
			triggerDelay = 0;
			shift_state = Strigger;
		}
		else{
			shift_state = Swait;
		}
		break;
		
		case Strigger:
		if(change == 0x01) {
			shift_state = Srun;
		}
		else if(change == 0x02) {
			shift_state = Strigger;
		}
		else{
			shift_state = Swait;
		}
		break;
		
		default:
		shift_state = INIT;
		break;
	}
}

void button_Tick(){
	switch(button_state) {
		case INITB:
		keypad = 0x00;
		keynum = 0;
		enabled = 0;
		break;
		
		case prompt1:
		LCD_DisplayString(1, "Enter a 4 digit code:");
		break;
		
		case prompt2:
		LCD_DisplayString(1, "Enter code to   disable:");
		break;
		
		case waitup:
		
		break;
		
		case iterate:
		
		break;
		
		case iterate2:
		
		break;
		
		case waitdown:
		
		break;
		
		default:
		break;
	}
	
	switch(button_state) {
		case INITB:
		button_state = prompt1;
		startDelay = 0;
		break;
		
		case prompt1:
		button_state = waitdown;
		break;
		
		case prompt2:
		button_state = waitdown;
		break;
		
		case waitup:
		keypad = GetKeypadKey();
		
		if(keypad != '\0'){
			if(enabled == 0x00) {
				key[keynum] = keypad;
			}
			else {
				key2[keynum] = keypad;
			}
			LCD_WriteData(keypad);
			keynum++;
			button_state = waitdown;
		}
		else{
			button_state = waitup;
		}
		if(keynum >= 4 && enabled == 0x00) {
			button_state = iterate;
			startDelay = 0;
			keynum = 0;
			LCD_ClearScreen();
		}
		else if(keynum >=4 && enabled == 0x01) {
			button_state = iterate2;
			keynum = 0;
			LCD_ClearScreen();
		}
		
		break;
		
		case iterate:
		startDelay++;
		if(startDelay == 1) {
			LCD_DisplayString(1, "1");
		}
		else if(startDelay == 20) {
			LCD_DisplayString(1, "2");
		}
		else if(startDelay == 40) {
			LCD_DisplayString(1, "3");
		}
		else if(startDelay == 60) {
			LCD_DisplayString(1, "4");
		}
		else if(startDelay == 80) {
			LCD_DisplayString(1, "5");
		}
		else if(startDelay == 100) {
			LCD_DisplayString(1, "6");
		}
		else if(startDelay == 120) {
			LCD_DisplayString(1, "7");
		}
		else if(startDelay == 140) {
			LCD_DisplayString(1, "8");
			
		}
		else if(startDelay == 160) {
			LCD_DisplayString(1, "9");
		}
		else if(startDelay == 180) {
			LCD_DisplayString(1, "10");
		}
		if(startDelay >= 200) {
			change = 0x01;
			enabled = 0x01;
			button_state = prompt2;
		}
		else {
			button_state = iterate;
		}
		break;
		
		case iterate2:
		if(key[0] == key2[0] && key[1] == key2[1] && key[2] == key2[2] && key[3] == key2[3]) {
			change = 0x00;
			enabled = 0x00;
			button_state = prompt1;
		}
		else {
			LCD_DisplayString(1, "Incorrect code! :");
			button_state = waitdown;
		}
		break;
		
		case waitdown:
		if(GetKeypadKey() != '\0') {
			button_state = waitdown;
		}
		else {
			button_state = waitup;
		}
		break;
		
		default:
		break;
	}
}

void USART_Tick(){
	switch(USART_state){
		case INITU:
		
		break;
		
		case Urun:
		sendData2 = (sendData2 & 0xFC) | (enabled & 0x03); // lowest two bits are enable value
		sendData2 = (sendData2 & 0xF3) | (pop & 0x0C); // Next two bit trigger the motor
		if(USART_HasReceived(1)) {
			receiveData2 = USART_Receive(1);
			USART_Flush(1);
		}
		if(USART_IsSendReady(1) != 0) {
			USART_Send(sendData2, 1);
		}
		
		if(((receiveData2 & 0x03) == 0x01) && enabled == 0x01) {
			change = 0x02; //lowest two bits are sensor data
		}
		else if(((receiveData2 & 0x03) == 0x00) && enabled == 0x01) {
			change = 0x01; //lowest two bits are sensor data
		}
		break;
		
		default:
		
		break;
	}
	
	switch(USART_state){
		case INITU:
		sendData2 = 0x00;
		receiveData2 = 0x00;
		USART_state = Urun;
		break;
		
		case Urun:
		USART_state = Urun;
		break;
		
		default:
		
		break;
	}
}

void shiftTask()
{
	shift_Init();
	for(;;)
	{
		shift_Tick();
		vTaskDelay(50);
	}
}

void buttonTask(){
	button_INIT();
	for(;;){
		button_Tick();
		vTaskDelay(50);
	}
}

void USARTTask(){
	USART_INIT();
	for(;;){
		USART_Tick();
		vTaskDelay(50);
	}
}

void shiftPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(shiftTask, (signed portCHAR *)"shiftTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void buttonPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(buttonTask, (signed portCHAR *)"buttonTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void USARTPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(USARTTask, (signed portCHAR *)"USARTTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
int main(void)
{
	DDRA = 0xF0; PORTA=0x0F;
	DDRC = 0xFF;
	DDRD = 0xFF; PORTD = 0xF0;
	DDRB = 0xFF; PORTB = 0x00;
	LCD_init();
	//Start Tasks
	shiftPulse(2);
	buttonPulse(1);
	USARTPulse(3);
	initUSART(0);
	initUSART(1);
	USART_Flush(0);
	USART_Flush(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}
