#ifndef CO_QUEUE_H
#define CO_QUEUE_H
#include "NonCopyable.h"
#include "CoSocket.h"
#include "Timer.h"

#include <deque>

template<typename T>
class CoQueue : private NonCopyable
{
public:
    typedef std::shared_ptr<T> DataPtr;

    CoQueue(CoSocket &cs)
        : m_condition(cs.GetCondition()),
          m_cs(cs)
    {
    }

    ~CoQueue()
    {
    }

    void Enque(const DataPtr &ptr)
    {
        m_queue.push_back(ptr);
        m_cs.ConditionNotify(m_condition);
    }

    DataPtr Deque()
    {
        while (m_queue.empty())
            m_cs.ConditionWait(m_condition);

        DataPtr data = m_queue.front();
        m_queue.pop_front();
        return data;
    }

private:
    uint64_t m_condition;
    CoSocket &m_cs;
    std::deque<DataPtr> m_queue;
};

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
        delete[] m_data;
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

    int Capacity() const
    {
        return m_capacity;
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

#endif // CO_QUEUE_H
