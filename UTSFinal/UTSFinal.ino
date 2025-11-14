// ----------------- RC522 -----------------
// SDA/SS = GPIO 5
// SCK = GPIO 18
// MOSI = GPIO 23
// MISO = GPIO 19
// RST = GPIO 4
// 3.3V
// GND

// ----------------- OLED -----------------
// SDA = GPIO 21
// SCL = GPIO 22
// 3.3V
// GND

// ----------------- HW-479 -----------------
// Red = GPIO 25
// Green = GPIO 26
// Blue = GPIO 27
// GND

// ----------------- Passive Buzzer -----------------
// Positive = GPIO 14
// Negative = GND

// ----------------- Buttons -----------------
// SCAN = GPIO 12
// STATUS = GPIO 13


// ----------------- Libraries -----------------
#include <WiFi.h>               //Controling the Wi-Fi Connection
#include <HTTPClient.h>         //Sending and Receiving HTTP data (POST and PATCH)
#include <SPI.h>                //Communication between devices using SPI protocol (MFRC522)
#include <MFRC522.h>            //Adjusting how the RFID card is being read from RC522 Module
#include <Wire.h>               //I2C communication between devices with SDA and SCL (I2C OLED)
#include <Adafruit_GFX.h>       //Basic functions for displaying text and graphics
#include <Adafruit_SSD1306.h>   //Specialized driver for OLED type SSD1306
#include <ArduinoJson.h>        //Make and edit JSON data using POST and PATCH

// ----------------- Wi-Fi -----------------
const char* ssid = "NAJY ALFATH";       // Declaring a constant char for ssid (Will be use in WiFi.begin to initialize Wi-Fi connection inside void setup)
const char* password = "tanyabunda";    // Declaring a constant char for password (Will be use in WiFi.begin to initialize Wi-Fi connection inside void setup)

// ----------------- Server -----------------
const String serverURL = "https://lang-seats-researcher-manchester.trycloudflare.com"; //Declaring a constant char for server url (Will be use in sendPOST and sendPATCH)

// ----------------- Pin Definitions -----------------
#define SS_PIN   5    //or so called SDA is different from the I2C one, it is to control the slave whose available from the master (ESP32)
#define RST_PIN  4    //to delete all the register, fifo buffer, state machine to ensure it standby before another use
#define RGB_R_PIN 25
#define RGB_G_PIN 26
#define RGB_B_PIN 27
#define BUZZER_PIN 14
#define BTN_SCAN 12
#define BTN_STATUS 13
                        //Why we don't define the pins for OLED? because just using &Wire below means it already initialize GPIO 21 and 22 for I2C Communication (written at the top)
                        //21 is for data traffic (bi-directional, address and ACK 7-bit address) and 22 is for clock synchronization
                        //Why we don't define the pins for RC522 except for SS and RST? because the rest of the pins (SCK, MOSI, MISO) already have its default pins (written at the top)
                        //MOSI is for the data that go from ESP32 to the RC522, while MISO is for the data that go back from RC522 to ESP32, and the SCK is for clock synchronization
                        //The data is 8-bit which is the max sequence of a single hexadecimal, 0 means absorbed a bit by the card, 1 means deflected fully by the card (ASK)


// ----------------- OLED -----------------
#define SCREEN_WIDTH 128      //Defining the screen width (pixel column) which means the X are going from 0-127
#define SCREEN_HEIGHT 64      //Defining the screen height (pixel row) which means the Y are going from 0-63
#define SCREEN_I2C_ADDR 0x3C  //Default address of SSD1306 in the connection line of I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Creating display object from the class AdaFruit_SSD1306, this is used to call of the display.blablabla that are applied in this code
                                                                  //&wire refers to wire.h library, it use to send data to OLED through SDA GPIO21 and SCL GPIO22 (I2C Controller by ESP32)
                                                                  //From the library aspect, it automatically trigger the begin() to initialize GPIO21 and GPIO22
                                                                  //-1 means no external RST pin on the I2C OLED

// ----------------- RFID -----------------
MFRC522 rfid(SS_PIN, RST_PIN);  //Creating RFID object from the class MFRC522 to read the tag because the rest of the pins are SPI default, we don't need to declare inside the constructor

