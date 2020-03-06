
#include "LPC17xx.h"                    // Device header
#include "RTE_Components.h"             // Component selection
#include "Board_LED.h"      
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "RTE_Device.h"                // ::Board Support:LED
#include "Board_Joystick.h"             // ::Board Support:Joystick
#include "Board_Buttons.h"              // ::Board Support:Buttons
#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "lcd_lib/asciiLib.h"
#include "TP_Open1768.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NX 320
#define NY 240

#define screenSizeX  0.065
#define screenSizeY  0.0525

double ballPositionX = 12*NX/13;
double ballPositionY = 3*NY/5;

double ballSpeedX = 0;
double ballSpeedY = 0;

double ballAccelerationX = 0;
double ballAccelerationY = 0;

uint8_t maska = 1 << 1;


void drawGreen(int i, int j);
void drawBlack(int i, int j);
void drawWhite(int i, int j);
void drawRed(int i, int j);
void draw();

void drawSquareRed(int i, int j);
void drawSquareWhite(int i, int j);
void drawSquareBlack(int i, int j);

void fun(int pos, char c, int x, int y);

void print(unsigned char *string, int x, int y);
void printNum(int num, int x, int y);

void writeReg(uint8_t registerAddress, uint8_t dataToSend);

uint8_t readReg(uint8_t registerAddress);

void SysTick_Handler(void)  {                            
	double elapsedTime = 1.0 / 60.0;

	uint8_t axesX_0 = readReg(0x32);
	uint8_t axesX_1 = readReg(0x33);
	uint16_t axesX;
	double accX;
	if(axesX_1 & maska){
		axesX = ~(((axesX_1 % 4 ) << 8) + axesX_0);
		axesX %= 1024;
		axesX += 1;
		accX = axesX;
		accX *= -(1);
	} else{
		accX  = (((axesX_1 % 4)<< 8) + axesX_0);
	}

	uint8_t axesY_0 = readReg(0x34);
	uint8_t axesY_1 = readReg(0x35);
	uint16_t axesY;
	int accY;
	if(axesY_1 & maska){
		axesY = ~(((axesY_1 % 4 ) << 8) + axesY_0);
		axesY %= 1024;
		axesY += 1;
		accY = axesY;
		accY *= -(1);
	} else{
		accY  = (((axesY_1 % 4)<< 8) + axesY_0);
	}

	ballAccelerationX = accX / 2.0;
	ballAccelerationY = accY / 2.0;

	double ballShiftX = ballSpeedX * elapsedTime + ( 0.5 * ballAccelerationX * elapsedTime * elapsedTime);
	double ballShiftY = ballSpeedY * elapsedTime + ( 0.5 * ballAccelerationY * elapsedTime * elapsedTime);
		
	if(ballSpeedX >= 100 && accX > 0){
		ballSpeedX = 100;
	} else if (ballSpeedX <= -100 && accX < 0) {
		ballSpeedX = -100;
	} else {
		ballSpeedX += ballAccelerationX * elapsedTime;
	}
	
	if(ballSpeedY >= 100 && accY > 0){
		ballSpeedY = 100;
	} else if (ballSpeedY <= -100 && accY < 0) {
		ballSpeedY = -100;
	} else {
		ballSpeedY += ballAccelerationY * elapsedTime;
	}
		
	drawSquareBlack(ballPositionX, ballPositionY);

	ballPositionX += (ballShiftX);
	ballPositionY += (ballShiftY);
	
	
	if(ballPositionX > NX)
		ballPositionX = 0;
	if(ballPositionY > NY)
		ballPositionY = 0;
	if(ballPositionX < 0)
		ballPositionX = NX;
	if(ballPositionY < 0)
		ballPositionY = NY;
	
	drawSquareRed((int)ballPositionX, (int)ballPositionY);
}
  
