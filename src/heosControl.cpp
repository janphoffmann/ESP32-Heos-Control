#include "heosControl.h"

hw_timer_t *heosControl::My_timer = NULL;
AsyncClient *heosControl::AVClient = NULL;
DENON_AVR heosControl::Denon;
bool heosControl::timerTriggered = false;

bool heosControl::begin(const char *FriendlyName, int port) {
  String toMatch = String(FriendlyName);
  if (!MDNS.begin("ESP32")) {
    return false;
  }
  int n = MDNS.queryService("http", "tcp");
  if (n == 0) {
    MDNS.end();
    return false;
  } else {
    for (int i = 0; i < n; ++i) {
      if (MDNS.hostname(i).indexOf(toMatch) >= 0) avr_ip = MDNS.IP(i);
    }
  }
  MDNS.end();
  begin(avr_ip);
}

void heosControl::DataHandler() {
   if (_heos_response_cb != NULL) _heos_response_cb(newdata, strlen(newdata));
  static int sid = 0;
  StaticJsonDocument<3800> doc;
  DeserializationError err = deserializeJson(doc, newdata);
  if (err.code() == DeserializationError::Ok) {
    JsonObject heos = doc["heos"];
    if (pid < 0) {
      JsonObject payload_0 = doc["payload"][0];
      int newPid = payload_0["pid"];
      if (newPid > 0) {
        pid = newPid;
        write("heos://system/register_for_change_events?enable=on");
      }
    }
    const char *heos_command = doc["heos"]["command"];
    if (heos_command != NULL) {
      const char *command = "event/player_now_playing_changed";
      if (strncmp(heos_command, command, 31) == 0) newmedia = true;
      const char *othercommand = "event/player_now_playing_progress";
      if (strncmp(heos_command, othercommand, 31) == 0 && sid == 10) {
        const char *heos_message = doc["heos"]["message"];
        char noTime[40];
        sprintf(noTime, "pid=%d&cur_pos=0&", pid);
        if (strncmp(heos_message, noTime, strlen(noTime)) != 0) {
          const char *dur = "cur_pos=";
          int f = -1;
          char *pch;
          char t[1001];
          strncpy(t, heos_message, 1000);
          pch = strstr(t, dur);
          f = pch - t + strlen(dur);
          if (f > 0) {
            strncpy(t, heos_message + f, strlen(heos_message) - f + 1);
            f = atoi(t);
            int durationInS = f / 1000;
            int durationMinutes = durationInS / 60;
            int durationS = 60;
            char totalDur[20];
            sprintf(totalDur, "Tidal    %02d:%02d/", durationMinutes, durationInS % durationS);
            const char *cur = "duration=";
            strncpy(t, heos_message, 1000);
            pch = strstr(t, cur);
            f = pch - t + strlen(cur);
            strncpy(t, heos_message + f, strlen(heos_message) - f + 1);
            f = atoi(t);
            durationInS = f / 1000;
            durationMinutes = durationInS / 60;
            durationS = 60;
            char buf[6];
            sprintf(buf, "%02d:%02d", durationMinutes, durationInS % durationS);
            strcat(totalDur, buf);
            if (_station_response_cb != NULL) _station_response_cb(totalDur, strlen(totalDur));
          }
        }
      }
    }


    const char *heos_message = heos["message"];
    char buf3[30];
    sprintf(buf3, "pid=%d&preset=", pid);
    char buf4[60];
    sprintf(buf4, "command under process&pid=%d&preset=", pid);
    if (strncmp(heos_message, buf4, 36) == 0) {
      timerRestart(My_timer);
      timerAlarmEnable(My_timer);
    } else if (strncmp(heos_message, buf3, 22) == 0) {
      newmedia = true;
    }
    JsonObject payload = doc["payload"];
    int payload_sid = payload["sid"];  // 3
    if (payload_sid == 3) {
      sid = 3;
      const char *payload_station = payload["station"];  // "Bayern 2 S��d"
      if (payload_station != NULL && strlen(payload_station) > 2) {
        if (_station_response_cb != NULL) _station_response_cb(payload_station, strlen(payload_station));
      }
    } else if (payload_sid == 10) {
      sid = 10;
      if (_station_response_cb != NULL) _station_response_cb("Tidal", strlen("Tidal"));
    }
    const char *payload_artist = payload["artist"];  // "Bayern 2 S��d"
    if (payload_artist != NULL && strlen(payload_artist) > 2) {
      if (_artist_response_cb != NULL) _artist_response_cb(payload_artist, strlen(payload_artist));
    } else if (payload_artist != NULL) {
      _artist_response_cb("---", strlen("---"));
    }

    const char *payload_song = payload["song"];  // "Bayern 2 S��d"
    if (payload_song != NULL && strlen(payload_song) > 2) {
      if (_song_response_cb != NULL) _song_response_cb(payload_song, strlen(payload_song));
    } else if (payload_song != NULL) {
      _song_response_cb("---", strlen("---"));
    }
  }
  handleData = false;
  memset(newdata, '\0', sizeof(newdata));
}

