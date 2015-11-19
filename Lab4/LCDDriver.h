/*
 * LCDDriver.h
 *
 *  R. Traylor 10.7.09
 *  Justin Goins 10.14.14
 */ 


#ifndef LCDDRIVER_H_
#define LCDDRIVER_H_

// Public LCD functions
void LCD_Init(void);
void LCD_SPIInit(void);
void LCD_PutChar(char a_char);
void LCD_PutDec8(uint8_t num);
void LCD_PutDec16(uint16_t num);
void LCD_PutStr(char *lcd_str);
void LCD_FillSpaces(void);
void LCD_Clr(void);
void LCD_MovCursorLn1(void);
void LCD_MovCursorLn2(void);
void LCD_MovCursor(uint8_t line, uint8_t position);
void LCD_CursorBlinkOn(void);
void LCD_CursorBlinkOff(void);


#endif /* LCDDRIVER_H_ */