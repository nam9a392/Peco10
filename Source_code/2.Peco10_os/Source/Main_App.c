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
#define DISPLAY_STRING              0
#define DISPLAY_INVERSE_STRING      1
#define DISPLAY_CHARACTER           2
#define CLEAR_CHARACTER             3

/*
*   @inplement: enumeration_MsgForDisplay_t
*   @brief: the massage type using for queue that comunicate  
*       PreviousStatus          : the nearest state that receive from keypad (Keypad_Button_Type)
*       CurrentStatus           : the current state that receive from keypad (Keypad_Button_Type)
*       PushTime                : number of pushing the same button continously
*/
typedef struct {
  uint8_t  *msg;
  uint16_t length;
  uint8_t  line;
  uint8_t  offset;
  uint8_t  type;
} MsgForDisplay_t;

/*
*   @inplement: enumeration_KeypadStatus_t
*   @brief: Hold the input state from keypad
*       PreviousStatus          : the nearest state that receive from keypad (Keypad_Button_Type)
*       CurrentStatus           : the current state that receive from keypad (Keypad_Button_Type)
*       PushTime                : number of pushing the same button continously
*/
typedef struct{
    Keypad_Button_Type  PreviousStatus;
    Keypad_Button_Type  CurrentStatus;
    uint8_t             PushTime;
}KeypadStatus_t;

/*
*   @inplement: enumeration_LcdSegmentInputControl_t
*   @brief: Hold the state to control display segment lcd
*       row,col                 : start position of input
*       max_size                : number of digit allowed to insert
*       clear_size              : number of 0 display after push clear button
*       mask                    : 1 hide input | 0 show input
*       blink                   : Cursor blinking : 1 blink   | 0 not blink
*       blink_error             : Error code blinking : 1 blink   | 0 not blink
*       enable                  : 1 allow insert data from keypad | 0 negative
*       row_error,col_error     : error code display position
*       buf[16]                 : temporary buffer storing input data display segment lcd to send to cpu
*       len                     : Input data length
*/
typedef struct{
    uint8_t  row;
    uint8_t  col;
    uint8_t  row_error;
    uint8_t  col_error;
    uint8_t  max_size;
    uint8_t  clear_size;
    uint8_t  mask;
    uint8_t  blink;
    uint8_t  enable;
    uint8_t  blink_error;
    uint8_t  buf[16];
    uint8_t  len;
}LcdSegmentInputControl_t;

/*
*   @inplement: enumeration_LcdCharacterInputControl_t
*   @brief: Hold the state to control display character lcd
*       row,col                 : start position of input
*       max_size                : number of digit allowed to insert
*       clear_size              : number of 0 display after push clear button
*       CursorEffect            : 0 Turn off | 1 Turn on | 2 Blinking
*       buf[64]                 : temporary buffer storing input data display character lcd to send to cpu 
*       len                     : Input data length
*/
typedef struct{
    uint8_t  row;
    uint8_t  col;
    uint8_t  max_size;
    uint8_t  clear_size;
    uint8_t  CursorEffect;
    uint8_t  buf[64];
    uint8_t  len;
    uint8_t  enable;
}LcdCharacterInputControl_t;

