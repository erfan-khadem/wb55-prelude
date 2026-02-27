/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/custom_stm.c
  * @author  MCD Application Team
  * @brief   Custom service used for GPIO control.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#ifdef PAUSE
#undef PAUSE
#endif

#include "common_blesvc.h"
#include "custom_stm.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint16_t CustomLedHdle;
  uint16_t CustomLedwriteHdle;
  uint16_t CustomSht40DataHdle;
  uint16_t CustomScd41DataHdle;
  uint16_t CustomMlx90614DataHdle;
} CustomContext_t;

/* Private defines -----------------------------------------------------------*/
#define UUID_128_SUPPORTED  1

#if (UUID_128_SUPPORTED == 1)
#define BM_UUID_LENGTH  UUID_TYPE_128
#else
#define BM_UUID_LENGTH  UUID_TYPE_16
#endif

#define CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET 1

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
do { \
  uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
  uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
  uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
  uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
} while(0)

#define COPY_LED_CONTROL_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct, 0x00, 0x00, 0x00, 0x00, 0xCC, 0x7A, 0x48, 0x2A, 0x98, 0x4A, 0x7F, 0x2E, 0xD5, 0xB3, 0xE5, 0x8F)

#define COPY_LEDWRITE_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x22, 0x45, 0x41, 0x9D, 0x4C, 0x21, 0xED, 0xAE, 0x82, 0xED, 0x19)

#define COPY_SHT40_DATA_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x22, 0x45, 0x41, 0x9D, 0x4C, 0x21, 0xED, 0xAE, 0x82, 0xED, 0x1A)

#define COPY_SCD41_DATA_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x22, 0x45, 0x41, 0x9D, 0x4C, 0x21, 0xED, 0xAE, 0x82, 0xED, 0x1B)

#define COPY_MLX90614_DATA_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x22, 0x45, 0x41, 0x9D, 0x4C, 0x21, 0xED, 0xAE, 0x82, 0xED, 0x1C)

/* Private variables ---------------------------------------------------------*/
uint16_t SizeLedwrite = 1;
uint16_t SizeSht40Data = 4;
uint16_t SizeScd41Data = 6;
uint16_t SizeMlx90614Data = 4;
static CustomContext_t CustomContext;
static uint8_t g_custom_svc_ready = 0;

/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t Custom_STM_Event_Handler(void *Event);
static int16_t Custom_ClampI16(int32_t value);
static uint16_t Custom_ClampU16(int32_t value);
static void Custom_WriteU16LE(uint8_t *dst, uint16_t value);
static void Custom_WriteI16LE(uint8_t *dst, int16_t value);

