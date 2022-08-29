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
/**
 * @brief Display data table for number in even position
 */
static const uint8_t DisplayDataFont1[26U] = 
{
    0xAFU,   /* 0 */        0xA0U,   /* 1 */        0xCBU,   /* 2 */        0xE9U,   /* 3 */
    0xE4U,   /* 4 */        0x6DU,   /* 5 */        0x6FU,   /* 6 */        0xA8U,   /* 7 */
    0xEFU,   /* 8 */        0xEDU,   /* 9 */        0x67U,   /* b */        0xA7U,   /* U */
    0x6DU,   /* S */        0xE5U,   /* Y */        0x0FU,   /* C */        0x63U,   /* o */
    0xE3U,   /* d */        0x4FU,   /* E */        0x00U,   /* blank */    0x40U,   /* minus */
    0x01U,   /* _ */        0xEEU,   /* A */        0x07U,   /* L */        0xECU,   /* P */
    0x11U,   /* COMMA */    0x10U    /* DOT */
};

/**
 * @brief Display data table for number in odd position
 */
static const uint8_t DisplayDataFont2[26U] = 
{
    0xFAU,   /* 0 */        0x60U,   /* 1 */        0xD6U,   /* 2 */        0xF4U,   /* 3 */
    0x6CU,   /* 4 */        0xBCU,   /* 5 */        0xBEU,   /* 6 */        0xE0U,   /* 7 */
    0xFEU,   /* 8 */        0xFCU,   /* 9 */        0x3EU,   /* b */        0x7AU,   /* U */
    0xBCU,   /* S */        0x7CU,   /* Y */        0x9AU,   /* C */        0x36U,   /* o */
    0x76U,   /* d */        0x9EU,   /* E */        0x00U,   /* blank */    0x04U,   /* minus */
    0x10U,   /* _ */        0xEEU,   /* A */        0x16U,   /* L */        0xCEU,   /* P */
    0x30U,   /* COMMA */    0x20U    /* DOT */
};

/* Lcd device code number */
static const uint8_t LcdDeviceCode = LCD_DEVICE_CODE;

/* Control data for the SPI frame DD=00 */
static const uint8_t ControlData0[3U] = 
{
    0x07U,          /* Seg56 output and Key Scan 2-6 */
    0x00U,          /* Segment output for S1-6, 1/3Bias, 1/4Duty, frame=Fosc/10752 */
    0x80U,          /* frame=Fosc/10752, Internal osc, segment on, Normal mode, DD=00 */
};

/* Control data for the SPI frame DD=01 */
static const uint8_t ControlData1[1U] = 
{
    0x01U,          /* Dont care PWM frequency, DD=01 */
};

/* Control data for the SPI frame DD=10 */
static const uint8_t ControlData2[4U] = 
{
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x02U,          /* Dont care PWM output, DD=10 */
};

/* Control data for the SPI frame DD=11 */
static const uint8_t ControlData3[4U] = 
{
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x00U,          /* Dont care PWM output */
    0x03U,          /* Dont care PWM output, DD=11 */
};
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define LCD_CS_ENABLE()           HAL_GPIO_WritePin(SCE_PORT,SCE_PIN,GPIO_PIN_SET)
#define LCD_CS_DISABLE()          HAL_GPIO_WritePin(SCE_PORT,SCE_PIN,GPIO_PIN_RESET)
#define LCD_CS_ENABLING()         ((GPIOC->IDR)&(uint32_t)0x4000U)

/* LCD driver Display off (Active Low level) */
//#define LCD_DISPLAY_ENABLE()             HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET)
//#define LCD_DISPLAY_DISABLE()            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET)

#define DOT_COMMA_MASK_FONT1          0x11U
#define DOT_COMMA_MASK_FONT2          0x30U
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
/* 14 characters Data display for each line (include comma or dot)*/
static uint8_t LcdSegmentDataDisplay[LCD_SEGMENT_ROWS][LCD_SEGMENT_COLS << 1U];

/* LCD display RAM */
static uint8_t LcdDisplayRam[LCD_DISPLAY_RAM_SIZE];

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
        case (uint8_t)'b':
            FontPos = 10U;
            break;
        case (uint8_t)'U':
            FontPos = 11U;
            break;
        case (uint8_t)'S':
            FontPos = 12U;
            break;
        case (uint8_t)'Y':
            FontPos = 13U;
            break;
        case (uint8_t)'C':
            FontPos = 14U;
            break;
        case (uint8_t)'o':
            FontPos = 15U;
            break;
        case (uint8_t)'d':
            FontPos = 16U;
            break;
        case (uint8_t)'E':
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
            FontPos = 21U;
            break;
        case (uint8_t)'L':
            FontPos = 22U;
            break;
        case (uint8_t)'P':
            FontPos = 23U;
            break;
        case (uint8_t)',':
            FontPos = 24U;
            break;
        case (uint8_t)'.':
            FontPos = 25U;
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
                LcdDisplayRam[21 - 9*row] = DisplayDataFont2[FontPos];
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
                LcdDisplayRam[24 - 9*row] = (LcdDisplayRam[24 - 9*row] & 0xF) | (DisplayDataFont2[FontPos] << 4);
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
                LcdDisplayRam[26 - 9*row] = DisplayDataFont2[FontPos];
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
    while(LCD_CS_ENABLING());
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief  This function uses to Display LCD segment
 *
 * @param[in]  None
 *
 * @retval void
 *
 * @implement Lcd_Segment_Display_App_Activity
 */
void Lcd_Segment_Display_App(void)
{
    uint8_t LcdSpiFrame[LCD_FRAME_LENGTH];
    uint8_t i,TempData;
    
    //LCD_DISPLAY_ENABLE();

    /* Send the LcdDisplayRam to IC driver */
    /*First frame 72 bit Display data 22 bit control data 2 bit direction data*/
    memcpy(LcdSpiFrame,LcdDisplayRam,9U);
    memcpy(&LcdSpiFrame[9],ControlData0,3U);
    
    Lcd_Frame_Transfer(LcdSpiFrame);
    
    /*Second frame 82 bit Display data 10 bit control data 2 bit direction data*/
    memcpy(LcdSpiFrame,&LcdDisplayRam[9],11U);
    LcdSpiFrame[10] &= (uint8_t)0xF0;
    memcpy(&LcdSpiFrame[11],ControlData1,1U);
    
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


void Lcd_Segment_Init(void)
{
    /* Clear LCD Display RAM, except first byte - indicator display byte */
    memset(&LcdDisplayRam[1U], 0U, LCD_DISPLAY_RAM_SIZE - 1U);
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
void Lcd_Segment_Put_Data(uint8_t* pData, uint8_t line)
{
    uint8_t i=0;
    uint8_t j=0;
    uint8_t len = strlen((char*)pData);
    if(line <= 2)
    {
        memset(&LcdDisplayRam[19 - 9 * line], 0U, 9);
        while(i<7)
        {
            if((i + j) < len)
            {
                Lcd_Segment_Prepare_Display_Ram((LCD_SEGMENT_COLS - i) ,line, pData[len-1-i-j]);
                if((pData[len - 1 - i - j] == '.')||(pData[len - 1 - i - j] == ','))
                {
                    j++;
                }else{
                    i++;
                }
            }else{
                Lcd_Segment_Prepare_Display_Ram((LCD_SEGMENT_COLS - i) ,line, ' ');
                i++;
            }
        }
    }
}

void Lcd_Segment_Put_Indicator(uint8_t Data)
{
    /* Copy indicator byte to display RAM */
    LcdDisplayRam[0U] = Data & 0xF8U;
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
    }
}
