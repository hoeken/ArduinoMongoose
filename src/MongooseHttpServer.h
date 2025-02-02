#ifndef MongooseHttpServer_h
#define MongooseHttpServer_h

#include "MongooseCore.h"
#include <http_status.h>
#include <list>

// Make a copy of the HTTP header so it is avalible outside of the onReceive
// callback. Setting to 0 will save some runtime memory but accessing the HTTP
// message details outside of the onReceive callback will give undefined behaviour.
// The body may not allways be avalible even in onReceive, eg file upload
#ifndef MG_COPY_HTTP_MESSAGE
  #define MG_COPY_HTTP_MESSAGE 1
#endif

class MongooseHttpServer;
class MongooseHttpServerRequest;
class MongooseHttpServerResponse;
#ifdef ARDUINO
  class MongooseHttpServerResponseStream;
#endif
class MongooseHttpWebSocketConnection;

class MongooseHttpServerRequest {
  friend MongooseHttpServer;

  protected:
    MongooseHttpServer *_server;
    mg_connection *_nc;
    mg_http_message *_msg;
    HttpRequestMethodComposite _method;
    MongooseHttpServerResponse *_response;
    bool _responseSent;

    #if MG_COPY_HTTP_MESSAGE
        mg_http_message *duplicateMessage(mg_http_message *);
    #endif

  public:
    MongooseHttpServerRequest(MongooseHttpServer *server, mg_connection *nc, mg_http_message *msg);
    virtual ~MongooseHttpServerRequest();

    virtual bool isUpload() { return false; }
    virtual bool isWebSocket() { return false; }

    HttpRequestMethodComposite method() {
      return _method;
    }

    MongooseString message() {
      return MongooseString(_msg->message);
    }

    MongooseString body() {
      return MongooseString(_msg->body);
    }

    MongooseString methodStr() {
      return MongooseString(_msg->method);
    }

    MongooseString uri() {
      return MongooseString(_msg->uri);
    }

    MongooseString queryString() {
      return MongooseString(_msg->query);
    }

    MongooseString proto() {
      return MongooseString(_msg->proto);
    }

    //TODO: verify this
    int respCode() {
      return mg_http_status(_msg);
    }

    //TODO: verify this
    MongooseString respStatusMsg() {
      return MongooseString(_msg->message);
    }

    //TODO: not sure this is needed
    // int headers() {
    //   int i;
    //   for (i = 0; i < MG_MAX_HTTP_HEADERS && _msg->headers[i].len > 0; i++) {
    //   }
    //   return i;
    // }

    MongooseString headers(const char *name) {
      MongooseString ret(mg_http_get_header(_msg, name));
      return ret;
    }

    //TODO: not sure this is possible
    // MongooseString headerNames(int i) {
    //   return MongooseString(_msg->header_names[i]);
    // }
    // MongooseString headerValues(int i) {
    //   return MongooseString(_msg->header_values[i]);
    // }

    MongooseString host() {
      return headers("Host");
    }

    MongooseString contentType() {
      return headers("Content-Type");
    }

    size_t contentLength() {
      return _msg->body.len;
    }

    void redirect(const char *url);
    #ifdef ARDUINO
        void redirect(const String& url);
    #endif

    MongooseHttpServerResponse *beginResponse();

    #ifdef ARDUINO
        MongooseHttpServerResponseStream *beginResponseStream();
    #endif

    // Takes ownership of `response`, will delete when finished. Do not use `response` after calling
    void send(MongooseHttpServerResponse *response);
    bool responseSent() {
      return NULL != _response;
    }

    void send(int code);
    void send(int code, const char *contentType, const char *content="");
    #ifdef ARDUINO
        void send(int code, const String& contentType, const String& content=String());
    #endif

    bool hasParam(const char *name) const;
    #ifdef ARDUINO
        bool hasParam(const String& name) const;
        bool hasParam(const __FlashStringHelper * data) const;
    #endif

    int getParam(const char *name, char *dst, size_t dst_len) const;
    #ifdef ARDUINO
        int getParam(const String& name, char *dst, size_t dst_len) const;
        int getParam(const __FlashStringHelper * data, char *dst, size_t dst_len) const;
    #endif

