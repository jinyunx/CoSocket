#ifndef TIMER_H
#define TIMER_H

#include "CoSocket.h"

class Timer
{
public:
    explicit Timer(CoSocket &cs);
    ~Timer();

    void Wait(int64_t ms);

private:
    CoSocket &m_cs;
    int m_fd;
};

#endif // TIMER_H