int main (void)  {
	// Trzeba zrobić tak aby odpalał się co 1/30 sekundy
	

	// ############## LCD CONF ####################
	LPC_PINCON->PINSEL0 = (1 << 4);
	
	LPC_UART0->LCR = (3<<0) | ( 1<<7 );
	LPC_UART0->DLL = 13;
	LPC_UART0->FCR = 6;
	LPC_UART0->LCR = LPC_UART0->LCR & ~(1<<7);
	LPC_UART0->THR = 'A';
	
	lcdConfiguration();
	touchpanelInit();
	init_ILI9325();
	// ############################################
	
	
	
	// fill(arr);
	draw();
	
	
	//############### I2C CONF ####################
	LPC_PINCON->PINSEL1 |= 1 << 24;
	LPC_PINCON->PINSEL1 |= 1 << 22;

	LPC_I2C0->I2SCLH = 500;
	LPC_I2C0->I2SCLL = 500;
	
	writeReg(0x31, 0x0);
	writeReg(0x24, 0x01);
	writeReg(0x27, 0x70);
	writeReg(0x2D, 0x08);
	// ############################################
	SysTick_Config(SystemCoreClock / 60);      /* Configure SysTick to generate an interrupt every millisecond */
	while(1){
		
	}
}




void fun(int pos, char c, int x, int y){
	unsigned char tab[16];
	GetASCIICode(0, tab, c);
	for(int i = 0 ; i < 15 ; i++){
		lcdSetCursor(pos * 8 + x, i + y);
		for(int j = 7 ; j >= 0 ; j--){
			if(tab[i] & (1 << j))
				lcdWriteReg(DATA_RAM, LCDRed);
			else
				lcdWriteReg(DATA_RAM, LCDWhite);
		}
	}
}

void print(unsigned char *string, int x, int y){
	int i = 0;
	while(string[i]){
		fun(i, string[i], x, y);
		i++;
	}
}

void printNum(int num, int x, int y){
	int i = 0;
	unsigned char tab[4];
	sprintf(tab, "%d    ", num);
	while(tab[i]){
		fun(i, tab[i], x, y);
		i++;
	}
}

void writeReg(uint8_t registerAddress, uint8_t dataToSend){
	// Wyczyszczenie rejestrow
	LPC_I2C0->I2CONCLR |= 1 << 2;
	LPC_I2C0->I2CONCLR |= 1 << 3;
	LPC_I2C0->I2CONCLR |= 1 << 4;
	LPC_I2C0->I2CONCLR |= 1 << 5;

	// Wyslanie flagi START + ustawienie I2Enable
	LPC_I2C0->I2CONSET |= 1 << 6;
	LPC_I2C0->I2CONSET |= 1 << 5;

	// Czekamy az I2STAT == 0x08
	while(LPC_I2C0->I2STAT != 0x08){}

	// Wyslanie adresu z bitem W
	LPC_I2C0->I2DAT = 0x3A;
	LPC_I2C0->I2CONCLR = (1 << 5) | (1 << 3);

	// Czekamy az I2STAT == 0x18
	while(LPC_I2C0->I2STAT != 0x18){}

	// Wysylamy adres rejestru do ktrego chcemy sie dostac
	LPC_I2C0->I2DAT = registerAddress;
	LPC_I2C0->I2CONCLR = (1 << 3);

	// Czekamy az I2STAT == 0x28
	while(LPC_I2C0->I2STAT != 0x28){}

	// Wysylamy dane do rejestru do ktrego chcemy sie dostac
	LPC_I2C0->I2DAT = dataToSend;
	LPC_I2C0->I2CONCLR = (1 << 3);

	// Czekamy az I2STAT == 0x28
	while(LPC_I2C0->I2STAT != 0x28){}

	// Wysylamy flage STOP
	LPC_I2C0->I2CONCLR = (1 << 5) | (1 << 3);
	LPC_I2C0->I2CONSET |= 1 << 4;
}

