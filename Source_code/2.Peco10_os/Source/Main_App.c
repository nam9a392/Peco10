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
  uint8_t *msg;
  uint16_t length;
  uint8_t line;
  uint8_t offset;
} MsgForDisplay_t;

typedef struct{
    Keypad_Button_Type  PreviousStatus;
    Keypad_Button_Type  CurrentStatus;
    uint8_t             PushTime;
}KeypadStatus_t;

/*
*   @inplement: enumeration_InputControl_t
*   @brief:
*   row,col : start position of input
*   mask    : 1 hide input | 0 show input
*   blink   : 1 blinking   | 0 not blinking
*/
typedef struct{
    uint8_t  row;
    uint8_t  col;
    uint8_t  size;
    uint8_t  mask;
    uint8_t  blink;
}InputControl_t;

typedef struct{
    uint8_t *pPasscode;
    uint8_t length;
    uint8_t CheckingStatus;
}Password_t;

//typedef struct{
//    Keypad_Button_Type InputKey;
//    uint8_t KeyInputComplete;
//}KeypadInputStatus_t;
/*Aplication states*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
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
#define INPUT_STATE                        ((uint8_t)0x00U)
#define USERCODE_STATE                     ((uint8_t)0x01U)
#define READING_STATE                      ((uint8_t)0x02U)
#define UNKNOWN_STATE                      ((uint8_t)0x05U)
/* State for printer setting */
#define PRINTER_SETTING_STATE              ((uint8_t)0x01U)

/* Event flag to start display */
#define LCD_SEGMENT_START_DISPLAY_FLAG        (0x00000001U)

/*Queues Object define*/
#define LCDCHARACTER_QUEUE_OBJ_T               MsgForDisplay_t
#define LCDCHARACTER_QUEUE_OBJ_NUMBER          3U

#define KEYPAD_QUEUE_OBJ_T                     Keypad_Button_Type
#define KEYPAD_QUEUE_OBJ_NUMBER                10

#define INPUT_CTRL_QUEUE_OBJ_T                 InputControl_t
#define INPUT_CTRL_QUEUE_OBJ_NUMBER            1

/*Pool memory data define*/
#define RXDATA_POOL_ELEMENT_NUMBER          1U
#define RXDATA_POOL_ELEMENT_SIZE            64U

#define LCDCHARACTER_POOL_ELEMENT_NUMBER    3U
#define LCDCHARACTER_POOL_ELEMENT_SIZE      32U

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
                   InputControlQueue_Id;

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
    .stack_size = 384U
};

osMemoryPoolId_t RxDataMemoryPool_Id,
                 LcdCharacter_MemoryPool_Id;

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
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length, uint32_t timeout);
static void LcdSegmentDisplaySetEvent(void);

static void LcdSegmentDisplay_Task(void *argument);
static void ScanKeypad_DisplayLcdCharacter_Task(void *argument);
static void CommandHandler_Task(void *argument);
static void Main_Preset_Task(void *argument);

static uint8_t LcdSegmentData_HexToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString);
static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData,uint8_t length);
static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData,uint8_t length);
static uint8_t SpecialKeyCheck(KeypadStatus_t *KeypadStatus);

