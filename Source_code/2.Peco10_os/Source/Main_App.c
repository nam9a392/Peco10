/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Main_App.h"

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
  uint8_t msg[16];
  uint8_t line;
  uint8_t offset;
} MsgForDisplay_t;

typedef struct{
    uint8_t Address;
    uint8_t OpCode;
    uint8_t DataLength;
    uint8_t *pData;
}command_t;

//typedef struct{
//    Keypad_Button_Type InputKey;
//    uint8_t KeyInputComplete;
//}KeypadInputStatus_t;
/*Aplication states*/
/**
 * @brief @SYSTEMSTATE : Current state of preset
 */
#define SYSTEM_PRESET                      ((uint8_t)1U)
#define SYSTEM_SETTING                     ((uint8_t)2U)
#define PRINTER_SETTING                    ((uint8_t)3U)
#define DISPLAY_SELF_TEST                  ((uint8_t)4U)

/**
 * @brief @SYSTEMSUBSTATE : Current Substate of preset
 */
#define LOGIN_STATE                        ((uint8_t)0x00U)
#define TOTALIZER_READING_STATE            ((uint8_t)0x01U)
#define USER_REGISTRATION_STATE            ((uint8_t)0x02U)
#define MAINTENANCE_STATE                  ((uint8_t)0x03U)
#define OIL_COMPANY_STATE                  ((uint8_t)0x04U)
#define UNKNOWN_STATE                      ((uint8_t)0x05U)
/* State for printer setting */
#define PRINTER_SETTING_STATE              ((uint8_t)0x01U)

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/* Event flag to start display */
#define LCD_SEGMENT_START_DISPLAY_FLAG        (0x00000001U)
#define LCD_CHARACTER_START_DISPLAY_FLAG      (0x00000002U)

/*Queues Object define*/
#define LCDCHARACTER_MSGQUEUE_OBJ_t            MsgForDisplay_t
#define LCDCHARACTER_MSGQUEUE_OBJECTS          2U

#define KEYPAD_QUEUE_OBJ_t                     Keypad_Button_Type
#define KEYPAD_QUEUE_OBJECTS                   20

/*Pool memory data define*/
#define NUMBER_OF_MEMORY_POOL_ELEMENT       3U
#define MEMORY_POOL_ELEMENT_SIZE            50U

#define RECEIVE_BUFFER_SIZE                 128U

/* Special keypad button that may push multiple times */
#define KEY_NEED_CHECK(x)      ((x == KEYPAD_NUMBER_0) || (x == KEYPAD_NUMBER_1) || \
                               (x == KEYPAD_NUMBER_2) || (x == KEYPAD_NUMBER_3) || \
                               (x == KEYPAD_NUMBER_4) || (x == KEYPAD_NUMBER_5) || \
                               (x == KEYPAD_NUMBER_6) || (x == KEYPAD_NUMBER_7) || \
                               (x == KEYPAD_NUMBER_8) || (x == KEYPAD_NUMBER_9) || \
                               (x == KEYPAD_ENTER) || (x == KEYPAD_CLEAR))
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/


/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
/****************************** Rtos Variables **********************************/
/* Threads ID*/
osThreadId_t LcdSegmentDisplay_Task_Id,
             ScanKeypad_DisplayLcdCharacter_Task_Id,
             CommandHandler_Task_Id,         // thread id
             Main_Preset_Task_Id;
/* Queues ID*/
osMessageQueueId_t LcdCharacterMsgQueue_Id,
                   KeypadInputQueue_Id,
                   PollingDataInforQueue_ID;

/*pool data*/
osMemoryPoolId_t PollingDataFrameMemoryPoolId;

/*Timer ID*/
osTimerId_t Timer1_Id;                            // timer id
osTimerId_t Timer2_Id;                            // timer id

/*Define attributes for each thread*/
const osThreadAttr_t LcdSegmentDisplay_Task_Attr = {
    .stack_size = 256U
};

const osThreadAttr_t ScanKeypad_DisplayLcdCharacter_Task_Attr = {
    .stack_size = 256U
};

const osThreadAttr_t CommandHandler_Task_Attr = {
    .stack_size = 384U
};

const osThreadAttr_t Main_Preset_Task_Attr = {
    .stack_size = 512U
};


