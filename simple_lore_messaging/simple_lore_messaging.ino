/*
 * Lore Messaging between 2 wifi networks
 * Pat Ruddy April 2020
 */
#include <WiFi.h>
#include <WebServer.h>
#include <U8x8lib.h>
#include <Layer1.h>
#include <LoRaLayer2.h>
#include "ArduinoTimer.h"

/* OLED settings */
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);

/* timers */
ArduinoTimer send_timer;
ArduinoTimer connect_timer;

/* WIFI creds - probably should be programmed by and initial AP setup*/
const char* ssid = "HomeNetwork";  // Enter SSID here
const char* password = "yeahright";  //Enter Password here

/* Handy Network Serial service for debugging */
WiFiServer debug_server(6666);
WiFiClient remote_client;

/* Web server on port 80 */
WebServer web_server(80);

/* Logging infra */
/* this should be refactored into a c++ class */
struct message_log {
       String buf[4];
       uint8_t log_index;
       uint8_t new_msg;
       uint8_t msg_count;
       uint8_t last_index;
};

struct message_log input_log;


/* LoRa Parameters */
uint8_t my_addr[ADDR_LENGTH] = {0xbe, 0xef, 0xbe, 0xef };
uint8_t tgt_addr[ADDR_LENGTH] = { 0xde, 0xad, 0xde, 0xad };
//uint8_t my_addr[ADDR_LENGTH] = {0xde, 0xad, 0xde, 0xad };
//uint8_t tgt_addr[ADDR_LENGTH] = { 0xbe, 0xef, 0xbe, 0xef};

#define RING_MAX 4
/* store message in a ring buffer */
void message_store(struct message_log *log, String msg)
{
  log->buf[log->log_index] = msg;
  log->last_index = log->log_index;
  log->log_index++;
  if (log->log_index == RING_MAX)
    log->log_index = 0;
  if(log->msg_count < RING_MAX)
    log->msg_count++;
  log->new_msg = true;
}

void CheckForConnections()
{
  if (debug_server.hasClient())
  {
    /* one connection at a time */
    if (remote_client.connected())
    {
      Serial.println("Connection rejected");
      debug_server.available().stop();
    }
    else
    {
      Serial.println("Connection accepted");
      remote_client = debug_server.available();
    }
  }
}

void sendMessage(uint8_t dst[ADDR_LENGTH], uint8_t* msg_data, size_t msg_length)
{
  remote_client.printf("Sending packet to %02x%02x%02x%02x with %d data bytes... %s\n", dst[0], dst[1], dst[2], dst[3], msg_length, msg_data);
  Datagram dg = {
    dst[0], dst[1], dst[2], dst[3],
    'm'
    };
  memcpy(dg.message, msg_data, msg_length);
  LL2.writeData(dg, msg_length+1+(ADDR_LENGTH*2));
}

void printAddress(uint8_t address[ADDR_LENGTH]){
    for( int i = 0 ; i < ADDR_LENGTH; i++){
      Serial.printf("%02x", address[i]);
    }
}