/* Timer callback to handle input from keypad*/
void Timer1_Callback (void *arg);
void Timer2_Callback (void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
#define ZERO_DISPLAY_MASK   0x1U
/***************************** logic function *************************************/
static uint8_t LcdSegmentData_HexToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString)
{
    uint8_t i,j,NumberOfDigit;
    i=j=NumberOfDigit=0;
    uint8_t StartOfNum = 0;
    uint8_t temp =0;
    
    /* Ctr1 0(1B) | Ctr1 1(1B) | Row(4B) */
    /* Ctr1 0: | 1 | 0 | 0 | 0 | 0 | 0 | 0 | Leading Zero |*/
    uint8_t Zero_Display = pData[0] & ZERO_DISPLAY_MASK;
    /* Ctr1 1: | 1 | Digit 6  | Digit 5  | Digit 4  | Digit 3  | Digit 2  | Digit 1  | Digit 0 |*/
    
    /* Leading zero number is displayed or not*/
    if(Zero_Display == 1)
    {
        StartOfNum = 1;
    }
    
    /* 1 Byte indicate 2 digits in ascii Ex: 0x23 => '2','3' */
    for(i=1;i<((DataLength-2)*2);i++)
    {
        if(i%2 == 0)
        {
            temp = (pData[2 + i/2] >> 4);
        }else
        {
            temp = (pData[2 + i/2] & 0xf);
        }
        /*Bypass zero number from the left*/
        if((temp != 0)&&(StartOfNum == 0))
        {
            StartOfNum = 1;
        }
        if(StartOfNum == 1)
        {
            pString[j] = temp + 0x30;
            j++;
        }
        /*Add decimal point to number*/
        if(0 != (pData[1] & (0x1 << (7-i))))
        {
            pString[j] = ',';
            j++;
        }
    }
    
    NumberOfDigit = j;
    return NumberOfDigit;
}



/***************************** Function to handle Display component *************************************/
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length, uint32_t timeout)
{
    MsgForDisplay_t pMsgQueue;
    pMsgQueue.msg = (uint8_t*)osMemoryPoolAlloc(LcdCharacter_MemoryPool_Id,0U);
    /*prepare queue data*/
    pMsgQueue.line=line;
    pMsgQueue.offset=offset;
    pMsgQueue.length=length;
    strcpy((char*)pMsgQueue.msg,(char*)pString);
    
    /* push data to lcd character massage queue*/
    osMessageQueuePut(LcdCharacterMsgQueue_Id, &pMsgQueue, 0U, timeout);
}

static void LcdSegmentDisplaySetEvent(void)
{
    osEventFlagsSet(LcdSegmentStartDisplayFlagId, LCD_SEGMENT_START_DISPLAY_FLAG);
}

static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData,uint8_t length)
{
    InputControl_t InputCtrl;
    uint8_t LcdSegmentBuffer[14] = {0};
    uint8_t DigitNumbers = 0;
    
#define INPUT_CTRL_ROW_MASK         0x3
#define INPUT_CTRL_ROW_SHIFT        5U
#define INPUT_CTRL_COL_MASK         0x7
#define INPUT_CTRL_COL_SHIFT        2U
#define INPUT_CTRL_HIDE_MASK        0x1
#define INPUT_CTRL_HIDE_SHIFT       1U
    switch(opcode) 
    {
        case 0x10:
            /* Ctr1 0(1B) | Ctr1 1(1B) | Row 1(7B) | Row 2(7B) | Row 3(7B) */
            /* Ctrl 0    :| Bit7     | Bit6        | Bit5        | Bit4 | Bit3 | Bit2 | Bit1          | Bit0        |
                          | Flag = 1 | Row = 0,1,2 ( 3 read only)| Digit = 0~7        | Hide Mask(0,1)| blink (0|1) |
                                     | Start position of input data                   |                         
               Ctrl 1     Max size of inputing (MSB = 1) */
            Lcd_Segment_Put_Data(&pData[2],7,0,0);
            Lcd_Segment_Put_Data(&pData[9],7,1,0);
            Lcd_Segment_Put_Data(&pData[16],7,2,0);
            InputCtrl.row  = (pData[0] >> INPUT_CTRL_ROW_SHIFT)  & INPUT_CTRL_ROW_MASK;
            InputCtrl.col  = (pData[0] >> INPUT_CTRL_COL_SHIFT)  & INPUT_CTRL_COL_MASK;
            InputCtrl.mask = (pData[0] >> INPUT_CTRL_HIDE_SHIFT) & INPUT_CTRL_HIDE_MASK;
            InputCtrl.size =  pData[1] & 0x7F;
            osMessageQueuePut(InputControlQueue_Id, &InputCtrl, 0U, 0);
            break;
        /*Control Byte : Leading Zero display | Sign(dot or comma) */
        case 0x90:
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) |Ctr1(2B) | Row 3(4B) */
            Lcd_Segment_Clear_Line(1);
            Lcd_Segment_Clear_Line(2);
            Lcd_Segment_Clear_Line(3);
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,0,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[6],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,1,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[12],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,2,0);
            break;
        case 0x91:
            Lcd_Segment_Clear_Line(1);
            Lcd_Segment_Clear_Line(2);
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) */
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,0,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[6],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,1,0);
            break;
        case 0x92:
            Lcd_Segment_Clear_Line(3);
            /* Ctr1(2B) | Row 3(4B) */
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,2,0);
            break;
    }
    LcdSegmentDisplaySetEvent();
}

