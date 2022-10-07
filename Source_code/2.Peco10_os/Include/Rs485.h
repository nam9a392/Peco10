#ifndef RS485_H
#define RS485_H

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
#define RS485_DE_PORT               GPIOA
#define RS485_DE_PIN                GPIO_PIN_0
#define RS485_INSTANCE              (&huart2)

/* 
 * Turn this macro to STD_ON if using timer to detect frame
 * In case using the delimiter to detect frame. Leave it as STD_OFF
 */
#define RS485_USING_TIMER_TO_DETECT_FRAME        STD_ON

/* RS485 Frame Loopback enabled */
#define RS485_FRAME_LOOPBACK_ENABLED             STD_ON

/* Time interval between two frames (in ms) */
#define RS485_TIME_INTERVAL                     (3U)
/*
 * Number Time base interrupt periods in 1ms. Ex Time base irq occurs every 500us -> Period per ms is 2U 
 * This macro is only used when RS485_USING_TIMER_TO_DETECT_FRAME = STD_ON
 */
#define RS485_SW_TIMER_PERIOD_PER_MILISECOND     (2U)

#define RS485_BUFF_SIZE                          256U

#define STX                0x02U
#define ETX                0x03U

#define EOT                0x04U
#define ACK                0x06U
#define NACK               0x15U
#define POL                0x05U
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
 * @brief Initialize RS485
 *
 * @param[in]  void
 *
 * @return void
 *
 */
void RS485_Init(void);

/**
 * @brief 
 *
 * @param[in]  pData  pointrer of data
 * @param[in]  length
 *
 *                                                                                          
 * @return Std_Return_Type
 *
 */
Std_Return_Type RS485_Transmit(uint8_t* pData_RS485, uint32_t length);

/**
 * @brief
 *
 * @param[in]
 * @param[in]
 *
 *
 * @return Std_Return_Type
 *
 */
Std_Return_Type RS485_Wait_For_Message(uint8_t* pData, uint32_t* length, uint32_t timeout);


#endif /* RS485_H */
