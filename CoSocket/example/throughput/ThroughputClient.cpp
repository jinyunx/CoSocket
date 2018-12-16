#include "CoSocket.h"
#include "Timer.h"
#include "TcpClient.h"
#include <arpa/inet.h>
#include <vector>

static ssize_t g_numSendAndAckPer1Sec;

void TimerFunc(CoSocket &coSocket)
{
    Timer timer(coSocket);
    while (1)
    {
        timer.Wait(1 * 1000);
        printf("Num Send and Ack per 1 sec: %zd\n", g_numSendAndAckPer1Sec);
        g_numSendAndAckPer1Sec = 0;
    }
}

void SendFunc(int bufferSize, CoSocket &coSocket)
{
    ssize_t dataSize = bufferSize - sizeof(int);
    if (dataSize < 0)
    {
        printf("buffer size %d is too small\n", bufferSize);
        return;
    }

    TcpClient tcpClient(coSocket);
    ssize_t ret = tcpClient.Connect("127.0.0.1", 12345);
    if (ret != 0)
    {
        printf("connect fail, error: %zd\n", ret);
        return;
    }
    printf("connect ok\n");

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
                printf("write fail, error: %zd\n", ret);
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
                printf("read fail, error: %zd\n", ret);
                return;
            }
            left -= ret;
            offset += ret;
        }
        if (ntohl(*reinterpret_cast<int*>(buffer.data())) != dataSize)
            printf("ack data error");
        g_numSendAndAckPer1Sec += bufferSize;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s buffer_size coroutine_num\n", argv[0]);
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