static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData,uint8_t length)
{
    uint8_t LcdCharacterBuffer[17];
    uint16_t LenthOfRow1 = 0;
    uint16_t LenthOfRow2 = 0;
    switch(opcode) 
    {
        /* Ctr1(1Byte) | Row 1(n Byte) |Ctr1(1Byte) | Row 2(n Byte) */
        /* Ctr1 = 0x80 | Length of row */
        case 0x90:
            // Row 1 data from position 2 length equal to pData[0]
            LenthOfRow1 = pData[0] & 0x7F;
            LcdCharacter_PutString(0,0,&pData[1],LenthOfRow1,0);
            // Row 2 data from position 2 + pData[1] length equal to pData[1 + LenthOfRow1]
            LenthOfRow2 = pData[1 + LenthOfRow1]  & 0x7F;
            LcdCharacter_PutString(1,0,&pData[1 + LenthOfRow1 + 1],LenthOfRow2,0);
            break;
        case 0x91:
            /* Ctr1(1Byte) | Row 1(n Byte)*/
            // Row 1 data from position 2 length equal to pData[0]
            LenthOfRow1 = pData[0] & 0x7F;
            LcdSegmentData_HexToString(&pData[2],LenthOfRow1,LcdCharacterBuffer);
            LcdCharacter_PutString(0,0,&pData[1],LenthOfRow1,0);
            break;
        case 0x92:
            /* Ctr1(1Byte) | Row 2(n Byte) */
            // Row 1 data from position 1 length equal to pData[0]
            LenthOfRow2 = pData[0] & 0x7F;
            LcdCharacter_PutString(1,0,&pData[1],LenthOfRow2,0);
            break;
    }
}

/***************************** Function to handle input data from keypad *************************************/
const uint8_t Password1[] = {KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_CLEAR,KEYPAD_ENTER,KEYPAD_CLEAR};
const uint8_t Password2[] = {KEYPAD_CLEAR,KEYPAD_CLEAR,KEYPAD_CLEAR};
const uint8_t Password3[] = {KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER};
#define NO_PASSWORD_CHECK       0
#define PASSWORD_CHECKING       1

#define NORMAL_KEY              0
#define SPECIAL_KEY             1
#define UNKNOWN_KEY             2

