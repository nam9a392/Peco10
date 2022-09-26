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
} msg_t;

typedef struct{
    uint8_t Address;
    uint8_t OpCode;
    uint8_t DataLength;
    uint8_t *pData;
}command_t;

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
#define LCDCHARACTER_MSGQUEUE_OBJ_t            msg_t
#define LCDCHARACTER_MSGQUEUE_OBJECTS          2U

#define KEYPAD_QUEUE_OBJ_t                     uint8_t
#define KEYPAD_QUEUE_OBJECTS                   30

/*Pool memory data define*/
#define NUMBER_OF_MEMORY_POOL_ELEMENT       2U
#define MEMORY_POOL_ELEMENT_SIZE            100U

#define RECEIVE_BUFFER_SIZE                 128U
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
                   KeypadInputQueue_Id;

/*pool data*/
osMemoryPoolId_t PollingDataFrameMemoryPoolId;

/*Timer ID*/
osTimerId_t Timer1_Id;                            // timer id
osTimerId_t Timer2_Id;                            // timer id

/*Define attributes for each thread*/
const osThreadAttr_t LcdSegmentDisplay_Task_Attr =
{
    .stack_size = 256U
};

const osThreadAttr_t ScanKeypad_DisplayLcdCharacter_Task_Attr =
{
    .stack_size = 256U
};

const osThreadAttr_t CommandHandler_Task_Attr =
{
    .stack_size = 512U
};

const osThreadAttr_t Main_Preset_Task_Attr =
{
    .stack_size = 512U
};


/* Event flag to trigger LCD start displaying */
static osEventFlagsId_t LcdSegmentStartDisplayFlagId;

/****************************** Other Variables **********************************/
static uint8_t gPresetAddress;
uint8_t curr_state = PRINTER_SETTING;
volatile Std_Return_Type KeypadScaningStatus = E_OK;


/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint32_t timeout);
static void LcdSegmentDisplaySetEvent(void);
static void LcdSegmentDisplay_Task(void *argument);
static void ScanKeypad_DisplayLcdCharacter_Task(void *argument);
static void CommandHandler_Task(void *argument);
static void Main_Preset_Task(void *argument);

Std_Return_Type extract_command(command_t *pCmd);
static void HexDataToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString);
static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData);
static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData);
static uint8_t CheckSum(uint8_t* pData, uint8_t Length);
static uint8_t SpecialKeyCheck(uint8_t PreviousPos,uint8_t CurrentPos);

