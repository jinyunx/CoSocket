#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

#include "TcpServer.h"
#include "HttpDecoder.h"
#include "HttpEncoder.h"
#include "WebSocketDecoder.h"
#include "WebSocketEncoder.h"
#include "NonCopyable.h"
#include <functional>
#include <string>
#include <map>

class HttpDispatch : private NonCopyable
{
public:
    typedef std::function< void(const HttpDecoder &,
                                HttpEncoder &) > HttpHandler;
    typedef std::function< void(const WebSocketDecoder &,
                                WebSocketEncoder &) > WebSocketHandler;

    struct Handler
    {
        HttpHandler httpHandler;
        WebSocketHandler webSocketHandler;
    };

    HttpDispatch(bool enableWebSocket);

    void AddHandler(const std::string &url, const HttpHandler &handler);
    void AddHandler(const std::string &url, const WebSocketHandler &handler);

    void ResponseHttpOk(HttpEncoder &httpResponse);
    void ResponseHttpError(HttpEncoder &httpResponse);
    void ResponseHttpNotFound(HttpEncoder &httpResponse);
    void ResponseWebSocketHandshake(HttpEncoder &httpResponse, std::string key);

    void operator () (TcpServer::ConnectorPtr connectorPtr);

private:
    typedef std::map<std::string, Handler> HanderMap;

    bool ParseHttpToDispatch(const char *data, size_t len, HttpDecoder &httpRequest);
    void Dispatch(const HttpDecoder &httpRequest, HttpEncoder &httpResponse);
    bool Response(const char *data, size_t len);
    bool ProcessWebSocketData(const char *data, size_t len,
                              WebSocketDecoder &wsDecoder);
    bool ResponseWebSocketData(WebSocketEncoder &wsEncoder);

    const static int kBufferSize = 102400;
    const static int kKeepAliveMs = 60000;

    bool m_isWebSocketReq;
    bool m_enableWebSocket;
    HanderMap m_handlers;
    WebSocketHandler m_wsHandler;

    TcpServer::ConnectorPtr m_connectorPtr;
};

#endif // HTTP_DISPATCH_H
