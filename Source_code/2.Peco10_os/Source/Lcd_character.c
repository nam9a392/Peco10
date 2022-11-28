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
#define LCD_RS          (uint8_t)(0x1U << 4)
#define LCD_EN          (uint8_t)(0x1U << 5)
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct{
    uint8_t line;
    uint8_t col;
}Lcd_Cursor_Pos_t;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
/* Variable to store pin state of the 74HC595 */
//static volatile uint8_t Current_74HC595_Data_Out = 0U;
static volatile uint8_t Current_74HC595_Data_Out = 0U;
/* Variable to store current position of cursor*/
static volatile Lcd_Cursor_Pos_t gCurrent_Cursor_Posistion = {0,0};
/* Variable to store current position of display*/
static volatile uint8_t gDisplayPosition = 0;
/* Cache buffer store current DDRAM values */
static volatile uint8_t LcdCharacterDataDisplay[LCD_CHARACTER_LINES*MAX_CHARACTER_OF_LINE] = {0};
static volatile uint8_t gDisplayShift = STD_OFF;
static volatile Lcd_Display_Mode_t gDisplayMode = NORMAL_MODE;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void Lcd_Instruction_Enable(void);
static void Lcd_Instruction_Disable(void);
static void Lcd_Enable_Pin_Low(void);
static void Lcd_Enable_Pin_High(void);
static void lcd_enable(void);

static void Lcd_Display_Move_Right(void);
static void Lcd_Display_Move_Left(void);
static void Lcd_Return_Home(void);
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
    Current_74HC595_Data_Out &= ~(LCD_RS);
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
    Current_74HC595_Data_Out |= (LCD_RS);
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
    Current_74HC595_Data_Out &= ~(LCD_EN);
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
    Current_74HC595_Data_Out |= (LCD_EN);
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
    /* cache input data to read later*/
    LcdCharacterDataDisplay[gCurrent_Cursor_Posistion.line*MAX_CHARACTER_OF_LINE + gCurrent_Cursor_Posistion.col] = data;
    
    /*RS = 1, for LCD user data*/
    Lcd_Instruction_Disable();
    /*Send 4 bit High of command*/
    Lcd_Write_4bits(data >> 4);
    /*Send 4 bit Low of command*/
    Lcd_Write_4bits(data & 0xF);
    udelay(40);
    
    /*move cursor back to previous position*/
    if((gCurrent_Cursor_Posistion.line == 1) && (gCurrent_Cursor_Posistion.col == (MAX_CHARACTER_OF_LINE - 1)))
    {
        Lcd_Set_Cursor(gCurrent_Cursor_Posistion.line,gCurrent_Cursor_Posistion.col);
    }else
    {
        lcd_send_command(LCD_CURSOR_MOVE|LCD_SHIFT_LEFT);
        udelay(40);
    }
}

static void Lcd_Display_Move_Right(void)
{
    uint8_t i=0;
    if(gDisplayPosition == 0)
    {
        /*shift display to end page when reach left horizon*/
        gDisplayPosition = (MAX_CHARACTER_OF_LINE - MAX_CHARACTER_OF_DISPLAY);
        for(i=0;i<16;i++)
        {
            lcd_send_command(LCD_DISPLAY_SHIFT|LCD_SHIFT_RIGHT);
            udelay(40);
        }
    }else{
        gDisplayPosition--;
        lcd_send_command(LCD_DISPLAY_SHIFT|LCD_SHIFT_RIGHT);
        udelay(40);
    }
}    

static void Lcd_Display_Move_Left(void)
{
    uint8_t i=0;
    if(gDisplayPosition  == (MAX_CHARACTER_OF_LINE - MAX_CHARACTER_OF_DISPLAY))
    {
        /*shift display to first page when reach right horizon*/
        gDisplayPosition = 0;
        for(i=0;i<16;i++)
        {
            lcd_send_command(LCD_DISPLAY_SHIFT|LCD_SHIFT_LEFT);
            udelay(40);
        }
    }else{
        gDisplayPosition++;
        lcd_send_command(LCD_DISPLAY_SHIFT|LCD_SHIFT_LEFT);
        udelay(40);
    }
}  