void Timer1_Callback (void *arg);
void Timer2_Callback (void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint32_t timeout)
{
    msg_t pMsgQueue;

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

static void HexDataToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString)
{
    uint8_t i=0;
    for(i=0;i<DataLength;i++)
    {
        sprintf((char*)pString,"%d%d",(pData[i]>>4),(pData[i]&0xf));
    }
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


static uint8_t CheckSum(uint8_t* pData, uint8_t Length)
{
    uint8_t CheckSumResult = 0;
    uint8_t i=0;
    for(i=0;i<Length;i++)
    {
        CheckSumResult ^= pData[i];
    }
    CheckSumResult ^= 0xFF;
    return CheckSumResult;
}


Std_Return_Type extract_command(command_t *pCmd)
{
    Std_Return_Type retValue = E_NOT_OK;
    uint32_t length;
    uint8_t Rxbuffer[RECEIVE_BUFFER_SIZE];
    
    /*waiting for new command*/
 	RS485_Wait_For_Message(Rxbuffer,&length,osWaitForever);
    
    // ACK frame or POL frame
    // Start Delimiter(1Byte) | Slave address(1Byte) | 05 or 06 (1Byte)  | End delimiter(1Byte)
    if((uint8_t)0x4 == length)
    {
        /* Check valid command slave address and checksum*/
        if((gPresetAddress == Rxbuffer[1])&& (Rxbuffer[length - 1] == CheckSum(&Rxbuffer[1],length - 2)))
        {
            pCmd->Address = Rxbuffer[1];
            pCmd->OpCode = Rxbuffer[2];
            pCmd->DataLength = 0;
            retValue = E_OK;
        }
    }else if((uint8_t)0x4 < length)
    {
        if((uint8_t)0x02 == Rxbuffer[1])
        {
            // SELLECT frame
            // Start Delimiter(1Byte) | 0x02(1Byte) | Slave address(1Byte) | Data(n Byte) | End delimiter(1Byte) | CheckSum(1Byte)
            if((gPresetAddress == Rxbuffer[2]) && (Rxbuffer[length - 1] == CheckSum(&Rxbuffer[1],length - 2)))
            {
                pCmd->Address = Rxbuffer[2];
                pCmd->OpCode = Rxbuffer[3];
                pCmd->DataLength = length - 5;
                memcpy(pCmd->pData,Rxbuffer,length);
                retValue = E_OK;
            }
        }else{
            
        }
    }
	return retValue;
}

/************************************ Task handler *******************************************/
static void LcdSegmentDisplay_Task(void *argument)
{
    uint32_t flag;
    Lcd_Segment_Put_Indicator(INDICATOR_A1|INDICATOR_A3|INDICATOR_A4|INDICATOR_A5);
    
    while(1)
    {
        flag = osEventFlagsWait(LcdSegmentStartDisplayFlagId, LCD_SEGMENT_START_DISPLAY_FLAG, osFlagsWaitAny, osWaitForever);
        Lcd_Segment_Start_Display();
    }
}

volatile Std_Return_Type gInputDone = E_NOT_OK;
#define REGISTER_KEY           0xff


#define SPECIAL_KEY         0xfcU
static void ScanKeypad_DisplayLcdCharacter_Task(void *argument)
{
    const uint8_t Add_string[]="Address num:";
    const uint8_t Keypad_string[]="Cur Button:";
    const uint8_t clear[]=" ";
    const uint8_t maintaince[]="code";
    const uint8_t codetest[]="8888888";
    msg_t data1;
    uint8_t key[3],Lcd_character_pos;
    Lcd_character_pos=0;
    uint8_t CurrentPos,PreviousPos;
    CurrentPos = 19;
    PreviousPos = 0;
    uint8_t KeyNotPushed = SPECIAL_KEY;
    osStatus_t status;
    Keypad_Button_Type KeypadPreviousStatus,KeypadCurrentStatus;
    KeypadPreviousStatus = KeypadCurrentStatus = BUTTON_UNKNOWN;

    
    uint8_t pushtime,temp;
    pushtime = temp = 0;
    uint8_t character_pos = LCD_CURRENT_POSITION;
//    Lcd_Segment_Put_Data((uint8_t*)codetest,0,0);
//    Lcd_Segment_Put_Data((uint8_t*)codetest,1,0);
//    Lcd_Segment_Put_Data((uint8_t*)codetest,2,0);
//    LcdSegmentDisplaySetEvent();
    //LcdCharacter_PutString(0,7,(uint8_t*)Keypad_string,0);
    //LcdCharacter_PutString(0,2,(uint8_t*)Add_string,0);
    osTimerStart(Timer1_Id,KEYPAD_SCANNING_DURATION);
    
    while(1)
    {
        /* Lcd character print*/
        /* pop data from lcd character massage queue*/
        status = osMessageQueueGet(LcdCharacterMsgQueue_Id, &data1, NULL, 0U);   // wait for message
        if (status == osOK) {
//            for(uint8_t i=0; i<=strlen((char*)data1.msg); i++)
//            {
//                Lcd_Put_String(data1.line,data1.offset + i,(uint8_t *)clear);    //clear data before display
//            }
            Lcd_Put_String(data1.line,data1.offset,data1.msg);
        }
        
        /* Keypad scan period 50ms */
        if(E_OK == KeypadScaningStatus)
        {
            KeypadScaningStatus = E_NOT_OK;
            KeypadCurrentStatus = Keypad_Scan_Position(&CurrentPos);
            if(KeypadCurrentStatus != KeypadPreviousStatus)
            {
                KeypadPreviousStatus = KeypadCurrentStatus;
                if(KEYPAD_PUSHED == KeypadCurrentStatus)
                {
                    KeyNotPushed = SpecialKeyCheck(PreviousPos,CurrentPos);
                    PreviousPos = CurrentPos;
                    /* Play Buzzer */
                    //Buzzer_Play();
                }
            }
        }
        if(KeyNotPushed != SPECIAL_KEY)
        {
            if(temp != 0xFF)
            {
                if(curr_state == PRINTER_SETTING)
                {
                    Keypad_Mapping_Printer(key,CurrentPos,KeyNotPushed);
                }else{
                    Keypad_Mapping(key,CurrentPos);
                }
                Lcd_Segment_Put_Data(key,2,5);
                LcdSegmentDisplaySetEvent();
                if(E_OK == gInputDone)
                {
                    gInputDone = E_NOT_OK;
                    LcdCharacter_PutString(LCD_NEXT_POSITION,LCD_NEXT_POSITION,key,0);
                }else{
                    LcdCharacter_PutString(LCD_CURRENT_POSITION,LCD_CURRENT_POSITION,key,0);
                }
            }else{
                LcdCharacter_PutString(1,0,(uint8_t*)maintaince,0);
                Lcd_Segment_Put_Data((uint8_t*)maintaince,1,0);
                LcdSegmentDisplaySetEvent();
            }
            KeyNotPushed = SPECIAL_KEY;
        }
    }
}
const uint8_t Password[] = "XXCXC";
const uint8_t Password1[] = "CCC";
uint8_t check_pos = 0;


static uint8_t SpecialKeyCheck(uint8_t PreviousPos,uint8_t CurrentPos)
{
    uint8_t retValue = SPECIAL_KEY;
    uint8_t checkCharacter[3];
    Keypad_Mapping(checkCharacter,CurrentPos);
    if(PRINTER_SETTING == curr_state)
    {
        if((checkCharacter[0] >='0') && (checkCharacter[0] <= '9'))
        {
            if(CurrentPos != PreviousPos)
            {
                gInputDone = E_OK;
                check_pos = 0;
            }else if((CurrentPos == PreviousPos) && (gInputDone == E_NOT_OK))
            {
                check_pos++;
            }
            retValue = check_pos;
            osTimerStart(Timer2_Id,500);
        }else if(CurrentPos == FORWARD)
        {
            Lcd_Move_Cursor_Right();
            gInputDone = E_NOT_OK;
        }else if(CurrentPos == BACKWARD)
        {
            Lcd_Move_Cursor_Left();
            gInputDone = E_NOT_OK;
        }
        else if(CurrentPos == CLEAR)
        {
            Lcd_Clear_Character();
        }else if(CurrentPos == FONT)
        {
            Keypad_Change_Font();
        }else if(CurrentPos != ENTER)
        {
            gInputDone = E_OK;
            osTimerStop(Timer2_Id);
            check_pos = 0;
            retValue = check_pos;
        }
    }else{
        if((CurrentPos == CLEAR) || (CurrentPos == ENTER))
        {
            if(checkCharacter[0] == Password[check_pos])
            {
                if(check_pos == (strlen((char*)Password) - 1))
                {
                    osTimerStop(Timer2_Id);
                    check_pos = 0;
                    retValue = 0xFF;
                }else{
                    check_pos++;
                    osTimerStart(Timer2_Id,1000);
                }
            }
        }
    }
    
    return retValue;
}

static void CommandHandler_Task(void *argument)
{
    Std_Return_Type retValue = E_OK;
    uint8_t EOT_FRAME[]={0x4,gPresetAddress,0x4,0x3};
    uint8_t ACK_FRAME[]={0x4,gPresetAddress,0x6,0x3};
    uint8_t NACK_FRAME[]={0x4,gPresetAddress,0x15,0x3};
	command_t cmd;
	while(1)
	{
		retValue = extract_command(&cmd);
        if (E_OK == retValue)
        {
            /*Notify the command to relevant task*/
            switch(cmd.OpCode)
            {
                /*POL from CPU*/
                case 0x05:
                    break;
                /*ACK from CPU*/
                case 0x06:
                    break;
                /*NACK from CPU*/
                case 0x07:
                    break;
                /*Go to system setting mode*/
                case 0x14:
                    curr_state = SYSTEM_SETTING;
                    break;
                /*Go to printing mode*/
                case 0x15:
                    curr_state = PRINTER_SETTING;
                    break;
                /*Go to preset mode*/
                case 0x16:
                    curr_state = SYSTEM_PRESET;
                    break;
                /*Special command*/
                case 0x11:
                case 0x12:
                    break;
                /*Dislay character Lcd hex type*/
                case 0x80:
                case 0x81:
                case 0x82:
                    LcdCharacterProcess(cmd.OpCode,cmd.pData);
                    break;
                /*Dislay segment Lcd hex type*/
                case 0x90:
                case 0x91:
                case 0x92:
                    LcdSegmentProcess(cmd.OpCode,cmd.pData);
                    break;
            }
        }else
        {
            /*Send NACK if receive invalid data frame*/
            //RS485_Transmit(NACK_FRAME,sizeof(NACK_FRAME));
        }
	}
}

static void Main_Preset_Task(void *argument)
{
    
}

/************************************ Callback handler *******************************************/
void Timer1_Callback (void *arg)
{
    KeypadScaningStatus = E_OK;
}

void Timer2_Callback (void *arg)
{
    gInputDone = E_OK;
    check_pos = 0;
}
/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
void Peco10_Preset_Init_All(void)
{
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  /*Get Address from the switch*/
  gPresetAddress = Config_Switch_Get_Value();
  
  Lcd_Segment_Init();
  Lcd_Init_4bits_Mode();
  
  /* Initialize event flag for start display LCD event */
  LcdSegmentStartDisplayFlagId = osEventFlagsNew(NULL);
    
  /*Create queues*/
  LcdCharacterMsgQueue_Id = osMessageQueueNew(LCDCHARACTER_MSGQUEUE_OBJECTS, sizeof(LCDCHARACTER_MSGQUEUE_OBJ_t), NULL);
  KeypadInputQueue_Id     = osMessageQueueNew(KEYPAD_QUEUE_OBJECTS, sizeof(KEYPAD_QUEUE_OBJ_t), NULL);
    
  /*Create threads */
  LcdSegmentDisplay_Task_Id                  = osThreadNew(LcdSegmentDisplay_Task, NULL, &LcdSegmentDisplay_Task_Attr);
  ScanKeypad_DisplayLcdCharacter_Task_Id     = osThreadNew(ScanKeypad_DisplayLcdCharacter_Task, NULL, &ScanKeypad_DisplayLcdCharacter_Task_Attr);
  CommandHandler_Task_Id                     = osThreadNew(CommandHandler_Task, NULL, &CommandHandler_Task_Attr);
  Main_Preset_Task_Id                        = osThreadNew(Main_Preset_Task, NULL, &Main_Preset_Task_Attr);
    
  /*Create pool data*/
  PollingDataFrameMemoryPoolId = osMemoryPoolNew(NUMBER_OF_MEMORY_POOL_ELEMENT, MEMORY_POOL_ELEMENT_SIZE, NULL);
  
  /*Create os timer*/
  Timer1_Id = osTimerNew(Timer1_Callback, osTimerPeriodic, NULL, NULL);
  Timer2_Id = osTimerNew(Timer2_Callback, osTimerOnce, NULL, NULL);
  
  /*use for backlight character lcd*/
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
 
  osKernelStart();                      // Start thread execution
}

