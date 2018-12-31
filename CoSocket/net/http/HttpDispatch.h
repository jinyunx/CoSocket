#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

#include "TcpServer.h"
#include "NonCopyable.h"
#include "HttpRequester.h"
#include "HttpResponser.h"
#include <functional>
#include <string>
#include <map>

class HttpDispatch : private NonCopyable
{
public:
    typedef std::function< void(const HttpRequester &,
                                HttpResponser &) > HttpHandler;

    HttpDispatch();

    void AddHandler(const std::string &url, const HttpHandler &handler);
    void ResponseOk(HttpResponser &resp);
    void ResponseError(HttpResponser &resp);

    void operator () (TcpServer::ConnectorPtr connectorPtr);
private:
    typedef std::map<std::string, HttpHandler> HanderMap;

    void Dispatch(const HttpRequester &request, HttpResponser &response);
    bool Response(TcpServer::ConnectorPtr &connectorPtr, HttpResponser &response);

    const static int kBufferSize = 1024;
    const static int kKeepAliveMs = 3000;

    HanderMap m_handlers;
};

#endif // HTTP_DISPATCH_H
