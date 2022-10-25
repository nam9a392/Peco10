#ifndef STANDARD_H
#define STANDARD_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/


#include "main.h"
#include "swtimer.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"
#include <assert.h>
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define STD_ON                          1
#define STD_OFF                         0
#define DUMMY_DATA                      0xFF

#define PROGRAM_USE_RTOS                STD_ON


/* 74HC595 Shift_reg pins define */
#define SHCP_PORT                       GPIOB       
#define SHCP_PIN                        GPIO_PIN_7
#define SHCP_SET                        HAL_GPIO_WritePin(SHCP_PORT,SHCP_PIN,GPIO_PIN_SET)
#define SHCP_CLR                        HAL_GPIO_WritePin(SHCP_PORT,SHCP_PIN,GPIO_PIN_RESET)
#define STCP_PORT                       GPIOA       
#define STCP_PIN                        GPIO_PIN_6
#define STCP_SET                        HAL_GPIO_WritePin(STCP_PORT,STCP_PIN,GPIO_PIN_SET)
#define STCP_CLR                        HAL_GPIO_WritePin(STCP_PORT,STCP_PIN,GPIO_PIN_RESET)
#define DS_PORT                         GPIOA      
#define DS_PIN                          GPIO_PIN_4
#define DS_SET                          HAL_GPIO_WritePin(DS_PORT,DS_PIN,GPIO_PIN_SET)
#define DS_CLR                          HAL_GPIO_WritePin(DS_PORT,DS_PIN,GPIO_PIN_RESET)

/* 74LS151 muxing pins define */
#define A_PORT                          GPIOB
#define A_PIN                           GPIO_PIN_3
#define B_PORT                          GPIOA
#define B_PIN                           GPIO_PIN_11
#define C_PORT                          GPIOA
#define C_PIN                           GPIO_PIN_5
#define Y_PORT                          GPIOA
#define Y_PIN                           GPIO_PIN_12

#define TEST_CODE       0
#define configASSERT( x ) if( ( x ) == 0 ) {for( ;; ); }	
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/
extern  TIM_HandleTypeDef htim1;
extern  TIM_HandleTypeDef htim2;

extern  UART_HandleTypeDef huart2;
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

typedef enum
{
	LCD_CHARACTER = 0U,
	KEYPAD = 1U
} Device_Type;

typedef struct {
	uint8_t * buffer;
	uint8_t head;
	uint8_t tail;
	uint8_t max; //of the buffer
}circular_buf_t ;

typedef circular_buf_t* cbuf_handle_t;
/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Latch input data in IC 74hc595
 *
 * @return void
 *
 */
void IC_74hc595(uint8_t data);

/**
 * @brief Shiftout data to output IC 74hc595
 *
 * @return void
 *
 */
void IC_74hc595_Output(void);

/**
 * @brief Shift data to specific devices
 *
 * @param[in]  uint8_t      :data
 * @param[in]  Device_Type  :KEYPAD
 *                           LCD_CHARACTER
 *
 * @return len of array containing number converted
 *
 */
void IC_74hc595_Send_Data(uint8_t data, Device_Type Component);

/**
 * @brief Get status of input selected IC mux 74LS151
 *
 * @param[in]  uint8_t      :line number
 *
 * @return GPIO_PinState of Input line selected of IC 74LS151
 *
 */
GPIO_PinState IC_74ls151(uint8_t Select_Input);

/**
 * @brief Convert decimal number to string array
 *
 * @return len of array containing number converted
 *
 */
uint8_t DecToString(uint8_t *pData,uint32_t number);

/******************************** Delay function **********************************************/
/**
 * @brief function use to delay in micro second
 */
void udelay(uint32_t us);

/**
 * @brief function use to delay in mini second
 */
void mdelay(uint32_t ms);

#endif /* STANDARD_H */