/* Event flag to trigger LCD start displaying */
static osEventFlagsId_t LcdSegmentStartDisplayFlagId;

/****************************** Other Variables **********************************/
/* main and sub Address of this preset */
static uint8_t gMainAddress,gSubAddress;
//SubAddress of corresponding preset device
static uint8_t gSubAddress_Coressponding;

/* Current State of Preset*/
uint8_t gCurrentState = SYSTEM_PRESET;
uint8_t gCurrentSubState = UNKNOWN_STATE;
//uint8_t gCurrentState = PRINTER_SETTING;

/* Variable to handle scanning Keypad*/
volatile Std_Return_Type KeypadScaningStatus = E_OK;
/* Variable to check the same input button push multiple times */
volatile Std_Return_Type gInputDone = E_NOT_OK;
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint32_t timeout);
static void LcdSegmentDisplaySetEvent(void);

static void LcdSegmentDisplay_Task(void *argument);
static void ScanKeypad_DisplayLcdCharacter_Task(void *argument);
static void CommandHandler_Task(void *argument);
static void Main_Preset_Task(void *argument);

static void HexDataToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString);
static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData);
static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData);
static uint8_t SpecialKeyCheck(Keypad_Button_Type *PreviousKey,Keypad_Button_Type CurrentKey, uint8_t *PushTime);
static uint8_t CheckSum(uint8_t* pData, uint8_t Length);

Std_Return_Type extract_command(command_t *pCmd, uint8_t *NeedToRespond, uint32_t timeout);
static uint8_t Framing_data(uint8_t *pBuffer ,uint8_t *pData, uint8_t length ,uint8_t opCode );

void Timer1_Callback (void *arg);
void Timer2_Callback (void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
/***************************** logic function *************************************/
static void HexDataToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString)
{
    uint8_t i=0;
    for(i=0;i<DataLength;i++)
    {
        sprintf((char*)pString,"%d%d",(pData[i]>>4),(pData[i]&0xf));
    }
}

static uint8_t CheckSum(uint8_t* pData, uint8_t Length)
{
    uint8_t CheckSumResult = 0;
    uint8_t i=0;
    for(i=0;i<Length;i++)
    {
        CheckSumResult ^= pData[i];
    }
    //CheckSumResult ^= 0xFF;
    return CheckSumResult;
}

/***************************** Ring buffer fuction to handle process command *************************************/

#define COMMAND_BUFFER_SIZE     128
uint8_t CommandBuffer[COMMAND_BUFFER_SIZE] = {0};
typedef struct{
    uint8_t head;
    uint8_t tail;
    uint8_t OpCode;
}Cmd_Queue_Obj_t;


volatile RingBufferManageQueue_t CurrCommandBuffInfor =
{
    .head = 0,
    .tail = 0,
    .n_elem = 0
};


Std_Return_Type PutCommandToBuffer(uint8_t OpCode ,uint8_t *pData ,uint8_t length);
Std_Return_Type GetCommandFromBuffer(uint8_t *OpCode,uint8_t *pData,uint8_t *length);

Std_Return_Type PutCommandToBuffer(uint8_t OpCode ,uint8_t *pData ,uint8_t length)
{
    uint8_t i;
    Cmd_Queue_Obj_t temp;
    osStatus_t status = osOK;
    temp.tail = CurrCommandBuffInfor.head;
    for(i=0; i<length; i++)
    {
        if((CurrCommandBuffInfor.head == CurrCommandBuffInfor.tail) && (CurrCommandBuffInfor.n_elem == COMMAND_BUFFER_SIZE))
        {
            // buffer full and return to backup head
            CurrCommandBuffInfor.head = temp.tail;
            return E_NOT_OK;
        }else
        {
            CommandBuffer[CurrCommandBuffInfor.head] = pData[i];
            CurrCommandBuffInfor.head++;
            CurrCommandBuffInfor.head &= (COMMAND_BUFFER_SIZE - 1);
        }
    }
    /* Mutex to avoid resource collision*/
    temp.head = CurrCommandBuffInfor.head;
    temp.OpCode = OpCode;
    status = osMessageQueuePut(PollingDataInforQueue_ID, (void*)&temp, 0U, 0U);
    if(status != osOK)
    {
        CurrCommandBuffInfor.head = temp.tail;
        return E_NOT_OK;
    }
    CurrCommandBuffInfor.n_elem = CurrCommandBuffInfor.n_elem + length;
    return E_OK;
}

