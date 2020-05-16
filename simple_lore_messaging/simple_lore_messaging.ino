/*
 * Lore Messaging between 2 wifi networks
 * Pat Ruddy April 2020
 */
#include <WiFi.h>
#include <WebServer.h>
#include <U8x8lib.h>
#include "ArduinoTimer.h"

/* OLED settings */
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);

/* timers */
ArduinoTimer send_timer;
ArduinoTimer connect_timer;

/* WIFI creds - probably should be programmed by and initial AP setup*/
const char* ssid = "ThePottingShed";  // Enter SSID here
const char* password = "";  //Enter Password here

/* Handy Network Serial service for debugging */
WiFiServer debug_server(6666);
WiFiClient remote_client;

/* Web server on port 80 */
WebServer web_server(80);
String message_log[4];
uint8_t log_index = 0;
uint8_t new_msg = 0;
uint8_t msg_count = 0;

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

void setup() {
  /* setup serial port */
  Serial.begin(115200);

  /* initilaise OLED */
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

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

  /* set up web server */
  web_server.on("/", connect_handler);
  web_server.on("/postform", post_handler);
  web_server.onNotFound(not_found_handler);
  web_server.begin();
  Serial.println("HTTP server started");
}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  int msgs, msg_index;
  char buf[16];

  msgs = msg_count;
  if(msg_count < 4)
    msg_index = 0;
  else
    msg_index = log_index;

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
      message_log[msg_index].toCharArray(buf, 16);
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
  message_log[log_index] = web_server.arg("txt");
  log_index++;
  if (log_index == 4)
    log_index = 0;
  if(msg_count < 4)
    msg_count++;
  new_msg = 1;
  Serial.printf("msg_count %d, log_index %d\n", msg_count, log_index);
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

  CheckForConnections();
   web_server.handleClient();
   if (send_timer.TimePassed_Milliseconds(1000))
   {
     remote_client.println("Hello Andy");
   }

   msgs = msg_count;

  if(msg_count < 4)
    msg_index = 0;
  else
    msg_index = log_index;

  if(new_msg) {
    Serial.printf("msg_index %d, msg_count %d\n", msg_index, msg_count);
    u8x8.clear();
    while(msgs) {
      u8x8.println(message_log[msg_index]);
      Serial.println(message_log[msg_index]);
      if (++msg_index == 4)
        msg_index = 0;
      msgs--;
    }
    new_msg = 0;
  }
}
