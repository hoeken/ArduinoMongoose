#ifndef MongooseCore_h
#define MongooseCore_h

#include <http_status.h>
#include <string> // the C++ Standard String Class

#ifdef ARDUINO
  #include "Arduino.h"
  #include <IPAddress.h>
  #ifdef ESP32
    #include <WiFi.h>
    #ifdef ENABLE_WIRED_ETHERNET
      #include <ETH.h>
    #endif
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
  #endif
#endif // ARDUINO

#if defined(ARDUINO) && !defined(CS_PLATFORM)
  #ifdef ESP32
    //#define ESP_PLATFORM
  #elif defined(ESP8266)
    //???
  #else
    #error Platform not supported
  #endif
#endif

#define ENABLE_DEBUG true

#include <MicroDebug.h>
#include <ArduinoTrace.h>
#include "mongoose.h"
#include <functional>
#include "MongooseString.h"
#include "MongooseHTTP.h"
#include "MongooseHTTPServer.h"

#ifndef ARDUINO_MONGOOSE_DEFAULT_ROOT_CA
  #define ARDUINO_MONGOOSE_DEFAULT_ROOT_CA ""
#endif

typedef std::function<const char *(void)> ArduinoMongooseGetRootCaCallback;

class MongooseCore
{
  private:
    #if MG_ENABLE_SSL
        const char *_rootCa;
        ArduinoMongooseGetRootCaCallback _rootCaCallback;
    #endif
    #ifdef ARDUINO
        String _nameserver;
    #endif // ARDUINO
    struct mg_mgr mgr;

  public:
    MongooseCore();
    void begin();
    void end();
    void poll(int timeout_ms);

    struct mg_mgr *getMgr();
    void getDefaultOpts(struct mg_connect_opts *opts, bool secure = false);

    void ipConfigChanged();

    #if MG_ENABLE_SSL
      void setRootCa(const char *rootCa) {
        _rootCa = rootCa;
      }

      void setRootCaCallback(ArduinoMongooseGetRootCaCallback callback) {
        _rootCaCallback = callback;
      }
    #endif
};

extern MongooseCore Mongoose;

#endif // MongooseCore_h
