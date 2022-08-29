#ifndef STANDARD_H
#define STANDARD_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/


#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define DUMMY_DATA  0xFF

/* 74HC595 Shift_reg pins define */
#define SHCP_PORT   GPIOB       
#define SHCP_PIN    GPIO_PIN_7
#define SHCP_SET    HAL_GPIO_WritePin(SHCP_PORT,SHCP_PIN,GPIO_PIN_SET)
#define SHCP_CLR    HAL_GPIO_WritePin(SHCP_PORT,SHCP_PIN,GPIO_PIN_RESET)
#define STCP_PORT   GPIOA       
#define STCP_PIN    GPIO_PIN_6
#define STCP_SET    HAL_GPIO_WritePin(STCP_PORT,STCP_PIN,GPIO_PIN_SET)
#define STCP_CLR    HAL_GPIO_WritePin(STCP_PORT,STCP_PIN,GPIO_PIN_RESET)
#define DS_PORT     GPIOA      
#define DS_PIN      GPIO_PIN_4
#define DS_SET      HAL_GPIO_WritePin(DS_PORT,DS_PIN,GPIO_PIN_SET)
#define DS_CLR      HAL_GPIO_WritePin(DS_PORT,DS_PIN,GPIO_PIN_RESET)

/* 74LS151 muxing pins define */
#define A_PORT      GPIOB
#define A_PIN       GPIO_PIN_3
#define B_PORT      GPIOA
#define B_PIN       GPIO_PIN_11
#define C_PORT      GPIOA
#define C_PIN       GPIO_PIN_5
#define Y_PORT      GPIOA
#define Y_PIN       GPIO_PIN_12
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/
extern  TIM_HandleTypeDef htim2;
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/**
 * @brief Std_Return_Type
 */
typedef enum
{
	E_OK = 0U,
	E_NOT_OK = 1U
} Std_Return_Type;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
void IC_74hc595(uint8_t data);
void IC_74hc595_Output(void);
GPIO_PinState IC_74ls151(uint8_t Select_Input);

/**
 * @brief Convert decimal number to string array
 *
 * @return len of array containing number converted
 *
 */
uint8_t DecToString(uint8_t *pData,uint32_t number);

/**
 * @brief function use to delay in micro second
 */
void udelay(uint32_t us);

/**
 * @brief function use to delay in mini second
 */
void mdelay(uint32_t ms);

#endif /* STANDARD_H */
