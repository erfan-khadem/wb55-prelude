/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/app_ble.c
  * @author  MCD Application Team
  * @brief   BLE application implementation
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

/* Private includes ----------------------------------------------------------*/
#include "app_ble.h"
#include "ble.h"
#include "cmsis_os2.h"
#include "shci.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint16_t ConnectionHandle;
} APP_BLE_Context_t;

/* Private defines -----------------------------------------------------------*/
#define INVALID_CONN_HANDLE    0xFFFFU
#define HCI_EVT_FLAG           1U

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

static osMutexId_t MtxHciId;
static osSemaphoreId_t SemHciId;
static osThreadId_t HciUserEvtProcessId;

static APP_BLE_Context_t BleCtx =
{
  .ConnectionHandle = INVALID_CONN_HANDLE,
};

static const char kDeviceName[] = CFG_GAP_DEVICE_NAME;
static const osThreadAttr_t HciUserEvtProcess_attr =
{
  .name = CFG_HCI_USER_EVT_PROCESS_NAME,
  .attr_bits = CFG_HCI_USER_EVT_PROCESS_ATTR_BITS,
  .cb_mem = CFG_HCI_USER_EVT_PROCESS_CB_MEM,
  .cb_size = CFG_HCI_USER_EVT_PROCESS_CB_SIZE,
  .stack_mem = CFG_HCI_USER_EVT_PROCESS_STACK_MEM,
  .priority = CFG_HCI_USER_EVT_PROCESS_PRIORITY,
  .stack_size = CFG_HCI_USER_EVT_PROCESS_STACK_SIZE
};

/* Private function prototypes -----------------------------------------------*/
static void APP_BLE_SetDiscoverable(void);
static void APP_BLE_CheckStatus(tBleStatus status);
static void APP_BLE_CheckShciStatus(SHCI_CmdStatus_t status);
static void APP_BLE_UserEvtRx(void *pPayload);
static void APP_BLE_Tl_Init(void);
static void APP_BLE_HciStatusNot(HCI_TL_CmdStatus_t status);
static void HciUserEvtProcess(void *argument);

