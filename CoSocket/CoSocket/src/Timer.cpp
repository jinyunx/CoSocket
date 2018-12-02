#include "Timer.h"
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

Timer::Timer(CoSocket &cs)
    : m_cs(cs),
      m_fd(::timerfd_create(CLOCK_MONOTONIC,
                            TFD_NONBLOCK | TFD_CLOEXEC))

{
    if (m_fd < 0)
    {
        printf("timerfd_create failed, error: %s\n",
               strerror(errno));
        exit(1);
    }
}

Timer::~Timer()
{
    if (m_fd)
        ::close(m_fd);
}

void Timer::Wait(int64_t ms)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value.tv_sec = ms / 1000;
    newValue.it_value.tv_nsec = ms % 1000 * 1000000;
    if (::timerfd_settime(m_fd, 0, &newValue, &oldValue) < 0)
    {
        printf("timerfd_settime failed, error: %s\n",
               strerror(errno));
        return;
    }

    uint64_t num = 0;
    m_cs.Read(m_fd, reinterpret_cast<char *>(&num), sizeof num);
    return;
}

