#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

#include "TcpServer.h"
#include "NonCopyable.h"
#include "HttpDecoder.h"
#include "HttpEncoder.h"
#include <functional>
#include <string>
#include <map>

class HttpDispatch : private NonCopyable
{
public:
    typedef std::function< void(const HttpDecoder &,
                                HttpEncoder &) > HttpHandler;

    HttpDispatch();

    void AddHandler(const std::string &url, const HttpHandler &handler);
    void ResponseOk(HttpEncoder &resp);
    void ResponseError(HttpEncoder &resp);

    void operator () (TcpServer::ConnectorPtr connectorPtr);
private:
    typedef std::map<std::string, HttpHandler> HanderMap;

    void Dispatch(const HttpDecoder &request, HttpEncoder &response);
    bool Response(TcpServer::ConnectorPtr &connectorPtr, HttpEncoder &response);

    const static int kBufferSize = 102400;
    const static int kKeepAliveMs = 3000;

    HanderMap m_handlers;
};

#endif // HTTP_DISPATCH_H
