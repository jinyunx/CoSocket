#include "CoSocket.h"
#include "TcpServer.h"
#include "SimpleLog.h"
#include "CoQueue.h"

#include <deque>
#include <memory>

class Buffer
{
public:
    Buffer(int capacity)
        : m_stop(false),
          m_size(0),
          m_capacity(capacity),
          m_data(new char[m_capacity])
    {
    }

    ~Buffer()
    {
        delete [] m_data;
        m_data = 0;
    }

    void Stop()
    {
        m_stop = true;
    }

    bool IsStop() const
    {
        return m_stop;
    }

    int Size() const
    {
        return m_size;
    }

    char *Data()
    {
        return m_data;
    }

    void SetSize(int size)
    {
        m_size = size;
    }

private:
    bool m_stop;
    int m_size;
    int m_capacity;
    char *m_data;
};

typedef CoQueue<Buffer> BufferQueue;
typedef std::shared_ptr<BufferQueue> BufferQueuePtr;
typedef BufferQueue::DataPtr BufferPtr;

void WriteCoroutine(BufferQueuePtr bufferQueue, int bufferSize,
                    TcpServer::ConnectorPtr connector)
{
    while (1)
    {
        BufferPtr dataToWrite = bufferQueue->Deque();

        if (dataToWrite->IsStop())
            break;

        ssize_t ret = connector->WriteAll(dataToWrite->Data(), dataToWrite->Size(), 3000);
        if (ret != dataToWrite->Size())
        {
            SIMPLE_LOG("write failed, error: %zd", ret);
            break;
        }
    }
}

void RequestHandle(
    int bufferSize, TcpServer::ConnectorPtr connector)
{
    BufferQueuePtr bufferQueue(new BufferQueue(connector->GetCoSocket()));

    connector->GetCoSocket().NewCoroutine(
        std::bind(&WriteCoroutine, bufferQueue, bufferSize, connector));

    while (1)
    {
        BufferPtr buffer(new Buffer(bufferSize));
        ssize_t ret = connector->Read(buffer->Data(), bufferSize, 3000);
        if (ret <= 0)
        {
            SIMPLE_LOG("read failed, error: %zd", ret);
            break;
        }
        buffer->SetSize(ret);
        bufferQueue->Enque(buffer);
    }

    BufferPtr buffer(new Buffer(4));
    buffer->Stop();
    bufferQueue->Enque(buffer);
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
