#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>

static void scan_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  //Serial.printf("PJDR: got event callback type %d\n", event);
  switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x1, 0);
      break;
    case ESP_BT_GAP_DISC_RES_EVT:
      Serial.printf("He's Here!!! Result : %02x:%02x:%02x:%02x:%02x:%02x\n", param->disc_res.bda[0], 
      param->disc_res.bda[1],param->disc_res.bda[2],param->disc_res.bda[3],param->disc_res.bda[4],
      param->disc_res.bda[5]);
      break;
  }

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

  if ((err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_LIMITED_INQUIRY, 0x1, 0)) != ESP_OK) {
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
