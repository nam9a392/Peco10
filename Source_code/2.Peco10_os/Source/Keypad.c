/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Keypad.h"

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/


/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define MUX_D4_SEL  4
#define MUX_D5_SEL  5
#define MUX_D6_SEL  6
#define MUX_D7_SEL  7

#define KEYPAD_CONNECTED            0
#define KEYPAD_NOT_CONNECTED        1

#define FONT_LOWERCASE              0
#define FONT_UPPERCASE              1
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

static uint8_t gFontType = FONT_LOWERCASE;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/


/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

static void Select_Col(uint8_t Num_Col)
{
    /*set state of each column to LOW*/
    IC_74hc595_Send_Data(~((uint8_t)0x1 << (Num_Col)),KEYPAD);
}

static void Release_Col(void)
{
    /*set state of all column to HIGH*/
    IC_74hc595_Send_Data(DUMMY_DATA,KEYPAD);
}

static GPIO_PinState Read_Row(uint8_t Num_Row)
{
    /* select 74LS151 input pin D0-D4 to read keypad row*/
    return (GPIO_PinState)IC_74ls151(Num_Row);
}

/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/


Keypad_Button_Type Keypad_Scan_Position(void)
{
    Keypad_Button_Type eKeypad_Status = KEYPAD_UNKNOWN;
    uint8_t row,col;
    uint8_t KeypadLoss = KEYPAD_NOT_CONNECTED;
    row = col = 0;
    //Release_Col();
    for(col=0;col<NUM_COLS;col++)
    {
        /*Write logic 0 to column i*/
        Select_Col(col);
        for(row=0;row<NUM_ROWS;row++)
        {
            /* Get row state sequentially */
            if(GPIO_PIN_RESET == Read_Row(row))
            {
                eKeypad_Status = (Keypad_Button_Type)(row * NUM_COLS + col);
                if(KeypadLoss == KEYPAD_CONNECTED)
                    break;
            }else{
                KeypadLoss = KEYPAD_CONNECTED;
                /* out of loop if at least one button checked HIGH*/
                if(eKeypad_Status != KEYPAD_UNKNOWN)
                    break;
            }
        }
    }
    if(KeypadLoss == KEYPAD_NOT_CONNECTED)
        eKeypad_Status = KEYPAD_LOSS;
    return eKeypad_Status;
}

uint8_t Keypad_Mapping(uint8_t pos)
{
    return KeyMap[pos];
}

uint8_t Keypad_Mapping_Printer(uint8_t pos, uint8_t push_number)
{
    char character_pos;
    if(gFontType == FONT_LOWERCASE)
    {
        character_pos = (push_number) % strlen(KeyMap_Printer_Lowercase[pos]);
        return KeyMap_Printer_Uppercase[pos][character_pos];
    }else{
        character_pos = (push_number) % strlen(KeyMap_Printer_Uppercase[pos]);
        return KeyMap_Printer_Lowercase[pos][character_pos];
    }
}

void Keypad_Change_Font(void)
{
    gFontType ^= 1;
}

Keypad_Button_Type Keypad_Scan(uint8_t *pkey)
{
    Keypad_Button_Type KeypadPosition;
    
    KeypadPosition = Keypad_Scan_Position();
    if((KEYPAD_UNKNOWN != KeypadPosition)&&(KEYPAD_LOSS != KeypadPosition))
    {
        *pkey = Keypad_Mapping(KeypadPosition);
    }
    return KeypadPosition;
}

uint8_t Config_Switch_Get_Value(void)
{
    uint8_t Switch_value = 0;
    uint8_t Main_Address = 0x20;
    uint8_t Mux_input_status = 0;
    for (uint8_t i =0; i < 4 ; i++)
    {
        /* select 74LS151 input pin from D4 to D7 to get Switch data*/
        Mux_input_status = (uint8_t)IC_74ls151(i + MUX_D4_SEL);
        Switch_value = Switch_value | (Mux_input_status << i);
    }
    /*Return main address*/
    Main_Address = 0x20 + (Switch_value & 0x3)*4 + ((Switch_value & 0x4) >> 2);
    return Main_Address;
}
