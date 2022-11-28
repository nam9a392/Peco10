/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Lcd_segment.h"
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/


/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define LCD_CS_ENABLE()           HAL_GPIO_WritePin(SCE_PORT,SCE_PIN,GPIO_PIN_SET)
#define LCD_CS_DISABLE()          HAL_GPIO_WritePin(SCE_PORT,SCE_PIN,GPIO_PIN_RESET)
//#define LCD_CS_ENABLING()         ((GPIOC->IDR)&(uint32_t)0x4000U)
#define LCD_CS_ENABLING()         HAL_GPIO_ReadPin(SCE_PORT,SCE_PIN)

/* LCD driver Display off (Active Low level) */

#define DOT_COMMA_MASK_FONT1          0x11U
#define DOT_COMMA_MASK_FONT2          0x30U

/* Option byte control */
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

typedef struct 
{
    /* 24 bit */
    /*| reserve (5b) | KMn (3b) | Pn (3b) | FL (1b) | DR (1b) | DT (1b) | FCn (3b) | OC (1b) | SC (1b) | BUn (3b) | DD(00) |*/
    /*|      0       |  1 1 1   |  0 0 0  |    0    |    0    |    0    |  0 0 1   |    0    |   0     |  0 0 0   |  0 0   |*/
    /*|          0x07           |                    0                        |                 0x80                       |*/
    uint32_t reserved0 : 8;
    uint32_t reserved1 : 5;
    uint32_t KM        : 3;
    uint32_t P         : 3;
    uint32_t FL        : 1;
    uint32_t DR        : 1;
    uint32_t DT        : 1;
    uint32_t FC        : 3;
    uint32_t OC        : 1;
    uint32_t SC        : 1;
    uint32_t BU        : 3;
    uint32_t DD        : 2;
}ControlData0_t;

typedef struct 
{
    /* 12 bit */
    /*|      PGn (6b)    |   PF (4b)   | DD(01) |*/
    /*|    0 0 0 0 0 0   |   0 0 0 0   |  0 1   |*/
    /*|           |            0x01             |*/
    uint16_t reserved  : 4;
    uint16_t PG        : 8;
    uint16_t DD        : 2;
}ControlData1_t;

typedef struct 
{
    /* 36 bit*/
    /*|  reserve (4b)  |   reserve (6b)  |      W1x (8b)     |      W2x (8b)     |      W3x (8b)     |  DD(10) |*/
    /*|    0 0 0 0     |   0 0 0 0 0 0   |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |   1 0   |*/
    /*|                |         0x0           |        0x0        |        0x0        |          0x03         |*/
    uint32_t reserved  : 6;
    uint32_t W1        : 8;
    uint32_t W2        : 8;
    uint32_t W3        : 8;
    uint32_t DD        : 2;
}ControlData2_t;

typedef struct 
{
    /* 36 bit*/
    /*|  reserve (4b)  |   reserve (6b)  |       W4x (8b)    |       W5x (8b)    |       W6x (8b)    |  DD(11) |*/
    /*|    0 0 0 0     |   0 0 0 0 0 0   |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |   1 1   |*/
    /*|                |         0x0           |        0x0        |        0x0        |          0x03         |*/
    uint32_t reserved  : 6;
    uint32_t W4        : 8;
    uint32_t W5        : 8;
    uint32_t W6        : 8;
    uint32_t DD        : 2;
}ControlData3_t;
/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
/**
 * @brief Display data table for number in even position
 */