Std_Return_Type GetCommandFromBuffer(uint8_t *OpCode,uint8_t *pData ,uint8_t *length)
{
    osStatus_t CmdStatus;
    Std_Return_Type RetValue = E_NOT_OK;
    Cmd_Queue_Obj_t GetCommandDataInfo;
    CmdStatus = osMessageQueueGet(PollingDataInforQueue_ID, &GetCommandDataInfo, NULL, 0);
    if(osOK == CmdStatus)
    {
        if(GetCommandDataInfo.tail < GetCommandDataInfo.head)
        {
            *length = GetCommandDataInfo.head - GetCommandDataInfo.tail;
            memcpy(pData, &CommandBuffer[GetCommandDataInfo.tail],*length);
        }else
        {
            *length = COMMAND_BUFFER_SIZE - GetCommandDataInfo.tail;
            /* Copy data from Tail to the end of buffer */
            memcpy(pData, &CommandBuffer[GetCommandDataInfo.tail],*length);
            if(0 != GetCommandDataInfo.head)
            {
                /* Copy the rest of data */
                memcpy(&pData[*length], &CommandBuffer[0],GetCommandDataInfo.head);
                *length += GetCommandDataInfo.head;
            }
        }
        CurrCommandBuffInfor.tail = GetCommandDataInfo.head;
        CurrCommandBuffInfor.n_elem = CurrCommandBuffInfor.n_elem - *length;
        *OpCode = GetCommandDataInfo.OpCode;
        RetValue = E_OK;
    }
    return RetValue;
}
/***************************** Function to handle commands *************************************/
Std_Return_Type extract_command(command_t *pCmd, uint8_t *NeedToRespond, uint32_t timeout)
{
    Std_Return_Type retValue = E_NOT_OK;
    uint32_t length;
    uint8_t Rxbuffer[RECEIVE_BUFFER_SIZE];
    *NeedToRespond = 0;
    
    /*waiting for new command*/
 	RS485_Wait_For_Message(Rxbuffer,&length,timeout);
    
    // ACK frame or POL frame
    // Start Delimiter(1Byte) | Slave address(1Byte) | 05 or 06 (1Byte)  | End delimiter(1Byte)
    if((uint8_t)0x1 == length)
    {
        pCmd->Address = 0xff;
        pCmd->OpCode = Rxbuffer[0];
        pCmd->DataLength = 0;
        retValue = E_OK;
        *NeedToRespond = 1;
    }else if((uint8_t)0x3 == length)
    {
        /* Check valid command slave address and checksum*/
        if(0x4 == Rxbuffer[0])
        {
            if((gMainAddress == Rxbuffer[1]) || (gSubAddress == Rxbuffer[1]) || (gSubAddress_Coressponding == Rxbuffer[1]))
            {
                pCmd->Address = Rxbuffer[1];
                pCmd->OpCode = Rxbuffer[2];
                pCmd->DataLength = 0;
                retValue = E_OK;
                *NeedToRespond = 1;
                if(gSubAddress_Coressponding == Rxbuffer[1])
                    *NeedToRespond = 0;
            }
        }
    }else if((uint8_t)0x3 < length)
    {
        if(0x4 == Rxbuffer[0])
        {
            if((uint8_t)0x02 == Rxbuffer[1])
            {
                // SELLECT frame
                // Start Delimiter(1Byte) | 0x02(1Byte) | Slave address(1Byte) | Data(n Byte) | End delimiter(1Byte) | CheckSum(1Byte)
                
                if((gMainAddress == Rxbuffer[2]) || (gSubAddress == Rxbuffer[2]) || (gSubAddress_Coressponding == Rxbuffer[2]))
                {
                    if(Rxbuffer[length - 1] == CheckSum(&Rxbuffer[1],length - 2))
                    {
                        pCmd->Address = Rxbuffer[2];
                        pCmd->OpCode = Rxbuffer[3];
                        pCmd->DataLength = length - 5;
                        memcpy(pCmd->pData,Rxbuffer,length);
                        retValue = E_OK;
                        *NeedToRespond = 1;
                        if(gSubAddress_Coressponding == Rxbuffer[1])
                            *NeedToRespond = 0;
                    }else
                    {
                        *NeedToRespond = 1;
                    }
                }
            }
        }
    }
	return retValue;
}


