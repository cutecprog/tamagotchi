// All includes
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "Button2.h"
#include <sys/time.h>

// All defines
//#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_R            35
#define BUTTON_L            0
#define TFT_AMBER           0xfca0

#define HOME                128
#define BRIGHTNESS          48
#define TAMAGOTCHI          192

// All Globals
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btnR(BUTTON_R);
Button2 btnL(BUTTON_L);
String menu = "";
unsigned char menu_selection;
int counter;

// All SRAM Globals
RTC_DATA_ATTR timeval age;
RTC_DATA_ATTR unsigned char brightness = 255;
RTC_DATA_ATTR struct {
  int i = 57;
} tama;

// Main functions
void setup(void) {
  //Serial.begin(115200);
  tft.init();
  init_brightness_control();
  set_brightness(brightness);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_AMBER, TFT_BLACK);  // Adding a black background colour erases previous text automatically
  home_screen();

  btnR.setReleasedHandler(button_handler);
  btnL.setReleasedHandler(button_handler);
}

void loop() {
  if ((menu_selection == HOME) && (millis()%1000 == 0))
    clock_loop();
  // Run button_handler if pressed
  btnR.loop();
  btnL.loop();
}

// Clock code
void clock_loop()
{
  gettimeofday(&age, NULL);
  printHMS(age.tv_sec, 0);
  tft.drawCentreString(String(analogRead(ADC_PIN)),64,20,4);
}

void printHMS(uint32_t t, uint32_t y)
{
  uint32_t s, m, h;
  String hms;
  s = t % 60;
  t = (t - s)/60;
  m = t % 60;
  t = (t - m)/60;
  h = t % 24;
  t = (t - h)/24;  // t holds the number of days

  hms = "   ";
  if (t > 0) {
    hms.concat("Day ");
    hms.concat(String(t));
    hms.concat(String(" "));
  }
  hms.concat((h <=9) ? "0" : "");
  hms.concat(String(h));
  hms.concat((m <=9) ?":0" : ":");
  hms.concat(String(m));
  hms.concat((s <=9) ?":0" : ":");
  hms.concat(String(s));
  hms.concat("   ");
  tft.drawCentreString(hms,64,y,2);
}

// Screen Brightness code
void set_brightness(uint32_t n)
{
  ledcWrite(0, n); // Max is 255, 0 is screen off
}

void init_brightness_control()
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
  tft.fillScreen(TFT_BLACK); // Clear out screen so screen is blank when coming back from sleep
  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200); // This delay is important
  esp_deep_sleep_start();
}

// Menu code
void menu_init()
{
  // init and display switch-case
  switch(menu_selection) {
    case BRIGHTNESS:
      counter = 64;
      brightness = 128;
      set_brightness(brightness);
      tft.drawCentreString("Brightness",64,130,4);
      tft.drawCentreString(String(brightness),64,32,4);
    break;
    case TAMAGOTCHI:
      tft.drawCentreString("Tamagotchi",64,0,2);
      tama.i =0;
    break;
    default:
      tft.drawCentreString(menu,64,130,4);
      tft.drawCentreString(String(menu_selection),64,180,4);
  }
}

void menu_loop(Button2& btn)
{
  // main switch-case
  switch(menu_selection) {
    case BRIGHTNESS:
      tft.drawCentreString("Brightness",64,130,4);
      brightness += (btn==btnL) ? -counter : counter;
      counter>>=1;
      set_brightness(brightness);
      tft.drawCentreString(String(brightness),64,32,4);
    break;
    case TAMAGOTCHI:
      tft.drawCentreString("Tamagotchi",64,0,2);
    break;
    default:
      // Select menu
      menu.concat((btn==btnL) ? "L" : "R");
      menu_selection += (btn==btnL) ? -counter : counter;
      counter>>=1;
      menu_init(); 
  }
}

void button_handler(Button2& btn)
{
  tft.fillScreen(TFT_BLACK);
  if (btn.wasPressedFor() > 600) {
    if (btn == btnL)
      home_screen();
    else
      confirm(btn);
    return;
  }
  menu_loop(btn);
}

void home_screen()
{
  tft.drawCentreString("Home",64,130,4);
  tft.drawCentreString(String(tama.i),64,180,4);
  clock_loop();
  menu = "";
  menu_selection = HOME;
  counter = 64;
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
