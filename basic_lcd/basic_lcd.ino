#include <U8x8lib.h>

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);

void setup()
{
  u8x8.begin();
}

void loop()
{
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"Hello World");
  delay(1000);  
}
