#include "commands.h"
#include "LiquidCrystal_I2C.h"
#include "WiFi.h"
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "myButton.h"
#include "heosControl.h"

hw_timer_t* My_variable_timer = NULL;

int lcdColumns = 20;
int lcdRows = 4;

int PowerPin = 32;
int VolumePot = 33;
int RightPin = 26;
int LeftPin = 25;
int UpPin = 0;
int DownPin = 2;
int RelayPin = 4;

volatile int favNo = 0;
volatile int AnzahlAnFav = 12;
bool HeosConnected = false;
volatile bool DenonPower = false;
int VolVal = 1;
int VolVal_old = 0;
volatile int StationCount = 0;
char ChannelName[20][50];
volatile int menu = 0;
volatile bool MenuEnd = false;
volatile bool MainZone = false;
volatile bool allFavs = false;

heosControl heosControler;

IPAddress DENON_IP(192, 168, 2, 173);
const char* FriendlyName = "Living-Room";

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

myButton PowerButton;
myButton UpButton;
myButton DownButton;
myButton LeftButton;
myButton RightButton;


void ARDUINO_ISR_ATTR onVariableTimer() {
  menu = 0;
  MenuEnd = true;
}

void denonResponded(const char* response, size_t len) {
  const char* zonezweian = "Z2ON";
  const char* zonezweiaus = "Z2OFF";
  const char* Mainan = "ZMON";
  const char* Mainaus = "ZMOFF";
  if (strncmp(zonezweian, response, 4) == 0) {
    DenonPower = true;
    digitalWrite(RelayPin, HIGH);
    delay(10);
    lcd.backlight();
    lcd.display();
  }
  if (strncmp(zonezweiaus, response, 4) == 0) {
    DenonPower = false;
    lcd.noBacklight();
    lcd.clear();
    lcd.noDisplay();
    delay(10);
    digitalWrite(RelayPin, LOW);
    // esp_deep_sleep_start();
  }
  if (strncmp(Mainan, response, 4) == 0 && DenonPower) {
    MainZone = true;
    lcd.myPrint("All Zones On", 1);
  }
  if (strncmp(Mainaus, response, 4) == 0 && DenonPower) {
    MainZone = false;
    lcd.setCursor(0, 1);
    lcd.myPrint("Zone 2 only", 1);
  }
  const char* BASS = "Z2PSBAS";
  if (strncmp(BASS, response, 6) == 0) {
    char buf8[10];
    strcpy(buf8, "Bass = ");
    buf8[7] = response[8];
    buf8[8] = response[9];
    buf8[9] = '\0';
    lcd.myPrint(buf8, 1);
  }
  const char* TREBEL = "Z2PSTRE";
  if (strncmp(TREBEL, response, 6) == 0) {
    char buf8[12];
    strcpy(buf8, "Treble = ");
    buf8[9] = response[8];
    buf8[10] = response[9];
    buf8[11] = '\0';
    lcd.myPrint(buf8, 1);
  }
  const char* Z2STBY = "Z2STBY";
  if (strncmp(Z2STBY, response, 5) == 0) {
    char buf8[14];
    strcpy(buf8, "Standby = ");
    buf8[10] = response[6];
    buf8[11] = response[7];
    buf8[12] = response[8];
    buf8[13] = '\0';
    lcd.myPrint(buf8, 1);
  }
}

void HeosDisconnCallback(void* arg, AsyncClient* client) {
  ESP.restart();
}

void newStationCb(const char* data, size_t len) {
  // lcd.clear();
  if (menu == 0) {
    lcd.myPrint("Now Playing: ", 0);
    lcd.myPrint(data, 1);
  }
}

void newArtistCb(const char* data, size_t len) {
  char Artist[41] = { "" };
  strcpy(Artist, "Artist: ");
  strncat(Artist, data, 40 - strlen("Artist: "));
  lcd.myPrint(Artist, 2);
}

void newSongCb(const char* data, size_t len) {
  char Song[61] = { "" };
  strcpy(Song, "Song: ");
  strncat(Song, data, 60 - strlen("Song: "));
  lcd.myPrint(Song, 3);
}

void PowerButtonPressed(void* arg, myButton* _Button) {
  if (!DenonPower && !MainZone) {
    heosControler.Denon.set(ZONE_2, ON);
    lcd.backlight();
    lcd.clear();
    lcd.display();
    lcd.myPrint("Zone 2, ON", 0);
    DenonPower = true;
  } else if (!DenonPower && MainZone) {
    heosControler.Denon.set(MAIN_ZONE, OFF);
    lcd.noBacklight();
    lcd.clear();
    // lcd.setCursor(0, 0);
    lcd.myPrint("Main Zone, OFF", 0);
    DenonPower = false;
    delay(500);
    heosControler.Denon.write("Z2?");
    heosControler.Denon.write("ZM?");
  } else if (DenonPower && !MainZone) {
    heosControler.Denon.set(ZONE_2, OFF);
    lcd.noBacklight();
    lcd.clear();
    // lcd.setCursor(0, 0);
    lcd.myPrint("Zone 2, OFF", 0);
    DenonPower = false;
    delay(500);
    heosControler.Denon.write("Z2?");
    heosControler.Denon.write("ZM?");
  } else if (DenonPower && MainZone) {
    heosControler.Denon.set(ZONE_2, OFF);
    delay(1000);
    heosControler.Denon.set(MAIN_ZONE, OFF);
    lcd.noBacklight();
    lcd.clear();
    // lcd.setCursor(0, 0);
    lcd.myPrint("All, OFF", 0);
    DenonPower = false;
    delay(500);
    heosControler.Denon.write("Z2?");
    heosControler.Denon.write("ZM?");
  }
}

