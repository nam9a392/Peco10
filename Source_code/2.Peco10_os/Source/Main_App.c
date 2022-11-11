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

typedef struct{
    uint8_t  row;
    uint8_t  col;
    uint8_t  max_size;
    uint8_t  clear_size;
    uint8_t  mask;
    uint8_t  blink;
    uint8_t  enable;
    uint8_t  blink_error;
    uint8_t  row_error;
    uint8_t  col_error;
}LcdSegmentInputControl_t;

typedef struct{
    uint8_t  row;
    uint8_t  col;
    uint8_t  max_size;
    uint8_t  clear_size;
    uint8_t  CursorEffect;
}LcdCharacterInputControl_t;
/*
*   @inplement: enumeration_InputControl_t
*   @brief: Hold the state to control input from keypad
*       row,col         : start position of input
*       clear_size      : number of 0 display after push clear button
*       max_size        : number of digit allowed to insert
*       mask            : 1 hide input | 0 show input
*       blink           : 1 blinking   | 0 not blinking
*       enable          : 1 allow insert data from keypad | 0 negative
*       preset_button   : 1 alow using P1~P4 button | 0 negative
*/
typedef struct{
    LcdSegmentInputControl_t    LcdSegment;
    LcdCharacterInputControl_t  LcdCharacter;
    uint8_t ErrorCodeLength;
    uint8_t ErrorCode[7];
    uint8_t preset_button;
    uint8_t buffer_size;
}InputControl_t;

/*
*   @inplement: enumeration_Password_t
*   @brief: Hold the state of password checking (eg: "XXCXC" , "CCC")
*       *pPasscode      : pointer to checking password array
*       length          : checking password length
*       CheckingStatus  : if it is checking password currently (1/0)
*/
typedef struct{
    uint8_t *pPasscode;
    uint8_t length;
    uint8_t CheckingStatus;
}Password_t;

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/**
 * @brief @SYSTEMSTATE : Current state of preset
 */
#define IDLE_STATE                         ((uint8_t)0U)
#define PUMP_SETTING_STATE                 ((uint8_t)1U)
#define PRINTER_SETTING_STATE              ((uint8_t)2U)
#define FUELING_STATE                      ((uint8_t)3U)


/* Event flag to start display */
#define LCD_SEGMENT_START_DISPLAY_FLAG        (0x00000001U)

/*Queues Object define*/
#define LCDCHARACTER_QUEUE_OBJ_T               MsgForDisplay_t
#define LCDCHARACTER_QUEUE_OBJ_NUMBER          3U

#define KEYPAD_QUEUE_OBJ_T                     Keypad_Button_Type
#define KEYPAD_QUEUE_OBJ_NUMBER                10

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
                               
#define REGISTRATION_PASS   0xA5U
#define PROCESS_PASS        0x5AU
    
#define INPUT_CTRL_MASK     1
#define INPUT_CTRL_UNMASK   0

/* Event Flags set*/
#define LCDCHARACTER_CLEAR_CHAR     (1 << 0)
#define LCDCHARACTER_PUT_CHAR       (1 << 1)
#define LCDCHARACTER_PUT_STRING     (1 << 2)

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

/*Timer ID*/
osTimerId_t Timer1_Id;                            // timer id
osTimerId_t Timer2_Id;                            // timer id
osTimerId_t Timer3_Id;                            // timer id

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

osMemoryPoolId_t RxDataMemoryPool_Id,
                 LcdCharacter_MemoryPool_Id;

/* Event flag to trigger LCD start displaying */
static osEventFlagsId_t LcdSegmentStartDisplayFlagId,
                        LcdCharaterDisplayFlag_Id;

/****************************** Other Variables **********************************/
/* main and sub Address of this preset */
static uint8_t gMainAddress,gSubAddress;
//SubAddress of corresponding preset device
static uint8_t gSubAddress_Coressponding;

/* Current State of Preset*/
uint8_t gCurrentState = IDLE_STATE;
//uint8_t gCurrentState = PRINTER_SETTING;

/* Variable to handle scanning Keypad*/
volatile Std_Return_Type KeypadScaningStatus = E_OK;
/* Variable to check the same input button push multiple times */
volatile Std_Return_Type gInputDone = E_NOT_OK;
volatile InputControl_t gInputCtrl = {0};
volatile uint8_t gBlink = 0;

/* temporary array storing data input from character lcd to send to cpu */
uint8_t LcdCharacterInput[64] = {0};
uint8_t LcdCharacterInputLength = 0;

const uint8_t ErrorCodeClear[7]="       ";
const uint8_t InitialString[]="PECO10";
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
static void LcdSegment_PrepareNewInput(void);

