#include "CoSocket.h"
#include "Timer.h"
#include "TcpClient.h"
#include "SimpleLog.h"
#include <arpa/inet.h>
#include <vector>

static ssize_t g_numSendAndAckPer1Sec = 0;

void TimerFunc(CoSocket &coSocket)
{
    Timer timer(coSocket);
    while (1)
    {
        timer.Wait(1 * 1000);
        SIMPLE_LOG("Num Send and Ack per 1 sec: %zd", g_numSendAndAckPer1Sec);
        g_numSendAndAckPer1Sec = 0;
    }
}

void SendFunc(int bufferSize, CoSocket &coSocket)
{
    ssize_t dataSize = bufferSize - sizeof(int);
    if (dataSize < 0)
    {
        SIMPLE_LOG("buffer size %d is too small", bufferSize);
        return;
    }

    TcpClient tcpClient(coSocket);
    ssize_t ret = tcpClient.Connect("127.0.0.1", 12345);
    if (ret != 0)
    {
        SIMPLE_LOG("connect fail, error: %zd", ret);
        return;
    }
    SIMPLE_LOG("connect ok");

    std::vector<char> buffer(bufferSize);
    *reinterpret_cast<int*>(buffer.data()) = htonl(dataSize);
    while (1)
    {
        ssize_t left = bufferSize;
        ssize_t offset = 0;
        while (left > 0)
        {
            ret = tcpClient.Write(buffer.data() + offset, left);
            if (ret < 0)
            {
                SIMPLE_LOG("write fail, error: %zd", ret);
                return;
            }
            left -= ret;
            offset += ret;
        }

        left = bufferSize;
        offset = 0;
        while (left > 0)
        {
            ret = tcpClient.Read(buffer.data() + offset, left);
            if (ret < 0)
            {
                SIMPLE_LOG("read fail, error: %zd", ret);
                return;
            }
            left -= ret;
            offset += ret;
        }
        if (ntohl(*reinterpret_cast<int*>(buffer.data())) != dataSize)
            SIMPLE_LOG("ack data error");
        g_numSendAndAckPer1Sec += bufferSize;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        SIMPLE_LOG("Usage: %s buffer_size coroutine_num", argv[0]);
        return 1;
    }
    int bufferSize = atoi(argv[1]);
    int coNum = atoi(argv[2]);

    CoSocket coSocket;
    for (int i = 0; i < coNum; ++i)
    {
        coSocket.NewCoroutine(
            std::bind(&SendFunc, bufferSize, std::ref(coSocket)));
    }
    coSocket.NewCoroutine(
        std::bind(&TimerFunc, std::ref(coSocket)));
    coSocket.Run();
    return 0;
}
