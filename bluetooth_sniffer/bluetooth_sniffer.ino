#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>

static void scan_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  //Serial.printf("PJDR: got event callback type %d\n", event);
  switch (event) {

    case ESP_BT_GAP_DISC_RES_EVT:
      /*!< device discovery result event */
      Serial.printf("ESP_BT_GAP_DISC_RES_EVT: "); 
      printBda(param->disc_res.bda);
      break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      /*!< discovery state changed event */
      esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x1, 0);
      break;
    case ESP_BT_GAP_RMT_SRVCS_EVT:
      /*!< get remote services event */
      Serial.printf("ESP_BT_GAP_RMT_SRVCS_EVT: "); 
      printBda(param->rmt_srvcs.bda);
      break;
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
      /*!< get remote service record event */
      Serial.printf("ESP_BT_GAP_RMT_SRVC_REC_EVT: ");
      printBda(param->rmt_srvc_rec.bda);
      break;
    case ESP_BT_GAP_AUTH_CMPL_EVT:
      /*!< AUTH complete event */
      Serial.printf("ESP_BT_GAP_AUTH_CMPL_EVT: ");
      printBda(param->auth_cmpl.bda);
      break;
    case ESP_BT_GAP_PIN_REQ_EVT:
      /*!< Legacy Pairing Pin code request */
      Serial.printf("ESP_BT_GAP_PIN_REQ_EVT: ");
      printBda(param->pin_req.bda);
      break;
    case ESP_BT_GAP_CFM_REQ_EVT:
      /*!< Simple Pairing User Confirmation request. */
      Serial.printf("ESP_BT_GAP_CFM_REQ_EVT: ");
      printBda(param->cfm_req.bda);
      break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      /*!< Simple Pairing Passkey Notification */
      Serial.printf("ESP_BT_GAP_KEY_NOTIF_EVT: ");
      printBda(param->key_notif.bda);
      break;
    case ESP_BT_GAP_KEY_REQ_EVT:
      /*!< Simple Pairing Passkey request */
      Serial.printf("ESP_BT_GAP_KEY_REQ_EVT: ");
      printBda(param->key_req.bda);
      break;
    case ESP_BT_GAP_READ_RSSI_DELTA_EVT:
      /*!< read rssi event */
      Serial.printf("ESP_BT_GAP_READ_RSSI_DELTA_EVT: ");
      printBda(param->read_rssi_delta.bda);
      break;
    case ESP_BT_GAP_EVT_MAX:
      Serial.printf("ESP_BT_GAP_EVT_MAX: ");
      break;
  }
}

static void printBda(const esp_bd_addr_t bda) {
   Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",bda[0],bda[1],bda[2],bda[3],bda[4],bda[5]);
}

static bool initBluetooth(const char *deviceName)
{
  esp_err_t err;
  if(!btStart()) {
    Serial.println("Failed to initialize controller");
    return false;
  }

  if (esp_bluedroid_init()!= ESP_OK) {
    Serial.println("Failed to initialize bluedroid");
    return false;
  }
  if (esp_bluedroid_enable()!= ESP_OK) {
    Serial.println("Failed to enable bluedroid");
  }

  esp_bt_dev_set_device_name(deviceName);

  if (esp_bt_gap_register_callback(&scan_callback) != ESP_OK) {
    Serial.println("Failed to register scan_callback");
  }

  if (esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE) != ESP_OK) {
    Serial.println("Failed to set scan mode");
  }

  if ((err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x1, 0)) != ESP_OK) {
    Serial.printf("Failed to start discovery %x\n", err);
  }
  esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
}

void setup()
{
  Serial.begin(115200);
  initBluetooth("dingledongle");
  Serial.println("starting...");
}

void loop()
{
  // put your main code here, to run repeatedly:

}