static uint8_t Framing_data(uint8_t *Frame ,uint8_t *pData, uint8_t length ,uint8_t opCode)
{
    uint8_t i = 0;
    uint8_t FrameLength = 0;
//TO DO
    if((pData == NULL) || (length == 0))
    {
        Frame[0] = opCode;
        FrameLength = 1;
    }else
    {
        Frame[0] = 0x02;
        Frame[1] = gMainAddress;
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

/***************************** Function to handle Display component *************************************/
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint32_t timeout)
{
    MsgForDisplay_t pMsgQueue;

    /*prepare queue data*/
    pMsgQueue.line=line;
    pMsgQueue.offset=offset;
    strcpy((char*)pMsgQueue.msg,(char*)pString);
    
    /* push data to lcd character massage queue*/
    osMessageQueuePut(LcdCharacterMsgQueue_Id, &pMsgQueue, 0U, timeout);
}

static void LcdSegmentDisplaySetEvent(void)
{
    osEventFlagsSet(LcdSegmentStartDisplayFlagId, LCD_SEGMENT_START_DISPLAY_FLAG);
}

static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData)
{
    uint8_t LcdSegmentBuffer[8];
    switch(opcode) 
    {
        /*Control Byte : Zero-supress | Sign(dot or comma) */
        case 0x90:
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) |Ctr1(2B) | Row 3(4B) */
            HexDataToString(&pData[2],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,1,0);
            HexDataToString(&pData[8],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,2,0);
            HexDataToString(&pData[14],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,2,0);
            break;
        case 0x91:
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) */
            HexDataToString(&pData[2],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,1,0);
            HexDataToString(&pData[8],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,2,0);
            break;
        case 0x92:
            /* Ctr1(2B) | Row 3(4B) */
            HexDataToString(&pData[2],4,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,3,0);
            break;
    }
    LcdSegmentDisplaySetEvent();
}

static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData)
{
    uint8_t LcdCharacterBuffer[17];
    uint8_t LenthOfRow1 = 0;
    uint8_t LenthOfRow2 = 0;
    switch(opcode) 
    {
        /*Control Byte : Unknown | length of data */
        case 0x90:
            /* Ctr1(2Byte) | Row 1(n Byte) |Ctr1(2Byte) | Row 2(n Byte) */
            // Row 1 data from position 2 length equal to pData[1]
            LenthOfRow1 = pData[1];
            HexDataToString(&pData[2],LenthOfRow1,LcdCharacterBuffer);
            LcdCharacter_PutString(0,0,LcdCharacterBuffer,0);
            // Row 2 data from position 2 + pData[1] + 2 length equal to pData[1]
            LenthOfRow2 = pData[2 + LenthOfRow1 + 2];
            HexDataToString(&pData[2 + LenthOfRow1 + 3],LenthOfRow2,LcdCharacterBuffer);
            LcdCharacter_PutString(1,0,LcdCharacterBuffer,0);
            break;
        case 0x91:
            /* Ctr1(2Byte) | Row 1(n Byte)*/
            // Row 1 data from position 2 length equal to pData[1]
            LenthOfRow1 = pData[1];
            HexDataToString(&pData[2],LenthOfRow1,LcdCharacterBuffer);
            LcdCharacter_PutString(0,0,LcdCharacterBuffer,0);
            break;
        case 0x92:
            /* Ctr1(2Byte) | Row 2(n Byte) */
            // Row 1 data from position 1 length equal to pData[1]
            LenthOfRow2 = pData[1];
            HexDataToString(&pData[2],LenthOfRow2,LcdCharacterBuffer);
            LcdCharacter_PutString(1,0,LcdCharacterBuffer,0);
            break;
    }
}

