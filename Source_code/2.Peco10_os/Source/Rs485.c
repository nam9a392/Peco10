/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Rs485.h"
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define RS485_TRANSMIT_DISABLE()            HAL_GPIO_WritePin(RS485_DE_PORT,RS485_DE_PIN,GPIO_PIN_RESET)
#define RS485_TRANSMIT_ENABLE()             HAL_GPIO_WritePin(RS485_DE_PORT,RS485_DE_PIN,GPIO_PIN_SET)

//#define RING_BUFFER_INDEX_INCREASE(x,BUFFER_SIZE)       do{x++; x &= (BUFFER_SIZE-1);}while(0)
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
#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)
    /* Software Timer for detecting Frame */
    static TM_DELAY_Timer_t* RS485_Sw_Timer = NULL;
#else
    /* Delimiter count */
    static volatile uint8_t gFrameReceiving = 0;
#endif /* RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON */

static osMessageQueueId_t RS485_RxMsgInforQueue_ID; 

/* Ring buffer use for receiving data through rs485*/
uint8_t gRxBuffer[RS485_BUFF_SIZE];
volatile uint8_t gLoopBackStatus = 0;

volatile RingBufferManageQueue_t gRxBufferInfo = {
    .tail = 0,
    .head = 0,
    .n_elem = 0
};
volatile uint8_t Framelength = 0;
volatile uint8_t gRxBufferPreviousHead = 0;
volatile uint8_t gRxBufferPreviousSize = 0;
volatile uint8_t gDurringTransmitTime = 0;
volatile uint8_t gDurringReceiveTime = 0;
volatile uint8_t count=0;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

