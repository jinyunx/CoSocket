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

