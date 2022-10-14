#ifndef KEYPAD_H
#define KEYPAD_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/


#include "main.h"
#include "Standard.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define DUMMY_DATA  0xFF

#define NUM_ROWS    4
#define NUM_COLS    4

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

//static char KeyMap[NUM_ROWS][NUM_COLS]={
//{'1','2','3','A'},
//{'4','5','6','B'},
//{'7','8','9','C'},
//{'*','0','#','D'}
//};

static const uint8_t KeyMap[NUM_ROWS*NUM_COLS]={
'7','8','9',0xf0,
'4','5','6',0xf1,
'1','2','3',0xf2,
'0',0xE0,0xE1,0xf3
};

static const char* KeyMap_Printer_Uppercase[NUM_ROWS*NUM_COLS]={
"7STU","8VWX","9YZ" ,"",
"4JKL","5MNO","6PQR","",
"1ABC","2DEF","3GHI","",
"0., ","X"   ,"C"   ,""
};

static const char* KeyMap_Printer_Lowercase[NUM_ROWS*NUM_COLS]={
"7stu","8vwx","9yz" ,"",
"4jkl","5mno","6pqr","",
"1abc","2def","3ghi","",
"0., ","X"   ,"C"   ,""
};

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum{
    KEYPAD_NUMBER_7  = 0U,      KEYPAD_NUMBER_8  = 1U,      KEYPAD_NUMBER_9  = 2U,      KEYPAD_FORWARD   = 3U,
    KEYPAD_NUMBER_4  = 4U,      KEYPAD_NUMBER_5  = 5U,      KEYPAD_NUMBER_6  = 6U,      KEYPAD_BACKWARD  = 7U,
    KEYPAD_NUMBER_1  = 8U,      KEYPAD_NUMBER_2  = 9U,      KEYPAD_NUMBER_3  = 10U,     KEYPAD_ENDLINE   = 11U,
    KEYPAD_NUMBER_0  = 12U,     KEYPAD_ENTER     = 13U,     KEYPAD_CLEAR     = 14U,     KEYPAD_FONT      = 15U,
    
    KEYPAD_UNKNOWN      ,
    KEYPAD_LOSS      
}Keypad_Button_Type;

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
 * @brief  This function uses to scan the button which is pushed
 *
 * @param[in]  uint8_t * pointer to store character corresponding to button pushed
 *
 * @retval     Keypad_Button_Type 
 *
 * @note Should be called in thread or created a new thread 
 */
Keypad_Button_Type Keypad_Scan(uint8_t *pkey);

uint8_t Keypad_Mapping(uint8_t pos);
uint8_t Keypad_Mapping_Printer(uint8_t pos, uint8_t push_number);
void Keypad_Change_Font(void);

Keypad_Button_Type Keypad_Scan_Position(void);
/**
 * @brief  This function uses to return vaue of the switch which define address of device 
 *
 * @param[in]  None
 *
 * @retval     uint8_t Address value of the device
 *
 * @note Should be called in thread or created a new thread 
 */
uint8_t Config_Switch_Get_Value(void);


#endif /* KEYPAD_H */
