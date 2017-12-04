/* Partner(s) Name & E-mail: Tony Phan (tphan022@ucr.edu)
* Lab Section: 21
* Assignment: Micro controller 2 : PIR Sensors and flashlight
* Exercise Description:
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
#include "headers/usart_ATmega1284.h"



//volatile unsigned char pwr2;
volatile unsigned char sendData1; // USART sending data to Microcontroller 1
volatile unsigned char receiveData1; // USART receiving data from M 1
volatile unsigned char enable;
volatile unsigned char motion1; // Each motion coresponds to a side
volatile unsigned char motion2; // front=1, left=2, back=3, right=4
volatile unsigned char motion3;
volatile unsigned char motion4;
volatile unsigned char motionTransfer;

volatile unsigned char change;
static unsigned int numPhase90 = (90 / 6.425) * 64;
static unsigned int numPhase180 = (180 / 6.425) * 64;
static unsigned int numPhase360 = (360 / 6.425) * 64;
volatile unsigned char complete;
volatile unsigned int i;


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



enum Sensor1State {INIT, S1wait} sensor1_state;
enum USRT {INITU, Urun} USART_state;
enum motor {INITM, waitup, iterate} motor_state;
//enum button {INITB, waitup, iterate, iterate2, waitdown} button_state;

void sensor1_Init(){
	sensor1_state = INIT;
}

void USART_INIT(){
	USART_state = INITU;
}

void motor_INIT(){
	motor_state = INITM;
}

void sensor1_Tick(){
	//Actions
	switch(sensor1_state){
		case INIT:
		
		break;
		case S1wait:
		if((PINA & 0x01) == 0x00 || enable == 0x00) {
			motion1 = 0x00;
			PORTC = PINC & 0xFE;
		}
		else if((PINA & 0x01) == 0x01 && enable == 0x01) {
			motion1 = 0x01;
			PORTC = PINC | 0x01;
		}
		
		if((PINA & 0x02) == 0x00 || enable == 0x00) {
			motion2 = 0x00;
			PORTC = PINC & 0xFD;
		}
		else if((PINA & 0x02) == 0x02 && enable == 0x01) {
			motion2 = 0x01;
			PORTC = PINC | 0x02;
		}
		
		if((PINA & 0x04) == 0x00 || enable == 0x00) {
			motion3 = 0x00;
			PORTC = PINC & 0xFB;
		}
		else if((PINA & 0x04) == 0x04 && enable == 0x01) {
			motion3 = 0x01;
			PORTC = PINC | 0x04;
		}
		
		if((PINA & 0x08) == 0x00 || enable == 0x00) {
			motion4 = 0x00;
			PORTC = PINC & 0xF7;
		}
		else if((PINA & 0x08) == 0x08 && enable == 0x01) {
			motion4 = 0x01;
			PORTC = PINC | 0x08;
		}
		if(motion1 == 0x01 || motion2 == 0x01 || motion3 == 0x01 || motion4 == 0x01) {
			motionTransfer = 0x01;
		}
		else {
			motionTransfer = 0x00;
		}
		break;
		
		default:
		PORTC = 0;
		break;
	}
	//Transitions
	switch(sensor1_state){
		case INIT:
		motion1 = 0x00;
		motion2 = 0x00;
		motion3 = 0x00;
		motion4 = 0x00;
		motionTransfer = 0x00;
		sensor1_state = S1wait;
		break;
		case S1wait:
		sensor1_state = S1wait;
		break;
		
		default:
		sensor1_state = INIT;
		break;
	}
}

void USART_Tick(){
	switch(USART_state){
		case INITU:
		enable = 0x00;
		break;
		
		case Urun:
		sendData1 = (sendData1 & 0xFC) | (motionTransfer & 0x03); // lowest two bits are enable value
		if(USART_HasReceived(1)) {
			receiveData1 = USART_Receive(1);
			USART_Flush(1);
		}
		if(USART_IsSendReady(1) != 0) {
			USART_Send(sendData1, 1);
		}
		enable = (receiveData1 & 0x03);
		if((receiveData1 & 0x0C) == 0x04) {
			complete = 0x00;
		}
		break;
		
		default:
		
		break;
	}
	
	switch(USART_state){
		case INITU:
		sendData1 = 0x00;
		receiveData1 = 0x00;
		enable = 0x00;
		USART_state = Urun;
		break;
		
		case Urun:
		USART_state = Urun;
		break;
		
		default:
		
		break;
	}
}

void motor_Tick(){
	switch(motor_state) {
		case INITM:
		change = 0x01;
		complete = 0x01;
		i = 0;
		break;
		
		case waitup:
		
		break;
		
		case iterate:
		
		if(change == 0x01) {
			change = 0x03;
		}
		else if(change == 0x03){
			change = 0x02;
		}
		else if(change == 0x02){
			change = 0x06;
		}
		else if(change == 0x06){
			change = 0x04;
		}
		else if(change == 0x04){
			change = 0x0C;
		}
		else if(change == 0x0C){
			change = 0x08;
		}
		else if(change == 0x08){
			change = 0x01;
		}
		PORTB = change;
		break;
		
		default:
		break;
	}
	
	switch(motor_state) {
		case INITM:
		motor_state = waitup;
		break;
		
		case waitup:
		if(complete == 0x00) {
			if(i < numPhase360) {
				i++;
				motor_state = iterate;
			}
			else {
				complete = 0x01;
				i = 0;
				motor_state = waitup;
			}
		}
		else {
			motor_state = waitup;
		}
		break;
		
		case iterate:
		motor_state = waitup;
		break;
		
		
		default:
		break;
	}
}

void sensor1Task()
{
	sensor1_Init();
	for(;;)
	{
		sensor1_Tick();
		vTaskDelay(100);
	}
}

void USARTTask(){
	USART_INIT();
	for(;;){
		USART_Tick();
		vTaskDelay(50);
	}
}

void motorTask(){
	motor_INIT();
	for(;;){
		motor_Tick();
		vTaskDelay(2);
	}
}

void sensor1Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(sensor1Task, (signed portCHAR *)"sensor1Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void USARTPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(USARTTask, (signed portCHAR *)"USARTTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void motorPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(motorTask, (signed portCHAR *)"motorTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF; // Inputs
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	//DDRD = 0xFF; PORTD = 0xF0;
	//DDRB = 0xFF; PORTB = 0x00;
	//Start Tasks
	sensor1Pulse(1);
	USARTPulse(2);
	motorPulse(3);
	initUSART(0);
	initUSART(1);
	USART_Flush(0);
	USART_Flush(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}