// ----------------- Timing -----------------
unsigned long lastButtonPress = 0;        //Store last button press after clicking SCAN/STATUS button
const unsigned long debounceDelay = 300;  //Delays for between button presses
unsigned long lastActivityTime = 0;       //Store last activities ex: button preses or scanning RFIDs

// ----------------- Mode -----------------
enum Mode { MODE_WELCOME, MODE_SCAN, MODE_STATUS }; //Initializing constant value which is MDOES, for a 3 different states
Mode currentMode = MODE_WELCOME;                    //Initialize the starting mode which is Welcome Mode

// ----------------- Status Control -----------------
String currentRFID = "";  //Store the current RFID from RC522
int statusIndex = 0;      //Status flow that users are currently in, starting from 0-5 (0 = Tersedia, and so on)
String statusList[5] = {"Tersedia", "Terjual", "Terpinjam", "Rusak", "Lainnya"};  //Array list of all the status needed for the glasses

// ----------------- LED -----------------
void setLED(bool r, bool g, bool b) {       //Boolean data type for true or false
  digitalWrite(RGB_R_PIN, r ? HIGH : LOW);  //Send signal to digital pin of ESP32 with HIGH/LOW
  digitalWrite(RGB_G_PIN, g ? HIGH : LOW);  //Send signal to digital pin of ESP32 with HIGH/LOW
  digitalWrite(RGB_B_PIN, b ? HIGH : LOW);  //Send signal to digital pin of ESP32 with HIGH/LOW
}

// ----------------- Animation from Wokwi Style -----------------
#define FRAME_DELAY 42    //Delays between frames in millis
#define FRAME_WIDTH 48    //Frames width in colums
#define FRAME_HEIGHT 48   //Frames height in rows
#define FRAME_COUNT 27    //The total of the animations frame based on bitmap below

