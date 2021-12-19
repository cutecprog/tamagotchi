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

// All Globals
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btnR(BUTTON_R);
Button2 btnL(BUTTON_L);
String menu = "";
RTC_DATA_ATTR int age = 0;
RTC_DATA_ATTR unsigned char brightness = 255;
int counter;

// Main functions
void setup(void) {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);  // Adding a black background colour erases previous text automatically
  tft.drawCentreString("Button TFT",64,130,4);
  configPins();
  setBrightness(brightness);

  btnR.setReleasedHandler(right);
  btnL.setReleasedHandler(left);
}

void loop() {
  tft.drawCentreString(String(age + millis()/1000),64,0,4);
  button_loop();
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
  delay(200);
  esp_deep_sleep_start();
}

// Button code
void button_loop()
{
  btnR.loop();
  btnL.loop();
}



void left(Button2& btn)
{
  tft.fillScreen(TFT_BLACK);
  if (btn.wasPressedFor() > 600) {
    tft.drawCentreString(String(btn.wasPressedFor()),64,200,4);
    cancel(btn);
  } else if (menu == "LLR") {
    tft.drawCentreString("Brightness",64,130,4);
    brightness -= (64>>counter);
    setBrightness(brightness);
    tft.drawCentreString(String(brightness),64,32,4);
    counter++;
  } else {
    menu.concat("L");
    tft.drawCentreString(menu,64,130,4);
  }
}

void right(Button2& btn)
{
  tft.fillScreen(TFT_BLACK);
  if (btn.wasPressedFor() > 600) {
    tft.drawCentreString(String(btn.wasPressedFor()),64,200,4);
    confirm(btn);
  } else if (menu == "LLR") {
    tft.drawCentreString("Brightness",64,130,4);
    brightness += (64>>counter);
    setBrightness(brightness);
    tft.drawCentreString(String(brightness),64,32,4);
    counter++;
    return;
  } else {
    menu.concat("R");
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

void cancel(Button2& btn)
{
  tft.drawCentreString("Cancel",64,130,4);
  menu = "";
}

void confirm(Button2& btn)
{
  tft.drawCentreString("Confirm",64,130,4);
  tft.drawCentreString(menu,64,0,4);
  menu = "";

  //espDelay(2000);
  delay(2000);
  
  // Deep sleep
  deep_sleep();
}