/*
*   @inplement: enumeration_InputControl_t
*   @brief: Hold the state to control input from keypad
*       LcdSegment      : group state control lcd segment display
*       LcdCharacter    : group state control lcd charcacter display
*       ErrorCode       : buffer storring current error code from keyboard
*       ErrorCodeLength : error code length 
*       preset_button   : 1 alow using P1~P4 button | 0 negative
*/
typedef struct{
    LcdSegmentInputControl_t    LcdSegment;
    LcdCharacterInputControl_t  LcdCharacter;
    uint8_t ErrorCodeLength;
    uint8_t ErrorCode[7];
    uint8_t preset_button;
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


#define NO_PASSWORD_CHECK       0
#define PASSWORD_CHECKING       1

#define NORMAL_KEY              0
#define SPECIAL_KEY             1
#define UNKNOWN_KEY             2
#define LOSS_KEY                3
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
             CommandHandler_Task_Id,
             Main_Preset_Task_Id;

/* Queues ID*/
osMessageQueueId_t LcdCharacterMsgQueue_Id,
                   KeypadInputQueue_Id;

/*Timer ID*/
osTimerId_t Timer1_Id;                            // timer id
osTimerId_t Timer2_Id;                            // timer id
osTimerId_t Timer3_Id;                            // timer id

/*Mutex ID*/
osMutexId_t LcdCharacter_Mutex_Id;

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
volatile uint8_t gCurrentState = IDLE_STATE;
//uint8_t gCurrentState = PRINTER_SETTING;

/* Variable to handle scanning Keypad*/
volatile Std_Return_Type KeypadScaningStatus = E_OK;
/* Variable to check the same input button push multiple times */
volatile Std_Return_Type gInputDone = E_NOT_OK;
volatile InputControl_t gInputCtrl = {0};
volatile uint8_t gBlink = 0;

/* Combine keys to change input state */
const uint8_t Registration_CombineKeys[]  = {KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_CLEAR,KEYPAD_ENTER,KEYPAD_CLEAR};
const uint8_t DirectSetting_CombineKeys[] = {KEYPAD_CLEAR,KEYPAD_CLEAR,KEYPAD_CLEAR};
const uint8_t Printing_CombineKeys[]      = {KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER,KEYPAD_ENTER};

const uint8_t Registration_Passcode  = REGISTRATION_PASS;
const uint8_t DirectSetting_Passcode = PROCESS_PASS;

const uint8_t cErrorCodeClear[8]="        ";
const uint8_t cCodeTest[] = "8,8,8,8,8,8,8,";
const uint8_t cInitialString[]="PECO10";


/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length, uint8_t action_type, uint32_t timeout);
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
static void SwitchInputState(uint8_t state);
static void ResetInputCtrlStatus(void);

/* Timer callback to handle input from keypad*/
void Timer1_Callback (void *arg);
void Timer2_Callback (void *arg);
void Timer3_Callback (void *arg);
/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/

