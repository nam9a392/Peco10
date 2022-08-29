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

#define SCE_PIN                                 GPIO_PIN_14
#define SCE_PORT                                GPIOC

#define LCD_SPI_INSTANCE                        &hspi1

#define LCD_DEVICE_CODE                         (0x42U)
#define LCD_DISPLAY_RAM_SIZE                    (35U)
#define LCD_FRAME_LENGTH                        (12U)
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
void Lcd_Segment_Display_App(void);
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
void Lcd_Segment_Put_Data(uint8_t* pData, uint8_t line);

/**
 * @brief  This function uses to prepare data which will be displayed in indicator position in LCD segment
 *
 * @param[in]  Data : Data of indicator line
 *
 * @retval void
 *
 */
void Lcd_Segment_Put_Indicator(uint8_t Data);

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
 * @brief  This function uses to get the current code is diaplayed in LCD segment
 *
 * @param[in,out]  uint8_t* : pointer to Current code is displayed in LCD (integer format)
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Current_Code(uint8_t *pCurrentCode);

/**
 * @brief  This function uses to get the current Subcode is diaplayed in LCD segment
 *
 * @param[in,out]  uint8_t* : pointer to Current Subcode is displayed in LCD (ASCII format)
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Current_Subcode(uint8_t *pCurrentSubcode);

/**
 * @brief  This function uses to get the current Subcode of code 24
 *
 * @param[in,out]  uint8_t* : pointer to Current Subcode is displayed in LCD (ASCII format)
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Current_Subcode_24(uint8_t* pCurrentSubcode);

/**
 * @brief  This function uses to get the current Subcode of code 37
 *
 * @param[in,out]  uint8_t* : pointer to Current Subcode is displayed in LCD (ASCII format)
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Current_Subcode_37(uint8_t* pCurrentSubcode);

/**
 * @brief  This function uses to get the current decimal place
 *
 * @param[in,out]  uint8_t* pCurrentDecimalPlace : pointer to Current decimal place
 * @param[in,out]  uint8_t* pDecimalPlaceType    : pointer to Current decimal place type 0 <-> ',' and 1 <-> '.'
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Decimal_Place(uint8_t* pCurrentDecimalPlace, uint8_t* pDecimalPlaceType);

/**
 * @brief  This function uses to get the current data display for code 95 (Calendar Setting)
 *
 * @param[in,out]  uint8_t* pData: pointer to output data
 *
 * @retval Std_Return_Type
 *
 */
Std_Return_Type Lcd_Segment_Get_Data_Code_95(uint8_t* pData);


#endif /* LCD_SEGMENT_H */