const byte PROGMEM frames[][288] = { //From Wokwi animations for loader or animated assets
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,7,224,0,0,0,0,12,48,0,0,0,0,28,48,0,0,0,0,24,24,0,0,0,0,24,24,0,0,0,62,12,48,56,0,0,99,15,112,108,0,0,99,3,224,198,0,0,65,0,0,130,0,0,99,0,0,198,0,0,115,0,0,124,0,0,30,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,0,1,192,0,64,0,0,2,0,2,64,0,0,2,0,3,128,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,40,0,0,12,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,0,0,0,0,7,224,0,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,8,16,0,0,0,12,12,48,0,0,0,62,12,48,16,0,0,99,7,224,124,0,0,193,3,192,68,0,0,193,128,0,70,0,0,65,0,0,68,0,0,99,0,0,124,0,0,62,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,2,192,0,0,0,128,4,64,0,0,0,64,4,64,0,0,0,64,2,192,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,16,0,0,0,0,0,40,0,0,28,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,224,0,0,0,0,6,96,0,0,0,0,12,48,0,0,0,0,8,48,0,0,0,28,12,48,0,0,0,127,12,48,0,0,0,99,7,224,56,0,0,193,129,192,68,0,0,193,128,0,68,0,0,193,128,0,68,0,0,99,0,0,56,0,0,127,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,0,0,0,6,64,0,0,0,128,4,96,0,0,0,64,4,96,0,0,1,64,6,64,0,0,0,128,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,16,0,0,0,0,0,8,0,0,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,7,224,0,0,0,0,4,48,0,0,0,0,12,48,0,0,0,62,12,48,0,0,0,127,6,96,0,0,0,225,135,224,16,0,0,193,128,0,32,0,0,193,128,0,68,0,0,193,128,0,40,0,0,195,128,0,16,0,0,127,0,0,0,0,0,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,0,4,96,0,0,0,0,12,32,0,0,1,64,12,32,0,0,1,0,4,96,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,20,0,0,16,0,0,34,0,0,8,0,0,20,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,224,0,0,0,0,4,32,0,0,0,0,4,32,0,0,0,28,4,32,0,0,0,127,6,96,0,0,0,99,3,192,0,0,0,193,128,0,40,0,0,193,128,0,0,0,0,193,128,0,40,0,0,99,0,0,0,0,0,127,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,192,0,0,0,0,12,96,0,0,0,0,12,32,0,0,1,64,12,32,0,0,1,64,12,96,0,0,0,0,7,224,0,0,0,0,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,34,0,0,16,0,0,34,0,0,8,0,0,34,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,64,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,6,96,0,0,0,0,4,32,0,0,0,12,4,32,0,0,0,62,3,192,0,0,0,99,1,128,0,0,0,193,0,0,40,0,0,193,128,0,0,0,0,65,0,0,24,0,0,99,0,0,0,0,0,62,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,14,224,0,0,0,0,12,32,0,0,0,0,8,48,0,0,1,64,8,48,0,0,1,64,12,48,0,0,0,0,7,224,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,0,0,34,0,0,16,0,0,98,0,0,40,0,0,34,0,0,16,0,0,62,0,0,0,0,0,8,0,0,0,0,0,0,0,128,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,4,0,0,0,0,0,2,64,0,0,0,62,3,192,0,0,0,99,0,0,0,0,0,99,0,0,24,0,0,65,0,0,40,0,0,99,0,0,16,0,0,115,0,0,0,0,0,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,0,14,224,0,0,0,0,28,48,0,0,0,0,24,48,0,0,1,64,24,48,0,0,1,64,12,48,0,0,0,0,14,96,0,0,0,0,7,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,62,0,0,0,0,0,99,0,0,0,0,0,65,0,0,40,0,0,99,0,0,16,0,0,54,0,0,0,0,0,28,0,0,0,0,0,0,0,64,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,28,1,128,0,0,0,54,0,0,0,0,0,99,0,0,0,0,0,65,0,0,32,0,0,99,0,0,0,0,0,62,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,0,0,0,7,224,0,0,0,0,14,96,0,0,0,0,24,48,0,0,0,0,24,48,0,0,1,64,24,16,0,0,1,64,24,48,0,0,0,0,14,112,0,0,0,0,7,224,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,0,0,103,0,0,0,0,0,99,0,0,0,0,0,65,0,0,32,0,0,99,0,0,24,0,0,99,0,0,0,0,0,62,1,128,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,2,0,0,0,0,8,0,0,0,0,0,62,0,0,0,0,0,34,0,0,0,0,0,99,0,0,8,0,0,34,0,0,0,0,0,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,0,14,224,0,0,0,0,28,48,0,0,0,0,24,48,0,0,1,64,24,48,0,0,1,64,12,48,0,0,0,0,14,96,0,0,0,0,7,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,62,0,0,0,0,0,99,0,0,0,0,0,193,0,0,56,0,0,193,128,0,32,0,0,65,0,0,24,0,0,99,0,0,0,0,0,62,3,192,0,0,0,24,2,64,0,0,0,0,0,32,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,34,0,0,0,0,0,34,0,0,8,0,0,38,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,14,224,0,0,0,0,12,32,0,0,0,0,8,48,0,0,1,64,8,48,0,0,1,64,12,48,0,0,0,0,15,224,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,127,0,0,0,0,0,99,0,0,0,0,0,193,128,0,40,0,0,193,128,0,0,0,0,193,128,0,40,0,0,99,1,128,0,0,0,127,3,192,0,0,0,28,4,32,0,0,0,0,4,32,0,0,0,0,4,96,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,2,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,20,0,0,0,0,0,34,0,0,8,0,0,20,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,224,0,0,0,0,12,96,0,0,0,0,12,32,0,0,1,64,12,32,0,0,1,64,12,96,0,0,0,0,7,224,0,0,0,0,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,0,0,127,0,0,0,0,0,227,128,0,16,0,0,193,128,0,40,0,0,193,128,0,68,0,0,193,128,0,40,0,0,195,131,192,16,0,0,127,6,96,0,0,0,62,4,32,0,0,0,0,4,32,0,0,0,0,4,32,0,0,0,0,3,224,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,0,0,0,0,0,0,8,0,0,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,0,4,96,0,0,0,0,12,32,0,0,1,64,12,32,0,0,1,0,4,96,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,127,0,0,0,0,0,99,0,0,56,0,0,193,128,0,100,0,0,193,128,0,68,0,0,193,128,0,68,0,0,99,7,192,56,0,0,127,6,96,0,0,0,28,12,48,0,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,7,224,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,0,0,0,4,0,0,8,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,0,0,0,6,64,0,0,0,128,4,96,0,0,0,64,4,96,0,0,1,64,6,64,0,0,0,128,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,62,0,0,0,0,0,99,0,0,124,0,0,193,0,0,68,0,0,193,128,0,70,0,0,65,3,128,68,0,0,99,7,224,124,0,0,62,12,48,16,0,0,24,12,48,0,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,6,96,0,0,0,0,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,16,0,0,32,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,2,192,0,0,0,128,4,64,0,0,0,64,4,64,0,0,0,64,2,192,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,0,56,0,0,103,0,0,108,0,0,99,0,0,198,0,0,65,0,0,130,0,0,99,3,192,198,0,0,99,7,224,108,0,0,62,12,48,56,0,0,0,12,48,0,0,0,0,8,16,0,0,0,0,12,48,0,0,0,0,14,48,0,0,0,0,7,224,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,0,20,0,0,40,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,0,1,192,0,64,0,0,2,0,0,64,0,0,2,0,3,128,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,0,124,0,0,62,0,0,198,0,0,99,0,0,198,0,0,65,0,0,130,0,0,99,7,224,198,0,0,54,14,240,198,0,0,28,12,48,124,0,0,0,24,24,0,0,0,0,24,24,0,0,0,0,12,56,0,0,0,0,12,48,0,0,0,0,7,224,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,40,0,0,20,0,0,32,0,0,8,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,3,64,2,0,0,0,2,32,2,0,0,0,2,32,1,0,0,0,3,64,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,124,0,0,62,0,0,198,0,0,34,0,0,130,0,0,98,0,1,131,0,0,34,3,192,131,0,0,62,7,224,198,0,0,8,12,48,124,0,0,0,12,48,48,0,0,0,8,16,0,0,0,0,12,48,0,0,0,0,14,48,0,0,0,0,7,224,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,40,0,0,20,0,0,0,0,0,0,0,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,1,0,0,0,2,32,2,0,0,0,6,32,2,0,0,0,6,32,1,0,0,0,2,96,0,0,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,254,0,0,28,0,0,198,0,0,34,0,1,131,0,0,34,0,1,131,0,0,34,3,129,131,0,0,28,7,224,198,0,0,0,12,48,254,0,0,0,12,48,56,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,6,96,0,0,0,0,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,32,0,0,20,0,0,68,0,0,0,0,0,40,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,224,0,0,0,0,6,32,2,0,0,0,4,48,2,128,0,0,4,48,0,0,0,0,6,32,0,0,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,124,0,0,0,0,0,254,0,0,8,0,1,195,0,0,20,0,1,131,0,0,34,0,1,131,0,0,20,0,1,131,0,0,8,7,193,199,0,0,0,6,96,254,0,0,0,12,48,124,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,6,224,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,68,0,0,20,0,0,68,0,0,0,0,0,68,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,0,0,0,0,7,224,0,0,0,0,6,48,2,0,0,0,4,48,0,128,0,0,4,48,0,0,0,0,6,48,0,0,0,0,3,224,0,0,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,254,0,0,0,0,0,198,0,0,20,0,1,131,0,0,0,0,1,131,0,0,20,0,1,131,0,0,0,3,192,198,0,0,0,6,96,254,0,0,0,4,32,56,0,0,0,4,32,0,0,0,0,4,32,0,0,0,0,7,224,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,124,0,0,0,0,0,68,0,0,20,0,0,70,0,0,8,0,0,68,0,0,0,0,0,124,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,7,240,0,0,0,0,12,48,2,128,0,0,12,16,0,128,0,0,12,16,0,0,0,0,4,48,0,0,0,0,7,240,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,124,0,0,0,0,0,198,0,0,28,0,0,130,0,0,0,0,1,131,0,0,28,0,0,131,0,0,0,1,128,198,0,0,0,3,192,124,0,0,0,4,32,48,0,0,0,4,32,0,0,0,0,4,96,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,2,0,0,0,0,0,0,0,56,0,0,0,0,0,108,0,0,0,0,0,198,0,0,20,0,0,130,0,0,0,0,0,198,0,0,0,0,0,124,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,224,0,0,0,0,6,112,0,0,0,0,12,48,2,128,0,0,12,24,0,128,0,0,12,24,0,0,0,0,12,56,0,0,0,0,7,112,0,0,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,124,0,0,0,0,0,198,0,0,16,0,0,194,0,0,20,0,0,130,0,0,12,0,0,198,0,0,0,0,0,198,0,0,0,3,192,124,0,0,0,2,64,0,0,0,0,0,32,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,192,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,1,128,124,0,0,0,0,0,198,0,0,24,0,0,194,0,0,20,0,0,130,0,0,8,0,0,198,0,0,0,0,0,230,0,0,0,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,224,0,0,0,0,14,112,0,0,0,0,12,24,2,128,0,0,8,24,0,128,0,0,12,24,0,0,0,0,12,24,0,0,0,0,14,112,0,0,0,0,7,224,0,0,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,108,0,0,0,0,0,198,0,0,20,0,0,130,0,0,8,0,0,198,0,0,0,0,0,108,0,0,0,1,128,56,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,2,64,0,0,0,0,4,0,0,0,0,0,2,64,24,0,0,0,3,192,124,0,0,0,0,0,198,0,0,20,0,0,130,0,0,4,0,1,131,0,0,12,0,0,131,0,0,0,0,0,198,0,0,0,0,0,124,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,224,0,0,0,0,6,112,0,0,0,0,12,48,2,128,0,0,12,24,0,128,0,0,12,24,0,0,0,0,12,56,0,0,0,0,7,112,0,0,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,124,0,0,0,0,0,68,0,0,20,0,0,70,0,0,0,0,0,68,0,0,0,0,0,124,0,0,0,0,0,16,0,0,0,0,64,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,6,96,0,0,0,0,4,32,0,0,0,0,4,32,56,0,0,0,3,192,254,0,0,0,1,128,199,0,0,20,0,1,131,0,0,0,0,1,131,0,0,20,0,1,131,0,0,0,0,0,198,0,0,0,0,0,254,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,7,240,0,0,0,0,12,48,2,128,0,0,12,16,0,128,0,0,12,16,0,0,0,0,4,48,0,0,0,0,7,240,0,0,0,0,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,100,0,0,20,0,0,68,0,0,0,0,0,68,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,2,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,224,0,0,0,0,4,32,0,0,0,0,4,32,0,0,0,0,4,32,124,0,0,0,6,96,254,0,0,8,3,193,195,0,0,20,0,1,131,0,0,34,0,1,131,0,0,20,0,1,131,0,0,8,0,1,199,0,0,0,0,0,254,0,0,0,0,0,124,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,0,0,0,0,7,224,0,0,0,0,6,48,2,0,0,0,4,16,0,128,0,0,4,48,0,0,0,0,6,48,0,0,0,0,3,224,0,0,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,40,0,0,20,0,0,68,0,0,0,0,0,40,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,64,0,0,0,0,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,0,0,0,7,224,0,0,0,0,4,48,0,0,0,0,12,48,0,0,0,0,12,48,56,0,0,0,6,96,254,0,0,28,3,224,199,0,0,34,0,1,131,0,0,34,0,1,131,0,0,38,0,1,131,0,0,28,0,0,198,0,0,0,0,0,254,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,224,0,0,0,0,6,32,2,0,0,0,4,48,2,128,0,0,4,48,0,0,0,0,6,32,0,0,0,0,3,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,40,0,0,20,0,0,0,0,0,0,0,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,0,0,0,0,6,96,0,0,0,0,12,48,0,0,0,0,8,48,0,0,0,0,12,48,24,0,0,8,12,48,124,0,0,62,7,224,198,0,0,34,1,192,130,0,0,98,0,1,131,0,0,34,0,0,131,0,0,62,0,0,198,0,0,0,0,0,124,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,1,0,0,0,2,32,2,0,0,0,6,32,2,0,0,0,6,32,1,0,0,0,2,96,0,0,0,0,1,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,20,0,0,32,0,0,0,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,0,0,7,224,0,0,0,0,12,48,0,0,0,0,12,48,0,0,0,0,8,16,0,0,0,0,12,48,0,0,0,28,12,48,124,0,0,54,7,224,198,0,0,99,3,192,194,0,0,65,0,0,130,0,0,99,0,0,198,0,0,62,0,0,230,0,0,28,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,0,3,64,2,0,0,0,2,32,2,0,0,0,2,32,1,0,0,0,3,64,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,0,32,0,0,8,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(RGB_R_PIN, OUTPUT);         //Pin 25 for OUPUT
  pinMode(RGB_G_PIN, OUTPUT);         //Pin 26 for OUPUT
  pinMode(RGB_B_PIN, OUTPUT);         //Pin 27 for OUPUT
  pinMode(BUZZER_PIN, OUTPUT);        //Pin 14 for OUPUT
  pinMode(BTN_STATUS, INPUT_PULLUP);  //Pin 13 for INPUT (High by default and low by presses and connected to GND)
  pinMode(BTN_SCAN, INPUT_PULLUP);    //Pin 12 for INPUT (High by default and low by presses and connected to GND)
  setLED(0, 0, 0);                    //Initialize GND to always OFF

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR)) {   //Charge the screen up because it requires 8+ volts but we don't need to match that, if the initialization fails do below
    Serial.println("OLED gagal diinisialisasi");  
    while (true);
  }

  SPI.begin();     //Initialize SPI and configure its default pins for RC522 
  rfid.PCD_Init(); //Initialize RC chip, Proximity Coupling Device to handle communication with RFID tag
  Serial.println("RC522 siap"); //Output to serail monitor for debug

  // WiFi Initialization
  Serial.printf("Menghubungkan ke WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);      //Connecting ESP32 to Wi-Fi which use the constant ssid and password declared above
  unsigned long start = millis();  //Starting a initial start time counter for the time limit of the Wi-Fi connection below using unsigned long (suitable for millis)
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { 
    //WL_CONNECTED are defined inside WiFi.h library with its other friends such IDLE_STATUS, DISCONNECTED, etc
    //If Wi-Fi status not equal WL_CONNECTED, loop it until it meets the WL_CONNECTED within the maximum range of 15 seconds
    //WL_CONNECTED = means the WiFi.begin success to achieve a handshake with the router and get the IP
    //which then being display in the status function (also means the ESP32 is now ready to sendPOST and sendPATCH)
    delay(500);
    Serial.print(".");             //Print "." with delay of 500ms when trying to connect to Wi-Fi
  }

  display.setTextColor(SSD1306_WHITE);                    //Controlling the text color (in the use case of OLED 128x64 means the pixel are shown) 
                                                          //else black means the pixel won't be shown (since the display are monochrome)

  // WiFi status texts configuration
  display.clearDisplay();                                 //Clear current displays before start writing a new one (clears framebuffer (1024 byte), entirely 0x00)
  display.setTextSize(1);                                 //Controlling the text size (adjusting font scale in the framebuffer)
  display.setCursor(20, 20);                              //Controlling the text position (x, y) (temporary stored in the internal library int16_t cursor_x, cursor_y;)

  // WiFi status function
  if (WiFi.status() == WL_CONNECTED) {    //Checking the WiFi status, whether it's still on WL_CONNECTED state or nor
    display.println("WiFi terhubung");    //If the WiFi initialization throw a WL_CONNECTED, this will display WiFi terhubung on the OLED Display
    Serial.println("\nWiFi terhubung");   //Same here but this one is for the Serial Monitor
    Serial.print("IP: "); Serial.println(WiFi.localIP()); //This one is also for the Serial Monitor which shows the IP of the WiFi
  } else {
    display.println("WiFi gagal");        //If the WiFi initialization throw any other, this will display WiFi gagal on the OLED Display
    Serial.println("\nWiFi gagal");       //Same here but this one is for the Serial Monitor
  }
  display.display();    //Display all of the configuration above it. Because when we initialize the text configuration it only sends buffer text through the I2C
  delay(1000);          //Because we want to inform the user whether the Wi-Fi is connected or not, we just display it with the interval of 1 second

  showWelcomeScreen();
  lastActivityTime = millis();           //Store last time triggered or activities in millis (All of the data stored in framebuffer will then be send through I2C witu address 0x3C and 0x40)
}