    #ifdef ARDUINO
        String getParam(const char *name) const;
        String getParam(const String& name) const;
        String getParam(const __FlashStringHelper * data) const;
    #endif

    bool authenticate(const char * username, const char * password);
    #ifdef ARDUINO
        bool authenticate(const String& username, const String& password) {
          return authenticate(username.c_str(), password.c_str());
        }
    #endif
    void requestAuthentication(const char* realm);
    #ifdef ARDUINO
        void requestAuthentication(const String& realm) {
          requestAuthentication(realm.c_str());
        }
    #endif
};

class MongooseHttpServerRequestUpload : public MongooseHttpServerRequest
{
  friend MongooseHttpServer;

  private:
    uint64_t index;

  public:
    MongooseHttpServerRequestUpload(MongooseHttpServer *server, mg_connection *nc, mg_http_message *msg) :
      MongooseHttpServerRequest(server, nc, msg),
      index(0)
    {
    }
    virtual ~MongooseHttpServerRequestUpload() {
    }

    virtual bool isUpload() { return true; }
};



class MongooseHttpServerResponse
{
  protected:
    int64_t _contentLength;
    int _code;
    std::list<mg_http_header> headers;
    //mg_str body;
    const char * body;

  public:
    MongooseHttpServerResponse();
    virtual ~MongooseHttpServerResponse();

    void setCode(int code) {
      _code = code;
    }
    void setContentType(const char *contentType);
    void setContentLength(int64_t contentLength) {
      _contentLength = contentLength;
    }

    void addHeader(const char *name, const char *value);

    #ifdef ARDUINO
        void setContentType(String &contentType) {
          setContentType(contentType.c_str());
        }
        void addHeader(const String& name, const String& value);
    #endif

    const char * getHeaderString();

    void setContent(const char *content);
    void setContent(const uint8_t *content, size_t len);
    void setContent(MongooseString &content) {
      setContent((const uint8_t *)content.c_str(), content.length());
    }

    virtual void send(struct mg_connection *nc);

    // // send the to `nc`, return true if more to send
    // virtual void sendHeaders(struct mg_connection *nc);

    // // send (a part of) the body to `nc`, return < `bytes` if no more to send
    // virtual size_t sendBody(struct mg_connection *nc, size_t bytes) = 0;
};

#ifdef ARDUINO
  class MongooseHttpServerResponseStream:
    public MongooseHttpServerResponse,
    public Print
  {
    private:
      mg_iobuf _content;

    public:
      MongooseHttpServerResponseStream();
      virtual ~MongooseHttpServerResponseStream();

      size_t write(const uint8_t *data, size_t len);
      size_t write(uint8_t data);

      virtual void send(struct mg_connection *nc);
  };
#endif

typedef std::function<void(MongooseHttpServerRequest *request)> MongooseHttpRequestHandler;
typedef std::function<size_t(MongooseHttpServerRequest *request, int ev, MongooseString filename, uint64_t index, uint8_t *data, size_t len)> MongooseHttpUploadHandler;
typedef std::function<void(MongooseHttpWebSocketConnection *connection)> MongooseHttpWebSocketConnectionHandler;
typedef std::function<void(MongooseHttpWebSocketConnection *connection, int flags, uint8_t *data, size_t len)> MongooseHttpWebSocketFrameHandler;

class MongooseHttpServerEndpoint
{
  friend MongooseHttpServer;

  private:
    MongooseHttpServer *server;
    MongooseString *uri;
    HttpRequestMethodComposite method;
    MongooseHttpRequestHandler request;
    MongooseHttpUploadHandler upload;
    MongooseHttpRequestHandler close;
    MongooseHttpWebSocketConnectionHandler wsConnect;
    MongooseHttpWebSocketFrameHandler wsFrame;
  public:
    MongooseHttpServerEndpoint(MongooseHttpServer *server, HttpRequestMethodComposite method) :
      server(server),
      method(method),
      request(NULL),
      upload(NULL),
      close(NULL),
      wsConnect(NULL),
      wsFrame(NULL)
    {
    }