/***************************** Function to handle input data from keypad *************************************/
const uint8_t Password1[] = {KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_CLEAR,KEYPAD_ENTER,KEYPAD_CLEAR};
const uint8_t Password2[] = {KEYPAD_CLEAR,KEYPAD_CLEAR,KEYPAD_CLEAR};
const uint8_t Password3[] = {KEYPAD_ENTER,KEYPAD_CLEAR,KEYPAD_CLEAR,KEYPAD_ENTER,KEYPAD_CLEAR};
#define NO_PASSWORD_CHECK       0
#define FIRST_PASSWORD_CHECK    1
#define SECOND_PASSWORD_CHECK   2
#define THIRD_PASSWORD_CHECK    3

#define NORMAL_KEY              0
#define SPECIAL_KEY             1
#define PASS_CODE_1             2
#define PASS_CODE_2             3
#define PASS_CODE_3             4
uint8_t Password_check = NO_PASSWORD_CHECK;
static uint8_t SpecialKeyCheck(Keypad_Button_Type *PreviousKey,Keypad_Button_Type CurrentKey, uint8_t *PushTime)
{
    uint8_t RetValue = SPECIAL_KEY;
    if(CurrentKey == KEYPAD_FORWARD)
    {
        if(PRINTER_SETTING == gCurrentState)
            Lcd_Move_Cursor_Right();
    }else if(CurrentKey == KEYPAD_BACKWARD)
    {
        if(PRINTER_SETTING == gCurrentState)
            Lcd_Move_Cursor_Left();
    }else if(CurrentKey == KEYPAD_FONT)
    {
        if(PRINTER_SETTING == gCurrentState)
            Keypad_Change_Font();
    }else if(CurrentKey == KEYPAD_SPACE)
    {
        if(PRINTER_SETTING == gCurrentState)
            LcdCharacter_PutString(LCD_NEXT_POSITION,LCD_NEXT_POSITION,(uint8_t*)" ",0);
    }else if(CurrentKey == KEYPAD_CLEAR)
    {
        if(*PushTime == 0)
            Password_check = SECOND_PASSWORD_CHECK;
        if(PRINTER_SETTING == gCurrentState)
            Lcd_Clear_Character();
    }else if(CurrentKey == KEYPAD_ENTER)
    {
        if(*PushTime == 0)
            Password_check = FIRST_PASSWORD_CHECK;
        if(PRINTER_SETTING == gCurrentState)
            Lcd_Move_Cursor_Next_Line();
    }else if(CurrentKey == KEYPAD_UNKNOWN)
    {
        *PushTime = 0;
    }else if(CurrentKey == KEYPAD_LOSS)
    {
        //TO DO
    }else
    {
        if(*PushTime == 0)
            Password_check = THIRD_PASSWORD_CHECK;
        if(PRINTER_SETTING == gCurrentState)
        {
            if(*PreviousKey == CurrentKey)
            {
                *PushTime = *PushTime + 1;
            }else
            {
                *PushTime = 0;
                Lcd_Move_Cursor_Right();
            }
        }
        RetValue = NORMAL_KEY;
    }
    if(Password_check != NO_PASSWORD_CHECK)
    {
        if(Password_check == FIRST_PASSWORD_CHECK)
        {
            if(*PushTime < sizeof(Password1))
            {
                if(CurrentKey != Password1[*PushTime])
                    Password_check = NO_PASSWORD_CHECK;
                else
                {
                    if(*PushTime == (sizeof(Password1) - 1))
                    {
                        Password_check = NO_PASSWORD_CHECK;
                        RetValue = PASS_CODE_1;
                    }
                }
            }
        }else if(Password_check == SECOND_PASSWORD_CHECK)
        {
            if(*PushTime < sizeof(Password2))
            {
                if(CurrentKey != Password2[*PushTime])
                    Password_check = NO_PASSWORD_CHECK;
                else
                {
                    if(*PushTime == (sizeof(Password2) - 1))
                    {
                        Password_check = NO_PASSWORD_CHECK;
                        RetValue = PASS_CODE_2;
                    }
                }
            }
        }else if(Password_check == THIRD_PASSWORD_CHECK)
        {
            if(*PushTime < sizeof(Password3))
            {
                if(CurrentKey != Password3[*PushTime])
                    Password_check = NO_PASSWORD_CHECK;
                else
                {
                    if(*PushTime == (sizeof(Password3) - 1))
                    {
                        Password_check = NO_PASSWORD_CHECK;
                        RetValue = PASS_CODE_3;
                    }
                }
            }
        }
        *PushTime = *PushTime + 1;
    }
    *PreviousKey = CurrentKey;
    return RetValue;
}

