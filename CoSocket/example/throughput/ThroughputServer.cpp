#include "CoSocket.h"
#include "TcpServer.h"
#include <vector>

void RequestHandle(
    int bufferSize, TcpServer::ConnectorPtr connector)
{
    std::vector<char> buffer(bufferSize);
    ssize_t wret = 0;
    while (1)
    {
        ssize_t ret = connector->Read(buffer.data(), bufferSize);
        if (ret <= 0)
        {
            printf("read failed, error: %zd, wret: %zd\n",
                   ret, wret);
            return;
        }

        ssize_t left = ret;
        ssize_t offset = 0;
        while (left > 0)
        {
            wret = connector->Write(buffer.data() + offset, left);
            if (wret < 0)
            {
                printf("write failed, error: %zd\n", wret);
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
        printf("Usage: %s buffer_size\n", argv[0]);
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