/***************************** logic function *************************************/
static void SwitchInputState(uint8_t state)
{
    gInputCtrl.LcdCharacter.len = 0;
    gInputCtrl.LcdSegment.len   = 0;
    gInputCtrl.LcdCharacter.row = 0;
    gInputCtrl.LcdCharacter.len = 80;
    gInputCtrl.LcdCharacter.col = 0;
    Lcd_ShiftDisplay_Turn_Off();
    switch(state)
    {
        case 0:
            gCurrentState = IDLE_STATE;
            break;
        case 0x01:
            /*Go to system setting mode*/
            gCurrentState = PUMP_SETTING_STATE;
            break;
        case 0x02:
            /*Go to printing mode*/
            gCurrentState = PRINTER_SETTING_STATE;
            Lcd_ShiftDisplay_Turn_On();
            break;
        case 0x03:
            /*Go to fueling mode*/
            gCurrentState = FUELING_STATE;
            break;
        default:
            gCurrentState = IDLE_STATE;
    }
}
static uint8_t LcdSegmentData_HexToString(uint8_t *pData,uint8_t DataLength, uint8_t *pString)
{
    uint8_t i,j,NumberOfDigit;
    i=j=NumberOfDigit=0;
    uint8_t StartOfNum = 0;
    uint8_t temp =0;

#define DOT_TYPE                        1
#define COMMA_TYPE                      0
#define DECIMAL_POINT_MASK              0x1U
#define DECIMAL_POINT_SHIFT             1
#define ZERO_DISPLAY_MASK               0x1U
#define ZERO_DISPLAY_SHIFT              0
#define DIGITLENGTH_DISPLAY_MASK        0x7U
#define DIGITLENGTH_DISPLAY_SHIFT       2
    /* Ctr1 0(1B) | Ctr1 1(1B) | Row(4B) */
    /* Ctr1 0: | Flag = 1 | 0 | 0 |  0   |  0   |  0   | Comma/Dot | Leading Zero |*/
    /*         |                  |    Digit Length    |  */
    uint8_t Zero_Display = (pData[0] >> ZERO_DISPLAY_SHIFT)         & ZERO_DISPLAY_MASK;
    uint8_t DecimalPoint = (pData[0] >> DECIMAL_POINT_SHIFT)        & DECIMAL_POINT_MASK;
    uint8_t digit_length = (pData[0] >> DIGITLENGTH_DISPLAY_SHIFT)  & DIGITLENGTH_DISPLAY_MASK;
    /* Decimal point position on row*/
    /* Ctr1 1: | Flag = 1 | Digit 6  | Digit 5  | Digit 4  | Digit 3  | Digit 2  | Digit 1  | Digit 0 |*/

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
        if(((temp != 0)&&(StartOfNum == 0)) || (i == (((DataLength-2)*2) - digit_length)))
        {
            StartOfNum = 1;
        }
        if(StartOfNum == 1)
        {
            pString[j] = temp + 0x30;
            j++;
        }
//        if((i == ((DataLength-2)*2 - 1)) && (StartOfNum == 0))
//        {
//            pString[j] = '0';
//            j++;
//        }
        /*Add decimal point to number*/
        if(0 != (pData[1] & (0x1 << (7-i))))
        {
            if(StartOfNum == 0)
            {
                StartOfNum = 1;
                pString[j] = '0';
                j++;
            }
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
static void LcdCharacter_PutString(uint8_t line, uint8_t offset, uint8_t *pString, uint16_t length, uint8_t action_type, uint32_t timeout)
{
    MsgForDisplay_t pMsgQueue;
    pMsgQueue.msg = (uint8_t*)osMemoryPoolAlloc(LcdCharacter_MemoryPool_Id,0U);
    /*prepare queue data*/
    pMsgQueue.line=line;
    pMsgQueue.offset=offset;
    pMsgQueue.length=length;
    pMsgQueue.type = action_type;
    if((length > 0)&&(pString != NULL))
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
    uint8_t DigitNumbers = 0;
    uint8_t row,col,temp,temp1;
    row=col=temp=temp1= 0;
    
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
            /* Display Ascii type*/
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
                    04 : Stop display and clear receive error code
            Data[1] : 0x80 | Indicator_select
                    Indicator behavior (1|0) ~ (ON|OFF)
                   | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 | 
                   |   1  |  Reserved   | Ind4 | Ind3 | Ind2 | Ind1 | Ind0 |
            */
            switch(pData[0] & 0x7F)
            {
                case 1:
                    /* Set all segments of display on */
                    for (row=0; row < LCD_SEGMENT_ROWS; row++)
                    {
                        Lcd_Segment_Put_Data((uint8_t*)cCodeTest,LCD_SEGMENT_COLS*2,row,0);
                    }
                    break;
                case 2:
                    /* Clear all segments */
                    for (row=0; row < LCD_SEGMENT_ROWS; row++)
                    {
                        Lcd_Segment_Clear_Line(row);
                    }
                    /* Clear input buffer*/
                    gInputCtrl.LcdSegment.len = 0;
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
                        Lcd_Segment_Put_Data((uint8_t*)cErrorCodeClear,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
                        gInputCtrl.ErrorCodeLength = 0;
                    }
                    break;
                default:
                    break;
            }
            break;
        case 0x14:
            /* Ctr1(1B)  | Len(1B)  | Data(nB) |
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
        /* Display Short Hex type
           1 Byte 0xab convert into ascii 2 digits "ab" 
        */
        /*Ctr1(2B) : Leading Zero display | Sign(dot or comma) */
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
    osMutexAcquire(LcdCharacter_Mutex_Id, osWaitForever);
    switch(opcode) 
    {
        /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 1(n Byte) | DataLen (1 Byte) | Row 2(n Byte) */
        /* Ctr1 0  | Bit7     | Bit6      | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
                   | Flag = 1 | Row = 0,1 | Digit = 0~63                            |
           Ctr1 1  | Bit7     | Bit6  | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0     |
                   | Flag = 1 |  Input Max Size                          | Enable   |
           Ctr1 2  | Bit7     | Bit6      | Bit5 | Bit4 |  Bit3  |  Bit2  | Bit1             | Bit0              |
                   | Flag = 1 | Reserve   |  Reserve    | Reset Max Size  | CursorEffect On/off (0/1) Blinking 2 |
        */
        case 0x80:
            gInputCtrl.LcdCharacter.row             = (pData[0] >> 6) & 0x1;
            gInputCtrl.LcdCharacter.col             = (pData[0] >> 0) & 0x3F;
            gInputCtrl.LcdCharacter.enable          = (pData[1] >> 0) & 0x1;
            gInputCtrl.LcdCharacter.max_size        = (pData[1] >> 1) & 0x3F;
            gInputCtrl.LcdCharacter.CursorEffect    = (pData[2] >> 0) & 0x3;
            gInputCtrl.LcdCharacter.clear_size      = (pData[2] >> 2) & 0x3;
            Lcd_Clear();
            Lcd_Cursor_Effect(gInputCtrl.LcdCharacter.CursorEffect);
            // Row 1 data from position 4 length equal to pData[3]
            LenthOfRow1 = pData[3] & 0x7F;
            LcdCharacter_PutString(0,0,&pData[4],LenthOfRow1,DISPLAY_STRING,0);
            // Row 2 data from position 5 + LenthOfRow1 length equal to pData[4 + LenthOfRow1]
            LenthOfRow2 = pData[4 + LenthOfRow1]  & 0x7F;
            LcdCharacter_PutString(1,0,&pData[4 + LenthOfRow1 + 1],LenthOfRow2,DISPLAY_STRING,0);
            /*Move cursor to receive position*/
            LcdCharacter_PutString(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,0,DISPLAY_STRING,0);
            break;
            
        case 0x81:
            /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 1(n Byte)*/
            // Row 1 data from position 2 length equal to pData[3]
            Lcd_Clear();
            LenthOfRow1 = pData[3] & 0x7F;
            LcdCharacter_PutString(0,0,&pData[4],LenthOfRow1,DISPLAY_STRING,0);
            break;
        case 0x82:
            /* Ctr1 0(1Byte) | Ctr1 1(1Byte) | Ctr1 2(1Byte) | DataLen (1 Byte) | Row 2(n Byte) */
            // Row 1 data from position 1 length equal to pData[3]
            Lcd_Clear();
            LenthOfRow2 = pData[3] & 0x7F;
            LcdCharacter_PutString(1,0,&pData[4],LenthOfRow2,DISPLAY_STRING,0);
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
                    /* Clear Input buffer*/
                    gInputCtrl.LcdCharacter.len = 0;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    osMutexRelease(LcdCharacter_Mutex_Id);
}

/* This function only serve Main_Preset_Task for clearing segment display to defaut value before new input*/
static void LcdSegment_PrepareNewInput(void)
{
    uint8_t j;
    j = 0;
    /* Clear segment display for new input*/
    for(j=0;j<gInputCtrl.LcdSegment.max_size;j++)
    {
        if((j < gInputCtrl.LcdSegment.clear_size) && (gInputCtrl.LcdSegment.mask == INPUT_CTRL_UNMASK))
        {
            Lcd_Segment_Put_Data((uint8_t*)"0",1,(gInputCtrl.LcdSegment.row - (j + gInputCtrl.LcdSegment.col) / LCD_SEGMENT_COLS),(j + gInputCtrl.LcdSegment.col)%LCD_SEGMENT_COLS);
        }else
        {
            Lcd_Segment_Put_Data((uint8_t*)" ",1,(gInputCtrl.LcdSegment.row - (j + gInputCtrl.LcdSegment.col) / LCD_SEGMENT_COLS),(j + gInputCtrl.LcdSegment.col)%LCD_SEGMENT_COLS); 
        }
    }
    LcdSegmentDisplaySetEvent();
}
/***************************** Function to handle input data from keypad *************************************/

static uint8_t LcdCharacter_CursorPosition_CheckRight(void)
{
    uint8_t row,col,temp,temp1,temp2;
    temp = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,gInputCtrl.LcdCharacter.max_size);
    lcd_Read_Current_Position(&row,&col);
    temp1 = row*MAX_CHARACTER_OF_LINE + col;
    temp2 = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col;
    if(temp1 >= gInputCtrl.LcdCharacter.max_size + temp2 - 1)
    {
        return 0;
    }else
    {
        if((temp2 + temp ) > temp1)
        {
            return 1;
        }else
        {
            return 0;
        }
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
                if(LcdCharacter_CursorPosition_CheckRight() == 1)
                {
                    Lcd_Move_Cursor_Right();
                }
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
                    LcdCharacter_PutString(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,NULL,0,DISPLAY_STRING,0);
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
                    if(temp == gInputCtrl.LcdCharacter.max_size)
                    {
                        temp = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col + temp - 1;
                    }else
                    {
                        temp = gInputCtrl.LcdCharacter.row*MAX_CHARACTER_OF_LINE + gInputCtrl.LcdCharacter.col + temp;
                    }
                    LcdCharacter_PutString(temp / MAX_CHARACTER_OF_LINE, temp % MAX_CHARACTER_OF_LINE, NULL, 0, DISPLAY_STRING, 0);
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
                    if(KEYPAD_NORMAL_KEY_CHECK(KeypadStatus->PreviousStatus))
                    {
                        if(LcdCharacter_CursorPosition_CheckRight() == 1)
                        {
                            Lcd_Move_Cursor_Right();
                        }
                    }
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

static void ResetInputCtrlStatus(void)
{
    gInputCtrl.preset_button  = STD_OFF;
    gInputCtrl.LcdSegment.col = 0;
    gInputCtrl.LcdSegment.row = 1;
    gInputCtrl.LcdSegment.len = 0;
    gInputCtrl.LcdSegment.col_error = 5;
    gInputCtrl.LcdSegment.row_error = 2;
    gInputCtrl.LcdSegment.max_size = LCD_SEGMENT_COLS * 2;
    gInputCtrl.LcdCharacter.max_size = 16;
    gInputCtrl.LcdCharacter.col = 0;
    gInputCtrl.LcdCharacter.row = 0;
    gInputCtrl.LcdCharacter.enable = 1;
}
/************************************ Task handler *******************************************/
static void LcdSegmentDisplay_Task(void *argument)
{
    //uint32_t flag;
    /* Lcd Segment initializing*/
    Lcd_Segment_Init();
//    Lcd_Segment_PWM_Config();
//    Lcd_Segment_PWM_Start();
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
    
    /* Lcd Charater initializing*/
    Lcd_Init_4bits_Mode();
    //LcdCharacter_PutString(0,(MAX_CHARACTER_OF_DISPLAY - strlen((char*)cInitialString))/2,(uint8_t*)cInitialString,strlen((char*)cInitialString),DISPLAY_STRING,0);
    osTimerStart(Timer1_Id,KEYPAD_SCANNING_DURATION);
#if (TEST_CODE == 1)
    LcdCharacter_PutString(0,0,(uint8_t*)"PF2 STATE: ",4,DISPLAY_STRING,0);
    GPIO_PinState pf2_pre_state,pf2_cur_state;
    pf2_pre_state = pf2_cur_state = GPIO_PIN_RESET;
    pf2_cur_state = HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_2);
    if(pf2_cur_state)
        LcdCharacter_PutString(1,0,(uint8_t*)"HIGH",4,DISPLAY_STRING,0);
    else
        LcdCharacter_PutString(1,0,(uint8_t*)"LOW ",4,DISPLAY_STRING,0);
#endif
//    uint8_t j,temp;
    while(1)
    {
        /* Lcd character print*/
        /* pop data from lcd character massage queue*/
#if (TEST_CODE == 1)
            pf2_cur_state = HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_2);
//            if(pf2_cur_state != pf2_pre_state)
//            {
                if(pf2_cur_state)
                    LcdCharacter_PutString(1,0,(uint8_t*)"HIGH",4,DISPLAY_STRING,0);
                else
                    LcdCharacter_PutString(1,0,(uint8_t*)"LOW",4,DISPLAY_STRING,0);
//            }
#endif
        LcdCharacterEventsFlag = osEventFlagsWait(LcdCharaterDisplayFlag_Id, LCDCHARACTER_CLEAR_CHAR, osFlagsWaitAny, 0);
        if(LcdCharacterEventsFlag == LCDCHARACTER_CLEAR_CHAR)
        {
            Lcd_Clear_Character();
        }
        LcdQueueStatus = osMessageQueueGet(LcdCharacterMsgQueue_Id, &data1, NULL, 0U);   // wait for message
        if (LcdQueueStatus == osOK) {
            if(data1.type == DISPLAY_STRING)
            {
                Lcd_Put_String(data1.line,data1.offset,data1.msg,data1.length);
            }else if(data1.type == DISPLAY_CHARACTER)
            {
                Lcd_Put_Char(data1.msg[0]);
            }else if(data1.type == CLEAR_CHARACTER)
            {
                Lcd_Clear_Character();
            }
//            else if(data1.type == DISPLAY_INVERSE_STRING)
//            {
//                Lcd_Put_String(data1.line,data1.offset,NULL,0);
//                for(j=0;j<data1.length;j++)
//                {
//                    Lcd_Read_Data(LCD_CURRENT_POSITION,LCD_CURRENT_POSITION,&temp,1);
//                    if((temp == '.')||(temp == ','))
//                    {
//                        Lcd_Move_Cursor_Left();
//                    }
//                    Lcd_Put_Char(data1.msg[data1.length-j-1]);
//                    Lcd_Move_Cursor_Left();
//                }
//                Lcd_Put_String(data1.line,data1.offset,NULL,0);
//            }
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
    uint32_t Msg_number = 0;
    uint16_t feadback = 0;
    LcdCharacter_PutString(0,9,(uint8_t*)"F 0",3,DISPLAY_STRING,0);
    LcdCharacter_PutString(1,9,(uint8_t*)"I 0",3,DISPLAY_STRING,0);
#endif
    uint8_t OpCode=0;
    uint16_t Length=0;
    uint8_t *RxData;
    uint8_t RxState = 0;
    
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
                LcdCharacter_PutString(0,0,(uint8_t*)"         ",9,DISPLAY_STRING,0);
            DecToString(num,Msg_number);
//            Lcd_Segment_Put_Data((uint8_t*)num,strlen((char*)num),0,0);
//            LcdSegmentDisplaySetEvent();
            LcdCharacter_PutString(0,0,(uint8_t*)num,strlen((char*)num),DISPLAY_STRING,0);
#endif
            /*Notify the command to relevant task*/
            switch(OpCode)
            {
#if (TEST_CODE == 1)
                case 0xff:
                {
                    False_Msg_number++;
                    DecToString(False_num,False_Msg_number);
                    LcdCharacter_PutString(0,12,(uint8_t*)False_num,strlen((char*)False_num),DISPLAY_STRING,0);
                    break;
                }
                case 0xfe:
                {
                    invalid_format++;
                    DecToString(False_num,invalid_format);
                    LcdCharacter_PutString(1,12,(uint8_t*)False_num,strlen((char*)False_num),DISPLAY_STRING,0);
                    break;
                }
                case 0xfc:
                {
                    feadback++;
                    if(feadback == 0)
                        LcdCharacter_PutString(1,0,(uint8_t*)"     ",5,DISPLAY_STRING,0);
                    DecToString(False_num,feadback);
                    LcdCharacter_PutString(1,0,(uint8_t*)False_num,strlen((char*)False_num),0);
                    break;
                }
                case 0xfd:
                {
                    LcdCharacter_PutString(1,0,(uint8_t*)"POL   ",6,DISPLAY_STRING,0);
                    break;
                }
#endif
                /* Switch input handling state*/
                case 0x11:
                {
                    /* Data = Data | 0x80 */
                    RxState = RxData[0] & 0x7F;
                    SwitchInputState(RxState);
                    break;
                }
                /*Special commands for character Lcd*/
                case 0x13:
                /*Dislay character Lcd hex type*/
                case 0x80:
                case 0x81:
                case 0x82:
#if (TEST_CODE == 1)
                    LcdCharacter_PutString(1,0,(uint8_t*)"SELECT",6,DISPLAY_STRING,0);
#endif
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
#if (TEST_CODE == 1)
                    LcdCharacter_PutString(1,0,(uint8_t*)"SELECT",6,DISPLAY_STRING,0);
#endif
                    LcdSegmentProcess(OpCode,RxData,Length);
                    break;
            }
        }
	}
}


static void Main_Preset_Task(void *argument)
{
    uint8_t key = 0;
    //uint8_t SegmentKey[14];
    //uint8_t i = 0;
    uint8_t Checking = NORMAL_KEY;
    uint8_t j;
    j = 0;
    
    osStatus_t status,lcdcharacter_status;
    KeypadStatus_t KeypadStatus = {.PreviousStatus = KEYPAD_UNKNOWN, .CurrentStatus = KEYPAD_UNKNOWN, .PushTime = 0};

#if (TEST_CODE == 1)
    /* Default display Segment Lcd indicator */
    Lcd_Segment_Put_Indicator(INDICATOR_A1|INDICATOR_A3|INDICATOR_A4|INDICATOR_A5);
    LcdSegmentDisplaySetEvent();
#endif
    
    Password_t Registration_check  = {(uint8_t*)Registration_CombineKeys , sizeof(Registration_CombineKeys) ,NO_PASSWORD_CHECK};
    Password_t DirectSetting_check = {(uint8_t*)DirectSetting_CombineKeys, sizeof(DirectSetting_CombineKeys),NO_PASSWORD_CHECK};
#if (TEST_CODE == 1)
    Password_t Printing_check      = {(uint8_t*)Printing_CombineKeys     , sizeof(Printing_CombineKeys)     ,NO_PASSWORD_CHECK};
#endif
    ResetInputCtrlStatus();
    
    
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
                    if(E_OK == Password_Check(&KeypadStatus,&Registration_check))
                    {
                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)&Registration_Passcode,1);
                        
#if (TEST_CODE == 1)
                        Lcd_Segment_Put_Data((uint8_t*)"code",4,2,0);
                        LcdSegmentDisplaySetEvent();
                        gCurrentState = PUMP_SETTING_STATE;
                        gInputCtrl.LcdSegment.mask = INPUT_CTRL_MASK;
                        gInputCtrl.LcdSegment.enable = 1;
                        gInputCtrl.LcdSegment.max_size = 9;
                        gInputCtrl.LcdSegment.row = 1;
#endif
                    }
#if (TEST_CODE == 1)
                    if(E_OK == Password_Check(&KeypadStatus,&Printing_check))
                    {
                        //RS485_Prepare_To_Transmit(0xA0,(uint8_t*)"XXXXX",5);
                        Lcd_Segment_Put_Data((uint8_t*)"print",5,2,0);
                        LcdSegmentDisplaySetEvent();
                        /*reset lcd character*/
                        Lcd_Clear();
                        Lcd_ShiftDisplay_Turn_On();
                        gCurrentState = PRINTER_SETTING_STATE;
                        gInputCtrl.LcdCharacter.row = 0;
                        gInputCtrl.LcdCharacter.col = 0;
                        gInputCtrl.LcdCharacter.max_size = 80;
                        Lcd_Turn_On_Cursor();
                    }
#endif
                    if(Checking == NORMAL_KEY)
                    {
                        if(lcdcharacter_status == osOK)
                        {
                            if(gInputCtrl.LcdCharacter.enable == 1)
                            {
                                if(gInputCtrl.LcdCharacter.len >= gInputCtrl.LcdCharacter.max_size)
                                {
                                    gInputCtrl.LcdCharacter.len = 0;
                                }
                                key = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,0);
                                if(!((gInputCtrl.LcdCharacter.len == 0) && (key == '0')))
                                {
                                    gInputCtrl.LcdCharacter.buf[gInputCtrl.LcdCharacter.len]=key;
                                    gInputCtrl.LcdCharacter.len++;
                                    //LcdCharacter_PutString(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,(uint8_t*)gInputCtrl.LcdCharacter.buf,gInputCtrl.LcdCharacter.len,DISPLAY_INVERSE_STRING,0);
                                    /* Send input from keypad by 0xA2 command*/
                                    RS485_Prepare_To_Transmit(0xA2,(uint8_t*)gInputCtrl.LcdCharacter.buf,gInputCtrl.LcdCharacter.len);
                                }
                            }
                        }
                    }else if(Checking == SPECIAL_KEY)
                    {
                        if(KEYPAD_SPECIAL_KEY_CHECK(KeypadStatus.CurrentStatus))
                        {
                            gInputCtrl.LcdCharacter.len = 0;
                        }
                        key = Keypad_Mapping(KeypadStatus.CurrentStatus);
                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)&key,1);
                    }
                    break;
                }
                case FUELING_STATE:
                {
                    break;
                }
                case PUMP_SETTING_STATE:
                {
                    if(E_OK == Password_Check(&KeypadStatus,&DirectSetting_check))
                    {
                        RS485_Prepare_To_Transmit(0xA0,(uint8_t*)&DirectSetting_Passcode,3);
#if (TEST_CODE == 1)
                        Lcd_Segment_Put_Data((uint8_t*)"00     ",7,2,0);
                        LcdSegmentDisplaySetEvent();
                        gInputCtrl.LcdSegment.mask = INPUT_CTRL_UNMASK;
#endif
                        
                    }else 
                    {
                        if(Checking == UNKNOWN_KEY)
                        {
                            /**/
                            if((KeypadStatus.PreviousStatus == KEYPAD_CLEAR) && (gInputCtrl.LcdSegment.enable == 1))
                            {
                                LcdSegment_PrepareNewInput();
                            }
                        }else if(Checking == NORMAL_KEY)
                        {
                            key = Keypad_Mapping_Printer(KeypadStatus.CurrentStatus,0);
                            if(gInputCtrl.LcdSegment.enable == 1)
                            {
                                if(gInputCtrl.LcdSegment.len >= gInputCtrl.LcdSegment.max_size)
                                {
                                    gInputCtrl.LcdSegment.len = 0;
                                }
                                if(gInputCtrl.LcdSegment.len == 0)
                                {
                                    LcdSegment_PrepareNewInput();
                                }
                                if(gInputCtrl.LcdSegment.len < 14)
                                {
                                    gInputCtrl.LcdSegment.buf[gInputCtrl.LcdSegment.len]=key;
                                    gInputCtrl.LcdSegment.len++;
                                    if(gInputCtrl.LcdSegment.mask == INPUT_CTRL_MASK)
                                    {
                                        /* mask number display by '-' character */
                                        Lcd_Segment_Put_Data((uint8_t*)"-" , 1 , (gInputCtrl.LcdSegment.row - (gInputCtrl.LcdSegment.len + gInputCtrl.LcdSegment.col -1) / LCD_SEGMENT_COLS) , (gInputCtrl.LcdSegment.len + gInputCtrl.LcdSegment.col - 1) % LCD_SEGMENT_COLS);
                                    }else if(gInputCtrl.LcdSegment.mask == INPUT_CTRL_UNMASK)
                                    {
                                        for(j = 0; j < gInputCtrl.LcdSegment.len; j++)
                                        {
                                            Lcd_Segment_Put_Data((uint8_t*)&gInputCtrl.LcdSegment.buf[gInputCtrl.LcdSegment.len - j - 1] , 1 , (gInputCtrl.LcdSegment.row - (j + gInputCtrl.LcdSegment.col) / LCD_SEGMENT_COLS) ,(j + gInputCtrl.LcdSegment.col) % LCD_SEGMENT_COLS);
                                        }
                                    }
                                }
                                LcdSegmentDisplaySetEvent();
                            }
                        }else if(Checking == SPECIAL_KEY)
                        {
                            if((KeypadStatus.CurrentStatus == KEYPAD_ENTER) || (KeypadStatus.CurrentStatus == KEYPAD_CLEAR))
                            {
                                if(KeypadStatus.CurrentStatus == KEYPAD_ENTER)
                                {
                                    /*Send input date to cpu*/
                                    RS485_Prepare_To_Transmit(0xA1,(uint8_t*)gInputCtrl.LcdSegment.buf,gInputCtrl.LcdSegment.len);
                                }
                                sprintf((char*)gInputCtrl.LcdSegment.buf,"");
                                gInputCtrl.LcdSegment.len=0;
                            }else 
                            {
                                if((gInputCtrl.preset_button == 1) && (gInputCtrl.LcdSegment.enable == 1))
                                {
                                    key = Keypad_Mapping(KeypadStatus.CurrentStatus);
                                    RS485_Prepare_To_Transmit(0xA0,(uint8_t*)&key,1);
                                    sprintf((char*)gInputCtrl.LcdSegment.buf,"");
                                    gInputCtrl.LcdSegment.len = 0;
                                }
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
                        lcdcharacter_status = osMutexAcquire(LcdCharacter_Mutex_Id, 0);
                        if(lcdcharacter_status == osOK)
                        {
                            LcdCharacter_PutString(LCD_CURRENT_POSITION,LCD_CURRENT_POSITION,&key,1,DISPLAY_CHARACTER,0);
                            osMutexRelease(LcdCharacter_Mutex_Id);
                        }
                    }else if(Checking == SPECIAL_KEY)
                    {
                        if(KeypadStatus.CurrentStatus == KEYPAD_ENTER)
                        {
                            /* Send content of character lcd in area that define in global input control struct */
                            gInputCtrl.LcdCharacter.len = Lcd_Read_Data(gInputCtrl.LcdCharacter.row,gInputCtrl.LcdCharacter.col,(uint8_t*)gInputCtrl.LcdCharacter.buf,gInputCtrl.LcdCharacter.max_size);
                            RS485_Prepare_To_Transmit(0xA2,(uint8_t*)&gInputCtrl.LcdCharacter.buf,gInputCtrl.LcdCharacter.len);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            /* backup current input status into previous state variable*/
            KeypadStatus.PreviousStatus = KeypadStatus.CurrentStatus;
        }
    }
}

/************************************ Callback handler *******************************************/
/* Timer calback function that handle keypad input */
void Timer1_Callback (void *arg)
{
    KeypadScaningStatus = E_OK;
}

void Timer2_Callback (void *arg)
{
    gInputDone = E_OK;
}

/* Timer calback function that handle segment lcd blinking */
void Timer3_Callback (void *arg)
{
    gBlink = gBlink ^1;
    if(gInputCtrl.LcdSegment.blink == 1)
    {
        if(gBlink == 1)
        {
            /*NOTE: This code may be wrong in some situations if col param set by input max_size*/
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
            Lcd_Segment_Put_Data((uint8_t*)cErrorCodeClear,gInputCtrl.ErrorCodeLength,gInputCtrl.LcdSegment.row_error,gInputCtrl.LcdSegment.col_error);
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
  
  /*Create Mutex*/
  LcdCharacter_Mutex_Id = osMutexNew(NULL);
  
  /*Create os timers*/
  Timer1_Id = osTimerNew(Timer1_Callback, osTimerPeriodic, NULL, NULL);
  Timer2_Id = osTimerNew(Timer2_Callback, osTimerOnce, NULL, NULL);
  Timer3_Id = osTimerNew(Timer3_Callback, osTimerPeriodic, NULL, NULL);
  
  HAL_TIM_Base_Start(&htim2);
  /*use for backlight character lcd*/
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
 
  osKernelStart();                      // Start thread execution

}

