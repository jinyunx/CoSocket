#include "CoSocket.h"
#include "TcpServer.h"
#include "SimpleLog.h"
#include <vector>

void RequestHandle(
    int bufferSize, TcpServer::ConnectorPtr connector)
{
    std::vector<char> buffer(bufferSize);
    ssize_t wret = 0;
    while (1)
    {
        ssize_t ret = connector->Read(buffer.data(), bufferSize, 3000);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %zd, wret: %zd",
                       ret, wret);
            return;
        }

        ssize_t left = ret;
        ssize_t offset = 0;
        while (left > 0)
        {
            wret = connector->Write(buffer.data() + offset, left, 3000);
            if (wret < 0)
            {
                SIMPLE_LOG("write failed, error: %zd", wret);
                return;
            }
            left -= wret;
            offset += wret;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        SIMPLE_LOG("Usage: %s buffer_size", argv[0]);
        return 1;
    }
    int bufferSize = atoi(argv[1]);

    TcpServer tcpServer("127.0.0.1", 12345);
    tcpServer.SetRequestHandler(
        std::bind(&RequestHandle, bufferSize,
                  std::placeholders::_1));
    std::list<int> childIds;
    tcpServer.ListenAndFork(1, childIds);
    tcpServer.MonitorSlavesLoop();
    return 0;
}