static void Password_Check(Keypad_Button_Type CurrentKey, uint8_t *PushTime, uint8_t PassCheckingStatus)
{
    
}

/************************************ Task handler *******************************************/
static void LcdSegmentDisplay_Task(void *argument)
{
    //uint32_t flag;
    Lcd_Segment_Init();
    while(1)
    {
        osEventFlagsWait(LcdSegmentStartDisplayFlagId, LCD_SEGMENT_START_DISPLAY_FLAG, osFlagsWaitAny, osWaitForever);
        Lcd_Segment_Start_Display();
    }
}

static void ScanKeypad_DisplayLcdCharacter_Task(void *argument)
{
    osStatus_t LcdQueueStatus;
    MsgForDisplay_t data1;
    uint8_t KeyPushed = 0;
    Keypad_Button_Type CurrentStatus,PreviousStatus,BreakTimeStatus;
    BreakTimeStatus = PreviousStatus = CurrentStatus = KEYPAD_UNKNOWN;
    
    Lcd_Init_4bits_Mode();
    osTimerStart(Timer1_Id,KEYPAD_SCANNING_DURATION);
    
    while(1)
    {
        /* Lcd character print*/
        /* pop data from lcd character massage queue*/
        LcdQueueStatus = osMessageQueueGet(LcdCharacterMsgQueue_Id, &data1, NULL, 0U);   // wait for message
        if (LcdQueueStatus == osOK) {
            Lcd_Put_String(data1.line,data1.offset,data1.msg);
        }
        
        /* Keypad scan period 50ms */
        if(E_OK == KeypadScaningStatus)
        {
            KeypadScaningStatus = E_NOT_OK;
            CurrentStatus = Keypad_Scan_Position();
            if(CurrentStatus != PreviousStatus)
            {
                if(KEYPAD_LOSS == CurrentStatus)
                {
                }
                else if(KEYPAD_UNKNOWN != CurrentStatus)
                {
                    KeyPushed = 1;
                    if(KEY_NEED_CHECK(CurrentStatus))
                    {
                        osTimerStart(Timer2_Id,KEYPAD_CHECKING_DURATION);
                    }else{
                        osTimerStop(Timer2_Id);
                    }
                    /* Play Buzzer */
                    //Buzzer_Play();
                }
                PreviousStatus = CurrentStatus;
            }
        }
        if(E_OK == gInputDone)
        {
            gInputDone = E_NOT_OK;
            osMessageQueuePut(KeypadInputQueue_Id, &BreakTimeStatus, 0U, 0);
        }
        if(1 == KeyPushed)
        {
            KeyPushed = 0;
            osMessageQueuePut(KeypadInputQueue_Id, &CurrentStatus, 0U, 0);
        }
    }
}