static uint8_t SpecialKeyCheck(KeypadStatus_t *KeypadStatus)
{
    uint8_t RetValue = SPECIAL_KEY;
    if(KeypadStatus->CurrentStatus == KEYPAD_UNKNOWN)
    {
        KeypadStatus->PushTime = 0;
        RetValue = UNKNOWN_KEY;
    }else{
        if(KeypadStatus->CurrentStatus == KEYPAD_FORWARD)
        {
            if(PRINTER_SETTING == gCurrentState)
                Lcd_Move_Cursor_Right();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_BACKWARD)
        {
            if(PRINTER_SETTING == gCurrentState)
                Lcd_Move_Cursor_Left();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_FONT)
        {
            if(PRINTER_SETTING == gCurrentState)
                Keypad_Change_Font();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_ENDLINE)
        {
            if(PRINTER_SETTING == gCurrentState)
                Lcd_Move_Cursor_Next_Line();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_CLEAR)
        {
            if(PRINTER_SETTING == gCurrentState)
                Lcd_Clear_Character();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_ENTER)
        {
            //TO DO
        }else if(KeypadStatus->CurrentStatus == KEYPAD_LOSS)
        {
            //TO DO
        }else
        {
            if(PRINTER_SETTING == gCurrentState)
            {
                if(KeypadStatus->PreviousStatus == KeypadStatus->CurrentStatus)
                {
                    KeypadStatus->PushTime = KeypadStatus->PushTime + 1;
                }else
                {
                    KeypadStatus->PushTime = 0;
                    Lcd_Move_Cursor_Right();
                }
            }
            RetValue = NORMAL_KEY;
        }
        if(PRINTER_SETTING != gCurrentState)
        {
            KeypadStatus->PushTime = KeypadStatus->PushTime + 1;
        }
    }
    KeypadStatus->PreviousStatus = KeypadStatus->CurrentStatus;
    return RetValue;
}