static void Lcd_Return_Home(void)
{
    gDisplayPosition = 0;
    lcd_send_command(LCD_CMD_DIS_RETURN_HOME);
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
    lcd_send_command(LCD_CMD_DON_CUROFF);
    udelay(40);
    
    Lcd_Clear();
    //entry mode set
    lcd_send_command(LCD_CMD_INCADD);
    udelay(40);
    
    gDisplayMode = NORMAL_MODE;
    gDisplayShift = STD_OFF;
}

void Lcd_Clear(void)
{
    /* clear cache data*/
    memset((uint8_t*)LcdCharacterDataDisplay,' ',LCD_CHARACTER_LINES*MAX_CHARACTER_OF_LINE);
    /* clear cache cursor value*/
    gDisplayPosition = 0;
    gCurrent_Cursor_Posistion.col = 0;
    gCurrent_Cursor_Posistion.line = 0;
    
    lcd_send_command(LCD_CMD_DIS_CLEAR);
    /*delay more than 1.52ms for command process 
    * Check page num24 of datasheet
    */
    mdelay(2);

}

void Lcd_Clear_Character(void)
{
    uint8_t temp;
    temp = gCurrent_Cursor_Posistion.line * MAX_CHARACTER_OF_LINE + gCurrent_Cursor_Posistion.col;
    /* Clear a character at current position or the backward position if current equal to space*/
    if(LcdCharacterDataDisplay[temp] == ' ')
    {
        Lcd_Move_Cursor_Left();
    }
    Lcd_Put_Char(' ');
}