// ----------------- UID to String -----------------
String uidToString(MFRC522::Uid &uid) {   //The top layer of the conversion logic from RFID tag to Hex
  String s = "";                          //Initialized empty variable
  for (byte i = 0; i < uid.size; i++) {   //To read all of the UID bytes
    if (uid.uidByte[i] < 0x10) s += "0";  //Adds zeros in front of every single hex digits (cause it's should be format in 2)
    s += String(uid.uidByte[i], HEX);     //Convert bytes to hex string and then add to variable S
  }
  s.toUpperCase(); //Converts all of the s variable member to uppercase
  return s; //Output the UID
}

// ----------------- Display Modes -----------------
void showWelcomeScreen() {
  currentMode = MODE_WELCOME;
  setLED(0, 0, 0);                            //LED OFF
  display.clearDisplay();                     //Clear current displays before start writing a new one
  display.setTextSize(1);                     //Controlling the text size
  display.setCursor(10, 20);                  //Controlling the text position (x, y)
  display.println("Welcome to LENS'Z");
	display.setCursor(13, 30);              
  display.println("by Optik Gembira");
  display.display();                          //Display all of the configuration above it
}

void showScanMode() {
  currentMode = MODE_SCAN;
  setLED(0, 0, 1);                            //LED Blue
  display.clearDisplay();                     //Clear current displays before start writing a new one
  display.setTextSize(1);                     //Controlling the text size
  display.setCursor(0, 10);                   //Controlling the text position (x, y)
  display.println("Mode: DAFTAR");
  display.setCursor(0, 40);
  display.println("Scan tag untuk daftar");
  display.display();                          //Display all of the configuration above it
}