/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void)
{
  SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet = {0};
  SHCI_CmdStatus_t shci_status;
  tBleStatus status;
  uint16_t gap_service_handle;
  uint16_t gap_dev_name_char_handle;
  uint16_t gap_appearance_char_handle;
  const uint8_t cfg_ble_er[16] = CFG_BLE_ER;
  const uint8_t cfg_ble_ir[16] = CFG_BLE_IR;
  uint64_t bd_addr_cfg = CFG_ADV_BD_ADDRESS;
  uint8_t bd_addr[CONFIG_DATA_PUBLIC_ADDRESS_LEN];

  ble_init_cmd_packet.Param.pBleBufferAddress = 0;
  ble_init_cmd_packet.Param.BleBufferSize = 0;
  ble_init_cmd_packet.Param.NumAttrRecord = CFG_BLE_NUM_GATT_ATTRIBUTES;
  ble_init_cmd_packet.Param.NumAttrServ = CFG_BLE_NUM_GATT_SERVICES;
  ble_init_cmd_packet.Param.AttrValueArrSize = CFG_BLE_ATT_VALUE_ARRAY_SIZE;
  ble_init_cmd_packet.Param.NumOfLinks = CFG_BLE_NUM_LINK;
  ble_init_cmd_packet.Param.ExtendedPacketLengthEnable = CFG_BLE_DATA_LENGTH_EXTENSION;
  ble_init_cmd_packet.Param.PrWriteListSize = CFG_BLE_PREPARE_WRITE_LIST_SIZE;
  ble_init_cmd_packet.Param.MblockCount = CFG_BLE_MBLOCK_COUNT;
  ble_init_cmd_packet.Param.AttMtu = CFG_BLE_MAX_ATT_MTU;
  ble_init_cmd_packet.Param.PeripheralSca = CFG_BLE_PERIPHERAL_SCA;
  ble_init_cmd_packet.Param.CentralSca = CFG_BLE_CENTRAL_SCA;
  ble_init_cmd_packet.Param.LsSource = CFG_BLE_LS_SOURCE;
  ble_init_cmd_packet.Param.MaxConnEventLength = CFG_BLE_MAX_CONN_EVENT_LENGTH;
  ble_init_cmd_packet.Param.HsStartupTime = CFG_BLE_HSE_STARTUP_TIME;
  ble_init_cmd_packet.Param.ViterbiEnable = CFG_BLE_VITERBI_MODE;
  ble_init_cmd_packet.Param.Options = CFG_BLE_OPTIONS;
  ble_init_cmd_packet.Param.HwVersion = 0;
  ble_init_cmd_packet.Param.max_coc_initiator_nbr = CFG_BLE_MAX_COC_INITIATOR_NBR;
  ble_init_cmd_packet.Param.min_tx_power = CFG_BLE_MIN_TX_POWER;
  ble_init_cmd_packet.Param.max_tx_power = CFG_BLE_MAX_TX_POWER;
  ble_init_cmd_packet.Param.rx_model_config = CFG_BLE_RX_MODEL_CONFIG;
  ble_init_cmd_packet.Param.max_adv_set_nbr = CFG_BLE_MAX_ADV_SET_NBR;
  ble_init_cmd_packet.Param.max_adv_data_len = CFG_BLE_MAX_ADV_DATA_LEN;
  ble_init_cmd_packet.Param.tx_path_compens = CFG_BLE_TX_PATH_COMPENS;
  ble_init_cmd_packet.Param.rx_path_compens = CFG_BLE_RX_PATH_COMPENS;
  ble_init_cmd_packet.Param.ble_core_version = CFG_BLE_CORE_VERSION;
  ble_init_cmd_packet.Param.Options_extension = CFG_BLE_OPTIONS_EXT;
  ble_init_cmd_packet.Param.MaxAddEattBearers = CFG_BLE_MAX_ADD_EATT_BEARERS;
  ble_init_cmd_packet.Param.extra_data_buffer = 0;
  ble_init_cmd_packet.Param.extra_data_buffer_size = 0;

  APP_BLE_Tl_Init();

  shci_status = SHCI_C2_BLE_Init(&ble_init_cmd_packet);
  APP_BLE_CheckShciStatus(shci_status);

  status = hci_reset();
  APP_BLE_CheckStatus(status);

  for (uint32_t i = 0; i < CONFIG_DATA_PUBLIC_ADDRESS_LEN; i++)
  {
    bd_addr[i] = (uint8_t)((bd_addr_cfg >> (8U * i)) & 0xFFU);
  }

  status = aci_hal_write_config_data(CONFIG_DATA_PUBLIC_ADDRESS_OFFSET,
                                     CONFIG_DATA_PUBLIC_ADDRESS_LEN,
                                     bd_addr);
  APP_BLE_CheckStatus(status);

  status = aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET,
                                     CONFIG_DATA_ER_LEN,
                                     cfg_ble_er);
  APP_BLE_CheckStatus(status);

  status = aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET,
                                     CONFIG_DATA_IR_LEN,
                                     cfg_ble_ir);
  APP_BLE_CheckStatus(status);

  status = aci_gatt_init();
  APP_BLE_CheckStatus(status);

  status = aci_gap_init(GAP_PERIPHERAL_ROLE,
                        PRIVACY_DISABLED,
                        (uint8_t)(sizeof(kDeviceName) - 1U),
                        &gap_service_handle,
                        &gap_dev_name_char_handle,
                        &gap_appearance_char_handle);
  APP_BLE_CheckStatus(status);

  status = aci_gatt_update_char_value(gap_service_handle,
                                      gap_dev_name_char_handle,
                                      0,
                                      (uint8_t)(sizeof(kDeviceName) - 1U),
                                      (const uint8_t *)kDeviceName);
  APP_BLE_CheckStatus(status);

  status = aci_gap_set_io_capability(CFG_IO_CAPABILITY);
  APP_BLE_CheckStatus(status);

  status = aci_gap_set_authentication_requirement(CFG_BONDING_MODE,
                                                  CFG_MITM_PROTECTION,
                                                  CFG_SC_SUPPORT,
                                                  CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                                  CFG_ENCRYPTION_KEY_SIZE_MIN,
                                                  CFG_ENCRYPTION_KEY_SIZE_MAX,
                                                  0U,
                                                  0U,
                                                  GAP_PUBLIC_ADDR);
  APP_BLE_CheckStatus(status);

  status = aci_hal_set_tx_power_level(1U, CFG_TX_POWER);
  APP_BLE_CheckStatus(status);

  SVCCTL_Init();

  APP_BLE_SetDiscoverable();
}

SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(void *pckt)
{
  hci_event_pckt *event_pckt;
  evt_le_meta_event *meta_evt;

  event_pckt = (hci_event_pckt *)((hci_uart_pckt *)pckt)->data;

  switch (event_pckt->evt)
  {
    case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
    {
      hci_disconnection_complete_event_rp0 *disconnection_event;
      disconnection_event = (hci_disconnection_complete_event_rp0 *)event_pckt->data;
      if (disconnection_event->Status == BLE_STATUS_SUCCESS)
      {
        BleCtx.ConnectionHandle = INVALID_CONN_HANDLE;
        APP_BLE_SetDiscoverable();
      }
      break;
    }

    case HCI_LE_META_EVT_CODE:
    {
      meta_evt = (evt_le_meta_event *)event_pckt->data;
      if (meta_evt->subevent == HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE)
      {
        hci_le_connection_complete_event_rp0 *connection_complete_event;
        connection_complete_event = (hci_le_connection_complete_event_rp0 *)meta_evt->data;
        BleCtx.ConnectionHandle = connection_complete_event->Connection_Handle;
      }
      break;
    }

    default:
      break;
  }

  return SVCCTL_UserEvtFlowEnable;
}

void hci_notify_asynch_evt(void *pdata)
{
  UNUSED(pdata);
  if (HciUserEvtProcessId != NULL)
  {
    osThreadFlagsSet(HciUserEvtProcessId, HCI_EVT_FLAG);
  }
}

void hci_cmd_resp_release(uint32_t flag)
{
  UNUSED(flag);
  osSemaphoreRelease(SemHciId);
}

void hci_cmd_resp_wait(uint32_t timeout)
{
  UNUSED(timeout);
  osSemaphoreAcquire(SemHciId, osWaitForever);
}

void SVCCTL_ResumeUserEventFlow(void)
{
  hci_resume_flow();
}

/* Private functions ---------------------------------------------------------*/
static void APP_BLE_SetDiscoverable(void)
{
  tBleStatus status;
  uint8_t local_name[1U + (sizeof(kDeviceName) - 1U)];

  local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
  memcpy(&local_name[1], kDeviceName, sizeof(kDeviceName) - 1U);

  status = aci_gap_set_discoverable(ADV_IND,
                                    CFG_FAST_CONN_ADV_INTERVAL_MIN,
                                    CFG_FAST_CONN_ADV_INTERVAL_MAX,
                                    BLE_ADDR_TYPE,
                                    0U,
                                    (uint8_t)sizeof(local_name),
                                    local_name,
                                    0U,
                                    NULL,
                                    0U,
                                    0U);
  APP_BLE_CheckStatus(status);
}

static void APP_BLE_CheckStatus(tBleStatus status)
{
  if (status != BLE_STATUS_SUCCESS)
  {
    Error_Handler();
  }
}

static void APP_BLE_CheckShciStatus(SHCI_CmdStatus_t status)
{
  if (status != SHCI_Success)
  {
    Error_Handler();
  }
}

static void APP_BLE_UserEvtRx(void *pPayload)
{
  SVCCTL_UserEvtFlowStatus_t svc_flow_status;
  tHCI_UserEvtRxParam *user_event;

  user_event = (tHCI_UserEvtRxParam *)pPayload;
  svc_flow_status = SVCCTL_UserEvtRx((void *)&(user_event->pckt->evtserial));

  if (svc_flow_status != SVCCTL_UserEvtFlowEnable)
  {
    user_event->status = HCI_TL_UserEventFlow_Disable;
  }
}

static void APP_BLE_Tl_Init(void)
{
  HCI_TL_HciInitConf_t hci_init_conf;

  MtxHciId = osMutexNew(NULL);
  SemHciId = osSemaphoreNew(1U, 0U, NULL);
  HciUserEvtProcessId = osThreadNew(HciUserEvtProcess, NULL, &HciUserEvtProcess_attr);

  if ((MtxHciId == NULL) || (SemHciId == NULL) || (HciUserEvtProcessId == NULL))
  {
    Error_Handler();
  }

  hci_init_conf.p_cmdbuffer = (uint8_t *)&BleCmdBuffer;
  hci_init_conf.StatusNotCallBack = APP_BLE_HciStatusNot;
  hci_init(APP_BLE_UserEvtRx, (void *)&hci_init_conf);
}

static void APP_BLE_HciStatusNot(HCI_TL_CmdStatus_t status)
{
  if (status == HCI_TL_CmdBusy)
  {
    osMutexAcquire(MtxHciId, osWaitForever);
  }
  else if (status == HCI_TL_CmdAvailable)
  {
    osMutexRelease(MtxHciId);
  }
  else
  {
    /* Nothing to do */
  }
}

static void HciUserEvtProcess(void *argument)
{
  UNUSED(argument);

  for (;;)
  {
    osThreadFlagsWait(HCI_EVT_FLAG, osFlagsWaitAny, osWaitForever);
    hci_user_evt_proc();
  }
}