static void CommandHandler_Task(void *argument)
{
    Std_Return_Type retValue = E_OK;
    
    uint8_t num[10]="";
    uint8_t False_num[10]="";
    uint16_t Msg_number = 0;
    uint16_t False_Msg_number = 0;
    
    uint8_t NeedToRespond = 1;
    Std_Return_Type TransmitSuccess = E_OK;
    uint8_t DurringPolling = 0;
    
    command_t cmd;
    uint8_t FrameLength,Cmdlength,OpCode;
    uint8_t *DataFrame;
    uint8_t *pCmd;
    
    RS485_Init();
    DataFrame = (uint8_t*)osMemoryPoolAlloc(PollingDataFrameMemoryPoolId,0U); 
    pCmd      = (uint8_t*)osMemoryPoolAlloc(PollingDataFrameMemoryPoolId,0U);
    cmd.pData = (uint8_t*)osMemoryPoolAlloc(PollingDataFrameMemoryPoolId,0U);
	while(1)
	{
        /* Waitting for command from CPU */
		retValue = extract_command(&cmd, &NeedToRespond,100);
        if (E_OK == retValue)
        {
            Msg_number++;
            DecToString(num,Msg_number);
            LcdCharacter_PutString(0,0,(uint8_t*)num,0);
            /*Notify the command to relevant task*/
            switch(cmd.OpCode)
            {
                /*POL from CPU*/
                case POL:
                    DurringPolling = 1;
                    LcdCharacter_PutString(1,0,(uint8_t*)"POL",0);
                    break;
                /*ACK from CPU*/
                case ACK:
                    if(1 == DurringPolling)
                    {
                        TransmitSuccess = E_OK;
                        LcdCharacter_PutString(1,0,(uint8_t*)"ACK",0);
                    }
                    break;
                /*NACK from CPU*/
                case NACK:
                    if(1 == DurringPolling)
                    {
                        TransmitSuccess = E_NOT_OK;
                        LcdCharacter_PutString(1,0,(uint8_t*)"NACK",0);
                    }
                    break;
// TO DO
                /*Go to system setting mode*/
                case 0x14:
                    gCurrentState = SYSTEM_SETTING;
                    break;
                /*Go to printing mode*/
                case 0x17:
                    gCurrentState = PRINTER_SETTING;
                    break;
                /*Go to preset mode*/
                case 0x16:
                    gCurrentState = SYSTEM_PRESET;
                    break;
                /*Special command*/
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x1F:
                    break;
                /*Dislay character Lcd hex type*/
                case 0x80:
                case 0x81:
                case 0x82:
                    LcdCharacterProcess(cmd.OpCode,cmd.pData);
                    FrameLength = Framing_data(DataFrame,NULL,0,ACK);
                    RS485_Transmit(DataFrame,FrameLength);
                    LcdCharacter_PutString(1,0,(uint8_t*)"SELECT",0);
                    break;
                /*Dislay segment Lcd hex type*/
                case 0x90:
                case 0x91:
                case 0x92:
                    LcdSegmentProcess(cmd.OpCode,cmd.pData);
                    FrameLength = Framing_data(DataFrame,NULL,0,ACK);
                    RS485_Transmit(DataFrame,FrameLength);
                    LcdCharacter_PutString(1,0,(uint8_t*)"SELECT",0);
                    break;
            }
            if(1 == DurringPolling)
            {
                if(E_OK == TransmitSuccess)
                {
                    if(E_OK == GetCommandFromBuffer(&OpCode,pCmd,&Cmdlength))
                    {
                        FrameLength = Framing_data(DataFrame,pCmd,Cmdlength,OpCode);
                        RS485_Transmit(DataFrame,FrameLength);
                    }else
                    {
                        DurringPolling = 0;
                        FrameLength = Framing_data(DataFrame,NULL,0,EOT);
                        RS485_Transmit(DataFrame,FrameLength);
                    }
                }else
                {
                    RS485_Transmit(DataFrame,FrameLength);
                }
            }
        }else
        {
            if(1 == NeedToRespond)
            {
                /*Send NACK if receive invalid data frame*/
                FrameLength = Framing_data(DataFrame,NULL,0,NACK);
                RS485_Transmit(DataFrame,FrameLength);
                False_Msg_number++;
                DecToString(False_num,False_Msg_number);
                LcdCharacter_PutString(0,10,(uint8_t*)False_num,0);
            }
        }
	}
}


