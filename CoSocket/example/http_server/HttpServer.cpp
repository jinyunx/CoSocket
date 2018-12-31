#include "TcpServer.h"
#include "SimpleLog.h"
#include "http/HttpDispatch.h"

void Handle(const HttpRequester &request, HttpResponser &response)
{
    response.SetStatusCode(HttpResponser::StatusCode_200Ok);
    response.SetBody("{\"code\": 0, \"message\": \"\"}\n");
    return;
}

int main()
{
    HttpDispatch httpDispatch;
    httpDispatch.AddHandler("/", Handle);

    TcpServer echo("0.0.0.0", 12345);
    echo.SetRequestHandler(std::ref(httpDispatch));
    std::list<int> childIds;
    echo.ListenAndFork(1, childIds);
    echo.MonitorSlavesLoop();
    return 0;
}
