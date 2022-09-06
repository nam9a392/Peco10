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

//static char KeyMap[NUM_ROWS * NUM_COLS]={
//'7','8','9','A',
//'4','5','6','B',
//'1','2','3','C',
//'0','C','X','D'
//};

static char* KeyMap[NUM_ROWS][NUM_COLS]={
{"7","8","9","F1"},
{"4","5","6","F2"},
{"1","2","3","F3"},
{"0","C","X","F4"}
};

/*==================================================================================================
*                                              ENUMS
==================================================================================================*/
typedef enum{
    BUTTON_UNKNOWN      = 0U,
    KEYPAD_LOSS         = 1U,
    KEYPAD_PUSHED       = 2U
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
