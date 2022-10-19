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

#define COMMAND_BUFFER_SIZE                 128U
#define RECEIVE_BUFFER_SIZE                 128U
#define RECEIVE_BUFFER_NUMBER               2U
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct{
    uint8_t  *buff;
    uint16_t size;
    uint16_t tail;
    uint16_t head;
    uint16_t n_elem;
    osMessageQueueId_t mq_id;
}RingBufferManage_t;

typedef struct{
    uint8_t head;
    uint8_t tail;
    uint8_t OpCode;
}Cmd_Queue_Obj_t;
/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
/********************************** Rtos variables ****************************************/
/* Queue */
osMessageQueueId_t      Rs485_ReceiveQueue_ID,
                        Rs485_TransmitQueue_ID;

/*pool data*/
osMemoryPoolId_t PollingDataPool_ID;

/********************************** Ring buffer to manage Rs485 transmit/receive ****************************************/
uint8_t RxBuffer[COMMAND_BUFFER_SIZE] = {0};
uint8_t TxBuffer[COMMAND_BUFFER_SIZE] = {0};

volatile RingBufferManage_t RxBufferInfor =
{
    .buff = RxBuffer,
    .size = COMMAND_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
    .n_elem = 0
};

volatile RingBufferManage_t TxBufferInfor =
{
    .buff = TxBuffer,
    .size = COMMAND_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
    .n_elem = 0
};
/********************************** other variables ****************************************/
#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)
    /* Software Timer for detecting Frame */
    static TM_DELAY_Timer_t* RS485_Sw_Timer = NULL;
#endif /* RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON */


volatile uint8_t gDurringTransmitTime = 0;
volatile uint8_t gLoopBackStatus = 0;

volatile uint8_t gTxDataLength = 0;

uint8_t gRs485_Address=0;

volatile uint8_t gRxData = 0;
volatile uint8_t gRxDataCount[RECEIVE_BUFFER_NUMBER] = {0};
volatile uint8_t gRxDataBuffer[RECEIVE_BUFFER_NUMBER][RECEIVE_BUFFER_SIZE]={0,0};
volatile uint8_t gRxBuffIndex = 0;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

void RS485_SwTimer_Callback(void *arg);

static uint16_t Framing_data(uint8_t *Frame ,uint8_t *pData, uint16_t length ,uint8_t opCode);
static Std_Return_Type RS485_Transmit(uint8_t* pData_RS485, uint32_t length);


Std_Return_Type PutCommandToBuffer(RingBufferManage_t *RingBuffer, uint8_t OpCode ,uint8_t *pData ,uint16_t length);
Std_Return_Type GetCommandFromBuffer(RingBufferManage_t *RingBuffer, uint8_t *OpCode,uint8_t *pData,uint16_t *length, uint32_t timeout);

/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
static uint8_t CheckSum(uint8_t* pData, uint8_t Length)
{
    uint8_t CheckSumResult = 0;
    uint8_t i=0;
    for(i=0;i<Length;i++)
    {
        CheckSumResult ^= pData[i];
    }
    return CheckSumResult;
}

static uint16_t Framing_data(uint8_t *Frame ,uint8_t *pData, uint16_t length ,uint8_t opCode)
{
    uint8_t i = 0;
    uint8_t FrameLength = 0;

    if((pData == NULL) || (length == 0))
    {
        Frame[0] = opCode;
        FrameLength = 1;
    }else
    {
        Frame[0] = 0x02;
        Frame[1] = gRs485_Address;
        Frame[2] = opCode;
        Frame[3] = 0x80 | length;
        for(i=0;i<length;i++)
        {
            Frame[4+i] = pData[i];
        }
        Frame[4+length] = 0x03;
        Frame[5+length] = CheckSum(&Frame[1],4 + length);
        FrameLength = 6 + length;
    }
    return FrameLength;
}


