#include "CoSocket.h"
#include "Epoller.h"

#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
#include "../3rd/coroutine.h"
}

CoSocket::CoSocket()
    : m_schedule(coroutine_open()),
      m_epoller(new Epoller(std::bind(&CoSocket::EventHandler,
                                      this, std::placeholders::_1,
                                      std::placeholders::_2)))
{
}

CoSocket::~CoSocket()
{
    if (m_schedule)
        coroutine_close(m_schedule);
}

void CoSocket::Run()
{
    while(1)
        m_epoller->Poll(-1);
}

int CoSocket::NewCoroutine(const CoFunction &func)
{
    int coId = coroutine_new(m_schedule, &InterCoFunc,
                             (void *)&func);
    coroutine_resume(m_schedule, coId);
    return coId;
}

void CoSocket::Sleep(int64_t ms)
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value.tv_sec = ms / 1000;
    newValue.it_value.tv_nsec = ms % 1000 * 1000000;
    ::timerfd_settime(timerfd, 0, &newValue, &oldValue);

    if (!m_epoller->Update(timerfd, EPOLLIN))
        printf("Udate failed\n");

    SaveToCoIdMap(CoKey(timerfd, EPOLLIN));

    coroutine_yield(m_schedule);

    uint64_t num = 0;
    ::read(timerfd, &num, sizeof num);

    m_epoller->Update(timerfd, 0);
}

void CoSocket::InterCoFunc(struct schedule *s, void *ud)
{
    (void)s;

    CoFunction *func = reinterpret_cast<CoFunction *>(ud);
    if (func) (*func)();
}

bool CoSocket::SaveToCoIdMap(const CoKey &coKey)
{
    CoIdMap::const_iterator it = m_coIdMap.find(coKey);
    if (it != m_coIdMap.end())
    {
        printf("Dumplicate co key is not allowed.\n");
        return false;
    }

    m_coIdMap[coKey] = coroutine_running(m_schedule);
    return true;
}

void CoSocket::EventHandler(int fd, uint32_t events)
{
    CoIdMap::const_iterator it = m_coIdMap.find(CoKey(fd, events));
    if (it == m_coIdMap.end())
    {
        printf("Cannot find the event handler\n");
        return;
    }

    coroutine_resume(m_schedule, it->second);
}

CoSocket coSocket;

void func()
{
    for (int i = 0; i < 10000; ++i)
    {
        printf("%d sleep starting\n", i);
        coSocket.Sleep(1000);
        printf("%d sleep end\n", i);
    }
}

int main()
{
    coSocket.NewCoroutine(std::bind(&func));
    coSocket.Run();
    return 0;
}

