// All includes
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "Button2.h"
#include <sys/time.h>

// All defines
//#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define VOLTAGE             34
#define BUTTON_R            35
#define BUTTON_L            0
#define TFT_AMBER           0xfca0

#define HOME                128
#define BRIGHTNESS          48
#define TAMAGOTCHI          192

#define TIME_OUT            20000   // Deep Sleep after 10 seconds
#define LONG_PRESS          600
#define MS_PER_FRAME        17      // About 58.82 fps

#define CHARGING_VOLTS      2511    // 700/512
#define MAX_VOLTS           2323
#define MIN_VOLTS           1811
#define VOLT_RANGE          512

#define FISHING_MAX_SPD     5
#define FISHING_BOUNCE_SPD  3

// All Globals
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

TFT_eSprite fishing_square = TFT_eSprite(&tft);
TFT_eSprite meter = TFT_eSprite(&tft);
TFT_eSprite background = TFT_eSprite(&tft);

Button2 btnR(BUTTON_R);
Button2 btnL(BUTTON_L);
String menu = "";
unsigned char menu_selection;
int counter;
uint32_t time_out;

// Fishing game globals
bool is_fishing = false;
bool fishing_paused = false;
uint8_t posy;
int8_t spdy;
uint8_t meter_value;
int8_t meter_change;
uint8_t ticks;

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
  
  time_out = millis()+TIME_OUT; // Sleep after TIME_OUT milliseconds
}

void loop() {
  // This is a mess please make more readable
  if (millis() > time_out)
    if (is_fishing)
      fishing_draw();
    else if (analogRead(VOLTAGE) < CHARGING_VOLTS) // When connected to usb the pin reads a value greater than MAX_VOLTS
      deep_sleep();
  if ((analogRead(VOLTAGE) > CHARGING_VOLTS) && (millis()%1000 == 0) && !is_fishing)
    time_out = millis()+TIME_OUT; // Sleep after TIME_OUT milliseconds
  if ((menu_selection == HOME) && (millis()%1000 == 0)) {
    clock_loop();
    // Analog value that relates to battery voltage
    String batt = "  ";
    batt.concat(String((analogRead(VOLTAGE)-MIN_VOLTS)));
    batt.concat("/512  ");
    /*
    The ADC value is a 12-bit number, so the maximum value is 4095 (counting from 0).
    To convert the ADC integer value to a real voltage you’ll need to divide it by the maximum value of 4095,
    then double it (note above that Adafruit halves the voltage), then multiply that by the reference voltage of the ESP32 which 
    is 3.3V and then vinally, multiply that again by the ADC Reference Voltage of 1100mV.
    */
    String volts = "  ";
    volts.concat(String((float)(analogRead(VOLTAGE)) / 4095*2*3.3*1.1));
    volts.concat(" V  ");
    tft.drawCentreString(volts,64,16,2);
    tft.drawCentreString(batt,64,32,2);  // This likely will only display 128 F but I'll leave it to test later
    tft.drawCentreString(String(time_out),64,48,2);
    tft.drawCentreString(String(millis()),64,64,2);
  }
  // Run button_handler if pressed
  btnR.loop();
  btnL.loop();
}

// Fishing game
void fishing_init()
{
  //btnR.setReleasedHandler(NULL);
  btnR.setReleasedHandler(fishing_click);
  btnL.setReleasedHandler(fishing_pause);
  is_fishing = true;
  // Palette colour table
  uint16_t palette[16] = 
        { TFT_BLACK,  TFT_ORANGE, TFT_DARKGREEN,  TFT_DARKCYAN, TFT_MAROON, TFT_PURPLE, TFT_OLIVE,  TFT_DARKGREY,
          TFT_ORANGE, TFT_BLUE,   TFT_GREEN,      TFT_CYAN,     TFT_RED,    TFT_NAVY,   TFT_YELLOW, TFT_WHITE };
  fishing_square.setColorDepth(4);
  fishing_square.createSprite(14, 55);
  fishing_square.createPalette(palette);
  fishing_square.fillSprite(9);
  meter.setColorDepth(4);
  meter.createSprite(64, 2);
  meter.createPalette(palette);
  meter.fillSprite(12);
  //background.setColorDepth(4);
  //background.createSprite(14, 213);
  //background.createPalette(palette);
  //background.fillSprite(1);
  //background.pushSprite(20, 11);
  //tft.fillRect(20,11,14,213, TFT_ORANGE);
  
  posy = 64;
  spdy = 1;
  meter_value = 170;
  meter_change = 1;
  ticks = 0;
  time_out = millis() + 17;
}

void fishing_draw()
{
  ticks++;
  time_out = millis() + 17;
  if (fishing_paused)
    tft.drawCentreString("    Paused    ",64,130,4);
  else {
    /*uint8_t clear_pos = posy-spdy;
    if (spdy < 0)
      clear_pos += 55;*/
    /*if (clear_pos < 11)
        clear_pos = 11;
    else if (clear_pos > 218)
      clear_pos = 218;*/
    //tft.fillRect(20,clear_pos,14, 6, TFT_ORANGE);
    //tft.fillRect(20,0,14, 11, TFT_BLACK);
    //tft.fillRect(20,224,14, 16, TFT_BLACK);
    tft.drawFastHLine(64, meter_value-meter_change, 64, 0);
    meter.pushSprite(64, meter_value);
    fishing_square.pushSprite(20, posy);
    tft.drawRect(20,11,14,posy-11, TFT_ORANGE);
    tft.drawRect(20,posy+55,14, 213-posy-55+11, TFT_ORANGE);
    tft.fillRect(21,12,12,posy-11-2, TFT_BLACK);              // Clear top
    tft.fillRect(21,posy+55+1,12, 213-posy-55+11-2, TFT_BLACK); // Clear bottom
    
    posy += spdy;
    if ((btnR.isPressed()) && (ticks%4 == 0))
      spdy -= 1;
    // End bouncing
    if ((posy >= 169) && (posy < 224)) {  // Hit bottom
      spdy = -FISHING_BOUNCE_SPD;
      posy += -FISHING_BOUNCE_SPD;
    } else if ((posy <= 11) || (posy >= 224)) { // Hit top
      spdy = FISHING_BOUNCE_SPD;
      posy += FISHING_BOUNCE_SPD;
    }
    if ((spdy < FISHING_MAX_SPD) && (ticks%10 == 0))
      spdy++;
    else if (spdy > FISHING_MAX_SPD)
      spdy = FISHING_MAX_SPD;
    else if (spdy < -FISHING_MAX_SPD)
      spdy = -FISHING_MAX_SPD;
    meter_value += meter_change;
    if (meter_value%222==0)
      meter_change = -meter_change;
    tft.drawCentreString("  ",64,0,2);
    tft.drawCentreString(String(spdy),64,0,2);
  }
}

void fishing_click(Button2& btn)
{
  spdy -= 1;
}

void fishing_pause(Button2& btn)
{
  tft.drawCentreString("    L    ",64,224,2);
  if (btn.wasPressedFor() > LONG_PRESS) {  // Exit game to home screen
    is_fishing = false;
    time_out = millis()+TIME_OUT;
    tft.fillScreen(TFT_BLACK);
    //btnR.setPressedHandler(NULL);
    button_init();
    home_screen();
  } else
    fishing_paused = !fishing_paused;   // Toggle game pause
    tft.drawCentreString("                  ",64,130,4); // Clear if case it's unpausing
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
      fishing_init();
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
      tft.drawCentreString("Error: menu loop in Tamagotchi mode",64,0,2);
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
  time_out = millis()+TIME_OUT; // Sleep after TIME_OUT milliseconds
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
