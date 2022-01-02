// All includes
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "Button2.h"
#include <sys/time.h>

// All defines
//#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_R            35
#define BUTTON_L            0
#define LONG_PRESS          600
#define TFT_AMBER           0xfca0

#define HOME                128
#define BRIGHTNESS          48
#define TAMAGOTCHI          192
#define TIME_OUT            20000  // Deep Sleep after 10 seconds

#define CHARGING_VOLTS      2760
#define MAX_VOLTS           2323
#define MIN_VOLTS           1811
#define VOLT_RANGE          512

// All Globals
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

Button2 btnR(BUTTON_R);
Button2 btnL(BUTTON_L);
String menu = "";
unsigned char menu_selection;
int counter;
uint32_t millis_until_sleep;

// Fishing game globals
bool fishing_paused = false;

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

  button_init();
  millis_until_sleep = millis()+TIME_OUT; // Sleep after 10 seconds
}

void loop() {
  if (millis() > millis_until_sleep)
    deep_sleep();
  if ((menu_selection == HOME) && (millis()%1000 == 0)) {
    clock_loop();
    // Analog value that relates to battery voltage
    String batt = "  ";
    batt.concat(String((analogRead(ADC_PIN)-MIN_VOLTS)));
    batt.concat("/512  ");
    /*
    The ADC value is a 12-bit number, so the maximum value is 4095 (counting from 0).
    To convert the ADC integer value to a real voltage you’ll need to divide it by the maximum value of 4095,
    then double it (note above that Adafruit halves the voltage), then multiply that by the reference voltage of the ESP32 which 
    is 3.3V and then vinally, multiply that again by the ADC Reference Voltage of 1100mV.
    */
    String volts = "  ";
    volts.concat(String((float)(analogRead(ADC_PIN)) / 4095*2*3.3*1.1));
    volts.concat(" V  ");
    tft.drawCentreString(volts,64,16,2);
    tft.drawCentreString(batt,64,32,2);  // This likely will only display 128 F but I'll leave it to test later
  }
  // Run button_handler if pressed
  btnR.loop();
  btnL.loop();
}

// Fishing game
void fishing()
{
  btnR.setReleasedHandler(fishing_click);
  btnL.setReleasedHandler(fishing_pause);
}

void fishing_click(Button2& btn)
{
  tft.drawCentreString("    R    ",64,0,4);
  if (fishing_paused)
    tft.drawCentreString("    Paused    ",64,130,4);
  else
    tft.drawCentreString("    Not paused    ",64,130,4);
}

void fishing_pause(Button2& btn)
{
  tft.drawCentreString("    L    ",64,0,4);
  if (btn.wasPressedFor() > LONG_PRESS)
    home_screen();
  else
    fishing_paused = !fishing_paused;
}

// Clock code
void clock_loop()
{
  gettimeofday(&age, NULL);
  printHMS(age.tv_sec, 0); 
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
      // tft.drawCentreString("Tamagotchi",64,0,2);
      tama.i =0;
      fishing();
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

void button_init()
{
  btnR.setReleasedHandler(button_handler);
  btnL.setReleasedHandler(button_handler);
}

void button_handler(Button2& btn)
{
  millis_until_sleep = millis()+TIME_OUT; // Sleep after 10 seconds
  tft.fillScreen(TFT_BLACK);
  if (btn.wasPressedFor() > LONG_PRESS) {
    if (btn == btnL)
      home_screen();
    else
      deep_sleep();
    return;
  }
  menu_loop(btn);
}

void home_screen()
{
  tft.drawCentreString("    Home    ",64,130,4);
  tft.drawCentreString(String(tama.i),64,180,4);
  clock_loop();
  menu = "";
  menu_selection = HOME;
  counter = 64;
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
  tft.drawCentreString("Deep Sleep",64,130,4);
  delay(1000);
  
  tft.fillScreen(TFT_BLACK); // Clear out screen so screen is blank when coming back from sleep
  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200); // This delay is important
  esp_deep_sleep_start();
}