/* Timer callback to handle input from keypad*/
void Timer1_Callback (void *arg);
void Timer2_Callback (void *arg);
void Timer3_Callback (void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

/***************************** logic function *************************************/
static uint8_t LcdSegmentData_HexToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString)
{
    uint8_t i,j,NumberOfDigit;
    i=j=NumberOfDigit=0;
    uint8_t StartOfNum = 0;
    uint8_t temp =0;

#define DOT_TYPE                1
#define COMMA_TYPE              0
#define DECIMAL_POINT_MASK      0x1U
#define DECIMAL_POINT_SHIFT     1
#define ZERO_DISPLAY_MASK       0x1U
#define ZERO_DISPLAY_SHIFT      0
    /* Ctr1 0(1B) | Ctr1 1(1B) | Row(4B) */
    /* Ctr1 0: | 1 | 0 | 0 | 0 | 0 | 0 | Comma/Dot | Leading Zero |*/
    uint8_t Zero_Display = (pData[0] >> ZERO_DISPLAY_SHIFT)  & ZERO_DISPLAY_MASK;
    uint8_t DecimalPoint = (pData[0] >> DECIMAL_POINT_SHIFT) & DECIMAL_POINT_MASK;
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
            if(DecimalPoint == DOT_TYPE)
            {
                pString[j] = '.';
            }else
            {
                pString[j] = ',';
            }
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
    if(length > 0)
    {
        memcpy(pMsgQueue.msg,pString,length);
    }
    /* push data to lcd character massage queue*/
    osMessageQueuePut(LcdCharacterMsgQueue_Id, &pMsgQueue, 0U, timeout);
}

static void LcdSegmentDisplaySetEvent(void)
{
    osEventFlagsSet(LcdSegmentStartDisplayFlagId, LCD_SEGMENT_START_DISPLAY_FLAG);
}

