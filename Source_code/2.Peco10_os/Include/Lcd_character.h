#ifndef LCD_CHARACTER_H
#define LCD_CHARACTER_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

#include "Standard.h"
#include "main.h"
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/*Lcd commands  */
/* 4bits, 2lines, 5x8 */
#define LCD_CMD_4DL_2N_5X8F			0x28

/* Display on, cursor on, cursor not blink */
#define LCD_CMD_DON_CURON			0x0E
/* Display on, cursor off, cursor not blink */
#define LCD_CMD_DON_CUROFF			0x0C

/* cursor move direction is increment */
#define LCD_CMD_INCADD				0x06
/* Clear entire display */
#define LCD_CMD_DIS_CLEAR			0x01
/* Return Cursor to home */
#define LCD_CMD_DIS_RETURN_HOME		0x02

#define LCD_DISPLAY_SHIFT           0x18
#define LCD_CURSOR_MOVE             0X10
#define LCD_SHIFT_RIGHT             0x4
#define LCD_SHIFT_LEFT              0

#define LCD_CURRENT_POSITION        0xff
#define LCD_NEXT_POSITION           0xfe
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * @brief  This function uses to initialize the LCD to work in 4 bits mode
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd_Init_4bits_Mode(void);

/**
 * @brief  This function uses to clear the LCD
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd_Clear(void);

/**
 * @brief  This function uses to clear character at current cursor position of LCD
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd_Clear_Character(void);

/**
 * @brief  This function uses to put character to current position
 *
 * @param[in]  data   :ASCII character
 *
 * @retval void
 *
 */
void Lcd_Put_Char(uint8_t data);

/**
 * @brief  This function uses to put string to (x,y) position
 *
 * @param[in]  x         :x position
 * @param[in]  y         :y position
 * @param[in]  pString   :pString string pointer
 *
 * @retval void
 *
 */
void Lcd_Put_String(uint8_t line, uint8_t offset, uint8_t *pString);
 
 /*
 * @brief  This function uses to set the lcd cursor position
 * @param[in]  line    : line number (0 to 1)
 *             offset  : offset position in line number (0 to 15) Assuming a 2 x 16 characters display
 *
 * @retval void
 *
 */
void Lcd_Set_Cursor(uint8_t line, uint8_t offset);
void Lcd_Move_Cursor_Right(void);
void Lcd_Move_Cursor_Left(void);

/**
 * @brief  This function uses to turn on the lcd cursor
 *
 * @param[in]  line    : line number
 *             offset  : offset position in line number
 *
 * @retval void
 *
 */
 void Lcd_Turn_On_Cursor(void);

/**
 * @brief  This function uses to turn off the lcd cursor
 *
 * @param[in]  none
 *
 * @retval void
 *
 */
 void Lcd_Turn_Off_Cursor(void);

/**
 * @brief  This function uses to get current 74HC595 value of LCD character
 *
 * @param[in]  None
 *
 * @retval uint8_t
 *
 */
uint8_t Lcd_Character_Get_Current_74HC595_Value(void);

#endif /* LCD_CHARACTER_H */
