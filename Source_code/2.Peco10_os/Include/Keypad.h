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

#define KEYPAD_FORWARD              KEYPAD_PRESET_1 
#define KEYPAD_BACKWARD             KEYPAD_PRESET_2
#define KEYPAD_ENDLINE              KEYPAD_PRESET_3
#define KEYPAD_FONT                 KEYPAD_PRESET_4

#define KEYPAD_SPECIAL_KEY_CHECK(x)   ((x == KEYPAD_ENTER) || (x == KEYPAD_CLEAR) || \
                                       (x == KEYPAD_PRESET_1) || (x == KEYPAD_PRESET_2) || \
                                       (x == KEYPAD_PRESET_3) || (x == KEYPAD_PRESET_4))

#define KEYPAD_NORMAL_KEY_CHECK(x)    ((x == KEYPAD_NUMBER_0) || (x == KEYPAD_NUMBER_1) || (x == KEYPAD_NUMBER_2) || \
                                       (x == KEYPAD_NUMBER_3) || (x == KEYPAD_NUMBER_4) || (x == KEYPAD_NUMBER_5) || \
                                       (x == KEYPAD_NUMBER_6) || (x == KEYPAD_NUMBER_7) || (x == KEYPAD_NUMBER_8) || \
                                       (x == KEYPAD_NUMBER_9))
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

static const uint8_t KeyMap[NUM_ROWS*NUM_COLS]={
0x30,0x31,0x32,0x33,
0x20,0x21,0x22,0x23,
0x10,0x11,0x12,0x13,
0   ,0x01,0x02,0x03
};

static const char* KeyMap_Printer_Uppercase[NUM_ROWS*NUM_COLS]={
"7STU","8VWX","9YZ" ,"",
"4JKL","5MNO","6PQR","",
"1ABC","2DEF","3GHI","",
"0., " ,"X"   ,"C"   ,""
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
    KEYPAD_NUMBER_7  = 0U,      KEYPAD_NUMBER_8  = 1U,      KEYPAD_NUMBER_9  = 2U,      KEYPAD_PRESET_1  = 3U,
    KEYPAD_NUMBER_4  = 4U,      KEYPAD_NUMBER_5  = 5U,      KEYPAD_NUMBER_6  = 6U,      KEYPAD_PRESET_2  = 7U,
    KEYPAD_NUMBER_1  = 8U,      KEYPAD_NUMBER_2  = 9U,      KEYPAD_NUMBER_3  = 10U,     KEYPAD_PRESET_3  = 11U,
    KEYPAD_NUMBER_0  = 12U,     KEYPAD_ENTER     = 13U,     KEYPAD_CLEAR     = 14U,     KEYPAD_PRESET_4  = 15U,
    
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

/**
 * @brief  This function uses to change font buffer that mapping keypad layout
 *
 * @param[in]  void
 *
 * @retval     void
 *
 * @note Should be called in thread or created a new thread 
 */
void Keypad_Change_Font(void);

Keypad_Button_Type Keypad_Scan_Position(void);
/**
 * @brief  This function uses to return vaue of the switch which define address of device 
 *
 * @param[in]  void
 *
 * @retval     uint8_t Address value of the device
 *
 * @note Should be called in thread or created a new thread 
 */
uint8_t Config_Switch_Get_Value(void);


#endif /* KEYPAD_H */