void showStatusMode() {
  currentMode = MODE_STATUS;
  setLED(0, 0, 1);                            //LED Blue
  display.clearDisplay();                     //Clear current displays before start writing a new one
  display.setTextSize(1);                     //Controlling the text size
  display.setCursor(0, 10);                   //Controlling the text position (x, y)
  display.println("Mode: STATUS");
  display.setCursor(0, 25);
  display.println("Pilih status dulu");
  display.setCursor(0, 40);
  display.println("(Klik lagi)");
  display.display();                          //Display all of the configuration above it
}

void showStatus(String status) {
  setLED(0, 0, 1);                            //LED Blue
  display.clearDisplay();                     //Clear current displays before start writing a new one
  display.setTextSize(1);                     //Controlling the text size
  display.setCursor(0, 10);                   //Controlling the text position (x, y)
  display.print("Status: ");
  display.println(status);
  display.setCursor(0, 40);
  display.println("Scan tag utk update");
  display.display();                          //Display all of the configuration above it
}

// ----------------- Animation Load -----------------
void showLoadingAnimation(unsigned long duration = 2000) {  //Display animation within a 2 seconds duration
  int frame = 0;                            //Variable to count index grame starting from the frame 0
  unsigned long startTime = millis();       //Store starting the starting time, this is to stop the animation after the 2 seconds frame
  while (millis() - startTime < duration) { //Always running before 2 seconds frame
    display.clearDisplay();                 //Clear current displays before start writing a new one
    display.drawBitmap(40, 8, frames[frame], FRAME_WIDTH, FRAME_HEIGHT, 1); //Position of X = 40 and Y = 8, fetch width and height from the declaration, and 1 for pixel on in the framebuffer
    display.display();                      //Display all the buffer
    frame = (frame + 1) % FRAME_COUNT;      //Always change to the next frame to loop the animation
    delay(FRAME_DELAY);                     //Pause by a bit before the next frame
  }
}

