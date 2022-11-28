#ifndef LCD_SEGMENT_H
#define LCD_SEGMENT_H

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

#include "Standard.h"
#include "main.h"
/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
#define LCD_SEGMENT_ROWS                         (3U)
#define LCD_SEGMENT_COLS                         (7U)
#define LCD_SEGMENT_DIGITS                       (LCD_SEGMENT_ROWS*LCD_SEGMENT_COLS)

//#define INDICATOR_A1                             (0x8U)
//#define INDICATOR_A2                             (0x80U)
//#define INDICATOR_A3                             (0x40U)
//#define INDICATOR_A4                             (0x20U)
//#define INDICATOR_A5                             (0x10U)

#define INDICATOR_A1                             (0x1U)
#define INDICATOR_A2                             (0x2U)
#define INDICATOR_A3                             (0x4U)
#define INDICATOR_A4                             (0x8U)
#define INDICATOR_A5                             (0x10U)

#define SCE_PIN                                 GPIO_PIN_14
#define SCE_PORT                                GPIOC

#define LCD_SPI_INSTANCE                        &hspi1

/* device code is constant value (0x42) which mentioned in datasheet */
#define LCD_DEVICE_CODE                         (0x42U)
#define LCD_DISPLAY_RAM_SIZE                    (35U)
#define LCD_FRAME_LENGTH                        (12U)


#if (PROGRAM_USE_RTOS == STD_ON)
    #define LCD_SEGMENT_END_OF_TRANSMIT                 0x01U
#endif
/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/
extern SPI_HandleTypeDef hspi1;
/*==================================================================================================
*                                              ENUMS
==================================================================================================*/

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * @brief  This function uses to start  display frame to LCD segment
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd_Segment_Start_Display(void);

/**
 * @brief  This function uses to initialize LCD segment and Key scan
 *
 * @param[in]  None
 *
 * @retval void
 *
 */
void Lcd_Segment_Init(void);

/**
 * @brief  This function uses to prepare data which will be displayed to LCD segment
 *
 * @param[in]  pData   : input data pointer
 *             line    : line which data will be displayed
 *
 * @retval void
 *
 */
void Lcd_Segment_Put_Data(uint8_t* pData,uint8_t len, uint8_t line, uint8_t col);

/**
 * @brief  This function uses to prepare data which will be displayed in indicator position in LCD segment
 *
 * @param[in]  Data : Data of indicator line
 *
 * @retval void
 *
 */
void Lcd_Segment_Put_Indicator(uint8_t Data);

void Lcd_Segment_Clear_Line(uint8_t line);


void Lcd_Segment_PWM_Config(void);
void Lcd_Segment_PWM_Start(void);


#endif /* LCD_SEGMENT_H */