static const uint8_t DisplayDataFont1[32U] = 
{
    0xAFU,   /* 0 */        0xA0U,   /* 1 */        0xCBU,   /* 2 */        0xE9U,   /* 3 */
    0xE4U,   /* 4 */        0x6DU,   /* 5 */        0x6FU,   /* 6 */        0xA8U,   /* 7 */
    0xEFU,   /* 8 */        0xEDU,   /* 9 */        0x67U,   /* b */        0xA7U,   /* U */
    0x6DU,   /* S */        0xE5U,   /* Y */        0x0FU,   /* C */        0x63U,   /* o */
    0xE3U,   /* d */        0x4FU,   /* E */        0x00U,   /* blank */    0x40U,   /* minus */
    0x01U,   /* _ */        0xEEU,   /* A */        0x07U,   /* L */        0xCEU,   /* P */
    0x4EU,   /* F */        0x11U,   /* COMMA */    0x10U,   /* DOT */      0xE6U,   /* X */
    0x20U,   /* I */        0x62,    /* n */        0x42U,   /* r */        0xE0U    /* T */
};

/**
 * @brief Display data table for number in odd position
 */
static const uint8_t DisplayDataFont2[32U] = 
{
    0xFAU,   /* 0 */        0x60U,   /* 1 */        0xD6U,   /* 2 */        0xF4U,   /* 3 */
    0x6CU,   /* 4 */        0xBCU,   /* 5 */        0xBEU,   /* 6 */        0xE0U,   /* 7 */
    0xFEU,   /* 8 */        0xFCU,   /* 9 */        0x3EU,   /* b */        0x7AU,   /* U */
    0xBCU,   /* S */        0x7CU,   /* Y */        0x9AU,   /* C */        0x36U,   /* o */
    0x76U,   /* d */        0x9EU,   /* E */        0x00U,   /* blank */    0x04U,   /* minus */
    0x10U,   /* _ */        0xEEU,   /* A */        0x16U,   /* L */        0xCEU,   /* P */
    0x8EU,   /* F */        0x30U,   /* COMMA */    0x20U,   /* DOT */      0x6EU,   /* X */
    0x20U,   /* I */        0x26,    /* n */        0x06,    /* r */        0x64U    /* T */
};

/* Lcd device code number */
static const uint8_t LcdDeviceCode = LCD_DEVICE_CODE;

/* Control data for the SPI frame DD=00 */
uint8_t ControlData0[3U] = 
{
    /* 24 bit */
    /*| reserve (5b) | KMn (3b) | Pn (3b) | FL (1b) | DR (1b) | DT (1b) | FCn (3b) | OC (1b) | SC (1b) | BUn (3b) | DD(00) |*/
    /*|      0       |  1 1 1   |  0 0 0  |    0    |    0    |    0    |  0 0 1   |    0    |   0     |  0 0 0   |   00   |*/
    /*|          0x07           |                    0                        |                 0x80                       |*/
    0x07U,          /* Seg56 output and Key Scan 2-6 */
    0x00U,          /* Segment output for S1-6, 1/3Bias, 1/4Duty, frame=Fosc/10752 */
    0x80U,          /* frame=Fosc/10752, Internal osc, segment on, Normal mode, DD=00 */
};

/* Control data for the SPI frame DD=01 */
uint8_t ControlData1[2U] = 
{
    /* 12 bit */
    /*|      PGn (6b)    |   PF (4b)   | DD(01) |*/
    /*|    0 0 0 0 0 0   |   0 0 0 0   |  0 1   |*/
    /*|           |            0x01             |*/
    0x0U,
    0x01U,          /* Dont care PWM frequency, DD=01 */
};

/* Control data for the SPI frame DD=10 */
uint8_t ControlData2[4U] = 
{
    /* 36 bit*/
    /*|  reserve (4b)  |   reserve (6b)  |      W1x (8b)     |      W2x (8b)     |      W3x (8b)     |  DD(10) |*/
    /*|    0 0 0 0     |   0 0 0 0 0 0   |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |   1 0   |*/
    /*|                |         0x0           |        0x0        |        0x0        |          0x03         |*/
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x02U,          /* Dont care PWM output, DD=10 */
};