static void LcdSegmentProcess(uint8_t opcode,uint8_t* pData,uint8_t length)
{
    uint8_t LcdSegmentBuffer[14] = {0};
    const uint8_t CodeTest[] = "8,8,8,8,8,8,8,";
    uint8_t DigitNumbers = 0;
    uint8_t row,col,temp,temp1;
    row=col=temp=temp1 = 0;
    
#define INPUT_CTRL_ROW_MASK             0x3
#define INPUT_CTRL_ROW_SHIFT            5U
#define INPUT_CTRL_COL_MASK             0x7
#define INPUT_CTRL_COL_SHIFT            2U
#define INPUT_CTRL_HIDE_MASK            0x1
#define INPUT_CTRL_HIDE_SHIFT           1U
#define INPUT_CTRL_BLINK_MASK           0x1
#define INPUT_CTRL_BLINK_SHIFT          0U
#define INPUT_CTRL_MAX_SIZE_MASK        0xf
#define INPUT_CTRL_MAX_SIZE_SHIFT       3U
#define INPUT_CTRL_CLEAR_SIZE_MASK      0x7
#define INPUT_CTRL_CLEAR_SIZE_SHIFT     0U
#define INPUT_CTRL_ENABLE_MASK          0x1
#define INPUT_CTRL_ENABLE_SHIFT         1U
#define INPUT_CTRL_PRESET_BUTTON_MASK   0x1
#define INPUT_CTRL_PRESET_BUTTON_SHIFT  0U
    
    switch(opcode) 
    {
        case 0x10:
            /* Ctr1 0(1B) | Ctr1 1(1B) | Ctr1 2(1B) | Row 1(7B) | Row 2(7B) | Row 3(7B) */
            /* Ctrl 0    :| Bit7     | Bit6        | Bit5        | Bit4 | Bit3 | Bit2 | Bit1          | Bit0        |
                          | Flag = 1 | Row = 0,1,2 ( 3 read only)| Digit = 0~7        | Hide Mask(0,1)| blink (0|1) |
                                     | Start position of input data                   |                         
               Ctrl 1     | Bit7     | Bit6 | Bit5 | Bit4 | Bit3 |   Bit2  |   Bit1   |   Bit0  |
                          | Flag = 1 | Max number of input digit | Number of 0 digit when clear |
               Ctrl 2     | Bit7     | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 |       Bit1       |        Bit0          |
                          | Flag = 1 |   0  |   0  |   0  |   0  |   0  | en/dis inputting | en/dis Preset button |
               digit = digit | 0x80 : case digit with decimal-point*/
            Lcd_Segment_Clear_Line(0);
            Lcd_Segment_Clear_Line(1);
            Lcd_Segment_Clear_Line(2);
            for(row=0;row<LCD_SEGMENT_ROWS;row++)
            {
                for(col=0; col < LCD_SEGMENT_COLS; col++ )
                {
                    /* Check decimal-point position */
                    if((pData[3 + (row * LCD_SEGMENT_COLS) + col] & 0x80) != 0)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)",",1,row,LCD_SEGMENT_COLS - col -1);
                    }
                    /* clear MSB bit */
                    temp = pData[3 + (row * LCD_SEGMENT_COLS) + col] & 0x7F;
                    Lcd_Segment_Put_Data(&temp,1,row,LCD_SEGMENT_COLS - col -1);
                }
            }
            temp1                                  = (pData[0] >> INPUT_CTRL_ROW_SHIFT)            & INPUT_CTRL_ROW_MASK ;                  
            gInputCtrl.LcdSegment.row              = (temp1 < LCD_SEGMENT_ROWS) ? temp1 : 1 ;
            /* beacause of the difference in define segment collumn between CPU and DPL so need to convert the RX collumn value*/
            gInputCtrl.LcdSegment.col              = LCD_SEGMENT_COLS - 1 - ((pData[0] >> INPUT_CTRL_COL_SHIFT)        & INPUT_CTRL_COL_MASK);
            gInputCtrl.LcdSegment.mask             = (pData[0] >> INPUT_CTRL_HIDE_SHIFT)           & INPUT_CTRL_HIDE_MASK;
            gInputCtrl.LcdSegment.blink            = (pData[0] >> INPUT_CTRL_BLINK_SHIFT)          & INPUT_CTRL_BLINK_MASK;
            gInputCtrl.LcdSegment.max_size         = (pData[1] >> INPUT_CTRL_MAX_SIZE_SHIFT )      & INPUT_CTRL_MAX_SIZE_MASK;
            gInputCtrl.LcdSegment.clear_size       = (pData[1] >> INPUT_CTRL_CLEAR_SIZE_SHIFT)     & INPUT_CTRL_CLEAR_SIZE_MASK;
            gInputCtrl.LcdSegment.enable           = (pData[2] >> INPUT_CTRL_ENABLE_SHIFT)         & INPUT_CTRL_ENABLE_MASK;
            gInputCtrl.preset_button               = (pData[2] >> INPUT_CTRL_PRESET_BUTTON_SHIFT)  & INPUT_CTRL_PRESET_BUTTON_MASK;
            
            if(gInputCtrl.LcdSegment.blink == 1)
            {
                if(!osTimerIsRunning(Timer3_Id))
                {
                    gBlink = 1;
                    Lcd_Segment_Put_Data((uint8_t*)"_",1,gInputCtrl.LcdSegment.row,gInputCtrl.LcdSegment.max_size);
                    LcdSegmentDisplaySetEvent();
                    osTimerStart(Timer3_Id,BLINK_CURSOR_DURATION);
                }
            }else
            {
                if((osTimerIsRunning(Timer3_Id)) && (gInputCtrl.LcdSegment.blink_error == 0))
                    osTimerStop(Timer3_Id);
            }
            break;
        case 0x12:
            /* Data[0] | Data[1] 
            Data[0] : 0x80 | special_command
                    01 : Set all segments of display on
                    02 : Clear all segments
                    03 : Set Specific indicator that is set by Data[1]
            Data[1] : 0x80 | Indicator_select
                    Indicator behavior (1|0) ~ (ON|OFF)
                   | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 | 
                   |   1  |  Reserved   | Ind4 | Ind3 | Ind2 | Ind1 | Ind0 |
            */
            switch(pData[0] & 0x7F)
            {
                case 1:
                    /* Set all segments of display on */
                    Lcd_Segment_Put_Data((uint8_t*)CodeTest,strlen((char*)CodeTest),0,0);
                    Lcd_Segment_Put_Data((uint8_t*)CodeTest,strlen((char*)CodeTest),1,0);
                    Lcd_Segment_Put_Data((uint8_t*)CodeTest,strlen((char*)CodeTest),2,0);
                    break;
                case 2:
                    /* Clear all segments */
                    Lcd_Segment_Clear_Line(0);
                    Lcd_Segment_Clear_Line(1);
                    Lcd_Segment_Clear_Line(2);
                    Lcd_Segment_Put_Indicator(0);
                    break;
                case 3:
                    /* Set Specific indicator that is set by Data[1] */
                    Lcd_Segment_Put_Indicator(pData[1] & 0x7F);
                    break;
                case 4:
                    /* Stop and clear error */
                    if(gInputCtrl.ErrorCodeLength > 0)
                    {
                        if(gInputCtrl.LcdSegment.blink_error == 1)
                        {
                            gInputCtrl.LcdSegment.blink_error = 0;
                            /* turn off timer*/
                            if((osTimerIsRunning(Timer3_Id)) && (gInputCtrl.LcdSegment.blink == 0))
                                osTimerStop(Timer3_Id);
                        }
                        Lcd_Segment_Put_Data((uint8_t*)ErrorCodeClear,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
                        gInputCtrl.ErrorCodeLength = 0;
                    }
                    break;
                default:
                    break;
            }
            break;
        case 0x14:
            /* Ctr1(1B) :| Len(1B)  | Data(nB) |
               Ctrl     :| Bit7     | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1        | Bit0     |
                         | Flag = 1 | Row = 0,1,2 | Digit = 0~7        | blink (0|1) | Reserved |
            */
            gInputCtrl.LcdSegment.row_error = (pData[0] >> 5) & 0x3;
            /* beacause of the difference in define segment collumn between CPU and DPL so need to convert the receive collumn value*/
            gInputCtrl.LcdSegment.col_error = LCD_SEGMENT_COLS - 1 - (pData[0] >> 2) & 0x7;
            gInputCtrl.ErrorCodeLength = pData[1] & 0x7F;
            gInputCtrl.LcdSegment.blink_error = gInputCtrl.ErrorCodeLength > 0 ? (pData[0] >> 1) & 0x1 : 0;
            if(gInputCtrl.ErrorCodeLength > 0)
            {
                memcpy((uint8_t*)&gInputCtrl.ErrorCode,&pData[2],gInputCtrl.ErrorCodeLength);
            }
            /* Print error code */
            if(gInputCtrl.ErrorCodeLength > 0)
            {
                /* Check Error blink behavior */
                if((gInputCtrl.LcdSegment.blink_error))
                {
                    if(!osTimerIsRunning(Timer3_Id))
                    {
                        gBlink = 1;
                        Lcd_Segment_Put_Data((uint8_t*)&gInputCtrl.ErrorCode,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
                        osTimerStart(Timer3_Id,BLINK_CURSOR_DURATION);
                    }
                }else
                {
                    Lcd_Segment_Put_Data((uint8_t*)&gInputCtrl.ErrorCode,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
                }
            }
            
            break;       
        /*Control Byte : Leading Zero display | Sign(dot or comma) */
        case 0x90:
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) |Ctr1(2B) | Row 3(4B) */
            Lcd_Segment_Clear_Line(0);
            Lcd_Segment_Clear_Line(1);
            Lcd_Segment_Clear_Line(2);
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,0,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[6],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,1,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[12],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,2,0);
            break;
        case 0x91:
            Lcd_Segment_Clear_Line(0);
            Lcd_Segment_Clear_Line(1);
            /* Ctr1(2B) | Row 1(4B) |Ctr1(2B) | Row 2(4B) */
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,0,0);
            DigitNumbers = LcdSegmentData_HexToString(&pData[6],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,1,0);
            break;
        case 0x92:
            Lcd_Segment_Clear_Line(2);
            /* Ctr1(2B) | Row 3(4B) */
            DigitNumbers = LcdSegmentData_HexToString(&pData[0],6,LcdSegmentBuffer);
            Lcd_Segment_Put_Data(LcdSegmentBuffer,DigitNumbers,2,0);
            break;
    }
    LcdSegmentDisplaySetEvent();
}