uint8_t readReg(uint8_t registerAddress){
	// Wyczyszczenie rejestrow
	LPC_I2C0->I2CONCLR |= 1 << 2;
	LPC_I2C0->I2CONCLR |= 1 << 3;
	LPC_I2C0->I2CONCLR |= 1 << 4;
	LPC_I2C0->I2CONCLR |= 1 << 5;

	// Wyslanie flagi START + ustawienie I2Enable
	LPC_I2C0->I2CONSET |= 1 << 6;
	LPC_I2C0->I2CONSET |= 1 << 5;

	// Czekamy az I2STAT == 0x08
	while(LPC_I2C0->I2STAT != 0x08){}

	// Wyslanie adresu z bitem W
	LPC_I2C0->I2DAT = 0x3A;
	LPC_I2C0->I2CONCLR = (1 << 5) | (1 << 3);

	// Czekamy az I2STAT == 0x18
	while(LPC_I2C0->I2STAT != 0x18){}

	// Wysylamy adres rejestru do ktrego chcemy sie dostac
	LPC_I2C0->I2DAT = registerAddress;
	LPC_I2C0->I2CONCLR = (1 << 3);

	// Czekamy az I2STAT == 0x28
	while(LPC_I2C0->I2STAT != 0x28){}
		


	// Wysylamy powtorzona flage START
	LPC_I2C0->I2CONCLR = (1 << 3);
	LPC_I2C0->I2CONSET |= 1 << 5;

	// Czekamy az I2STAT == 0x10
	while(LPC_I2C0->I2STAT != 0x10){}
		

	// Wyslanie adresu z bitem R
	LPC_I2C0->I2DAT = 0x3B;
	LPC_I2C0->I2CONCLR = (1 << 5) | (1 << 3);

	// Czekamy az I2STAT == 0x40
	while(LPC_I2C0->I2STAT != 0x40){}
		


	// Ustawiamy AA na 0 - czytamy jeden bajt
	LPC_I2C0->I2CONCLR = 0x08;


	// Czekamy az I2STAT == 0x58
		
	while(LPC_I2C0->I2STAT != 0x58){
	}
		
				

	uint8_t dataReturned = LPC_I2C0->I2DAT;

	// Wysylamy flage STOP
	LPC_I2C0->I2CONCLR = (1 << 5) | (1 << 3);
	LPC_I2C0->I2CONSET |= 1 << 4;

	return dataReturned;
}

void drawBlack(int i, int j){
    lcdSetCursor(j,i);
    lcdWriteReg(DATA_RAM, LCDBlack);
}

void drawWhite(int i, int j){
    lcdSetCursor(j,i);
    lcdWriteReg(DATA_RAM, LCDWhite);
}

void drawRed(int i, int j){
    lcdSetCursor(j,i);
    lcdWriteReg(DATA_RAM, LCDRed);
}
void drawGreen(int i, int j){
    lcdSetCursor(j,i);
    lcdWriteReg(DATA_RAM, LCDGreen);
}

void drawSquareWhite(int i, int j){
	for(int i = 0 ; i < 7 ; i++){
		for(int j = 0 ; j < 7 ; j++){
			drawWhite(ballPositionX - i+1, ballPositionY - j+1);
		}
	}
}

void drawSquareRed(int i, int j){	
    for(int i = 0 ; i <7 ; i++){
		for(int j = 0 ; j < 7 ; j++){
			if(lcdReadData() == LCDWhite){
				ballPositionX = 12*NX/13;
				ballPositionY = 3*NY/5;
				draw();
				break;
			} else if(lcdReadData() == LCDGreen){
				print("WIN!!! WIN!!!", 100, 200);
				break;
			}
			drawRed(ballPositionX - i+1, ballPositionY - j+1);
		}
	};
}

void drawSquareBlack(int i, int j){	
    for(int i = 0 ; i < 7 ; i++){
		for(int j = 0 ; j < 7 ; j++){
			drawBlack(ballPositionX - i+1, ballPositionY - j+1);
		}
	};
}

