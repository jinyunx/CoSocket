/*
Q6  Will closing a file descriptor cause it to be removed
    from all epoll sets automatically?

A6  Yes, but be aware of the following point. A file descriptor
    is a reference to an open file description (see open(2)).
    Whenever a descriptor is duplicated via dup(2), dup2(2),
    fcntl(2) F_DUPFD, or  fork(2), a new file descriptor referring
    to the same open file description is created. An open file
    description continues to exist until all file descriptors
    referring to it have been closed. A file descriptor is
    removed from an epoll set only after all the file descriptors
    referring to the underlying open file description have been closed
    (or before if the descriptor is explicitly removed using epoll_ctl(2)
    EPOLL_CTL_DEL). This means that even after a file descriptor that
    is part of an  epoll set has been closed, events may be reported
    for that file descriptor if other file descriptors referring to
    the same underlying file description remain open.
*/

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <functional>

struct epoll_event;

class Epoller
{
public:
    typedef std::function<void (int, int)> HandlerFunc;

    Epoller(const HandlerFunc &handler);
    ~Epoller();

    void Poll(int timeoutMs);
    int Update(int fd, uint32_t events);

private:
    typedef std::vector<struct epoll_event> EventList;

    int EpollCtl(int operation, int fd, uint32_t events);

    const static int kInitEventListSize = 16;

    int m_epollFd;
    EventList m_events;
    HandlerFunc m_handler;
};

