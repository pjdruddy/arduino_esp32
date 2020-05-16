//#include <Adafruit_Circuit_Playground.h>
//#include <Adafruit_CircuitPlayground.h>

#include <HTTPUpdate.h>

#include <ssd1306_16bit.h>
#include <ssd1306_generic.h>
#include <ssd1306.h>
#include <font6x8.h>
#include <ssd1306_1bit.h>
#include <ssd1306_console.h>
#include <ssd1306_8bit.h>
#include <ssd1306_fonts.h>
#include <ssd1306_uart.h>
#include <sprite_pool.h>
#include <nano_gfx.h>
#include <nano_engine.h>
#include <nano_gfx_types.h>

#include <ssd1306_16bit.h>
#include <ssd1306_generic.h>
#include <ssd1306.h>
#include <font6x8.h>
#include <ssd1306_1bit.h>
#include <ssd1306_console.h>
#include <ssd1306_8bit.h>
#include <ssd1306_fonts.h>
#include <ssd1306_uart.h>
#include <sprite_pool.h>
#include <nano_gfx.h>
#include <nano_engine.h>
#include <nano_gfx_types.h>

#include <LiquidCrystal.h>

#include <LiquidCrystal.h>

#include <WiFi.h>
#include <Wire.h>

#include "esp_wifi.h"

//include "SSD1306.h"
//#include "SH1106.h"

String maclist[64][3]; 
int listcount = 0;

