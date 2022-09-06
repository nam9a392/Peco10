/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Lcd_character.h"
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
/* Variable to store pin state of the 74HC595 */
static volatile uint8_t Current_74HC595_Data_Out = 0U;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void Lcd_Instruction_Enable(void);
static void Lcd_Instruction_Disable(void);
static void Lcd_Enable_Pin_Low(void);
static void Lcd_Enable_Pin_High(void);
static void lcd_enable(void);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @Brief: Write 0 to RS pin 
 * @implement Lcd_Instruction_Enable_Activity
 * 
 */
static void Lcd_Instruction_Enable(void)
{
    /* Write 0 logic to Q4 Pin of 74HC595 */
    Current_74HC595_Data_Out &= ~((uint8_t)0x10U);
    /* Shift data to 74HC595 */
    IC_74hc595_Send_Data(Current_74HC595_Data_Out,LCD_CHARACTER);
    
}

/**
 * @Brief: Write 1 to RS pin 
 * @implement Lcd_Instruction_Disable_Activity
 *
 */
static void Lcd_Instruction_Disable(void)
{
    /* Write 1 logic to Q4 Pin of 74HC595 */
    Current_74HC595_Data_Out |= ((uint8_t)0x10U);
    /* Shift data to 74HC595 */
    IC_74hc595_Send_Data(Current_74HC595_Data_Out,LCD_CHARACTER);
}
    
/**
 * @Brief: Write 0 to EN pin 
 * @implement Lcd_Enable_Pin_Low_Activity
 *
 */
static void Lcd_Enable_Pin_Low(void)
{
    /* Write 0 logic to Q5 Pin of 74HC595 */
    Current_74HC595_Data_Out &= ~((uint8_t)0x20U);
    /* Shift data to 74HC595 */
    IC_74hc595_Send_Data(Current_74HC595_Data_Out,LCD_CHARACTER);
}

/**
 * @Brief: Write 1 to EN pin 
 * @implement Lcd_Enable_Pin_High_Activity
 *
 */
static void Lcd_Enable_Pin_High(void)
{
    /* Write 1 logic to Q5 Pin of 74HC595 */
    Current_74HC595_Data_Out |= ((uint8_t)0x20U);
    /* Shift data to 74HC595 */
    IC_74hc595_Send_Data(Current_74HC595_Data_Out,LCD_CHARACTER);
}

static void lcd_enable(void)
{
    /*EN = 1*/
    Lcd_Enable_Pin_High();
    udelay(10);
    /*EN = 0*/
    Lcd_Enable_Pin_Low();
    udelay(100);
}

/**
 *
 * @implement Lcd_Write_Activity
 *
 */
static void Lcd_Write_4bits(uint8_t data)
{
    /* Clear 4 bits low (Q0-Q1-Q2-Q3) */
	Current_74HC595_Data_Out &= (uint8_t)0xF0U;
    /* Override the 4 bit low */
    Current_74HC595_Data_Out |= (data & (uint8_t)0x0FU);
	/* Shift data to 74HC595 */
    
    IC_74hc595_Send_Data(Current_74HC595_Data_Out,LCD_CHARACTER);
    
	lcd_enable();
}
static void lcd_send_command(uint8_t cmd)
{
    /*RS = 0, for LCD command*/
    Lcd_Instruction_Enable();
    /*Send 4 bit High of command*/
    Lcd_Write_4bits(cmd >> 4);
    /*Send 4 bit Low of command*/
    Lcd_Write_4bits(cmd & 0xF);
}

/*
 * This function sends a character to the LCD
 * Here we are using 4 bits parallel data transmission
 * */
void Lcd_Put_Char(uint8_t data)
{
    /*RS = 1, for LCD user data*/
    Lcd_Instruction_Disable();
    /*Send 4 bit High of command*/
    Lcd_Write_4bits(data >> 4);
    /*Send 4 bit Low of command*/
    Lcd_Write_4bits(data & 0xF);
}

/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/


void Lcd_Init_4bits_Mode(void)
{
    //1. Do the lcd initialization
    mdelay(40);
    /*RS = 0, for LCD command*/
    Lcd_Instruction_Enable();
    
    Lcd_Write_4bits(0x3U);
    mdelay(5);
    Lcd_Write_4bits(0x3U);
    udelay(150);
    
    Lcd_Write_4bits(0x3U);
    Lcd_Write_4bits(0x2U);
    
    //function set command
    lcd_send_command(LCD_CMD_4DL_2N_5X8F);
    udelay(40);
    
    //Display on cursor on
    lcd_send_command(LCD_CMD_DON_CURON);
    udelay(40);
    
    Lcd_Clear();
    //entry mode set
    lcd_send_command(LCD_CMD_INCADD);
    udelay(40);
}

void Lcd_Clear(void)
{
    lcd_send_command(LCD_CMD_DIS_CLEAR);
    /*delay more than 1.52ms for command process 
    * Check page num24 of datasheet
    */
    mdelay(2);
}

void Lcd_Put_String(uint8_t line, uint8_t offset, uint8_t *pString)
{
    uint8_t count=0;
    Lcd_Set_Cursor(line,offset);
    do
    {
        Lcd_Put_Char((uint8_t)*pString++);
        count++;
        if((offset + (16*line) + count) == 16)
            Lcd_Set_Cursor(1,0);
    }while(*pString != '\0');
}

void Lcd_Set_Cursor(uint8_t line, uint8_t offset)
{
    switch(line)
    {
        case 0:
            lcd_send_command(offset | 0x80);
            break;
        case 1:
            lcd_send_command(offset | 0xC0);
            break;
        default:
            break;
    }
    /*delay more than 37us for command process 
    * Check page num24 of datasheet
    */
    udelay(40);
}

void Lcd_Turn_On_Cursor(void)
{
    lcd_send_command(LCD_CMD_DON_CURON);
    /*delay more than 37us for command process 
    * Check page num24 of datasheet
    */
    udelay(40);
}

void Lcd_Turn_Off_Cursor(void)
{
    lcd_send_command(LCD_CMD_DON_CUROFF);
    /*delay more than 37us for command process 
    * Check page num24 of datasheet
    */
    udelay(40);
}


uint8_t Lcd_Character_Get_Current_74HC595_Value(void)
{
    return Current_74HC595_Data_Out;
}
