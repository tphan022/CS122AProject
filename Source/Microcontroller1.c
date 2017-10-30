/* Partner(s) Name & E-mail: Tony Phan (tphan022@ucr.edu)
* Lab Section: 21
* Assignment: Micro controller 1 : LEDs, keypad, and LCD display
* Exercise Description: shift regs/ Festive lights
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
enum button {INITB, waitup, iterate, iterate2, waitdown} button_state;

void shift_Init(){
	shift_state = INIT;
}

void button_INIT(){
	button_state = INITB;
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
		if(triggerDelay > 5 && trigger_tick == 0x00){
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
		else if(triggerDelay > 5 && trigger_tick == 0x01){
			triggerDelay = 0;
			if(LED_trigger == 0xFF) {
				shift_reg_data(0x01, LED_trigger);
				//shift_reg_data(0x01, 0x00);
				trigger_tick = 0x00;
				LED_trigger = 0x01;
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
		
		case Srun:
		shift_state = Srun;
		break;
		
		case Strigger:
		shift_state = Strigger;
		break;
		
		default:
		shift_state = INIT;
		break;
	}
}

void button_Tick(){
	switch(button_state) {
		case INITB:
		
		break;
		
		case waitup:
		
		break;
		
		case iterate:
		change = 0x01;
		break;
		
		case iterate2:
		change = 0x02;
		break;
		
		case waitdown:
		
		break;
		
		default:
		break;
	}
	
	switch(button_state) {
		case INITB:
		button_state = waitup;
		break;
		
		case waitup:
		if(PINA == 0xFF){
			button_state = waitup;
		}
		else if(PINA == 0xFE){
			button_state = iterate;
		}
		else if(PINA == 0xFD){
			button_state = iterate2;
		}
		
		break;
		
		case iterate:
		button_state = waitdown;
		break;
		
		case iterate2:
		button_state = waitdown;
		break;
		
		case waitdown:
		if(PINA == 0xFE || PINA == 0xFD){
			button_state = waitdown;
		}
		else{
			button_state = waitup;
		}
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

void shiftPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(shiftTask, (signed portCHAR *)"shiftTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void buttonPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(buttonTask, (signed portCHAR *)"buttonTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
	DDRA = 0x00; PORTA=0xFF;
	DDRC = 0xFF;
	//Start Tasks
	shiftPulse(2);
	buttonPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}