// ----------------- HTTP -----------------
void sendPOST(String endpoint, String payload) {      //Initializing endpoint (/scanner/scan) and the JSON payload ({"rfid": "xxxxx"})
  showLoadingAnimation();                             //Show animation loading screen after the user scan the rfid tag
  if (WiFi.status() != WL_CONNECTED) return;          //Verfying whether the ESP32 are still connected to the WiFi
  HTTPClient http;                                    //Creating an object = http from HTTPClient class
  String url = serverURL + endpoint;                  //Creating a combination of main server address (serverURL) with the endpoint
  http.begin(url);                                    //Engage connection to the url "https://bapakdareenhargyo.com/scanner/scan"
  http.addHeader("Content-Type", "application/json"); //Inform the server that the data is JSON (application/json)
  int httpCode = http.POST(payload);                  //Send the JSON payload with POST in the body request
  String response = http.getString();                 //Receiving the response body, ex: {"message": "RFID received successfully"} (a method from HTTPClient)
  Serial.println("=== POST " + endpoint + " ===");    //Debug, showing request details from the server to serial monitor
  Serial.println("Payload: " + payload);              //======
  Serial.println("Kode: " + String(httpCode));        //======
  Serial.println("Resp: " + response);                //======
  http.end();                                         //Cutting the HTTP connection and freeing the memory (one time request every hit)
}