/* Control data for the SPI frame DD=11 */
uint8_t ControlData3[4U] = 
{
    /* 36 bit*/
    /*|  reserve (4b)  |   reserve (6b)  |       W4x (8b)    |       W5x (8b)    |       W6x (8b)    |  DD(11) |*/
    /*|    0 0 0 0     |   0 0 0 0 0 0   |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |  0 0 0 0 0 0 0 0  |   1 1   |*/
    /*|                |         0x0           |        0x0        |        0x0        |          0x03         |*/
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x03U,          /* Dont care PWM output, DD=11 */
};

/* LCD display RAM */
uint8_t LcdDisplayRam[LCD_DISPLAY_RAM_SIZE];

#if (PROGRAM_USE_RTOS == STD_ON)
    osEventFlagsId_t evtFrameTransfer_id;
#endif

void Lcd_Segment_PWM_Config(void)
{
    /* Set new duty */
    /* W1x = 0x3F ~ Duty = (253/256)* Tp*/
    ControlData2[2] |= 0x3F;
    /* Set period */
    /* PF = 0xF   ~ Period = fosc/256  */
    ControlData1[1] |= 0x3C;
    Lcd_Segment_Start_Display();
}

void Lcd_Segment_PWM_Start(void)
{
    /* Set Output pin S1 as alternate function   */
    /* Pn = 0x1 ~  S1 -> P1/G1  */
    ControlData0[1] |= 0x20;
    /* Set S1 pin generate PWM */
    /* PG1 = 0 ~ PWM output*/
    ControlData1[0] &= 0xF0;
    Lcd_Segment_Start_Display();
}
/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
static void Lcd_Frame_Transfer(uint8_t* LcdSpiFrame);
static void Lcd_Segment_Prepare_Display_Ram(uint8_t col, uint8_t row, uint8_t Data);
static uint8_t FontPosition(uint8_t character);

/*==================================================================================================
*                                         LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief  This function uses to return position of character in font table
 *
 * @param[in]  character    : input data character
 *             
 * @retval     void
 */
static uint8_t FontPosition(uint8_t character)
{
    uint8_t FontPos;
    switch(character)
    {
        case (uint8_t)'0':
        case (uint8_t)'1':
        case (uint8_t)'2':
        case (uint8_t)'3':
        case (uint8_t)'4':
        case (uint8_t)'5':
        case (uint8_t)'6':
        case (uint8_t)'7':
        case (uint8_t)'8':
        case (uint8_t)'9':
            FontPos = character - '0';
            break;
        case (uint8_t)'B':
        case (uint8_t)'b':
            FontPos = 10U;
            break;
        case (uint8_t)'U':
        case (uint8_t)'u':
            FontPos = 11U;
            break;
        case (uint8_t)'S':
        case (uint8_t)'s':
            FontPos = 12U;
            break;
        case (uint8_t)'Y':
        case (uint8_t)'y':
            FontPos = 13U;
            break;
        case (uint8_t)'C':
        case (uint8_t)'c':
            FontPos = 14U;
            break;
        case (uint8_t)'O':
        case (uint8_t)'o':
            FontPos = 15U;
            break;
        case (uint8_t)'D':
        case (uint8_t)'d':
            FontPos = 16U;
            break;
        case (uint8_t)'E':
        case (uint8_t)'e':
            FontPos = 17U;
            break;
        case (uint8_t)' ':
            FontPos = 18U;
            break;
        case (uint8_t)'-':
            FontPos = 19U;
            break;
        case (uint8_t)'_':
            FontPos = 20U;
            break;
        case (uint8_t)'A':
        case (uint8_t)'a':
            FontPos = 21U;
            break;
        case (uint8_t)'L':
        case (uint8_t)'l':
            FontPos = 22U;
            break;
        case (uint8_t)'P':
        case (uint8_t)'p':
            FontPos = 23U;
            break;
        case (uint8_t)'F':
        case (uint8_t)'f':
            FontPos = 24U;
            break;
        case (uint8_t)',':
            FontPos = 25U;
            break;
        case (uint8_t)'.':
            FontPos = 26U;
            break;
        case (uint8_t)'X':
        case (uint8_t)'x':
            FontPos = 27U;
            break;
        case (uint8_t)'i':
        case (uint8_t)'I':
            FontPos = 28U;
            break;
        case (uint8_t)'N':
        case (uint8_t)'n':
            FontPos = 29U;
            break;
        case (uint8_t)'r':
        case (uint8_t)'R':
            FontPos = 30U;
            break;
        case (uint8_t)'T':
        case (uint8_t)'t':
            FontPos = 31U;
            break;
        default:
            FontPos = 18U;
            break;
    }
    return FontPos;
}

