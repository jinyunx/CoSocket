#include "TcpServer.h"
#include "SimpleLog.h"
#include "http/HttpDispatch.h"

void Handle(const HttpDecoder &request, HttpEncoder &response)
{
    response.SetStatusCode(HttpEncoder::StatusCode_200Ok);
    response.SetBody("{\"code\": 0, \"message\": \"\"}\n");
    return;
}

void HandleRequest(TcpServer::ConnectorPtr connector)
{
    HttpDispatch httpDispatch(true);
    httpDispatch.AddHandler("/", Handle);
    httpDispatch.AddHandler("/ws", Handle);
    httpDispatch(connector);
}

int main()
{
    TcpServer httpServer("0.0.0.0", 12345);
    httpServer.SetRequestHandler(HandleRequest);
    std::list<int> childIds;
    httpServer.ListenAndFork(1, childIds);
    httpServer.MonitorSlavesLoop();
    return 0;
}
