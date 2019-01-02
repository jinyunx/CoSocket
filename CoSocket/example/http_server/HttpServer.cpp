#include "TcpServer.h"
#include "SimpleLog.h"
#include "http/HttpDispatch.h"

void Handle(const HttpDecoder &request, HttpEncoder &response)
{
    response.SetStatusCode(HttpEncoder::StatusCode_200Ok);
    response.SetBody("{\"code\": 0, \"message\": \"\"}\n");
    return;
}

int main()
{
    HttpDispatch httpDispatch;
    httpDispatch.AddHandler("/", Handle);

    TcpServer httpServer("0.0.0.0", 12345);
    httpServer.SetRequestHandler(std::ref(httpDispatch));
    std::list<int> childIds;
    httpServer.ListenAndFork(1, childIds);
    httpServer.MonitorSlavesLoop();
    return 0;
}
