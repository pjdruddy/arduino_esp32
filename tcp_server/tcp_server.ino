#include <WiFi.h>
#include <WebServer.h>
#include <U8x8lib.h>
#include "ArduinoTimer.h"

/* Put your SSID & Password */
const char* ssid = "ThePottingShed";  // Enter SSID here
const char* password = "61135031";  //Enter Password here
const uint ServerPort = 6666;
WiFiServer TCPServer(ServerPort);


String LEDstate, webPage,notice;

WebServer server(80);
ArduinoTimer SendTimer;
ArduinoTimer ConnectTimer;

uint8_t LED1pin = 0;
bool LED1status = LOW;

uint8_t LED2pin = 25;
bool LED2status = LOW;

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);

WiFiClient RemoteClient;

void CheckForConnections()
{
  if (TCPServer.hasClient())
  {
    // If we are already connected to another computer, 
    // then reject the new connection. Otherwise accept
    // the connection. 
    if (RemoteClient.connected())
    {
      Serial.println("Connection rejected");
      TCPServer.available().stop();
    }
    else
    {
      Serial.println("Connection accepted");
      RemoteClient = TCPServer.available();
    }
  }
}

void setup() {
  Serial.begin(115200);
  u8x8.begin();
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);

  //WiFi.persistent(false);
  //WiFi.softAP(ssid, password);
 // WiFi.softAPConfig(local_ip, gateway, subnet);
 // Connect to Wi-Fi network with SSID and password

  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.println(ssid);
  u8x8.println(WiFi.localIP());
  
  TCPServer.begin();
  Serial.printf("TCP server started port %d", ServerPort);
  Serial.println("Waiting for connection...");
  while (1) {
    CheckForConnections();
    if (RemoteClient.connected())
      break;
    if (ConnectTimer.TimePassed_Milliseconds(1000))
      Serial.printf(".");
  }
}

void loop() 
{
   CheckForConnections();
   web_server.handleClient();
   if (SendTimer.TimePassed_Milliseconds(1000))
   {
     RemoteClient.println("Hello Andy");
   }
}