void RS485_SwTimer_Callback(void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/


/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief  This function uses to initialize RS485
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void RS485_Init(void)
{
   HAL_StatusTypeDef Hal_Status;

#if(STD_ON == RS485_USING_TIMER_TO_DETECT_FRAME)
   TM_DELAY_Init();
   RS485_Sw_Timer = TM_DELAY_TimerCreate(RS485_TIME_INTERVAL * RS485_SW_TIMER_PERIOD_PER_MILISECOND,
                                         SOFTWARE_TIMER_AUTO_RELOAD_DISABLE,
                                         SOFTWARE_TIMER_START_TIMER_DISABLE,
                                         RS485_SwTimer_Callback,
                                         NULL);
#endif
   RS485_RxMsgInforQueue_ID = osMessageQueueNew(4, sizeof(RingBufferManageQueue_t), NULL);
   
   /* RS485 in receiver mode only*/
   RS485_TRANSMIT_DISABLE();
   Hal_Status = HAL_UART_Receive_IT(RS485_INSTANCE, gRxBuffer, 1U);
   configASSERT(Hal_Status == HAL_OK);
   gDurringReceiveTime = 1;
}

/**
 * @brief  This function uses to transmit data via RS486 gate
 *
 * @param[in]  RS485_Instance    : RS485 Instance
 * @param[in]  pData             : data pointer
 * @param[in]  length            : data length
 *
 * @retval Std_Return_Type
 *         E_OK     : If successful
 *         E_NOT_OK : Otherwise
 *
 */
Std_Return_Type RS485_Transmit(uint8_t* pData_RS485, uint32_t length)
{
    while(gDurringTransmitTime);
    Std_Return_Type eRetValue = E_NOT_OK;

    //TO DO
    RS485_TRANSMIT_ENABLE();
#if(STD_ON == RS485_FRAME_LOOPBACK_ENABLED)
    gLoopBackStatus = 1;
#endif
    HAL_UART_Transmit_IT(RS485_INSTANCE,pData_RS485,length);
    gDurringTransmitTime = 1;

    return eRetValue;
}
/**
 * @brief  This function uses to transmit data via RS486 gate
 *
 * @param[in]  RS485_Instance    : RS485 Instance
 * @param[in]  pData             : data pointer
 * @param[in]  length            : pointer points to length of data received
 * @param[in]  timeout           : timeout for waiting
 *
 * @retval Std_Return_Type
 *         E_OK     : If successful
 *         E_NOT_OK : Otherwise
 *
 */
Std_Return_Type RS485_Wait_For_Message(uint8_t* pData, uint32_t* length, uint32_t timeout)
{
    osStatus_t status;
    Std_Return_Type eRetValue = E_NOT_OK;
    RingBufferManageQueue_t RxMassage;
    status = osMessageQueueGet(RS485_RxMsgInforQueue_ID, &RxMassage, NULL, timeout);   // wait for message
    if(osOK == status)
    {
        if(RxMassage.tail < RxMassage.head)
        {
            *length = RxMassage.head - RxMassage.tail;
            memcpy(pData, &gRxBuffer[RxMassage.tail],*length);
        }else
        {
            *length = RS485_BUFF_SIZE - RxMassage.tail;
            /* Copy data from Tail to the end of buffer */
            memcpy(pData, &gRxBuffer[RxMassage.tail],*length);
            /* Copy the rest of data */
            memcpy(&pData[*length], &gRxBuffer,RxMassage.head);
            *length += RxMassage.head;
        }
        gRxBufferInfo.tail = RxMassage.head;
        gRxBufferInfo.n_elem = gRxBufferInfo.n_elem - *length;
        eRetValue = E_OK;
    }
    if(gDurringReceiveTime == 0)
    {
        gDurringReceiveTime = 1;
        HAL_UART_Receive_IT(RS485_INSTANCE, &gRxBuffer[gRxBufferInfo.head], 1U);
    }
    return eRetValue;
}

/*==================================================================================================
*                                         HANDLER FUNCTIONS
==================================================================================================*/

#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)

void RS485_SwTimer_Callback(void *arg)
{
    RingBufferManageQueue_t temp;
#if(STD_ON == RS485_FRAME_LOOPBACK_ENABLED)
	if(1U == gLoopBackStatus)
	{
		/* This frame is loopback frame. It will be discarded. */
        Framelength              = 0;
        gLoopBackStatus          = 0;
        gRxBufferInfo.head       = gRxBufferPreviousHead;
        gDurringTransmitTime     = 0;
		/* Release the DE pin to disable transmiter block*/
        RS485_TRANSMIT_DISABLE();
	}
	else
#endif
	{
        /*End of receiving and push rx frame info into queue */
        gRxBufferInfo.n_elem    += Framelength;
        Framelength              = 0;
        temp.head                = gRxBufferInfo.head;
        temp.tail                = gRxBufferPreviousHead;
		osMessageQueuePut(RS485_RxMsgInforQueue_ID, (void*)&temp, 0U, 0U);
        gRxBufferPreviousHead    = gRxBufferInfo.head;
	}
}

#endif

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    
    //RS485_TRANSMIT_DISABLE();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == RS485_INSTANCE)
    {
#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)
			/* Reset Software timer */
			TM_DELAY_TimerStart(RS485_Sw_Timer);
#else
        //TO DO
        if(STX == huart->pRxBuffPtr[gRxBufferInfo.head])
        {
            gFrameReceiving = 1;
        }else if(ETX == huart->pRxBuffPtr[gRxBufferInfo.head])
        {
            gFrameReceiving = 0;
            /*End of receiving and push rx frame info into queue */
            osMessageQueuePut(RS485_RxMsgInforQueue_ID, (void*)&gRxBufferInfo, 0U, 0U);
        }
#endif
        // Increase data index of receiving ring buffer
        gRxBufferInfo.head++;
        gRxBufferInfo.head &= (RS485_BUFF_SIZE-1);
        if((gRxBufferInfo.head == gRxBufferInfo.tail) && (gRxBufferInfo.n_elem == RS485_BUFF_SIZE))
        {
            //queue full turn of receive massage and wait until buffer have space
            gDurringReceiveTime = 0; 
        }else
        {
            Framelength++;
            /* Continue to receive next byte */            
          HAL_UART_Receive_IT(RS485_INSTANCE, &gRxBuffer[gRxBufferInfo.head], 1U);
        }
    }
}