/**
 * @brief  This function uses to prepare data which will be displayed to LCD segment
 *
 * @param[in]  Data    : input data character
 *             col      : col index of the data need displaying
 *             row      : row index of the data need displaying
 *
 * @retval void
 *
 */
static void Lcd_Segment_Prepare_Display_Ram(uint8_t col, uint8_t row, uint8_t Data)
{
    uint8_t FontPos = FontPosition(Data);
    switch (col)
    {
        case 7:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[19 - 9*row] = (LcdDisplayRam[19 - 9*row] & ~DOT_COMMA_MASK_FONT1) | DisplayDataFont1[FontPos];
            }else{
                LcdDisplayRam[19 - 9*row] = (LcdDisplayRam[19 - 9*row] & 0xF1) | (DisplayDataFont1[FontPos] >> 4);
                LcdDisplayRam[20 - 9*row] = (LcdDisplayRam[20 - 9*row] & 0xF) | (DisplayDataFont1[FontPos] << 4);
            }
            break;
        case 6:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[20 - 9*row] = (LcdDisplayRam[20 - 9*row] & ~(DOT_COMMA_MASK_FONT2 >> 4)) | (DisplayDataFont2[FontPos] >> 4);
            }else{
                LcdDisplayRam[21 - 9*row] = (LcdDisplayRam[21 - 9*row] & 0x1) | DisplayDataFont2[FontPos];
            }
            break;
        case 5:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[21 - 9*row] = (LcdDisplayRam[21 - 9*row] & ~(DOT_COMMA_MASK_FONT1 >> 4)) | (DisplayDataFont1[FontPos] >> 4);
                LcdDisplayRam[22 - 9*row] = (LcdDisplayRam[22 - 9*row] & ~(DOT_COMMA_MASK_FONT1 << 4)) | (DisplayDataFont1[FontPos] << 4);
            }else{
                LcdDisplayRam[22 - 9*row] = (LcdDisplayRam[22 - 9*row] & 0x10U) | DisplayDataFont1[FontPos];
            }
            break;
        case 4:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[23 - 9*row] = (LcdDisplayRam[23 - 9*row] & ~DOT_COMMA_MASK_FONT2) | DisplayDataFont2[FontPos];
            }else{
                LcdDisplayRam[23 - 9*row] = (LcdDisplayRam[23 - 9*row] & 0xF0) | (DisplayDataFont2[FontPos] >> 4);
                LcdDisplayRam[24 - 9*row] = (LcdDisplayRam[24 - 9*row] & 0x1F) | (DisplayDataFont2[FontPos] << 4);
            }
            break;
        case 3:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[24 - 9*row] = (LcdDisplayRam[24 - 9*row] & ~DOT_COMMA_MASK_FONT1) | DisplayDataFont1[FontPos];
            }else{
                LcdDisplayRam[24 - 9*row] = (LcdDisplayRam[24 - 9*row] & 0xF1) | (DisplayDataFont1[FontPos] >> 4);
                LcdDisplayRam[25 - 9*row] = (LcdDisplayRam[25 - 9*row] & 0xF) | (DisplayDataFont1[FontPos] << 4);
            }
            break;
        case 2:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[25 - 9*row] = (LcdDisplayRam[25 - 9*row] & ~(DOT_COMMA_MASK_FONT2 >> 4)) | (DisplayDataFont2[FontPos] >> 4);
            }else{
                LcdDisplayRam[26 - 9*row] = (LcdDisplayRam[26 - 9*row] & 0x1U) | DisplayDataFont2[FontPos];
            }
            break;
        case 1:
            if((Data == '.')||(Data == ','))
            {
                LcdDisplayRam[26 - 9*row] = (LcdDisplayRam[26 - 9*row] & ~(DOT_COMMA_MASK_FONT1 >> 4)) | (DisplayDataFont1[FontPos] >> 4);
                LcdDisplayRam[27 - 9*row] = (LcdDisplayRam[27 - 9*row] & ~(DOT_COMMA_MASK_FONT1 << 4)) | (DisplayDataFont1[FontPos] << 4);
            }else{
                LcdDisplayRam[27 - 9*row] = (LcdDisplayRam[27 - 9*row] & 0x10) | DisplayDataFont1[FontPos];
            }
            break;
        default:
            break;
    }
}

