#include "Epoller.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Epoller::Epoller(const HandlerFunc &handler)
    : m_epollFd(::epoll_create1(EPOLL_CLOEXEC)),
      m_events(kInitEventListSize),
      m_handler(handler)
{
    if (m_epollFd < 0)
    {
        printf("epoll_create1 error: %s\n", strerror(errno));
        exit(1);
    }
}

Epoller::~Epoller() 
{
    ::close(m_epollFd);
}

void Epoller::Poll(int timeoutMs)
{
    int numEvents = ::epoll_wait(m_epollFd, &m_events[0],
                                 static_cast<int>(m_events.size()),
                                 timeoutMs);
    if (numEvents < 0)
    {
        if (errno == EINTR)
            return;

        printf("epoll_wait error: %s\n", strerror(errno));
        exit(1);
    }

    for (int i = 0; i < numEvents; ++i)
        m_handler(m_events[i].data.fd, m_events[i].events);

    if (static_cast<std::size_t>(numEvents) == m_events.size())
        m_events.resize(numEvents * 2);
}

int Epoller::Update(int fd, uint32_t events)
{
    int operation = EPOLL_CTL_ADD;
    if (!events)
        operation = EPOLL_CTL_DEL;

    int ret = EpollCtl(operation, fd, events);

    if (ret == ENOENT && operation == EPOLL_CTL_DEL)
        ret = 0;
    else if (ret == EEXIST && operation == EPOLL_CTL_ADD)
        ret = EpollCtl(EPOLL_CTL_MOD, fd, events);

    return -ret;
}

int Epoller::EpollCtl(int operation, int fd, uint32_t events)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = events;
    event.data.fd = fd;
    if (::epoll_ctl(m_epollFd, operation, fd, &event) < 0)
    {
        printf("epoll_ctl failed, operation: %d, fd: %d, error: %s\n",
               operation, fd, strerror(errno));
        return errno;
    }
    return 0;
}

