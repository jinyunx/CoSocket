#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

#include "TcpServer.h"
#include "WebSocketParser.h"
#include "HttpDecoder.h"
#include "HttpEncoder.h"
#include "NonCopyable.h"
#include <functional>
#include <string>
#include <map>

class HttpDispatch : private NonCopyable
{
public:
    typedef std::function< void(const HttpDecoder &,
                                HttpEncoder &) > HttpHandler;

    HttpDispatch(bool enableWebSocket);

    void AddHandler(const std::string &url, const HttpHandler &handler);
    void ResponseOk(HttpEncoder &resp);
    void ResponseError(HttpEncoder &resp);
    void ResponseNotFound(HttpEncoder &resp);
    void ResponseWebSocketHandshake(HttpEncoder &resp, std::string key);

    void operator () (TcpServer::ConnectorPtr connectorPtr);

private:
    typedef std::map<std::string, HttpHandler> HanderMap;

    bool ParseToDispatch(const char *data, size_t len, HttpDecoder &request);
    void Dispatch(const HttpDecoder &request, HttpEncoder &response);
    bool Response(const char *data, size_t len);
    bool ProcessWebSocketData(const char *data, size_t len,
                              WebSocketParser &wsParser);
    bool ResponseWebSocketData(const char *data, size_t len);

    const static int kBufferSize = 102400;
    const static int kKeepAliveMs = 3000000;

    bool m_isWebSocketReq;
    bool m_enableWebSocket;
    HanderMap m_handlers;

    TcpServer::ConnectorPtr m_connectorPtr;
};

#endif // HTTP_DISPATCH_H