void heosControl::run() {
  if (handleData) DataHandler();
  if (timerTriggered || newmedia) {
    if (timerTriggered) timerTriggered = false;
    if (newmedia) {
      if (AVClient->canSend() && AVClient->connected() && pid > 0) {
        char buf1[75];
        sprintf(buf1, "heos://player/get_now_playing_media?pid=%d", pid);
        // write(buf1);
        // char *buf = new char[strlen(buf1) + 2];
        char buf[77];
        strcpy(buf, buf1);
        strcat(buf, "\n");
        AVClient->write(buf, strlen(buf));
        // delete[] buf;
        newmedia = false;
      }
    }
  }
}

void heosControl::updateMedia() {
  newmedia = true;
}

bool heosControl::begin(IPAddress _ip) {
  handleData = false;
  avr_ip = _ip;
  DenonConnected = false;
  Denon.onConnect([=](void *, AsyncClient *) {
    DenonConnected = true;
    delay(100);
    Denon.write("Z2?");
    delay(100);
    Denon.write("ZM?");
  });
  Denon.onDisconnect([=](void *, AsyncClient *) {
    DenonConnected = false;
  });
  if (__denon_response_cb != NULL) {
    Denon.onDenonResponse([=](const char *data, size_t len) {
      __denon_response_cb(data, len);
    });
  }
  if (!Denon.begin(_ip)) {
    return false;
  }
  AVClient = new AsyncClient;
  My_timer = timerBegin(2, 80, true);
  timerAttachInterrupt(My_timer, &TimerIsr, true);
  timerAlarmWrite(My_timer, 1000000, false);  // .5s
  attachCb();
  if (AVClient->connect(avr_ip, 1255)) {
    return true;
  }
  return false;
}


void heosControl::attachCb() {
  AVClient->onConnect([=](void *, AsyncClient *) {
    write("heos://player/get_players");
  },
                      this);

  AVClient->onData([=](void *, AsyncClient *, void *data, size_t len) {
    // Serial.println((const char *)data);
    if (strlen((const char *)data) > 2 && strlen((const char *)data) < 3800) {
      const char *isJson = "{\"heos\": {\"";
      if (strncmp((const char *)data, isJson, 10) == 0) {
        memset(newdata, '\0', sizeof(newdata));
        memcpy(newdata, (const char *)data, len);
        handleData = true;
      }
    }
  },
                   this);  //this eingefügt!!!!!!!!!!!
  if (_disconnCallback != NULL) AVClient->onDisconnect(_disconnCallback, AVClient);
  if (_conErr_cb != NULL) AVClient->onError([=](void *, AsyncClient *, int8_t error) {
    _conErr_cb(AVClient->errorToString(error));
  });
}

int heosControl::pid = -1;
bool heosControl::newmedia = false;

void heosControl::TimerIsr() {
  timerTriggered = true;
}

bool heosControl::write(const char *buf, size_t i) {
  if (AVClient->canSend() && AVClient->connected()) {
    AVClient->write(buf, i);
    return true;
  }
  return false;
}
void heosControl::write(const char *toWrite) {
  char *buf = new char[strlen(toWrite) + 2];
  strcpy(buf, toWrite);
  strcat(buf, "\n");
  this->write(buf, strlen(buf));
  delete[] buf;
}


void heosControl::onConnect(ConnHandler callbackFunc) {
  _connCallback = callbackFunc;
}
void heosControl::onDisconnect(DisconnHandler callbackFunc) {
  _disconnCallback = callbackFunc;
}

void heosControl::onHeosResponse(ResponseHandler callbackFunc) {
  _heos_response_cb = callbackFunc;
}

void heosControl::onNewStation(ResponseHandler callbackFunc) {
  _station_response_cb = callbackFunc;
}
void heosControl::onNewArtist(ResponseHandler callbackFunc) {
  _artist_response_cb = callbackFunc;
}
void heosControl::onNewSong(ResponseHandler callbackFunc) {
  _song_response_cb = callbackFunc;
}
void heosControl::onError(ErrorHandler callbackFunc) {
  _conErr_cb = callbackFunc;
}
void heosControl::onDenonResponed(ResponseHandler callbackFunc) {
  __denon_response_cb = callbackFunc;
}