void draw(){

    // filling center and boundaries with 'X' values

    for(int i=0; i<NX; i++){
			for(int j=1;j<NY-1; j++)
				drawBlack(i, j);
    }

    for(int j=0; j<NY; j++){
        drawWhite(0, j);
        drawWhite(NX-1, j);
    }

    for(int i=0; i<NX; i++){
        drawWhite(i, 0);
        drawWhite(i, NY-1);
    }
    

    // making a path 


    drawWhite(NX-1, NY/2);
    drawWhite(NX-1, NY/2+1);



    for(int i=8*NX/9; i<NX-1; i++){
        drawWhite(i, 5*NY/7);
        drawWhite(i, 5*NY/7-1);
    }

    for(int j=5*NY/7-1; j<5*NY/7+5; j++){
        drawWhite(8*NX/9, j);
        drawWhite(8*NX/9-1, j);
    }
    for(int i=8*NX/9-1; i<8*NX/9+2; i++){
        drawWhite(i, 5*NY/7+5);
        drawWhite(i, 5*NY/7+5-1);
    }
    for(int j=5*NY/7+5-1; j<5*NY/7+12; j++){
        drawWhite(8*NX/9+2, j);
        drawWhite(8*NX/9+2-1, j);
    }
    for(int i=8*NX/9+2; i>5*NX/6; i--){
        drawWhite(i, 5*NY/7+12);
        drawWhite(i, 5*NY/7+12-1);
    }
    for(int j=5*NY/7+12; j>3*NY/5; j--){
        drawWhite(5*NX/6, j);
        drawWhite(5*NX/6-1, j);
    }
    for(int i=5*NX/6-1; i<5*NX/6+8; i++){
        drawWhite(i, 3*NY/5);
        drawWhite(i, 3*NY/5-1);
    }
    for(int j=3*NY/5; j>2*NY/9; j--){
        drawWhite(5*NX/6+8, j);
        drawWhite(5*NX/6+8-1, j);
    }
    for(int i=5*NX/6+8; i>2*NX/3; i--){
        drawWhite(i, 2*NY/9);
        drawWhite(i, 2*NY/9-1);
    }
    for(int j=2*NY/9-1; j<2*NY/3; j++){
        drawWhite(2*NX/3, j);
        drawWhite(2*NX/3-1, j);
    }
    for(int i=2*NX/3; i>4*NX/7; i--){
        drawWhite(i, 2*NY/3);
        drawWhite(i, 2*NY/3-1);
    }
    for(int j=2*NY/3-1; j<5*NY/7; j++){
        drawWhite(4*NX/7, j);
        drawWhite(4*NX/7-1, j);
    }
    for(int i=4*NX/7-1; i<5*NX/7; i++){
        drawWhite(i, 5*NY/7);
        drawWhite(i, 5*NY/7-1);
    }
    
    for(int i=5*NX/7; i>3*NX/5; i--){
        drawWhite(i, 6*NY/7);
        drawWhite(i, 6*NY/7-1);
    }
    for(int j=6*NY/7; j>4*NY/5; j--){
        drawWhite(3*NX/5, j);
        drawWhite(3*NX/5-1, j);
    }
    for(int i=3*NX/5; i>1*NX/7; i--){
        drawWhite(i, 4*NY/5);
        drawWhite(i, 4*NY/5-1);
    }
    for(int j=4*NY/5; j>3*NY/5; j--){
        drawWhite(5*NX/11, j);
        drawWhite(5*NX/11-1, j);
    }
    for(int i=5*NX/11-1; i<3*NX/5; i++){
        drawWhite(i, 3*NY/5);
        drawWhite(i, 3*NY/5-1);
    }
    for(int j=3*NY/5; j>1*NY/3; j--){
        drawWhite(3*NX/5-1, j);
        drawWhite(3*NX/5-1-1, j);
    }
    for(int i=3*NX/5-1; i>1*NX/2; i--){
        drawWhite(i, 1*NY/3);
        drawWhite(i, 1*NY/3-1);
    }
    for(int j=1*NY/3; j>1*NY/7; j--){
        drawWhite(1*NX/2, j);
        drawWhite(1*NX/2-1, j);
    }
    for(int i=1*NX/2; i>2*NX/5; i--){
        drawWhite(i, 1*NY/7);
        drawWhite(i, 1*NY/7-1);
    }    
		for(int i=1*NX/2-2; i>2*NX/5; i--){
        drawGreen(i, 1*NY/7+2);
        drawGreen(i, 1*NY/7+1);
        drawGreen(i, 1*NY/7+3);
    }
    for(int j=0; j<1*NY/2; j++){
        drawWhite(2*NX/5+1, j);
        drawWhite(2*NX/5+2, j);
    }
		
			for(int j=1*NX/7; j<3*NY/4; j++){
        drawWhite(1*NX/5+1, j);
        drawWhite(1*NX/5+2, j);
    }

    // moving square

        
}