/**
 * @brief  This function uses to send each frame data to display lcd through spi transmit
 *
 * @param[in]  pLcdSpiFrame    : Data frame to transmit
 *
 * @retval     void
 */
static void Lcd_Frame_Transfer(uint8_t* pLcdSpiFrame)
{
    /*disable SCE Lcd pin*/
    LCD_CS_DISABLE();
    /*Transmit first 8 bit device code*/
    HAL_SPI_Transmit(LCD_SPI_INSTANCE,(uint8_t*)&LcdDeviceCode, 1U, 10);
    LCD_CS_ENABLE();
    HAL_SPI_Transmit_IT(LCD_SPI_INSTANCE,(uint8_t*)pLcdSpiFrame, LCD_FRAME_LENGTH);
    /*Waiting transmition is done, Lcd SCE will be set to LOW by SPI callback*/
    while(LCD_CS_ENABLING())
    {
#if (PROGRAM_USE_RTOS == STD_ON)
        osEventFlagsWait(evtFrameTransfer_id, LCD_SEGMENT_END_OF_TRANSMIT, osFlagsWaitAny, osWaitForever);
#endif
    }
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief  This function uses to Initialize LCD segment
 *
 * @param[in]  None
 *
 * @retval void
 *
 * @implement Lcd_Segment_Display_App_Activity
 */
void Lcd_Segment_Init(void)
{
#if (PROGRAM_USE_RTOS == STD_ON)
    osEventFlagsId_t evtFrameTransfer_id;
    evtFrameTransfer_id = osEventFlagsNew(NULL);
    if (evtFrameTransfer_id == NULL) {
    ; // Event Flags object not created, handle failure
  }
#endif
    /* Clear LCD Display RAM, except first byte - indicator display byte */
    memset(&LcdDisplayRam[1U], 0U, LCD_DISPLAY_RAM_SIZE - 1U);
    Lcd_Segment_Start_Display();
}

/**
 * @brief  This function uses to Display LCD segment
 *
 * @param[in]  None
 *
 * @retval void
 *
 * @implement Lcd_Segment_Start_Display_Activity
 */
void Lcd_Segment_Start_Display(void)
{
    uint8_t LcdSpiFrame[LCD_FRAME_LENGTH];
    uint8_t i,TempData;

    /* Send the LcdDisplayRam to IC driver */
    /*First frame 72 bit Display data 22 bit control data 2 bit direction data*/
    memcpy(LcdSpiFrame,LcdDisplayRam,9U);
    memcpy(&LcdSpiFrame[9],ControlData0,3U);
    
    Lcd_Frame_Transfer(LcdSpiFrame);
    
    /*Second frame 82 bit Display data 10 bit control data 2 bit direction data*/
    memcpy(LcdSpiFrame,&LcdDisplayRam[9],11U);
    LcdSpiFrame[10] = (LcdSpiFrame[10] & (uint8_t)0xF0) | ((ControlData1[0] >> 4) & 0xF);
    memcpy(&LcdSpiFrame[11],&ControlData1[1],1U);
    
    Lcd_Frame_Transfer(LcdSpiFrame);
    
    /*Third frame 60 bit Display data 34 bit control data 2 bit direction data*/
    for(i=0U; i<8U; i++)
    {
        TempData = ((LcdDisplayRam[19U+i] & 0x0FU) << 4U)|(LcdDisplayRam[20U+i] >> 4U);
        LcdSpiFrame[i] = TempData;
    }
    LcdSpiFrame[7U] &= (uint8_t)0xF0;
    memcpy(&LcdSpiFrame[8U],ControlData2,4U);
    
    Lcd_Frame_Transfer(LcdSpiFrame);
    
    /*Final frame 60 bit Display data 34 bit control data 2 bit direction data*/
    memcpy(LcdSpiFrame,&LcdDisplayRam[27],8U);
    LcdSpiFrame[7U] &= (uint8_t)0xF0;
    memcpy(&LcdSpiFrame[8],ControlData3,4U);
    
    Lcd_Frame_Transfer(LcdSpiFrame);
}





/**
 * @brief  This function uses to prepare data which will be displayed to LCD segment
 *
 * @param[in]  pData  : data to be displayed
 *             line   : LCD line index [0-2]
 *
 * @retval void
 *
 * @implement Lcd_Segment_Put_Data_Activity
 */
void Lcd_Segment_Put_Data(uint8_t* pData,uint8_t len, uint8_t line, uint8_t col)
{
    uint8_t i=0;
    uint8_t j=0;
    if((line <= 2) && (col < 7))
    {
        /*clear line Lcd ram buffer data*/
        //memset(&LcdDisplayRam[19 - 9 * line], 0U, 9);
        while(i<(LCD_SEGMENT_COLS - col))
        {
            if((i + j) < len)
            {
                Lcd_Segment_Prepare_Display_Ram((LCD_SEGMENT_COLS - col - i) ,line, pData[len-1-i-j]);
                if((pData[len - 1 - i - j] == '.')||(pData[len - 1 - i - j] == ','))
                {
                    j++;
                }else{
                    i++;
                }
            }
            else{
                //Lcd_Segment_Prepare_Display_Ram((LCD_SEGMENT_COLS - col - i) ,line, ' ');
                i++;
            }
        }
    }
}

void Lcd_Segment_Clear_Line(uint8_t line)
{
    if(line < LCD_SEGMENT_ROWS)
    {
        /*clear line Lcd ram buffer data*/
        memset(&LcdDisplayRam[19 - 9 * line], 0U, 9);
    }
}

void Lcd_Segment_Put_Indicator(uint8_t Data)
{
    uint8_t A1,A2,A3,A4,A5;
    A1=A2=A3=A4=A5=0;
    A1 = (Data >> 0) & 0x1;
    A2 = (Data >> 1) & 0x1;
    A3 = (Data >> 2) & 0x1;
    A4 = (Data >> 3) & 0x1;
    A5 = (Data >> 4) & 0x1;
    /* Copy indicator byte to display RAM
    Input data format;
     | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 | 
     |      Reserved      |  A5  |  A4  |  A3  |  A2  |  A1  |
    Segment display ram format
     | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 | 
     |  A2  |  A3  |  A4  |  A5  |  A1  |     Reserved       |
    */
    LcdDisplayRam[0U] = ((A1 << 3) | (A5 << 4) | (A4 << 5) | (A3 << 6) | (A2 << 7)) & 0xF8U;
    return; 
}
/*==================================================================================================
*                                        INTERUPT HANDLER FUNCTIONS
==================================================================================================*/
/**
 * @brief  This function is SPI Tx complete callback
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{   
    if(hspi == &hspi1)
    {
        LCD_CS_DISABLE();
#if (PROGRAM_USE_RTOS == STD_ON)
        osEventFlagsSet(evtFrameTransfer_id, LCD_SEGMENT_END_OF_TRANSMIT);
#endif
    }
}
