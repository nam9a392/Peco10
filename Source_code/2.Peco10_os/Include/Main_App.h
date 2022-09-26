#ifndef MAIN_APP_H
#define MAIN_APP_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

#include "Standard.h"
#include "Lcd_character.h"
#include "Lcd_segment.h"
#include "keypad.h"
#include "Rs485.h"

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define NUMBER_OF_MEMORY_POOL_ELEMENT       2U
#define MEMORY_POOL_ELEMENT_SIZE            100U

#define KEYPAD_SCANNING_DURATION            30U
#define KEYPAD_CHECKING_DURATION            300U
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
void Peco10_Preset_Init_All(void);

#endif /* MAIN_APP_H */
