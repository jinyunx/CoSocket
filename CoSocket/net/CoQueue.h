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

#endif // CO_QUEUE_H
