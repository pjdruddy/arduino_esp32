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

#include <WiFi.h>
#include <Wire.h>

#include "esp_wifi.h"


#define LIVE_TIME 60

const wifi_promiscuous_filter_t filt={ 
  .filter_mask=WIFI_PROMIS_FILTER_MASK_ALL
};


typedef struct {
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
  int16_t fctl;
  int16_t duration;
  MacAddr bssid;
  MacAddr sa;
  MacAddr da;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

/* MAC HASH table */

typedef struct mac_bucket{
  MacAddr mac;
  time_t last_seen;
  mac_bucket *next;
} MacBucket;

typedef void (*mac_callback)(MacBucket *);

MacBucket *mac_hash_table[256];
MacBucket *target_mac_hash_table[256];
uint32_t num_macs = 0;

#define MAC_HASH(macaddr) ((macaddr)->mac[0] ^ (macaddr)->mac[4])
#define TO_DS_ONLY (0x0001)

#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14

int curChannel = 1;

char mac_buf[20];
void sprint_mac (MacAddr *macaddr)
{

  sprintf(mac_buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr->mac[0], macaddr->mac[1], macaddr->mac[2], macaddr->mac[3], macaddr->mac[4], macaddr->mac[5]);
}

MacBucket *mac_hash_find(MacBucket **hash_table, MacAddr *macaddr)
{
  MacBucket *bucket = NULL;
  unsigned char hash;
  
  hash = MAC_HASH(macaddr);
  bucket = hash_table[hash];
  while(bucket) {
    if (memcmp(macaddr->mac, bucket->mac.mac, 6) == 0)
      break;
    bucket = bucket->next;
  }

  return bucket;
}

void mac_hash_add(MacBucket **hash_table, MacBucket *elem)
{
  unsigned char hash;
  
  hash = MAC_HASH(&elem->mac);

  elem->next = hash_table[hash];
  hash_table[hash] = elem;
}

void mac_hash_del(MacBucket **hash_table, MacBucket *elem)
{
  unsigned char hash;
  MacBucket *prev;
  
  hash = MAC_HASH(&elem->mac);
  if (mac_hash_table[hash] == elem)     
    mac_hash_table[hash] = NULL;
  else {
    prev = mac_hash_table[hash];
    while (prev && prev->next != elem)
      prev = prev->next;
    if (!prev) {
       Serial.printf("weird prev = 0 element %x\n", prev);
       return;
    }
    else
      prev->next = prev->next->next;
  }
  num_macs--;
  free(elem);
}

void hash_iterate(mac_bucket **hash_table, mac_callback callback)
{
  int hash;
  MacBucket *bucket, *next;
  
  noInterrupts()
  for(hash = 0; hash < 256; hash++) {
    bucket = hash_table[hash];
    while(bucket) {
      /* save next in case bucket gets deleted in the callback */
      next = bucket->next;
      callback(bucket);
      bucket = next;
    }
  }
  interrupts();
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  int fctl = ntohs(wh->fctl);
  MacBucket *mac_hash_elem;
  unsigned char hash;

  len -= sizeof(WifiMgmtHdr);
  if (len < 0){
    Serial.println("Recieved 0");
    return;
  }

  /* only look at to-DS packets */
  if (!(wh->fctl & 0x0080)) 
    return;

  /* have we seen this before */
  if (mac_hash_find(mac_hash_table, (MacAddr *)&wh->sa))
    return;
  
  mac_hash_elem = (MacBucket *)calloc(1, sizeof(*mac_hash_elem));
  memcpy(mac_hash_elem->mac.mac, wh->sa.mac, 6);
  mac_hash_elem->last_seen = time(NULL);
  mac_hash_add(mac_hash_table, mac_hash_elem);
  num_macs++;
}



//===== SETUP =====//
void setup() {
  MacAddr target;
  MacBucket *bucket; 
  
  /* start Serial */
  Serial.begin(115200);
  
  /* sample target mac */
  target.mac[0] = 0xf0; 
  target.mac[1] = 0x98; 
  target.mac[2] = 0x9d; 
  target.mac[3] = 0x1F; 
  target.mac[4] = 0xE4; 
  target.mac[5] = 0x04;
  memset(mac_hash_table, 0, sizeof mac_hash_table);
  memset(target_mac_hash_table, 0, sizeof target_mac_hash_table);
  add_target_mac(&target);

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

void purge_callback(MacBucket *bucket)
{
  time_t inactive;

  if (!bucket)
    Serial.printf("%s bucket NULL", __func__);
  inactive = difftime(time(NULL), bucket->last_seen);
  if (inactive > LIVE_TIME) {
    sprint_mac(&bucket->mac);
    mac_hash_del(mac_hash_table, bucket);
  }
  
}
void purge()
{ 
  hash_iterate(mac_hash_table, purge_callback);
}

void display_callback(MacBucket *bucket)
{
    sprint_mac(&bucket->mac);
    Serial.printf("%s  %u\n", mac_buf, time(NULL) - bucket->last_seen);
}

void display()
{
  Serial.printf("\nMAC Table (%d)\n", num_macs);
  Serial.printf("   MAC            age\n", num_macs);
  Serial.printf("--------------------------------\n", num_macs);

  hash_iterate(mac_hash_table, display_callback);
}

void add_target_mac(MacAddr *macaddr)
{
  MacBucket *bucket;
  unsigned char hash;
  
  hash = MAC_HASH(macaddr);

  if (!mac_hash_find(target_mac_hash_table, macaddr)) {
    bucket = (MacBucket *)calloc(1, sizeof(*bucket));
    bucket->mac = *macaddr;
    mac_hash_add(target_mac_hash_table, bucket);
  }
}

void find_target_callback(MacBucket *bucket)
{
  sprint_mac(&bucket->mac);
  if (mac_hash_find(mac_hash_table, &bucket->mac)) {
      sprint_mac(&bucket->mac);
      Serial.printf("Found MAC %s\n", mac_buf);
    } 
}

void check_for_target()
{
  hash_iterate(target_mac_hash_table, find_target_callback);
}
//===== LOOP =====//
void loop() {
  if(curChannel > maxCh){ 
    curChannel = 1;
  }
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  delay(1000);
  display();
  purge();
  check_for_target();
  curChannel++;
}