void sendPATCH(String endpoint, String payload) {     //Same as POST basically
  showLoadingAnimation();
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = serverURL + endpoint;                  //Creating a combination of main server address (serverURL) with the endpoint
  http.begin(url);                                    //Engage connection to the url "https://bapakdareenhargyo.com/scanner/status"
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.sendRequest("PATCH", payload);  //Send the JSON payload with PATCH to update any of the specific glasses status
  String response = http.getString();
  Serial.println("=== PATCH " + endpoint + " ===");
  Serial.println("Payload: " + payload);
  Serial.println("Kode: " + String(httpCode));
  Serial.println("Resp: " + response);
  http.end();                                         //Same as POST basically
}

// ----------------- Main Loop -----------------
void loop() {
  if (millis() - lastActivityTime > 10000 && currentMode != MODE_WELCOME) { //If the last activites are more than 10 seconds and the current mode isn't MODE_WELCOME
    showWelcomeScreen();  //Then display welcome screen
  }

  // Tombol mode
  if (digitalRead(BTN_SCAN) == LOW && millis() - lastButtonPress > debounceDelay) { //If the the scan button pressed and passed debounce time run below
    lastButtonPress = millis();   //Store the time when the button is pressed
    lastActivityTime = millis();  //Update the last activity time after click
    tone(BUZZER_PIN, 800, 100);   //Set the buzzer tone
    showScanMode();               //Call the function to show the DAFTAR mode
  }
  if (digitalRead(BTN_STATUS) == LOW && millis() - lastButtonPress > debounceDelay && currentMode != MODE_STATUS) { //If the status button pressed and passed debounce time run below
    lastButtonPress = millis();   //Store the time when the button is pressed
    lastActivityTime = millis();  //Update last activity time after click
    tone(BUZZER_PIN, 1000, 100);  //Set the buzzer tone
    showStatusMode();             //Call the function to show the STATUS mode
  }

  // MODE SCAN
  if (currentMode == MODE_SCAN) { //If the current mode are the scan mode then run below
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { //checking the true or false loop for rfid detection and to read a unique UID
      lastActivityTime = millis();        //Update last activity time after click
      tone(BUZZER_PIN, 1000, 150);        //Set the buzzer tone
      String uid = uidToString(rfid.uid);
      currentRFID = uid;

      StaticJsonDocument<128> doc;  //From the ArduinoJson library to set a fixed size to allocate the stack (buffer in the byte)
      doc["rfid"] = uid;            //UID from the string conversion
      String jsonData;
      serializeJson(doc, jsonData); //doc means the static json document with the RFID data inside

      sendPOST("/scanner/scan", jsonData);  //Send the jsonData to the endpoint /scanner/scan to server using HTTP POST via WiFi
      setLED(0, 1, 0);
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("ID Terkirim");
      display.display();
      delay(800);
      showScanMode();

      rfid.PICC_HaltA();      //Send halt to the card to tag that the process is done
      rfid.PCD_StopCrypto1(); //Turning off the encryption inside the module reader if it was active
    }
  }

  // MODE STATUS
  if (currentMode == MODE_STATUS) { //If the current mode are the status mode then run below
    if (digitalRead(BTN_STATUS) == LOW && millis() - lastButtonPress > debounceDelay) { //If the status pressed again and passing the 300ms time to to the debounce
      lastButtonPress = millis();
      lastActivityTime = millis();
      statusIndex = (statusIndex + 1) % 5;  //Display all of the status one by one from a 0 to 4th index
      tone(BUZZER_PIN, 1200, 100);
      showStatus(statusList[statusIndex]);  //Display the current status
    }

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { //checking the true or false loop for rfid detection and to read a unique UID
      lastActivityTime = millis();        
      tone(BUZZER_PIN, 1000, 150);        
      String uid = uidToString(rfid.uid); 

      StaticJsonDocument<256> doc;  //From the ArduinoJson library to set a fixed size to allocate the stack (buffer in the byte)
      doc["rfid"] = uid;            
      doc["status"] = statusList[statusIndex];  
      String jsonData;
      serializeJson(doc, jsonData);

      sendPATCH("/scanner/status", jsonData); //Send the jsonData to the endpoint /scanner/status to server using HTTP PATCH via WiFi
      setLED(0, 1, 0);
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Status Terkirim");
      display.display();

      delay(800);
      showStatus(statusList[statusIndex]);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}