Std_Return_Type PutCommandToBuffer(RingBufferManage_t *RingBuffer, uint8_t OpCode ,uint8_t *pData ,uint16_t length)
{
    uint8_t i;
    Cmd_Queue_Obj_t temp;
    osStatus_t status = osOK;
    temp.tail = RingBuffer->head;
    for(i=0; i<length; i++)
    {
        if((RingBuffer->head == RingBuffer->tail) && (RingBuffer->n_elem == RingBuffer->size))
        {
            // buffer full and return to backup head
            RingBuffer->head = temp.tail;
            return E_NOT_OK;
        }else
        {
            RingBuffer->buff[RingBuffer->head] = pData[i];
            RingBuffer->head++;
            RingBuffer->head &= (COMMAND_BUFFER_SIZE - 1);
        }
    }
    /* Mutex to avoid resource collision*/
    temp.head = RingBuffer->head;
    temp.OpCode = OpCode;
    status = osMessageQueuePut(RingBuffer->mq_id, (void*)&temp, 0U, 0U);
    if(status != osOK)
    {
        RingBuffer->head = temp.tail;
        return E_NOT_OK;
    }
    RingBuffer->n_elem = RingBuffer->n_elem + length;
    return E_OK;
}

Std_Return_Type GetCommandFromBuffer(RingBufferManage_t *RingBuffer, uint8_t *OpCode,uint8_t *pData ,uint16_t *length, uint32_t timeout)
{
    osStatus_t CmdStatus;
    Std_Return_Type RetValue = E_NOT_OK;
    Cmd_Queue_Obj_t GetCommandDataInfo;
    *length = 0;
    CmdStatus = osMessageQueueGet(RingBuffer->mq_id, &GetCommandDataInfo, NULL, timeout);
    if(osOK == CmdStatus)
    {
        if(GetCommandDataInfo.tail < GetCommandDataInfo.head)
        {
            *length = GetCommandDataInfo.head - GetCommandDataInfo.tail;
            memcpy(pData, &(RingBuffer->buff[GetCommandDataInfo.tail]),*length);
        }else if(GetCommandDataInfo.tail > GetCommandDataInfo.head)
        {
            *length = COMMAND_BUFFER_SIZE - GetCommandDataInfo.tail;
            /* Copy data from Tail to the end of buffer */
            memcpy(pData, &(RingBuffer->buff[GetCommandDataInfo.tail]),*length);
            if(0 != GetCommandDataInfo.head)
            {
                /* Copy the rest of data */
                memcpy(&pData[*length], &(RingBuffer->buff[0]),GetCommandDataInfo.head);
                *length += GetCommandDataInfo.head;
            }
        }
        RingBuffer->tail = GetCommandDataInfo.head;
        RingBuffer->n_elem = RingBuffer->n_elem - *length;
        *OpCode = GetCommandDataInfo.OpCode;
        RetValue = E_OK;
    }
    return RetValue;
}