String KnownMac[10][2] = {  // Put devices you want to be reconized
  {"Will-Phone","EC1F7ffffffD"},
  {"Will-PC","E894Fffffff3"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"}
  
};

String defaultTTL = "60"; // Maximum time (Apx seconds) elapsed before device is consirded offline

const wifi_promiscuous_filter_t filt={ //Idk what this does
    .filter_mask=WIFI_PROMIS_FILTER_MASK_ALL
};


typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct { // still dont know much about this
  int16_t fctl;
  int16_t duration;
  MacAddr bssid;
  MacAddr sa;
  MacAddr da;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;


typedef struct mac_bucket{
  MacAddr mac;
  mac_bucket *next;
} MacBucket;

MacBucket *mac_hash_table[256];
uint32_t num_macs = 0;

#define MAC_HASH(macaddr) ((macaddr)->mac[0] ^ (macaddr)->mac[4])
#define TO_DS_ONLY (0x0001)

  
#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14


int curChannel = 1;

void print_mac (MacAddr *macaddr)
{

  Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x", macaddr->mac[0], macaddr->mac[1], macaddr->mac[2], macaddr->mac[3], macaddr->mac[4], macaddr->mac[5]);
}

MacBucket *mac_hash_find(MacAddr *macaddr)
{
  MacBucket *bucket;
  unsigned char hash;
  
  hash = MAC_HASH(macaddr);
  bucket = mac_hash_table[hash];
  while(bucket) {
    if (memcmp(macaddr->mac, bucket->mac.mac, 6) == 0)
      break;
    bucket = bucket->next;
  }

  return bucket;
}

void mac_hash_add(MacBucket *elem)
{
  unsigned char hash;
  
  hash = MAC_HASH(&elem->mac);

  elem->next = mac_hash_table[hash];
  mac_hash_table[hash] = elem;
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf; // Dont know what these 3 lines do
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  len -= sizeof(WifiMgmtHdr);
  if (len < 0){
    Serial.println("Receuved 0");
    return;
  }
  String packet;
  String mac;
  int fctl = ntohs(wh->fctl);
  int found = 0;
  MacBucket *mac_hash_elem;
  unsigned char hash;

  if (!(wh->fctl & 0x0080))
    return;
  if (mac_hash_find((MacAddr *)&wh->sa))
    return;
  mac_hash_elem = (MacBucket *)calloc(1, sizeof(*mac_hash_elem));
  memcpy(mac_hash_elem->mac.mac, wh->sa.mac, 6);
  mac_hash_add(mac_hash_elem);
  num_macs++;
  Serial.printf("FCTL: %04x SRC: ", wh->fctl);
  print_mac(&wh->sa);
  Serial.printf(" DST: ");
  print_mac(&wh->da);
  Serial.printf(" BSSID: ");
  print_mac(&wh->bssid);
  Serial.printf(" Hash %d ", MAC_HASH(&wh->sa));
  Serial.printf(" count %d\n", num_macs);
//  if (p->payload[16] == 0x92) {
//    Serial.printf("PJDR Got MAC: ");
//    found = 1;
//  }
//  for(int i=10;i<=17;i++){ // This reads the first couple of bytes of the packet. This is where you can read the whole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
//        Serial.printf("%02x:", p->payload[i]);
//     packet += String(p->payload[i],HEX);
//  }
//  for(int i=4;i<=15;i++){ // This removes the 'nibble' bits from the stat and end of the data we want. So we only get the mac address.
//    mac += packet[i];
//  }
  mac.toUpperCase();
 
    Serial.printf("\n");
  int added = 0;
  for(int i=0;i<=63;i++){ // checks if the MAC address has been added before
    if(mac == maclist[i][0]){
      maclist[i][1] = defaultTTL;
      if(maclist[i][2] == "OFFLINE"){
        maclist[i][2] = "0";
      }
      added = 1;
    }
  }
  
  if(added == 0){ // If its new. add it to the array.
    maclist[listcount][0] = mac;
    maclist[listcount][1] = defaultTTL;
//    Serial.println(mac);
    listcount ++;
    if(listcount >= 64){
      Serial.println("Too many addresses");
      listcount = 0;
    }
  }
}



//===== SETUP =====//
void setup() {

  /* start Serial */
  Serial.begin(115200);
 // setupOLED();

  memset(mac_hash_table, 0, sizeof mac_hash_table);
  /* setup wifi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  
  Serial.println("starting!");
}

void purge(){ // This maanges the TTL
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      int ttl = (maclist[i][1].toInt());
      ttl --;
      if(ttl <= 0){
        //Serial.println("OFFLINE: " + maclist[i][0]);
        maclist[i][2] = "OFFLINE";
        maclist[i][1] = defaultTTL;
      }else{
        maclist[i][1] = String(ttl);
      }
    }
  }
}

void updatetime(){ // This updates the time the device has been online for
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      if(maclist[i][2] == "")maclist[i][2] = "0";
      if(!(maclist[i][2] == "OFFLINE")){
          int timehere = (maclist[i][2].toInt());
          timehere ++;
          maclist[i][2] = String(timehere);
      }
      
      //Serial.println(maclist[i][0] + " : " + maclist[i][2]);
      
    }
  }
}

void showpeople(){ // This checks if the MAC is in the reckonized list and then displays it on the OLED and/or prints it to serial.
  String forScreen = "";
  for(int i=0;i<=63;i++){
    String tmp1 = maclist[i][0];
    if(!(tmp1 == "")){
      for(int j=0;j<=9;j++){
        String tmp2 = KnownMac[j][1];
        if(tmp1 == tmp2){
          forScreen += (KnownMac[j][0] + " : " + maclist[i][2] + "\n");
//          Serial.print(KnownMac[j][0] + " : " + tmp1 + " : " + maclist[i][2] + "\n -- \n");
        }
      }
    }
  }
  //update_screen_text(forScreen);
}

//===== LOOP =====//
void loop() {
//    Serial.println("Changed channel:" + String(curChannel));
    if(curChannel > maxCh){ 
      curChannel = 1;
    }
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
    delay(10000);
//    updatetime();
//    purge();
//    showpeople();
    curChannel++;
}

// Looks like ill have to add the OLED in the same file.
// ----------------------------------- OLED FILE ---------------
// VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
// This will all still work like it is, I just put this part in a differant tab.