static Std_Return_Type Password_Check(KeypadStatus_t *KeypadStatus,Password_t *Password)
{
    Std_Return_Type RetValue = E_NOT_OK;
    if(KeypadStatus->PushTime == 1)
        Password->CheckingStatus = PASSWORD_CHECKING;
    if(Password->CheckingStatus == PASSWORD_CHECKING)
    {
        if(KeypadStatus->PushTime <= Password->length)
        {
            if(KeypadStatus->CurrentStatus == (Password->pPasscode)[KeypadStatus->PushTime-1])
            {
                if(KeypadStatus->PushTime == Password->length)
                {
                    Password->CheckingStatus = NO_PASSWORD_CHECK;
                    RetValue = E_OK;
                }
            }else{
                Password->CheckingStatus = NO_PASSWORD_CHECK;
            }
        }
    }
    return RetValue;
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
            Lcd_Put_String(data1.line,data1.offset,data1.msg,data1.length);
            osMemoryPoolFree(LcdCharacter_MemoryPool_Id,data1.msg);
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
    
    uint16_t False_Msg_number = 0;
    uint8_t invalid_format = 0;
    uint16_t Msg_number = 0;
    uint16_t feadback = 0;
    
    uint8_t OpCode=0;
    uint16_t Length=0;
    uint8_t *RxData;
    
    RxData = (uint8_t*)osMemoryPoolAlloc(RxDataMemoryPool_Id,0U);
    
    RS485_Init(gMainAddress);
    //LcdCharacter_PutString(1,0,(uint8_t*)"Invalid",0);
	while(1)
	{
        /* Waitting for command from CPU */
        RS485_Wait_For_Message(&OpCode, (uint8_t*)RxData, &Length, osWaitForever);
        if (E_OK == retValue)
        {
            Msg_number++;
            if(Msg_number == 0)
                LcdCharacter_PutString(0,0,(uint8_t*)"     ",5,0);
            DecToString(num,Msg_number);
            LcdCharacter_PutString(0,0,(uint8_t*)num,strlen((char*)num),0);
            /*Notify the command to relevant task*/
            switch(OpCode)
            {
                case 0xff:
                    False_Msg_number++;
                    DecToString(False_num,False_Msg_number);
                    LcdCharacter_PutString(0,12,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                case 0xfe:
                    invalid_format++;
                    DecToString(False_num,invalid_format);
                    LcdCharacter_PutString(1,12,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                case 0xfc:
                    feadback++;
                    if(feadback == 0)
                        LcdCharacter_PutString(1,0,(uint8_t*)"     ",5,0);
                    DecToString(False_num,feadback);
                    LcdCharacter_PutString(1,0,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                case 0x05:
                    LcdCharacter_PutString(0,7,(uint8_t*)"POL",3,0);
                    break;
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
                    LcdCharacterProcess(OpCode,RxData,Length);
                    break;
                /*Dislay segment Lcd ascii type*/
                case 0x10:
                /*Dislay segment Lcd hex type*/
                case 0x90:
                case 0x91:
                case 0x92:
                    LcdSegmentProcess(OpCode,RxData,Length);
                    break;
            }
        }
	}
}




static void Main_Preset_Task(void *argument)
{
    uint8_t key[2] = "";
    uint8_t SegmentKey[14];
    uint8_t Checking = NORMAL_KEY;
    uint8_t i = 0;
    InputControl_t InputCtrl ={.row = 2, .col = 0, .mask = 0, .size = 11};
    
    osStatus_t status;
    Lcd_Segment_Put_Indicator(INDICATOR_A1|INDICATOR_A3|INDICATOR_A4|INDICATOR_A5);
    LcdSegmentDisplaySetEvent();
    
    KeypadStatus_t KeypadStatus = {KEYPAD_UNKNOWN,KEYPAD_UNKNOWN,0};
    Password_t passcheck_1 = {(uint8_t*)Password1, sizeof(Password1),NO_PASSWORD_CHECK};
    Password_t passcheck_2 = {(uint8_t*)Password2, sizeof(Password2),NO_PASSWORD_CHECK};
    Password_t passcheck_3 = {(uint8_t*)Password3, sizeof(Password3),NO_PASSWORD_CHECK};
#define REGISTRATION_PASS   0xA5U
#define PROCESS_PASS        0x5AU
    
#define INPUT_CTRL_MASK     1
#define INPUT_CTRL_UNMASK   0
    uint8_t Registration_Pass = REGISTRATION_PASS;
    uint8_t Process_Pass      = PROCESS_PASS;
    while(1)
    {
        status = osMessageQueueGet(KeypadInputQueue_Id, &(KeypadStatus.CurrentStatus), NULL, osWaitForever);
        if (osOK == osMessageQueueGet(InputControlQueue_Id, &InputCtrl, NULL, 0))
        {
            ;
        }
        if(status == osOK)
        {
            Checking = SpecialKeyCheck(&KeypadStatus);
            switch(gCurrentState)
            {
                case SYSTEM_PRESET:
                {
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_1))
                    {
                        RS485_Prepare_To_Transmit(0xA0,&Registration_Pass,1);
                        Lcd_Segment_Put_Data((uint8_t*)"code",4,2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentState = SYSTEM_SETTING;
                        gCurrentSubState = USERCODE_STATE;
                        InputCtrl.mask = INPUT_CTRL_MASK;
                    }
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_3))
                    {
                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)"XXXXX",5);
                        Lcd_Segment_Put_Data((uint8_t*)"print",5,2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentState = PRINTER_SETTING;
                    }
                    break;
                }
                case SYSTEM_SETTING:
                {
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_2))
                    {
                        RS485_Prepare_To_Transmit(0xA0,&Process_Pass,3);
                        Lcd_Segment_Put_Data((uint8_t*)"00     ",7,2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentSubState = INPUT_STATE;
                        InputCtrl.mask = INPUT_CTRL_UNMASK;
                        
                    }else if(Checking != UNKNOWN_KEY)
                    {
                        key[0] = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,0);
                        if(Checking == NORMAL_KEY)
                        {
                            
                            /*Push previous key*/
                            //PutCommandToBuffer(0xA1,(uint8_t*)&key,1);
                            if(i < InputCtrl.size)
                            {
                                if(InputCtrl.mask == INPUT_CTRL_MASK)
                                {
                                    SegmentKey[i]=key[0];
                                    if(i < 14)
                                    {
                                        if(i >= 7)
                                        {
                                            Lcd_Segment_Put_Data((uint8_t*)"-",1,(InputCtrl.row - 1),i - 7);
                                        }else
                                        {
                                            Lcd_Segment_Put_Data((uint8_t*)"-",1,InputCtrl.row,i);
                                        }
                                        i++;
                                    }
                                }else if(InputCtrl.mask == INPUT_CTRL_UNMASK)
                                {
                                    if(i < 14)
                                    {
                                        SegmentKey[i]=key[0];
                                        i++;
                                        if(i >= 7)
                                        {
                                            Lcd_Segment_Put_Data((uint8_t*)SegmentKey,i-7,InputCtrl.row - 1,0);
                                            Lcd_Segment_Put_Data((uint8_t*)&SegmentKey[i-7],7,InputCtrl.row,0);
                                        }else
                                        {
                                            Lcd_Segment_Put_Data((uint8_t*)SegmentKey,i,InputCtrl.row,0);
                                        }
                                    }
                                }
                                LcdSegmentDisplaySetEvent();
                            }
                        }else if(Checking == SPECIAL_KEY)
                        {
                            if((KeypadStatus.CurrentStatus == KEYPAD_ENTER) || (KeypadStatus.CurrentStatus == KEYPAD_CLEAR))
                            {
                                if((KeypadStatus.CurrentStatus == KEYPAD_ENTER) && ( i != 0 ))
                                {
                                    RS485_Prepare_To_Transmit(0xA1,(uint8_t*)SegmentKey,i);
                                    Lcd_Segment_Clear_Line(0x02);
                                    Lcd_Segment_Put_Data((uint8_t*)SegmentKey,i,2,0);
                                }
                                sprintf((char*)SegmentKey,"");
                                Lcd_Segment_Put_Data((uint8_t*)"       ",7,0,0);
                                Lcd_Segment_Put_Data((uint8_t*)"       ",7,1,0);
                                LcdSegmentDisplaySetEvent();
                                i=0;
                            }
                        }
                    }
                   
                    break;
                }
                case PRINTER_SETTING:
                {
                    if(Checking == NORMAL_KEY)
                    {
                        key[0] = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,KeypadStatus.PushTime);
                        //key[1]='\0';
                        LcdCharacter_PutString(LCD_CURRENT_POSITION,LCD_CURRENT_POSITION,key,1,0);
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
  LcdCharacterMsgQueue_Id  = osMessageQueueNew(LCDCHARACTER_QUEUE_OBJ_NUMBER, sizeof(LCDCHARACTER_QUEUE_OBJ_T), NULL);
  KeypadInputQueue_Id      = osMessageQueueNew(KEYPAD_QUEUE_OBJ_NUMBER, sizeof(KEYPAD_QUEUE_OBJ_T), NULL);
  InputControlQueue_Id     = osMessageQueueNew(INPUT_CTRL_QUEUE_OBJ_NUMBER, sizeof(INPUT_CTRL_QUEUE_OBJ_T), NULL);
  
  /* Initialize event flag for start display LCD event */
  LcdSegmentStartDisplayFlagId = osEventFlagsNew(NULL);
  
  /*Create pool data*/
  RxDataMemoryPool_Id = osMemoryPoolNew(RXDATA_POOL_ELEMENT_NUMBER, RXDATA_POOL_ELEMENT_SIZE, NULL);
  LcdCharacter_MemoryPool_Id = osMemoryPoolNew(LCDCHARACTER_POOL_ELEMENT_NUMBER, LCDCHARACTER_POOL_ELEMENT_SIZE, NULL);
  
  /*Create os timers*/
  Timer1_Id = osTimerNew(Timer1_Callback, osTimerPeriodic, NULL, NULL);
  Timer2_Id = osTimerNew(Timer2_Callback, osTimerOnce, NULL, NULL);
  
  HAL_TIM_Base_Start(&htim2);
  /*use for backlight character lcd*/
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
 
  osKernelStart();                      // Start thread execution

}

