#include "Timestamp.h"

Timestamp::Timestamp()
{
    memset(&m_timeValue, 0, sizeof(timeval));
    gettimeofday(&m_timeValue, 0);
}

Timestamp::~Timestamp()
{
}

void Timestamp::AddMillisecond(int64_t millisecond)
{
    m_timeValue.tv_sec += millisecond / 1000;
    m_timeValue.tv_usec += millisecond % 1000 * 1000;
}

bool operator < (const Timestamp &lhs, const Timestamp &rhs)
{
    if (lhs.m_timeValue.tv_sec < rhs.m_timeValue.tv_sec)
        return true;
    else if (lhs.m_timeValue.tv_sec == rhs.m_timeValue.tv_sec)
        return lhs.m_timeValue.tv_usec < rhs.m_timeValue.tv_usec;
    return false;
}

bool operator == (const Timestamp &lhs, const Timestamp &rhs)
{
    return (lhs.m_timeValue.tv_sec == rhs.m_timeValue.tv_sec) &&
           (lhs.m_timeValue.tv_usec == rhs.m_timeValue.tv_usec);
}

bool operator > (const Timestamp &lhs, const Timestamp &rhs)
{
    if (lhs.m_timeValue.tv_sec > rhs.m_timeValue.tv_sec)
        return true;
    else if (lhs.m_timeValue.tv_sec == rhs.m_timeValue.tv_sec)
        return lhs.m_timeValue.tv_usec > rhs.m_timeValue.tv_usec;
    return false;
}

int64_t operator - (const Timestamp &lhs,
                    const Timestamp &rhs)
{
    int64_t millisec =
        1000 * (lhs.m_timeValue.tv_sec - rhs.m_timeValue.tv_sec);
    millisec +=
        (lhs.m_timeValue.tv_usec - rhs.m_timeValue.tv_usec) / 1000;
    return millisec;
}

