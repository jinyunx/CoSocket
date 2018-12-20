#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <sys/time.h>
#include <string.h>
#include <inttypes.h>

class Timestamp
{
public:
    Timestamp();
    ~Timestamp();

    void AddMillisecond(int64_t millisecond);
    friend bool operator < (const Timestamp &lhs,
                            const Timestamp &rhs);
    friend bool operator == (const Timestamp &lhs,
                             const Timestamp &rhs);
    friend bool operator > (const Timestamp &lhs,
                            const Timestamp &rhs);
    friend int64_t operator - (const Timestamp &lhs,
                               const Timestamp &rhs);
private:
    timeval m_timeValue;
};

#endif // TIMESTAMP_H