static void Main_Preset_Task(void *argument)
{
    uint8_t PushTime=0;
    uint8_t key[2] = "";
    uint8_t SegmentKey[9]="";
    uint8_t Checking = NORMAL_KEY;
    uint8_t i = 0;
    uint8_t temp = 0;
    Keypad_Button_Type PreviousKey,CurrentKey;
    PreviousKey = CurrentKey = KEYPAD_UNKNOWN;
    osStatus_t status;

    Lcd_Segment_Put_Indicator(INDICATOR_A1|INDICATOR_A3|INDICATOR_A4|INDICATOR_A5);
    LcdSegmentDisplaySetEvent();
    while(1)
    {
        status = osMessageQueueGet(KeypadInputQueue_Id, &CurrentKey, NULL, osWaitForever);
        if(status == osOK)
        {
            Checking = SpecialKeyCheck(&PreviousKey,CurrentKey,&PushTime);
            switch(gCurrentState)
            {
                case SYSTEM_PRESET:
                {
                    if(Checking == PASS_CODE_1)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)"code",2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentState = SYSTEM_SETTING;
                    }
                    break;
                }
                case SYSTEM_SETTING:
                {
                    if(Checking == PASS_CODE_1)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)"code",1,0);
                        LcdSegmentDisplaySetEvent();
                    }else if(Checking == PASS_CODE_2)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)"busy",1,0);
                        LcdSegmentDisplaySetEvent();
                    }else if(Checking == PASS_CODE_3)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)"User",1,0);
                        LcdSegmentDisplaySetEvent();
                    }else if(Checking == NORMAL_KEY)
                    {
                        key[0] = Keypad_Mapping_Printer(CurrentKey,0);
                        /*Push previous key*/
                        //PutCommandToBuffer(0xA1,(uint8_t*)&key,1);
                        sprintf((char*)SegmentKey,"%s%c",SegmentKey,key[0]);
                        temp =  (CurrentKey % 4) | ((CurrentKey / 4) << 4) | 0x80;
                        PutCommandToBuffer(0xA0,(uint8_t*)&temp,1);
                        //Send_Data_Frame(0xA1,&key[0],1);
                        LcdSegmentDisplaySetEvent();
                        i++;
                        if(i == 7)
                        {
                            i=0;
                            sprintf((char*)SegmentKey,"%c",key[0]);
                        }
                        Lcd_Segment_Put_Data((uint8_t*)SegmentKey,0,0);
                        key[1]='\0';
                    }else if(Checking == SPECIAL_KEY)
                    {
                        temp = CurrentKey | 0x80;
                        PutCommandToBuffer(0xA0,(uint8_t*)&temp,1);
                    }
                    break;
                }
                case PRINTER_SETTING:
                {
                    if(Checking == NORMAL_KEY)
                    {
                        key[0] = Keypad_Mapping_Printer(CurrentKey,PushTime);
                        key[1]='\0';
                        LcdCharacter_PutString(LCD_CURRENT_POSITION,LCD_CURRENT_POSITION,key,0);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

/************************************ Callback handler *******************************************/
void Timer1_Callback (void *arg)
{
    KeypadScaningStatus = E_OK;
}

void Timer2_Callback (void *arg)
{
    gInputDone = E_OK;
}
/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
void Peco10_Preset_Init_All(void)
{
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  /*Get Address from the switch*/
  gMainAddress = Config_Switch_Get_Value();
  gSubAddress = gMainAddress + 2;
  gSubAddress_Coressponding = gMainAddress + 3;
  
  
  /*Create Threads*/
  LcdSegmentDisplay_Task_Id                  = osThreadNew(LcdSegmentDisplay_Task, NULL, &LcdSegmentDisplay_Task_Attr);
  ScanKeypad_DisplayLcdCharacter_Task_Id     = osThreadNew(ScanKeypad_DisplayLcdCharacter_Task, NULL, &ScanKeypad_DisplayLcdCharacter_Task_Attr);
  Main_Preset_Task_Id                        = osThreadNew(Main_Preset_Task, NULL, &Main_Preset_Task_Attr);
  CommandHandler_Task_Id                     = osThreadNew(CommandHandler_Task, NULL, &CommandHandler_Task_Attr);
  
   /*Create queues*/
  LcdCharacterMsgQueue_Id  = osMessageQueueNew(LCDCHARACTER_MSGQUEUE_OBJECTS, sizeof(LCDCHARACTER_MSGQUEUE_OBJ_t), NULL);
  KeypadInputQueue_Id      = osMessageQueueNew(KEYPAD_QUEUE_OBJECTS, sizeof(KEYPAD_QUEUE_OBJ_t), NULL);
  PollingDataInforQueue_ID = osMessageQueueNew(5,sizeof(Cmd_Queue_Obj_t), NULL);
  
  /* Initialize event flag for start display LCD event */
  LcdSegmentStartDisplayFlagId = osEventFlagsNew(NULL);
  
  /*Create pool data*/
  PollingDataFrameMemoryPoolId = osMemoryPoolNew(NUMBER_OF_MEMORY_POOL_ELEMENT, MEMORY_POOL_ELEMENT_SIZE, NULL);
  
  /*Create os timers*/
  Timer1_Id = osTimerNew(Timer1_Callback, osTimerPeriodic, NULL, NULL);
  Timer2_Id = osTimerNew(Timer2_Callback, osTimerOnce, NULL, NULL);
  
  /*use for backlight character lcd*/
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
 
  osKernelStart();                      // Start thread execution

}

