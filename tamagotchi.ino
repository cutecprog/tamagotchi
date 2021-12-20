// All includes
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include "bmp.h"

// All defines
#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_R            35
#define BUTTON_L            0
#define TFT_AMBER           0xfca0

// All Globals
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btnR(BUTTON_R);
Button2 btnL(BUTTON_L);
String menu = "";
RTC_DATA_ATTR uint32_t age = 0;
RTC_DATA_ATTR unsigned char brightness = 255;
int counter;

// Main functions
void setup(void) {
  tft.init();
  configPins();
  setBrightness(brightness);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_AMBER, TFT_BLACK);  // Adding a black background colour erases previous text automatically
  tft.drawCentreString("Home",64,130,4);
  
  btnR.setReleasedHandler(button_handler);
  btnL.setReleasedHandler(button_handler);
}

void loop() {
  printHMS(age + millis()/1000);
  button_loop();
}

// Utility code
void printHMS(uint32_t t)
{
  uint32_t s, m, h;
  String hms;
  s = t % 60;
  t = (t - s)/60;
  m = t % 60;
  t = (t - m)/60;
  h = t;
  hms = (h <=9) ? "0" : "";
  hms.concat(String(h));
  hms.concat(String((m <=9) ?":0" : ":"));
  hms.concat(String(m));
  hms.concat(String((s <=9) ?":0" : ":"));
  hms.concat(String(s));
  tft.drawCentreString(hms,64,0,2);
}

// Screen Brightness code
void setBrightness(uint32_t newBrightness)
{
  ledcWrite(0, newBrightness); // Max is 255, 0 is screen off
}

void configPins()
{
  pinMode(TFT_BL, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
}

// Sleep code
void espDelay(int ms)
// Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void deep_sleep()
{
  age += millis()/1000; // saving seconds awake
  
  tft.fillScreen(TFT_BLACK); // Clear out screen so screen is blank when coming back from sleep
  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200); // This delay is important
  esp_deep_sleep_start();
}

// Button code
void button_loop()
{
  btnR.loop();
  btnL.loop();
}

void button_handler(Button2& btn)
{
  tft.fillScreen(TFT_BLACK);
  if (btn.wasPressedFor() > 600) {
    if (btn == btnL)
      home_screen(btn);
    else
      confirm(btn);
  } else if (menu == "LLR") {
    tft.drawCentreString("Brightness",64,130,4);
    brightness += (btn==btnL) ? -(64>>counter) : 64>>counter;
    setBrightness(brightness);
    tft.drawCentreString(String(brightness),64,32,4);
    counter++;
  } else {
    menu.concat((btn==btnL) ? "L" : "R");
    tft.drawCentreString(menu,64,130,4);
    // Init brightness
    if (menu == "LLR") {
      counter = 0;
      brightness = 128;
      setBrightness(brightness);
      tft.drawCentreString(String(brightness),64,32,4);
    }
  }
}

void home_screen(Button2& btn)
{
  tft.drawCentreString("Home",64,130,4);
  menu = "";
}

void confirm(Button2& btn)
{
  tft.drawCentreString("Deep Sleep",64,130,4);
  tft.drawCentreString(menu,64,0,4);
  menu = "";
  delay(1000);
  // Deep sleep
  deep_sleep();
}
