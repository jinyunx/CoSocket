#include "TcpServer.h"
#include "SimpleLog.h"
#include "http/HttpDispatch.h"

void HttpHandle(const HttpDecoder &request, HttpEncoder &response)
{
    response.SetStatusCode(HttpEncoder::StatusCode_200Ok);
    response.SetBody("{\"code\": 0, \"message\": \"\"}\n");
    return;
}

void WebSocketHandle(const WebSocketDecoder &request, WebSocketEncoder &response)
{
    std::string data(request.GetPayLoad(), request.GetPayloadLen());
    SIMPLE_LOG("WebSocket data: %s", data.c_str());

    response.SetOpCode(WebSocketParser::OpCodeType_Text);
    response.SetPayload(request.GetPayLoad(), request.GetPayloadLen());
    return;
}

void HandleRequest(TcpServer::ConnectorPtr connector)
{
    HttpDispatch httpDispatch(true);
    httpDispatch.AddHandler("/", HttpHandle);
    httpDispatch.AddHandler("/", WebSocketHandle);
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