void UpButtonPressed(void* arg, myButton* Button) {
  menu += 1;
  timerRestart(My_variable_timer);
  timerAlarmEnable(My_variable_timer);
  MenuFun();
}

void DownButtonPressed(void* arg, myButton* Button) {
  menu -= 1;
  timerRestart(My_variable_timer);
  timerAlarmEnable(My_variable_timer);
  MenuFun();
}

void LeftButtonPressed(void* arg, myButton* Button) {
  if (menu == 0) {
    if (favNo > 0) favNo -= 1;
    else if (favNo == 0) favNo = AnzahlAnFav;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Play Favourite No%d", favNo);
    char buf[70];
    sprintf(buf, "heos://browse/play_preset?pid=%d&preset=%d", heosControler.pid, favNo);
    heosControler.write(buf);
  } else if (menu == 1) {
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
    heosControler.Denon.set(MAIN_ZONE, OFF);
    heosControler.Denon.set(ZONE_2, ON);
  } else if (menu == 2) {
    heosControler.Denon.write("Z2STBYOFF");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  } else if (menu == 3) {
    heosControler.Denon.write("Z2PSBAS DOWN");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  } else if (menu == 4) {
    heosControler.Denon.write("Z2PSTRE DOWN");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  }
}

void RightButtonPressed(void* arg, myButton* Button) {
  if (menu == 0) {
    if (favNo < AnzahlAnFav) favNo += 1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Play Favourite No%d", favNo);
    char buf[70];
    sprintf(buf, "heos://browse/play_preset?pid=%d&preset=%d", heosControler.pid, favNo);
    heosControler.write(buf);
  } else if (menu == 1) {
    heosControler.Denon.set(MAIN_ZONE, ON);
    delay(1000);
    heosControler.Denon.write("MSMCH STEREO");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  } else if (menu == 2) {
    heosControler.Denon.write("Z2STBY4H");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  } else if (menu == 3) {
    heosControler.Denon.write("Z2PSBAS UP");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  } else if (menu == 4) {
    heosControler.Denon.write("Z2PSTRE UP");
    timerRestart(My_variable_timer);
    timerAlarmEnable(My_variable_timer);
  }
}


void setup() {
  PowerButton.begin(PowerPin, INPUT, 250000);
  PowerButton.onButtonPress(PowerButtonPressed);
  UpButton.begin(UpPin, INPUT_PULLUP, 250000);
  UpButton.onButtonPress(UpButtonPressed);
  DownButton.begin(DownPin, INPUT_PULLUP, 250000);
  DownButton.onButtonPress(DownButtonPressed);
  LeftButton.begin(LeftPin, INPUT_PULLUP, 250000);
  LeftButton.onButtonPress(LeftButtonPressed);
  RightButton.begin(RightPin, INPUT_PULLUP, 250000);
  RightButton.onButtonPress(RightButtonPressed);

  My_variable_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_variable_timer, &onVariableTimer, true);
  timerAlarmWrite(My_variable_timer, 7000000, false);  // 7s

  Serial.begin(115200);

  lcd.init();

  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "pwd");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
  WiFi.setAutoReconnect(true);

  lcd.setCursor(0, 0);
  lcd.print("WiFi, OK!");
  lcd.noBacklight();
  lcd.noBlink();
  lcd.noCursor();

  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);

  pinMode(VolumePot, INPUT);
  analogReadResolution(12);
  analogRead(VolumePot);

  delay(2000);

  heosControler.onNewStation(newStationCb);
  heosControler.onNewArtist(newArtistCb);
  heosControler.onNewSong(newSongCb);
  heosControler.onDenonResponed(denonResponded);
  heosControler.begin(DENON_IP);
  heosControler.updateMedia();
}

void loop() {
  heosControler.run();
  PowerButton.run();
  UpButton.run();
  DownButton.run();
  LeftButton.run();
  RightButton.run();
  lcd.run();

  VolVal = map(analogRead(VolumePot), 0, 4095, 70, 28);
  if (abs(VolVal_old - VolVal) > 3) {
    char buf[40];
    if (heosControler.DenonConnected) {
      sprintf(buf, "Z2%d", VolVal);
      heosControler.Denon.write(buf, strlen(buf));
      VolVal_old = VolVal;
      if (MainZone) heosControler.Denon.Volume = (VolVal + 3);
    }
  }
  if (MenuEnd) {
    timerAlarmDisable(My_variable_timer);
    heosControler.updateMedia();
    MenuEnd = false;
  }
}


void MenuFun() {
  if (heosControler.DenonConnected) {
    if (menu == 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Zone/Speakers");
      lcd.setCursor(0, 2);
      lcd.print("<-  Zone2");
      lcd.setCursor(0, 3);
      lcd.print("->  MultiChannel");
    } else if (menu == 2) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Standby");
      lcd.setCursor(0, 2);
      lcd.print("<-  Off");
      lcd.setCursor(0, 3);
      lcd.print("->  4h");
      heosControler.Denon.write("Z2STBY?");
    } else if (menu == 3) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bass Setup");
      lcd.setCursor(0, 2);
      lcd.print("<-  Less Bass");
      lcd.setCursor(0, 3);
      lcd.print("->  More Bass");
    } else if (menu == 4) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Treble Setup");
      lcd.setCursor(0, 2);
      lcd.print("<-  Less Treble");
      lcd.setCursor(0, 3);
      lcd.print("->  More Treble");
    }
  }
}
