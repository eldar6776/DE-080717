/******************************************************************************
 *
 *            Kontroler zracnog i kompresorskog hladnjaka vode
 *
 *                          LCD driver
 *
 *******************************************************************************
 * Ime fajla:		lcd.h
 *
 * Procesor:        	PIC18F452
 *
 * Kompajler:       	C18 3.40
 *
 * IDE:                 MPLAB X IDE v1.85 (java 1.7.0_17)
 *
 * Firma:           	Proven d.o.o. Sarajevo
 *
 * <mailto> eldar6776@hotmail.com
 *
 ******************************************************************************/

#ifndef LCD_H
#define	LCD_H

#include<p18cxxx.h>
#include<delays.h>
#include "typedefs.h"
#include"io_cfg.h"
//
//----------------------------------------------- M A K R O I
//
#define mLCD_Strobe() ((LCD_EN = HIGH),(Delay10TCYx(4)),(LCD_EN = LOW))
//
//----------------------------------------------- DEFINICIJE KOMANDI LCD DISPLEJA
//
#define FIRST_LINE          0x80
#define SECOND_LINE         0xC0
#define CURSOR_OFF          0x0C
#define CURSOR_UNDERLINE    0x0E
#define CURSOR_BLINK        0x0F
#define CURSOR_LEFT         0x10
#define CURSOR_RIGHT        0x14
#define CURSOR_STILL        0x03
#define DISPLAY_LEFT        0x18
#define DISPLAY_RIGHT       0x1C
#define CLEAR               0x01
#define HOME                0x02
#define READ_CHAR           0x22
//
//----------------------------------------------- F U N K C I J E
//
void LCD_Write(unsigned char data) {
    if(data & 0x80) LCD_DATA7 = HIGH; else LCD_DATA7 = LOW;
    if(data & 0x40) LCD_DATA6 = HIGH; else LCD_DATA6 = LOW;
    if(data & 0x20) LCD_DATA5 = HIGH; else LCD_DATA5 = LOW;
    if(data & 0x10) LCD_DATA4 = HIGH; else LCD_DATA4 = LOW;
    mLCD_Strobe();
    if(data & 0x08) LCD_DATA7 = HIGH; else LCD_DATA7 = LOW;
    if(data & 0x04) LCD_DATA6 = HIGH; else LCD_DATA6 = LOW;
    if(data & 0x02) LCD_DATA5 = HIGH; else LCD_DATA5 = LOW;
    if(data & 0x01) LCD_DATA4 = HIGH; else LCD_DATA4 = LOW;
    mLCD_Strobe();
    Delay10TCYx(40);
}// End of lcd write

void LCD_Clear(void){
    LCD_RS = LOW;
    LCD_Write(0x01);
    Delay1KTCYx(20);
}// End of lcd clear

void LCD_Goto(unsigned char line, unsigned char pos){
    LCD_RS = LOW;
    LCD_Write(line + (pos - 1));
}// End of lcd goto

void LCD_Putch(unsigned char data) {//Routine for writing the data to the LCD
    LCD_RS = HIGH;
    LCD_Write(data);
}// End of lcd putch

void LCD_Putrs(unsigned char line, unsigned char pos, const rom far char *data) {
    LCD_Goto(line, pos);
    LCD_RS = HIGH;
    while (*data)LCD_Putch(*data++);
}// End of lcd putrs

void LCD_Puts(unsigned near char *data) {
    LCD_RS = HIGH;
    while (*data) LCD_Putch(*data++);
}// End of lcd puts

void LCD_Reset(void) {//LCD Reset Function
    LCD_RS = 0;
    Delay10KTCYx(15); //Waits for 20ms
    LCD_DATA_PORT = 0x30;
    mLCD_Strobe();
    Delay10KTCYx(5); //Waits for 10ms
    mLCD_Strobe();
    Delay10TCYx(100); //Waits for 1ms
    mLCD_Strobe();
    Delay10KTCYx(5); //Waits for 1ms
    LCD_DATA_PORT = 0x20;
    mLCD_Strobe();
    Delay10TCYx(40);
    LCD_Write(0x28); //;4-bit, 2 line, 5x7 dots
    LCD_Write(0x0C); //Display ON cursor OFF
    LCD_Write(0x06); //Automatic Increment
    LCD_Write(0x01); //Bring cursor to line 1
}// End of lcs reset

void LCD_Command(unsigned char command) {
        LCD_EN = LOW;
        LCD_RS = LOW;
        LCD_Write(command);
}// End of Lcd Command

#endif	/* LCD_H */