    MongooseHttpServerEndpoint *onRequest(MongooseHttpRequestHandler handler) {
      this->request = handler;
      return this;
    }

    MongooseHttpServerEndpoint *onUpload(MongooseHttpUploadHandler handler) {
      this->upload = handler;
      return this;
    }

    MongooseHttpServerEndpoint *onClose(MongooseHttpRequestHandler handler) {
      this->close = handler;
      return this;
    }

    MongooseHttpServerEndpoint *onConnect(MongooseHttpWebSocketConnectionHandler handler) {
      this->wsConnect = handler;
      return this;
    }

    MongooseHttpServerEndpoint *onFrame(MongooseHttpWebSocketFrameHandler handler) {
      this->wsFrame = handler;
      return this;
    }
};

class MongooseHttpWebSocketConnection : public MongooseHttpServerRequest
{
  friend MongooseHttpServer;

  public:
    MongooseHttpWebSocketConnection(MongooseHttpServer *server, mg_connection *nc, mg_http_message *msg);
    virtual ~MongooseHttpWebSocketConnection();

    virtual bool isWebSocket() { return true; }

    void send(int op, const void *data, size_t len);
    void send(const char *buf) {
      send(WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    #ifdef ARDUINO
        void send(String &str) {
          send(str.c_str());
        }
    #endif

    const mg_addr *getRemoteAddress() {
      return &(_nc->rem);
    }
    const mg_connection *getConnection() {
      return _nc;
    }
};

class MongooseHttpServer
{
  protected:
    struct mg_connection *nc;
    MongooseHttpServerEndpoint defaultEndpoint;
    std::list<MongooseHttpServerEndpoint*> endpoints;

    //static void defaultEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    //static void endpointEventHandler(struct mg_connection *nc, int ev, void *p, void *u);
    static void eventHandler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

  public:
    MongooseHttpServer();
    ~MongooseHttpServer();

    void begin(uint16_t port);

    #if MG_ENABLE_SSL
      void begin(uint16_t port, const char *cert, const char *private_key);
    #endif

    MongooseHttpServerEndpoint *on(const char* uri);
    MongooseHttpServerEndpoint *on(const char* uri, HttpRequestMethodComposite method);
    MongooseHttpServerEndpoint *on(const char* uri, MongooseHttpRequestHandler onRequest);
    MongooseHttpServerEndpoint *on(const char* uri, HttpRequestMethodComposite method, MongooseHttpRequestHandler onRequest);

    void onNotFound(MongooseHttpRequestHandler fn);

    void reset();

    void sendAll(MongooseHttpWebSocketConnection *from, const char *endpoint, int op, const void *data, size_t len);

    void sendAll(MongooseHttpWebSocketConnection *from, int op, const void *data, size_t len) {
      sendAll(from, NULL, op, data, len);
    }
    void sendAll(int op, const void *data, size_t len) {
      sendAll(NULL, NULL, op, data, len);
    }
    void sendAll(MongooseHttpWebSocketConnection *from, const char *buf) {
      sendAll(from, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *buf) {
      sendAll(NULL, NULL, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *endpoint, int op, const void *data, size_t len) {
      sendAll(NULL, endpoint, op, data, len);
    }
    void sendAll(MongooseHttpWebSocketConnection *from, const char *endpoint, const char *buf) {
      sendAll(from, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    void sendAll(const char *endpoint, const char *buf) {
      sendAll(NULL, endpoint, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
    #ifdef ARDUINO
      void sendAll(MongooseHttpWebSocketConnection *from, String &str) {
        sendAll(from, str.c_str());
      }
      void sendAll(String &str) {
        sendAll(str.c_str());
      }
      void sendAll(MongooseHttpWebSocketConnection *from, const char *endpoint, String &str) {
        sendAll(from, endpoint, str.c_str());
      }
      void sendAll(const char *endpoint, String &str) {
        sendAll(endpoint, str.c_str());
      }
    #endif
};

//static void http_event_callback(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
//static void https_event_callback(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

#endif /* _MongooseHttpServer_H_ */