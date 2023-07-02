# ESP32-Heos-Control
Simple to use Heos Control for ESP32 / Arduino IDE
It is developed for controlling the heos inside an Denon AVR.  
It uses HardwareTimer 2!  


depencies:
```
<WiFi.h>  
<AsyncTCP.h>  
<ESPmDNS.h>  
<functional>  
<ArduinoJson.h>  
"DenonAVR.h"    // this is of my own making, have a look at:
https://github.com/janphoffmann/Denon-AVR-control-ESP32  
```
  
  
As usual I implemented this with a lot of callbacks  
Basic setup is fairly easy, a basic setup looks like this;  
This example will print you changes in the Serial Console  
  
  
```
//include the library  
#include <WiFi.h>  
#include "heosControl.h"  
  
//delace an instance  
heosControl HEOS;  
  
  
//if you know the IP address of your heos device  
IPAddress IP(192.168.1.2);  
//otherwise you can try with the known FrienldyName  
const char*FriendlyName = "Denon";  
  
  
//declare callbacks  
//don't take too much time in these callsbacks!  
  
//for getting info about a Radiostation or Spotify or Tidal  
void newStationCb(const char *data, size_t len){  
  Serial.print("new Station received:");  
  Serial.println(data);  
}  
  
//for info about the artist  
void newArtistCb(const char *data,size_t len){  
  Serial.print("Playing Artist: ");  
  Serial.println(data);  
}  
  
//for info about the song  
void newSongCb(const char *data,size_t len){  
  Serial.print("Playing Song: ");  
  Serial.println(data);  
}  
  
// for everything that heos is answering:  
// inside this you have all the time in the world as it is updated in the run() function  
void HeosResponseCb(const char* data,size_t len){  
  Serial.print("Heos texted: ");  
  Serial.println(data);  
}  
  
  
// in setup() function:  
void setup(){  
  
  //initialize your WiFi the way you usually do, mine looks like this:  
  WiFi.disconnect(true, true);  
  delay(200);  
  WiFi.mode(WIFI_STA);  
  WiFi.begin("mySSID", "myPassowrd");  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) return;  
  WiFi.setAutoReconnect(true);  
  
  //attatch the callbacks  
  HEOS.onNewStation(newStationCb);  
  HEOS.onNewArtist(newArtistCb);  
  HEOS.onNewSong(newSongCb);  
  HEOS.onHeosResponse(HeosResponseCb);  
  
  //begin function with known IP  
  HEOS.begin(IP);  
  //or with friendlyName  
  //HEOS.begin(FrienldyName);  
  
  //call this function to receive Info for first time  
  heosControler.updateMedia();  
}  
  
//in loop function:  
void loop(){  
//call this periodically, as the callbacks are not running in a ISR  
HEOS.run();  
}
```
  
If you want e.g. want to play sth from your favourite list, call  
```
int favNo = 1;  
char buf[70];  
sprintf(buf, "heos://browse/play_preset?pid=%d&preset=%d", HEOS.pid, favNo);  
HEOS.write(buf);  
```
  
if you want to turn your Main Zone on:  
```
HEOS.Denon.set(MAIN_ZONE, ON);  
```  
if you want to change the Volume to say 40:  
```
HEOS.Denon.Volume = 40;  
```  
  
for a more advanced example with a 4x20 lcd_i2c and 5 Buttons, have a look at the example folder  
The LiquidCrystal_I2C library is modified for automated sroll text with HardwareTimer!  