/* if Line/col equal by 0xff print at current posistion*/
void Lcd_Put_String(uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length)
{
    uint16_t i=0;
    /* Check input validation */
    if((line > LCD_CHARACTER_LINES - 1) || (offset > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }
    if((line != LCD_CURRENT_POSITION) && (offset != LCD_CURRENT_POSITION))
    {
//        gCurrent_Cursor_Posistion.col = offset;
//        gCurrent_Cursor_Posistion.line = line;
        Lcd_Set_Cursor(line,offset);
    }
    for (i=0 ; i<length ; i++)
    {
        Lcd_Put_Char((uint8_t)pString[i]);
        Lcd_Move_Cursor_Right();
    }
}

void lcd_Read_Current_Position(uint8_t *line,uint8_t *offset)
{
    *line   = gCurrent_Cursor_Posistion.line;
    *offset = gCurrent_Cursor_Posistion.col;
}

uint8_t Lcd_Read_Data(uint8_t line, uint8_t offset, uint8_t *pData, uint8_t length)
{
    uint8_t temp,count;
    temp = count = 0;
    count = length;
    /*check input position valid*/
    if((line == LCD_CURRENT_POSITION) && (offset == LCD_CURRENT_POSITION))
    {
        line   = gCurrent_Cursor_Posistion.line;
        offset = gCurrent_Cursor_Posistion.col;
    }else if((line > LCD_CHARACTER_LINES - 1) || (offset > MAX_CHARACTER_OF_LINE - 1))
    {
        return 0;
    }
    
    temp  = line*MAX_CHARACTER_OF_LINE + offset + length;
    if(temp > (LCD_CHARACTER_LINES*MAX_CHARACTER_OF_LINE))
    {
        temp = LCD_CHARACTER_LINES*MAX_CHARACTER_OF_LINE;
        count = LCD_CHARACTER_LINES*MAX_CHARACTER_OF_LINE - (line*MAX_CHARACTER_OF_LINE + offset);
    }
    for( ;temp>line*MAX_CHARACTER_OF_LINE + offset;temp--)
    {
        if(LcdCharacterDataDisplay[temp - 1] == ' ')
        {
            count--;
        }else
        {
            break;
        }
    }
    if((length > 0) && (pData != NULL))
        memcpy(pData,(uint8_t*)&LcdCharacterDataDisplay[line*MAX_CHARACTER_OF_LINE + offset],length);
    return count;
}

void Lcd_Move_Cursor_Right(void)
{
    /*stop cursor at max horizone*/
    if(!((gCurrent_Cursor_Posistion.col == (MAX_CHARACTER_OF_LINE - 1)) && (gCurrent_Cursor_Posistion.line == (LCD_CHARACTER_LINES - 1))))
    {
        gCurrent_Cursor_Posistion.col++;
        /*shift display to the left if cursor reach right horizon*/
        if((gCurrent_Cursor_Posistion.col > (gDisplayPosition + (MAX_CHARACTER_OF_DISPLAY - 1))) && (gDisplayShift == STD_ON))
        {
            /*Shift display to the left*/
            Lcd_Display_Move_Left();
        }
        if(gCurrent_Cursor_Posistion.col > (MAX_CHARACTER_OF_LINE - 1))
        {
            gCurrent_Cursor_Posistion.col = 0;
            gCurrent_Cursor_Posistion.line ^= 1;
        }
        /*shift cursor position to the right*/    
        lcd_send_command(LCD_CURSOR_MOVE|LCD_SHIFT_RIGHT);
        udelay(40);
    }
}

void Lcd_Move_Cursor_Left(void)
{
    /*stop cursor at min horizone*/
    if((gCurrent_Cursor_Posistion.col != 0) || (gCurrent_Cursor_Posistion.line != 0))
    {
        /*shift display to the right if cursor reach left horizon*/
        if(gCurrent_Cursor_Posistion.col == 0)
        {
            gCurrent_Cursor_Posistion.col = MAX_CHARACTER_OF_LINE - 1;
            gCurrent_Cursor_Posistion.line ^= 1;
            /*Shift display to the right*/
            Lcd_Display_Move_Right();
        }else
        {
            gCurrent_Cursor_Posistion.col--;
            if((gCurrent_Cursor_Posistion.col < gDisplayPosition) && (gDisplayShift == STD_ON))
            {
                Lcd_Display_Move_Right();
            }
        }
        /*shift cursor position to the left*/ 
        lcd_send_command(LCD_CURSOR_MOVE|LCD_SHIFT_LEFT);
        udelay(40);
    }
}

void Lcd_Move_Cursor_Next_Line(void)
{
    gCurrent_Cursor_Posistion.line ^= 1;
    Lcd_Set_Cursor(gCurrent_Cursor_Posistion.line,gCurrent_Cursor_Posistion.col);
}

void Lcd_Set_Cursor(uint8_t line, uint8_t offset)
{
    /*check input position valid*/
    if((line > LCD_CHARACTER_LINES - 1) || (offset > MAX_CHARACTER_OF_LINE - 1))
    {
        return;
    }
    if(gDisplayShift == STD_ON)
    {
        while(offset < gDisplayPosition)
        {
            /*Shift display to the right*/
            Lcd_Display_Move_Right();
        }
        while(offset > (gDisplayPosition + (MAX_CHARACTER_OF_DISPLAY - 1)))
        {
            /*Shift display to the left*/
            Lcd_Display_Move_Left();
        }
    }
    gCurrent_Cursor_Posistion.line = line;
    gCurrent_Cursor_Posistion.col  = offset;
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

void Lcd_Cursor_Effect(uint8_t CursorEffect)
{
    switch(CursorEffect)
    {
        case 0:
            lcd_send_command(LCD_CMD_DON_CUROFF);
            break;
        case 1:
            lcd_send_command(LCD_CMD_DON_CURON);
            break;
        case 2:
            lcd_send_command(LCD_CMD_DON_CURBLINK);
            break;
        default:
            lcd_send_command(LCD_CMD_DON_CUROFF);
            break;
    }
    /*delay more than 37us for command process 
    * Check page num24 of datasheet
    */
    udelay(40);
}

void Lcd_ShiftDisplay_Turn_On(void)
{
    gDisplayShift = STD_ON;
}

void Lcd_ShiftDisplay_Turn_Off(void)
{
    gDisplayShift = STD_OFF;
}

void Lcd_SetDisplayMode(Lcd_Display_Mode_t mode)
{
    gDisplayMode = mode;
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
