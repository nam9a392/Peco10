/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Standard.h"
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

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/


/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
void IC_74hc595(uint8_t data)
{
    uint8_t i;
    SHCP_CLR;
    for(i=0;i<8;i++)
    {
        if((data & 0x80) == 0)
        {
            DS_CLR;
        }else
        {
            DS_SET;
        }
        SHCP_SET;
        SHCP_CLR;
        data = data << 1;
    }
}


void IC_74hc595_Output(void)
{
    STCP_CLR;
    udelay(5);
    STCP_SET;
    STCP_CLR;
}

GPIO_PinState IC_74ls151(uint8_t Select_Input)
{
    GPIO_PinState temp = GPIO_PIN_SET;
    GPIO_PinState A_State = (GPIO_PinState)((Select_Input & 0x01) >> 0);
    GPIO_PinState B_State = (GPIO_PinState)((Select_Input & 0x02) >> 1);
    GPIO_PinState C_State = (GPIO_PinState)((Select_Input & 0x04) >> 2);
    
    HAL_GPIO_WritePin(A_PORT,A_PIN,A_State);
    HAL_GPIO_WritePin(B_PORT,B_PIN,B_State);
    HAL_GPIO_WritePin(C_PORT,C_PIN,C_State);
    
    temp = HAL_GPIO_ReadPin(Y_PORT,Y_PIN);
    return temp;
}

uint8_t DecToString(uint8_t *pData,uint32_t number)
{
    return sprintf((char*)pData,"%d",number);
}

void udelay(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim2) < us);  // wait for the counter to reach the us input in the parameter
}

void mdelay(uint32_t ms)
{
    __HAL_TIM_SET_COUNTER(&htim2,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim2) < (ms*1000));  // wait for the counter to reach the us input in the parameter
}