static Std_Return_Type RS485_Transmit(uint8_t* pData_RS485, uint32_t length)
{
    while(gDurringTransmitTime);
    Std_Return_Type eRetValue = E_NOT_OK;

    //TO DO
    RS485_TRANSMIT_ENABLE();
#if(STD_ON == RS485_FRAME_LOOPBACK_ENABLED)
    gLoopBackStatus = 1;
    gTxDataLength = length;
#endif
    HAL_UART_Transmit_IT(RS485_INSTANCE,pData_RS485,length);
    gDurringTransmitTime = 1;
    return eRetValue;
}
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
void RS485_Init(uint8_t DeviceAddress)
{
   HAL_StatusTypeDef Hal_Status;

#if(STD_ON == RS485_USING_TIMER_TO_DETECT_FRAME)
/* 
* Modbus-RTU Baud 38400 uses 3.5 characters:
* 3.5 characters ~ 1ms
*/
   TM_DELAY_Init();
   RS485_Sw_Timer = TM_DELAY_TimerCreate(RS485_TIME_INTERVAL * RS485_SW_TIMER_PERIOD_PER_MILISECOND,
                                         SOFTWARE_TIMER_AUTO_RELOAD_DISABLE,
                                         SOFTWARE_TIMER_START_TIMER_DISABLE,
                                         RS485_SwTimer_Callback,
                                         NULL);
#endif
    
   Rs485_ReceiveQueue_ID    = osMessageQueueNew(2, sizeof(Cmd_Queue_Obj_t), NULL);
   Rs485_TransmitQueue_ID   = osMessageQueueNew(2, sizeof(Cmd_Queue_Obj_t), NULL);

   RxBufferInfor.mq_id = Rs485_ReceiveQueue_ID;
   TxBufferInfor.mq_id = Rs485_TransmitQueue_ID;
   
   /*Create pool data*/
   PollingDataPool_ID = osMemoryPoolNew(1, 128, NULL);
   
   /*Store device address to use later */
   gRs485_Address = DeviceAddress;
   /* RS485 in receiver mode only*/
   RS485_TRANSMIT_DISABLE();
   
   /*Start uart receive interrupt */
   Hal_Status = HAL_UART_Receive_IT(RS485_INSTANCE, (uint8_t*)&gRxData, 1U);
   configASSERT(Hal_Status == HAL_OK);
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
Std_Return_Type RS485_Prepare_To_Transmit(uint8_t OpCode ,uint8_t *pData ,uint16_t length)
{
    Std_Return_Type eRetValue;
    
    eRetValue = PutCommandToBuffer((RingBufferManage_t*)&TxBufferInfor,OpCode,pData,length);
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
Std_Return_Type RS485_Wait_For_Message(uint8_t* Opcode, uint8_t* pData, uint16_t* length, uint32_t timeout)
{
    Std_Return_Type eRetValue = E_NOT_OK;
    
    eRetValue = GetCommandFromBuffer((RingBufferManage_t*)&RxBufferInfor,Opcode,(uint8_t*)pData,length,timeout);  // wait for message
    
    return eRetValue;
}


/*==================================================================================================
*                                         TASKS HANDLE FUNCTIONS
==================================================================================================*/

#define RS485_IDLE_FLAG         0
#define RS485_SELLECT_FLAG      1
#define RS485_ACK_FLAG          2
#define RS485_NACK_FLAG         3
#define RS485_POLLING_FLAG      4


#define RS485_IDLE_STATE                    0
#define RS485_POLLING_STATE                 1
#define RS485_RECEPTION_STATE               2
#define RS485_TRANSMITION_STATE             3

/*==================================================================================================
*                                         HANDLER FUNCTIONS
==================================================================================================*/
const uint8_t ack_Frame = ACK;
const uint8_t nack_Frame= NACK;
const uint8_t eot_Frame = EOT;
volatile uint8_t Proccess_State = RS485_IDLE_STATE;
volatile uint8_t DataFrame[128] = {0};
volatile uint16_t FrameLength = 0;


#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)

void RS485_SwTimer_Callback(void *arg)
{
    uint8_t OpCode;
    uint16_t Cmdlength;
    uint8_t *pCmd;
    uint8_t CheckSumResult = 0;
    uint8_t CurRxBuffIndex = gRxBuffIndex;
    
    uint32_t flag = RS485_IDLE_FLAG;
    
#if(STD_ON == RS485_FRAME_LOOPBACK_ENABLED)
	if(1U == gLoopBackStatus)
	{
        /* This frame is loopback frame. It will be discarded. */
        gLoopBackStatus          = 0;
        gDurringTransmitTime     = 0;
        /* Release the DE pin to disable transmiter block*/
        RS485_TRANSMIT_DISABLE();
        PutCommandToBuffer((RingBufferManage_t*)&RxBufferInfor, 0xfc,0,0);
	}
	else
#endif
	{ 
        /*switch to next buffer to receive data from ISR*/
        gRxBuffIndex++;
        if(gRxBuffIndex == RECEIVE_BUFFER_NUMBER)
        {
            gRxBuffIndex = 0;
        }
        /*Small state machine to check valid massage*/
        if (gRxDataBuffer[CurRxBuffIndex][0] == ACK)
        {
            /* ACK massage*/
            if(gRxDataCount[CurRxBuffIndex] == 1)
                flag = RS485_ACK_FLAG;
        }else if(gRxDataBuffer[CurRxBuffIndex][0] == NACK)
        {
            /* NACK massage*/
            if(gRxDataCount[CurRxBuffIndex] == 1)
                flag = RS485_NACK_FLAG;
        }else if(gRxDataBuffer[CurRxBuffIndex][0] == EOT)
        {
            if(gRxDataCount[CurRxBuffIndex] == 3)
            {
                /* Polling masssage*/
                /* EOT(1B) | SA(1B) | POL(1B) */
                /* SA(1B) : slave address check valid*/
                if(gRxDataBuffer[CurRxBuffIndex][1] == gRs485_Address)
                {
                    /* POL(1B) : Polling character check valid*/
                    if(gRxDataBuffer[CurRxBuffIndex][2] == POL)
                    {
                        Proccess_State = RS485_POLLING_STATE;
                        flag = RS485_POLLING_FLAG;
                    }else
                    {
                        PutCommandToBuffer((RingBufferManage_t*)&RxBufferInfor, 0xff,0,0);
                        RS485_Transmit((uint8_t*)&nack_Frame,1);
                    }
                }
            }else if(gRxDataCount[CurRxBuffIndex] > 3)
            {
                /* Sellecting masssage*/
                /* EOT(1B) | STX(1B) | SA(1B) | OP(1B) | Data(nB) | ETX(1B) | BCC(1B) */
                if(gRxDataBuffer[CurRxBuffIndex][1] == STX)
                {
                    /* Check valid command slave address and checksum*/
                    /* SA(1B) : slave address check valid*/
                    if(gRxDataBuffer[CurRxBuffIndex][2] == gRs485_Address)
                    {
                        /* BCC(1B) : check sum valid*/
                        CheckSumResult = CheckSum((uint8_t*)&gRxDataBuffer[CurRxBuffIndex][2],gRxDataCount[CurRxBuffIndex] - 3);
                        if(gRxDataBuffer[CurRxBuffIndex][gRxDataCount[CurRxBuffIndex] - 1] == CheckSumResult)
                        {
                           PutCommandToBuffer((RingBufferManage_t*)&RxBufferInfor, gRxDataBuffer[CurRxBuffIndex][3],(uint8_t*)&gRxDataBuffer[CurRxBuffIndex][4],gRxDataCount[CurRxBuffIndex] - 6 );
                           RS485_Transmit((uint8_t*)&ack_Frame,1);
                        }else
                        {
                           PutCommandToBuffer((RingBufferManage_t*)&RxBufferInfor, 0xff,0,0);
                           RS485_Transmit((uint8_t*)&nack_Frame,1);
                        }
                    }
                }
            }
        }else
        {
            PutCommandToBuffer((RingBufferManage_t*)&RxBufferInfor, 0xfe,0,0);
        }
        if(RS485_POLLING_STATE == Proccess_State)
        {
            if((RS485_POLLING_FLAG == flag) || (RS485_ACK_FLAG == flag))
            {
                pCmd      = (uint8_t*)osMemoryPoolAlloc(PollingDataPool_ID,0U);
                if(E_OK == GetCommandFromBuffer((RingBufferManage_t*)&TxBufferInfor,&OpCode,pCmd,&Cmdlength,0))
                {
                    FrameLength = Framing_data((uint8_t*)DataFrame,(uint8_t*)pCmd,Cmdlength,OpCode);
                    RS485_Transmit((uint8_t*)DataFrame,FrameLength);
                    osMemoryPoolFree(PollingDataPool_ID,pCmd);
                }else
                {
                    Proccess_State = RS485_IDLE_STATE;
                    RS485_Transmit((uint8_t*)&eot_Frame,1);
                }
            }else if(RS485_NACK_FLAG == flag)
            {
                RS485_Transmit((uint8_t*)DataFrame,FrameLength);
            }
        }
        gRxDataCount[CurRxBuffIndex] = 0;
        
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

#if(STD_ON == RS485_FRAME_LOOPBACK_ENABLED)
        if(1U == gLoopBackStatus)
        {
            /* This frame is loopback frame. It will be discarded. */
            gRxDataCount[gRxBuffIndex]++;
            if(gRxDataCount[gRxBuffIndex] == gTxDataLength)
            {
                gRxDataCount[gRxBuffIndex] = 0;
#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)
                /* Reset Software timer */
                TM_DELAY_TimerStart(RS485_Sw_Timer);
#endif
            }
        }
        else
#endif
        {
#if(RS485_USING_TIMER_TO_DETECT_FRAME == STD_ON)
            /* Reset Software timer */
            TM_DELAY_TimerStart(RS485_Sw_Timer);
#endif
            gRxDataBuffer[gRxBuffIndex][gRxDataCount[gRxBuffIndex]] = gRxData;
            gRxDataCount[gRxBuffIndex]++;
        }
        HAL_UART_Receive_IT(RS485_INSTANCE, (uint8_t*)&gRxData, 1U);
    }
}