/* Functions Definition ------------------------------------------------------*/
void SVCCTL_InitCustomSvc(void)
{
  Char_UUID_t uuid;
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint8_t max_attr_record = 9;
  uint8_t initial_ledwrite[1] = {0};
  uint8_t initial_sht40[4] = {0};
  uint8_t initial_scd41[6] = {0};
  uint8_t initial_mlx90614[4] = {0};

  SVCCTL_RegisterSvcHandler(Custom_STM_Event_Handler);

  COPY_LED_CONTROL_UUID(uuid.Char_UUID_128);
  ret = aci_gatt_add_service(BM_UUID_LENGTH,
                             (Service_UUID_t *)&uuid,
                             PRIMARY_SERVICE,
                             max_attr_record,
                             &(CustomContext.CustomLedHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_service command: LED, error code: 0x%x \n\r", ret);
  }

  COPY_LEDWRITE_UUID(uuid.Char_UUID_128);
  ret = aci_gatt_add_char(CustomContext.CustomLedHdle,
                          BM_UUID_LENGTH,
                          &uuid,
                          SizeLedwrite,
                          CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                          ATTR_PERMISSION_NONE,
                          GATT_NOTIFY_ATTRIBUTE_WRITE,
                          0x10,
                          CHAR_VALUE_LEN_CONSTANT,
                          &(CustomContext.CustomLedwriteHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command: LEDWRITE, error code: 0x%x \n\r", ret);
  }

  COPY_SHT40_DATA_UUID(uuid.Char_UUID_128);
  ret = aci_gatt_add_char(CustomContext.CustomLedHdle,
                          BM_UUID_LENGTH,
                          &uuid,
                          SizeSht40Data,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_CONSTANT,
                          &(CustomContext.CustomSht40DataHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command: SHT40, error code: 0x%x \n\r", ret);
  }

  COPY_SCD41_DATA_UUID(uuid.Char_UUID_128);
  ret = aci_gatt_add_char(CustomContext.CustomLedHdle,
                          BM_UUID_LENGTH,
                          &uuid,
                          SizeScd41Data,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_CONSTANT,
                          &(CustomContext.CustomScd41DataHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command: SCD41, error code: 0x%x \n\r", ret);
  }

  COPY_MLX90614_DATA_UUID(uuid.Char_UUID_128);
  ret = aci_gatt_add_char(CustomContext.CustomLedHdle,
                          BM_UUID_LENGTH,
                          &uuid,
                          SizeMlx90614Data,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_CONSTANT,
                          &(CustomContext.CustomMlx90614DataHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command: MLX90614, error code: 0x%x \n\r", ret);
  }

  g_custom_svc_ready = 1;
  (void)Custom_STM_App_Update_Char(CUSTOM_STM_LEDWRITE, initial_ledwrite);
  (void)Custom_STM_App_Update_Char(CUSTOM_STM_SHT40_DATA, initial_sht40);
  (void)Custom_STM_App_Update_Char(CUSTOM_STM_SCD41_DATA, initial_scd41);
  (void)Custom_STM_App_Update_Char(CUSTOM_STM_MLX90614_DATA, initial_mlx90614);
}

tBleStatus Custom_STM_App_Update_Char(Custom_STM_Char_Opcode_t CharOpcode, uint8_t *pPayload)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;

  if (g_custom_svc_ready == 0U)
  {
    return BLE_STATUS_INVALID_PARAMS;
  }

  switch (CharOpcode)
  {
    case CUSTOM_STM_LEDWRITE:
      ret = aci_gatt_update_char_value(CustomContext.CustomLedHdle,
                                       CustomContext.CustomLedwriteHdle,
                                       0,
                                       SizeLedwrite,
                                       pPayload);
      break;

    case CUSTOM_STM_SHT40_DATA:
      ret = aci_gatt_update_char_value(CustomContext.CustomLedHdle,
                                       CustomContext.CustomSht40DataHdle,
                                       0,
                                       SizeSht40Data,
                                       pPayload);
      break;

    case CUSTOM_STM_SCD41_DATA:
      ret = aci_gatt_update_char_value(CustomContext.CustomLedHdle,
                                       CustomContext.CustomScd41DataHdle,
                                       0,
                                       SizeScd41Data,
                                       pPayload);
      break;

    case CUSTOM_STM_MLX90614_DATA:
      ret = aci_gatt_update_char_value(CustomContext.CustomLedHdle,
                                       CustomContext.CustomMlx90614DataHdle,
                                       0,
                                       SizeMlx90614Data,
                                       pPayload);
      break;

    default:
      break;
  }

  return ret;
}

tBleStatus Custom_STM_UpdateSHT40(int32_t temp_c_x100, int32_t rh_x100)
{
  uint8_t payload[4];

  Custom_WriteI16LE(&payload[0], Custom_ClampI16(temp_c_x100));
  Custom_WriteU16LE(&payload[2], Custom_ClampU16(rh_x100));

  return Custom_STM_App_Update_Char(CUSTOM_STM_SHT40_DATA, payload);
}

tBleStatus Custom_STM_UpdateSCD41(uint16_t co2_ppm, int32_t temp_c_x100, int32_t rh_x100)
{
  uint8_t payload[6];

  Custom_WriteU16LE(&payload[0], co2_ppm);
  Custom_WriteI16LE(&payload[2], Custom_ClampI16(temp_c_x100));
  Custom_WriteU16LE(&payload[4], Custom_ClampU16(rh_x100));

  return Custom_STM_App_Update_Char(CUSTOM_STM_SCD41_DATA, payload);
}

tBleStatus Custom_STM_UpdateMLX90614(int32_t ta_c_x100, int32_t to_c_x100)
{
  uint8_t payload[4];

  Custom_WriteI16LE(&payload[0], Custom_ClampI16(ta_c_x100));
  Custom_WriteI16LE(&payload[2], Custom_ClampI16(to_c_x100));

  return Custom_STM_App_Update_Char(CUSTOM_STM_MLX90614_DATA, payload);
}

/* Private functions ----------------------------------------------------------*/
static SVCCTL_EvtAckStatus_t Custom_STM_Event_Handler(void *Event)
{
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;

  event_pckt = (hci_event_pckt *)(((hci_uart_pckt *)Event)->data);

  if (event_pckt->evt != HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE)
  {
    return SVCCTL_EvtNotAck;
  }

  blecore_evt = (evt_blecore_aci *)event_pckt->data;
  if (blecore_evt->ecode != ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE)
  {
    return SVCCTL_EvtNotAck;
  }

  {
    aci_gatt_attribute_modified_event_rp0 *attribute_modified;
    attribute_modified = (aci_gatt_attribute_modified_event_rp0 *)blecore_evt->data;

    if (attribute_modified->Attr_Handle == (CustomContext.CustomLedwriteHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
    {
      uint8_t data = attribute_modified->Attr_Data[0];

      if (data == (uint8_t)'a')
      {
        HAL_GPIO_WritePin(GP0_GPIO_Port, GP0_Pin, GPIO_PIN_SET);
      }
      else if (data == (uint8_t)'b')
      {
        HAL_GPIO_WritePin(GP0_GPIO_Port, GP0_Pin, GPIO_PIN_RESET);
      }
      else
      {
        /* keep GPIO unchanged */
      }

      return SVCCTL_EvtAckFlowEnable;
    }
  }

  return SVCCTL_EvtNotAck;
}

static int16_t Custom_ClampI16(int32_t value)
{
  if (value > 32767)
  {
    return 32767;
  }
  if (value < -32768)
  {
    return -32768;
  }
  return (int16_t)value;
}

static uint16_t Custom_ClampU16(int32_t value)
{
  if (value < 0)
  {
    return 0U;
  }
  if (value > 65535)
  {
    return 65535U;
  }
  return (uint16_t)value;
}

static void Custom_WriteU16LE(uint8_t *dst, uint16_t value)
{
  dst[0] = (uint8_t)(value & 0xFFU);
  dst[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void Custom_WriteI16LE(uint8_t *dst, int16_t value)
{
  uint16_t encoded = (uint16_t)value;
  Custom_WriteU16LE(dst, encoded);
}