static void LcdCharacterProcess(uint8_t opcode,uint8_t* pData,uint8_t length)
{
    uint16_t LenthOfRow1 = 0;
    uint16_t LenthOfRow2 = 0;
    switch(opcode) 
    {
        /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 1(n Byte) | DataLen (1 Byte) | Row 2(n Byte) */
        /* Ctr1 0  | Bit7     | Bit6      | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
                   | Flag = 1 | Row = 0,1 | Digit = 0~63                            |
           Ctr1 1  | Bit7     | Bit6      | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
                   | Flag = 1 | Reserve   | Input Max Size                          |
           Ctr1 2  | Bit7     | Bit6      | Bit5 | Bit4 |  Bit3  |  Bit2  | Bit1             | Bit0              |
                   | Flag = 1 | Reserve   |  Reserve    | Reset Max Size  | CursorEffect On/off (0/1) Blinking 2 |
        */
        case 0x80:
            gInputCtrl.LcdCharacter.row             = (pData[0] >> 6) & 0x1;
            gInputCtrl.LcdCharacter.col             = (pData[0] >> 0) & 0x3F;
            gInputCtrl.LcdCharacter.max_size        = (pData[1] >> 0) & 0x3F;
            gInputCtrl.LcdCharacter.CursorEffect    = (pData[2] >> 0) & 0x3;
            gInputCtrl.LcdCharacter.clear_size      = (pData[2] >> 2) & 0x3;
            Lcd_Clear();
            Lcd_Cursor_Effect(gInputCtrl.LcdCharacter.CursorEffect);
            // Row 1 data from position 4 length equal to pData[3]
            LenthOfRow1 = pData[3] & 0x7F;
            LcdCharacter_PutString(0,0,&pData[4],LenthOfRow1,0);
            // Row 2 data from position 5 + LenthOfRow1 length equal to pData[4 + LenthOfRow1]
            LenthOfRow2 = pData[4 + LenthOfRow1]  & 0x7F;
            LcdCharacter_PutString(1,0,&pData[4 + LenthOfRow1 + 1],LenthOfRow2,0);
            /*Move cursor to receive position*/
            LcdCharacter_PutString(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,0,0);
            break;
            
        case 0x81:
            /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 1(n Byte)*/
            // Row 1 data from position 2 length equal to pData[3]
            Lcd_Clear();
            LenthOfRow1 = pData[3] & 0x7F;
            LcdCharacter_PutString(0,0,&pData[4],LenthOfRow1,0);
            break;
        case 0x82:
            /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 2(n Byte) */
            // Row 1 data from position 1 length equal to pData[3]
            Lcd_Clear();
            LenthOfRow2 = pData[3] & 0x7F;
            LcdCharacter_PutString(1,0,&pData[4],LenthOfRow2,0);
            break;
        case 0x13:
            /* Data[0] | Data[1] 
            Data[0] : 0x80 | special_command
                    01 : clear whole display
            Data[1] : reserved
            */
            switch(pData[0] & 0x7F)
            {
                case 1:
                    Lcd_Cursor_Effect(LCD_CURSOR_OFF);
                    Lcd_Clear();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/* This function only serve Main_Preset_Task for clearing segment display to defaut value before new input*/
static void LcdSegment_PrepareNewInput(void)
{
    uint8_t j,temp_row;
    j = temp_row = 0;
    /* Clear segment display for new input*/
    for(j=0;j<gInputCtrl.LcdSegment.max_size;j++)
    {
        if((((j + gInputCtrl.LcdSegment.col) % LCD_SEGMENT_COLS) == 0) && (j != 0))
            temp_row++;
        if((j < gInputCtrl.LcdSegment.clear_size) && (gInputCtrl.LcdSegment.mask == INPUT_CTRL_UNMASK))
        {
            Lcd_Segment_Put_Data((uint8_t*)"0",1,(gInputCtrl.LcdSegment.row - temp_row),(j + gInputCtrl.LcdSegment.col) - temp_row*LCD_SEGMENT_COLS);
        }else
        {
            Lcd_Segment_Put_Data((uint8_t*)" ",1,(gInputCtrl.LcdSegment.row - temp_row),(j + gInputCtrl.LcdSegment.col) - temp_row*LCD_SEGMENT_COLS); 
        }
    }
    LcdSegmentDisplaySetEvent();
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
#define LOSS_KEY                3

static uint8_t LcdCharacter_CursorPosition_CheckRight(void)
{
    uint8_t row,col,temp,temp1,temp2;
    temp = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,gInputCtrl.LcdCharacter.max_size);
    lcd_Read_Current_Position(&row,&col);
    temp1 = row*MAX_CHARACTER_OF_LINE + col;
    temp2 = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col;
    if((temp2 + temp) > temp1)
    {
        return 1;
    }else
    {
        return 0;
    }
}

static uint8_t LcdCharacter_CursorPosition_CheckLeft(void)
{
    uint8_t row,col,temp1,temp2;
    lcd_Read_Current_Position(&row,&col);
    temp1 = row*MAX_CHARACTER_OF_LINE + col;
    temp2 = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col;
    if(temp2 < temp1 )
    {
        return 1;
    }else
    {
        return 0;
    }
}
static uint8_t SpecialKeyCheck(KeypadStatus_t *KeypadStatus)
{
    uint8_t RetValue = SPECIAL_KEY;
    uint8_t temp=0;
    if(KeypadStatus->CurrentStatus == KEYPAD_UNKNOWN)
    {
        KeypadStatus->PushTime = 0;
        if(PRINTER_SETTING_STATE == gCurrentState)
        {
            if(!((KeypadStatus->PreviousStatus == KEYPAD_ENTER) || (KeypadStatus->PreviousStatus == KEYPAD_CLEAR)))
            {
                Lcd_Move_Cursor_Right();
            }
        }
        RetValue = UNKNOWN_KEY;
    }else if(KeypadStatus->CurrentStatus == KEYPAD_LOSS)
    {
        //TO DO
    }
    else{
        if(KeypadStatus->CurrentStatus == KEYPAD_FORWARD)
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
            {
                if(LcdCharacter_CursorPosition_CheckRight() == 1)
                {
                    Lcd_Move_Cursor_Right();
                }
                else
                {
                    temp = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,gInputCtrl.LcdCharacter.max_size);
                    LcdCharacter_PutString(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,0,0);
                }
            }
        }else if(KeypadStatus->CurrentStatus == KEYPAD_BACKWARD)
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
            {
                if(LcdCharacter_CursorPosition_CheckLeft() == 1)
                {
                    Lcd_Move_Cursor_Left();
                }
                else
                {
                    temp = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,gInputCtrl.LcdCharacter.max_size);
                    temp = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col + temp;
                    LcdCharacter_PutString(temp/MAX_CHARACTER_OF_LINE,temp%MAX_CHARACTER_OF_LINE,NULL,0,0);
                }
                
            }
        }else if(KeypadStatus->CurrentStatus == KEYPAD_FONT)
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
                Keypad_Change_Font();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_ENDLINE)
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
                Lcd_Move_Cursor_Next_Line();
        }else if(KeypadStatus->CurrentStatus == KEYPAD_CLEAR)
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
            {
                /*genious code*/
                osEventFlagsSet(LcdCharaterDisplayFlag_Id, LCDCHARACTER_CLEAR_CHAR);
            }
        }else if(KeypadStatus->CurrentStatus == KEYPAD_ENTER)
        {
            //TEST
            
        }else
        {
            if(PRINTER_SETTING_STATE == gCurrentState)
            {
                if(KeypadStatus->PreviousStatus == KeypadStatus->CurrentStatus)
                {
                    /*CHANGE*/
                    KeypadStatus->PushTime = KeypadStatus->PushTime + 1;
                }else
                {
                    KeypadStatus->PushTime = 0;
                    if(!((KeypadStatus->PreviousStatus == KEYPAD_UNKNOWN)||(KeypadStatus->PreviousStatus == KEYPAD_BACKWARD)
                        ||(KeypadStatus->PreviousStatus == KEYPAD_FORWARD)||(KeypadStatus->PreviousStatus == KEYPAD_CLEAR)
                        ||(KeypadStatus->PreviousStatus == KEYPAD_ENDLINE)||(KeypadStatus->PreviousStatus == KEYPAD_FONT)
                        ||(KeypadStatus->PreviousStatus == KEYPAD_ENTER)))
                        Lcd_Move_Cursor_Right();
                }
            }
            RetValue = NORMAL_KEY;
        }
        if(PRINTER_SETTING_STATE != gCurrentState)
        {
            KeypadStatus->PushTime = KeypadStatus->PushTime + 1;
        }
    }
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
    uint32_t   LcdCharacterEventsFlag;
    MsgForDisplay_t data1;
    uint8_t KeyPushed = 0;
    Keypad_Button_Type CurrentStatus,PreviousStatus,BreakTimeStatus;
    BreakTimeStatus = PreviousStatus = CurrentStatus = KEYPAD_UNKNOWN;
    
    Lcd_Init_4bits_Mode();
    LcdCharacter_PutString(0,(MAX_CHARACTER_OF_DISPLAY - strlen((char*)InitialString))/2,(uint8_t*)InitialString,strlen((char*)InitialString),0);
    osTimerStart(Timer1_Id,KEYPAD_SCANNING_DURATION);
    
    while(1)
    {
        /* Lcd character print*/
        /* pop data from lcd character massage queue*/
        LcdCharacterEventsFlag = osEventFlagsWait(LcdCharaterDisplayFlag_Id, LCDCHARACTER_CLEAR_CHAR, osFlagsWaitAny, 0);
        if(LcdCharacterEventsFlag == LCDCHARACTER_CLEAR_CHAR)
        {
            Lcd_Clear_Character();
        }
        LcdQueueStatus = osMessageQueueGet(LcdCharacterMsgQueue_Id, &data1, NULL, 0U);   // wait for message
        if (LcdQueueStatus == osOK) {
            if((data1.line == LCD_PRINT_CHARACTER) || (data1.offset == LCD_PRINT_CHARACTER))
            {
                Lcd_Put_Char(data1.msg[0]);
            }else
            {
                Lcd_Put_String(data1.line,data1.offset,data1.msg,data1.length);
            }
            osMemoryPoolFree(LcdCharacter_MemoryPool_Id,data1.msg);
        }
        
        /* Keypad scan period define in macro KEYPAD_SCANNING_DURATION in ms */
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
#if (TEST_CODE == 1)
    uint8_t num[10]="";
    uint8_t False_num[10]="";
    
    uint16_t False_Msg_number = 0;
    uint8_t invalid_format = 0;
    uint16_t Msg_number = 0;
    uint16_t feadback = 0;
#endif
    uint8_t OpCode=0;
    uint16_t Length=0;
    uint8_t *RxData;
    uint8_t StateSwitch = 0;
    
    RxData = (uint8_t*)osMemoryPoolAlloc(RxDataMemoryPool_Id,0U);
    
    RS485_Init(gMainAddress);
	while(1)
	{
        /* Waitting for command from CPU */
        RS485_Wait_For_Message(&OpCode, (uint8_t*)RxData, &Length, osWaitForever);
        if (E_OK == retValue)
        {
#if (TEST_CODE == 1)
            Msg_number++;
            if(Msg_number == 0)
                LcdCharacter_PutString(0,0,(uint8_t*)"     ",5,0);
            DecToString(num,Msg_number);
            LcdCharacter_PutString(0,0,(uint8_t*)num,strlen((char*)num),0);
#endif
            /*Notify the command to relevant task*/
            switch(OpCode)
            {
#if (TEST_CODE == 1)
                case 0xff:
                {
                    False_Msg_number++;
                    DecToString(False_num,False_Msg_number);
                    LcdCharacter_PutString(0,12,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                }
                case 0xfe:
                {
                    invalid_format++;
                    DecToString(False_num,invalid_format);
                    LcdCharacter_PutString(1,12,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                }
                case 0xfc:
                {
                    feadback++;
                    if(feadback == 0)
                        LcdCharacter_PutString(1,0,(uint8_t*)"     ",5,0);
                    DecToString(False_num,feadback);
                    LcdCharacter_PutString(1,0,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                }
                case 0x05:
                {
                    LcdCharacter_PutString(0,7,(uint8_t*)"POL",3,0);
                    break;
                }
#endif
                /* Switch input handling state*/
                case 0x11:
                {
                    /* Data = Data | 0x80 */
                    StateSwitch = RxData[0] & 0x7F;
                    switch(StateSwitch)
                    {
                        case 0:
                            gCurrentState = IDLE_STATE;
                            Lcd_DisplayShift_Turn_Off();
                            break;
                        case 0x01:
                            /*Go to system setting mode*/
                            gCurrentState = PUMP_SETTING_STATE;
                            Lcd_DisplayShift_Turn_Off();
                            break;
                        case 0x02:
                            /*Go to printing mode*/
                            gCurrentState = PRINTER_SETTING_STATE;
                            Lcd_DisplayShift_Turn_On();
                            break;
                        case 0x03:
                            /*Go to fueling mode*/
                            gCurrentState = FUELING_STATE;
                            Lcd_DisplayShift_Turn_Off();
                            break;
                        default:
                            gCurrentState = IDLE_STATE;
                            Lcd_DisplayShift_Turn_Off();
                    }
                    break;
                }
                /*Special commands for character Lcd*/
                case 0x13:
                /*Dislay character Lcd hex type*/
                case 0x80:
                case 0x81:
                case 0x82:
                    LcdCharacterProcess(OpCode,RxData,Length);
                    break;
                /*Special commands for segment Lcd*/
                case 0x12:
                case 0x14:
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
    uint8_t key = 0;
    uint8_t SegmentKey[14];
    uint8_t Checking = NORMAL_KEY;
    uint8_t i=0;
    
    osStatus_t status;
    KeypadStatus_t KeypadStatus = {KEYPAD_UNKNOWN,KEYPAD_UNKNOWN,0};
    Lcd_Segment_Put_Indicator(INDICATOR_A1|INDICATOR_A3|INDICATOR_A4|INDICATOR_A5);
    LcdSegmentDisplaySetEvent();
    
    //KeypadStatus_t KeypadStatus = {KEYPAD_UNKNOWN,KEYPAD_UNKNOWN,0};
    Password_t passcheck_1 = {(uint8_t*)Password1, sizeof(Password1),NO_PASSWORD_CHECK};
    Password_t passcheck_2 = {(uint8_t*)Password2, sizeof(Password2),NO_PASSWORD_CHECK};
#if (TEST_CODE == 0)
    Password_t passcheck_3 = {(uint8_t*)Password3, sizeof(Password3),NO_PASSWORD_CHECK};
#endif
    gInputCtrl.LcdCharacter.max_size = 80;
    uint8_t Registration_Pass = REGISTRATION_PASS;
    uint8_t Process_Pass      = PROCESS_PASS;
    while(1)
    {
        status = osMessageQueueGet(KeypadInputQueue_Id, &(KeypadStatus.CurrentStatus), NULL, osWaitForever);
        if(status == osOK)
        {
            Checking = SpecialKeyCheck(&KeypadStatus);
            switch(gCurrentState)
            {
                case IDLE_STATE:
                {
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_1))
                    {
                        RS485_Prepare_To_Transmit(0xA0,&Registration_Pass,1);
                        
#if (TEST_CODE == 1)
                        Lcd_Segment_Put_Data((uint8_t*)"code",4,2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentState = PUMP_SETTING_STATE;
                        gInputCtrl.mask = INPUT_CTRL_MASK;
#endif
                    }
#if (TEST_CODE == 0)
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_3))
                    {
                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)"XXXXX",5);
                        Lcd_Segment_Put_Data((uint8_t*)"print",5,2,0);
                        LcdSegmentDisplaySetEvent();
                        /*reset lcd character*/
                        Lcd_Clear();
                        Lcd_DisplayShift_Turn_On();
                        gCurrentState = PRINTER_SETTING_STATE;
                        Lcd_Turn_On_Cursor();
                    }
#endif
                    if(Checking == NORMAL_KEY)
                    {
                        
                    }
                    break;
                }
                case FUELING_STATE:
                {
                    break;
                }
                case PUMP_SETTING_STATE:
                {
                    if(E_OK == Password_Check(&KeypadStatus,&passcheck_2))
                    {
                        RS485_Prepare_To_Transmit(0xA0,&Process_Pass,3);
#if (TEST_CODE == 1)
                        Lcd_Segment_Put_Data((uint8_t*)"00     ",7,2,0);
                        LcdSegmentDisplaySetEvent();
                        gInputCtrl.mask = INPUT_CTRL_UNMASK;
#endif
                        
                    }else 
                    {
                        if(Checking != UNKNOWN_KEY)
                        {
                            if(Checking == NORMAL_KEY)
                            {
                                key = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,0);
                                if(gInputCtrl.LcdSegment.enable == 1)
                                {
                                    if(i < gInputCtrl.LcdSegment.max_size)
                                    {
                                        if(i==0)
                                        {
                                            LcdSegment_PrepareNewInput();
                                        }
                                        if(gInputCtrl.LcdSegment.mask == INPUT_CTRL_MASK)
                                        {
                                            SegmentKey[i]=key;
                                            if(i < 14)
                                            {
                                                if(i >= (7-gInputCtrl.LcdSegment.col))
                                                {
                                                    Lcd_Segment_Put_Data((uint8_t*)"-",1,(gInputCtrl.LcdSegment.row - 1),i - 7);
                                                }else
                                                {
                                                    Lcd_Segment_Put_Data((uint8_t*)"-",1,gInputCtrl.LcdSegment.row,i);
                                                }
                                                i++;
                                            }
                                        }else if(gInputCtrl.LcdSegment.mask == INPUT_CTRL_UNMASK)
                                        {
                                            if(i < 14)
                                            {
                                                SegmentKey[i]=key;
                                                i++;
                                                if(i >= (7-gInputCtrl.LcdSegment.col))
                                                {
                                                    Lcd_Segment_Put_Data((uint8_t*)SegmentKey,(i-7+gInputCtrl.LcdSegment.col),gInputCtrl.LcdSegment.row - 1,0);
                                                    Lcd_Segment_Put_Data((uint8_t*)&SegmentKey[i- 7 + gInputCtrl.LcdSegment.col],(7 - gInputCtrl.LcdSegment.col),gInputCtrl.LcdSegment.row,gInputCtrl.LcdSegment.col);
                                                }else
                                                {
                                                    Lcd_Segment_Put_Data((uint8_t*)SegmentKey,i,gInputCtrl.LcdSegment.row,gInputCtrl.LcdSegment.col);
                                                }
                                            }
                                        }
                                        LcdSegmentDisplaySetEvent();
                                    }
                                }
                            }else if(Checking == SPECIAL_KEY)
                            {
                                if((KeypadStatus.CurrentStatus == KEYPAD_ENTER) || (KeypadStatus.CurrentStatus == KEYPAD_CLEAR))
                                {
                                    if(KeypadStatus.CurrentStatus == KEYPAD_ENTER)
                                    {
                                        RS485_Prepare_To_Transmit(0xA1,(uint8_t*)SegmentKey,i);
                                    }
                                    sprintf((char*)SegmentKey,"");
                                    i=0;
                                }else 
                                {
                                    if((gInputCtrl.preset_button == 1) && (gInputCtrl.LcdSegment.enable == 1))
                                    {
                                        key = Keypad_Mapping(KeypadStatus.CurrentStatus);
                                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)&key,1);
                                        sprintf((char*)SegmentKey,"");
                                        i=0;
                                    }
                                }
                            }
                        }else
                        {
                            if((KeypadStatus.PreviousStatus == KEYPAD_CLEAR) && (gInputCtrl.LcdSegment.enable == 1))
                            {
                                LcdSegment_PrepareNewInput();
                            }
                        }
                    }
                    break;
                }
                case PRINTER_SETTING_STATE:
                {
                    if(Checking == NORMAL_KEY)
                    {
                        key = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,KeypadStatus.PushTime);
                        /*stupid code*/
                        LcdCharacter_PutString(LCD_PRINT_CHARACTER,LCD_PRINT_CHARACTER,&key,1,0);
                    }else if(Checking == SPECIAL_KEY)
                    {
                        if(KeypadStatus.CurrentStatus == KEYPAD_ENTER)
                        {
                            LcdCharacterInputLength = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,(uint8_t*)&LcdCharacterInput,gInputCtrl.LcdCharacter.max_size);
                            RS485_Prepare_To_Transmit(0xA2,(uint8_t*)&LcdCharacterInput,LcdCharacterInputLength);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            /* backup previous input status */
            KeypadStatus.PreviousStatus = KeypadStatus.CurrentStatus;
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

void Timer3_Callback (void *arg)
{
    gBlink = gBlink ^1;
    if(gInputCtrl.LcdSegment.blink == 1)
    {
        if(gBlink == 1)
        {
            Lcd_Segment_Put_Data((uint8_t*)"_",1,gInputCtrl.LcdSegment.row,gInputCtrl.LcdSegment.max_size);
        }else
        {
            Lcd_Segment_Put_Data((uint8_t*)" ",1,gInputCtrl.LcdSegment.row,gInputCtrl.LcdSegment.max_size);
        }
    }
    if(gInputCtrl.LcdSegment.blink_error == 1)
    {
        if(gBlink == 1)
        {
            Lcd_Segment_Put_Data((uint8_t*)gInputCtrl.ErrorCode,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
        }else
        {
            Lcd_Segment_Put_Data((uint8_t*)ErrorCodeClear,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
        }
        LcdSegmentDisplaySetEvent();
    }
    LcdSegmentDisplaySetEvent();
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
  
  /* Initialize event flag for start display LCD event */
  LcdSegmentStartDisplayFlagId = osEventFlagsNew(NULL);
  LcdCharaterDisplayFlag_Id    = osEventFlagsNew(NULL);
    
  /*Create pool data*/
  RxDataMemoryPool_Id = osMemoryPoolNew(RXDATA_POOL_ELEMENT_NUMBER, RXDATA_POOL_ELEMENT_SIZE, NULL);
  LcdCharacter_MemoryPool_Id = osMemoryPoolNew(LCDCHARACTER_POOL_ELEMENT_NUMBER, LCDCHARACTER_POOL_ELEMENT_SIZE, NULL);
  
  /*Create os timers*/
  Timer1_Id = osTimerNew(Timer1_Callback, osTimerPeriodic, NULL, NULL);
  Timer2_Id = osTimerNew(Timer2_Callback, osTimerOnce, NULL, NULL);
  Timer3_Id = osTimerNew(Timer3_Callback, osTimerPeriodic, NULL, NULL);
  
  HAL_TIM_Base_Start(&htim2);
  /*use for backlight character lcd*/
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
 
  osKernelStart();                      // Start thread execution

}

