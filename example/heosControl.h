#ifndef HEOSCONTROL_H
#define HEOSCONTROL_H

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <functional>
#include <ArduinoJson.h>
#include "DenonAVR.h"
#else
#error Platform not supported
#endif


typedef std::function<void(void*, AsyncClient*)> ConnHandler;
typedef std::function<void(void*, AsyncClient*)> DisconnHandler;
typedef std::function<void(const char* response, size_t len)> ResponseHandler;
typedef std::function<void(const char* errorMessage)> ErrorHandler;


class heosControl {
public:
  static DENON_AVR Denon;
  static AsyncClient* AVClient;
  bool begin(IPAddress _ip);
  bool begin(const char* FriendlyName = "Denon", int port = 1255);

  void onConnect(ConnHandler callbackFunc);
  void onDisconnect(DisconnHandler callbackFunc);
  void onHeosResponse(ResponseHandler callbackFunc);
  void onError(ErrorHandler callbackFunc);

  void onNewStation(ResponseHandler callbackFunc);
  void onNewArtist(ResponseHandler callbackFunc);
  void onNewSong(ResponseHandler callbackFunc);

  void onDenonResponed(ResponseHandler callbackFunc);

  ConnHandler _connCallback = NULL;
  DisconnHandler _disconnCallback = NULL;
  ResponseHandler _heos_response_cb = NULL;
  ResponseHandler _station_response_cb = NULL;
  ResponseHandler _artist_response_cb = NULL;
  ResponseHandler _song_response_cb = NULL;
  ResponseHandler __denon_response_cb = NULL;
  ErrorHandler _conErr_cb = NULL;

  bool write(const char* buf, size_t i);
  void write(const char* toWrite);
  static int pid;
  static bool newmedia;
  void run();
  bool DenonConnected;
  void updateMedia();
private:
  char newdata[8000];
  bool handleData;
  static bool timerTriggered;
  void attachCb();
  IPAddress avr_ip;
  static void beginTimer();
  static void ARDUINO_ISR_ATTR TimerIsr();
  static hw_timer_t* My_timer;
  
  void DataHandler();
};



#endif