void setup() {
  /* setup serial port */
  Serial.begin(115200);
  char local_addr[9];

  /* initilaise OLED */
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  input_log.log_index = 0;
  input_log.new_msg = false;
  input_log.msg_count = 0;
  input_log.last_index = 0;

 /* setup WIFI */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  u8x8.println(ssid);
  u8x8.println(WiFi.localIP());

  /* setup debug server and wait for connection */
  debug_server.begin();
  Serial.printf("TCP server started port %d", 6666);
  Serial.println("Waiting for connection...");
  while (1) {
    CheckForConnections();
    if (remote_client.connected())
      break;
    if (connect_timer.TimePassed_Milliseconds(1000))
      Serial.printf(".");
  }

  /* set up LoRa layer 1 */
  Serial.println("HTTP server started");
  Serial.printf("LoRa Layer1 setup...");
  Layer1.setPins(18, 14, 26);
  Layer1.setSPIFrequency(100E3);
  Layer1.setLoRaFrequency(866E6);
  Layer1.setSpreadingFactor(9);
  Layer1.setTxPower(20);
  Layer1.init();
  Serial.println(" complete!");

  /* set up LoRa Later 2 */
   Serial.print("LoRa Layer2 setup... ");
  LL2.init();
  snprintf(local_addr, sizeof(local_addr), "%02x%02x%02x%02x", my_addr[0], my_addr[1], my_addr[2], my_addr[3]);
  remote_client.printf("Local Address %s\n", local_addr);
  LL2.setLocalAddress((const char *)local_addr);
  LL2.setInterval(20000);
  Serial.println(" complete!");
  Serial.print("Local device address is: ");
  printAddress(LL2.localAddress());
  Serial.println("");

  /* set up web server */
  web_server.on("/", connect_handler);
  web_server.on("/postform", post_handler);
  web_server.onNotFound(not_found_handler);
  web_server.begin();

}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  int msgs, msg_index;
  char buf[16];

  msgs = input_log.msg_count;
  if(input_log.msg_count < RING_MAX)
    msg_index = 0;
  else
    msg_index = input_log.log_index;

  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LORE Internet</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 LORE Internet</h1>\n";
  ptr +="<h3>Connected to";
  ptr +=ssid;
  ptr +="</h3>\n";
  ptr +="<FORM ACTION=\"/postform\" method=\"POST\">";
  ptr +="free text: <INPUT TYPE=TEXT NAME=\"txt\" VALUE=\"\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>";
  ptr +="<INPUT TYPE=SUBMIT NAME=\"submit\" VALUE=\"submit\">";
  ptr +="</FORM>";
  while(msgs) {
      input_log.buf[msg_index].toCharArray(buf, 16);
      ptr += buf;
      ptr+= "<br>";
      if (++msg_index == 4)
	msg_index = 0;
      msgs--;
  }
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void connect_handler()
{
  Serial.println("got connect");
  web_server.send(200, "text/html", SendHTML());
}

void post_handler()
{
    //webPage = htmlPage;
  message_store(&input_log, web_server.arg("txt"));

  Serial.printf("msg_count %d, log_index %d\n", input_log.msg_count, input_log.log_index);
  web_server.send(200, "text/html", SendHTML());
}

void not_found_handler()
{
  web_server.send(404, "text/plain", "Not found");
}


void loop()
{
  char buf[16];
  int msgs, msg_index;
  Packet pkt;
  Datagram dg;

  CheckForConnections();
   web_server.handleClient();

   msgs = input_log.msg_count;

  if(msgs < 4)
    msg_index = 0;
  else
    msg_index = input_log.log_index;

  if(input_log.new_msg) {
    Serial.printf("msg_index %d, msg_count %d\n", msg_index, input_log.msg_count);
    u8x8.clear();
    input_log.buf[input_log.last_index].toCharArray(buf,16);
    sendMessage(tgt_addr, (uint8_t *)buf, 16);
    while(msgs) {
      Serial.println(input_log.buf[msg_index]);
      if (++msg_index == 4)
	msg_index = 0;
      msgs--;
    }
    input_log.new_msg = 0;
  }

  /* Service the LoRa Layer2 stack */
  if (LL2.daemon()==1) {
    /* Self-routing packet recieved - pop it off the buffer and do nothing */
    remote_client.printf("\nself routing Message recieved\n");
    pkt = LL2.readData();
  }

  /* Check for any new packets and process them */
  pkt = LL2.readData();
    if (pkt.totalLength > 0) {
      Datagram dg;
      remote_client.printf("\nMessage recieved\n");
      memcpy(&dg, &pkt.datagram, PACKET_LENGTH-HEADER_LENGTH);
      remote_client.printf("Packet %d recieved from %02x%02x%02x%02x for %02x%02x%02x%02x via %02x%02x%02x%02x (%d bytes): Type %02x: %s\r\n",
      pkt.sequence,
      pkt.sender[0], pkt.sender[1], pkt.sender[2], pkt.sender[3],
      dg.destination[0], dg.destination[1], dg.destination[2], dg.destination[3],
      pkt.receiver[0], pkt.receiver[1], pkt.receiver[2], pkt.receiver[3],
      pkt.totalLength, dg.type, dg.message);
      u8x8.clear();
     u8x8.printf("%s", dg.message);
    }

   if (send_timer.TimePassed_Milliseconds(10000))
   {
     remote_client.println("*");
     //sendMessage(tgt_addr, (uint8_t *)"1", 2);
   }

}
